#include "img.hpp"
#include "utils.hpp"

void IMGArchive::get_archive_file_v1(UDWord id, File* archive_file) {
  archive_file->headerV1 = &archive_V1[id];
  archive_file->offset   =  archive_V1[id].offset   * SECTOR_SIZE;
  archive_file->size     =  archive_V1[id].filesize * SECTOR_SIZE;
}

uint IMGArchive::open_archive_v1() {
  size_t ret;
  UDWord num_entries;

  FILE* dir = fopen(dir_path.c_str(), "rb"); // open gta3.dir
  CHECK((dir == NULL), ("Failed opening file: " + dir_path).c_str(), FAIL);

  std::filesystem::path p{dir_path}; // gta3.dir contains all the headers.

  // Avoid being off by multiple bytes or extra padding when calculating the number of entries.
  if (std::filesystem::file_size(p) % HEADER_SIZE != 0)
    ERR("Data corruption: All headers in " + dir_path + " are not 32 byte-aligned.");

  num_entries = std::filesystem::file_size(p) / HEADER_SIZE; // Each header is 32 bytes.
  for (UDWord i = 0; i < num_entries; i++) {
    HeaderV1 entry;
    ret = fread(&entry, 1, HEADER_SIZE, dir);
    CHECK_FREAD(dir_path, dir, ret, HEADER_SIZE, FAIL);
    archive_V1.push_back(entry);
  }

  fclose(dir);
  version = v1;
  return SUCCESS;
}

bool IMGArchive::write_to_archive_v1(File* file, UDWord id) {
  size_t ret;
  HeaderV1* header = new HeaderV1;

  // The header for .dir:
  header->offset   = file->offset / SECTOR_SIZE;
  header->filesize = file->size   / SECTOR_SIZE;

  strncpy(header->filename, archive_V1[id].filename, 24);
  header->filename[23] = '\0';

  if (file->size % SECTOR_SIZE)
    ERR(String(header->filename) + ": The file size must be 2048 byte aligned");

  // Write to .dir:
  FILE* dir = fopen(dir_path.c_str(), "r+");
  CHECK((dir == nullptr), (String("failed opening directory: ") + dir_path), FAIL);
  // Write 32 bytes from the offset: id * HEADER_SIZE
  fseek(dir, id * HEADER_SIZE, SEEK_SET);
  ret = fwrite(header, sizeof(HeaderV1), 1, dir);
  CHECK_FWRITE(dir_path, dir, ret, 1, FAIL);
  fclose(dir);

  // Write to .img:
  FILE* img = fopen(img_path.c_str(), "r+");
  CHECK((img == nullptr), (String("failed opening image: ") + img_path), FAIL);
  fseek(img, file->offset, SEEK_SET);
  ret = fwrite(file->content, sizeof(UByte), file->size, img);
  CHECK_FWRITE(img_path, img, ret, file->size, FAIL);
  fclose(img);

  DELETE_PTR(header);

  return SUCCESS;
}

static inline
void pad_null_bytes(UByte* arr, UDWord arr_size, UDWord pad_len) {
  for (UDWord i = arr_size-pad_len; i < arr_size; i++)
    arr[i] = 0x0;
}

bool IMGArchive::replace_archive_files_v1(std::vector<String>  old_files,
                                          std::vector<String>  new_files) {
  UDWord file_idx;
  std::vector<Files<String, UDWord>> files_idxs;

  if (old_files.size() != new_files.size())
    ERR("There is an inequivalent amount of old and new files to be replaced.");

  // Sort the files that are going to get replaced by their ID, to keep the
  // file order in the archive intact.
  for (UDWord i = 0; i < old_files.size(); i++) {

    file_idx = get_file_idx<HeaderV1>(archive_V1, old_files[i]);
    CHECK((file_idx == archive_V1.size()),
      "Failed to find the replaceable file: " + old_files[i] +
      " in the archive: " + img_path, FAIL);

    if (!std::filesystem::exists(new_files[i]))
      ERR("The replacing file does not exist: " + new_files[i]);

    files_idxs.push_back(Files(new_files[i], file_idx));
  }

  std::sort(files_idxs.begin(), files_idxs.end(),
    [](const Files<String, UDWord> &x,
       const Files<String, UDWord> &y) {
      return x.idx < y.idx;
    });

  std::vector<File*>  archive;
  std::vector<UDWord> idxs;

  UDWord pad_size, remainder;
  UDWord i = (files_idxs[0].idx ? files_idxs[0].idx - 1 : 0);

  // Replace the files by their ID in sorted order:
  for (UDWord j = 0; i < num_files(); i++) {
    File* file = new File;
    pad_size   = 0;

    if (i == files_idxs[j].idx) {
      file->offset  = archive_V1[i].offset * SECTOR_SIZE;
      file->size    = std::filesystem::file_size(files_idxs[j].path);

      remainder = file->size % SECTOR_SIZE;
      // Pad sufficient futile null-byte sleds to the file and update its size
      if (remainder)
        pad_size = SECTOR_SIZE - remainder;

      file->content = new UByte[file->size + pad_size];
      pad_null_bytes(file->content, file->size + pad_size, pad_size);
      if (read_from_file(files_idxs[j].path.c_str(), 0, file->content, file->size))
        return FAIL;
      file->size += pad_size;

      j++;
    }
    else
      file = get_archive_file(i);

    CHECK((file == nullptr), "Failed to retrieve file from the archive.", FAIL);
    idxs.push_back(i);
    archive.push_back(file);
  }

  // Rectify and align offsets:
  for (i = 0; i < archive.size(); i++) {
    if (i)
      archive[i]->offset = archive[i-1]->offset + archive[i-1]->size;

    if(write_to_archive_v1(archive[i], idxs[i]))
      ERR("Failed writing to archive.");
  }

  files_idxs.clear();
  idxs.clear();
  DELETE_VEC(archive);
  archive_V1.clear();

  // Re-open the new archive.
  open_archive();

  return 0;
}
