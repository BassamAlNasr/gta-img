#include "img.hpp"
#include "utils.hpp"

void IMGArchive::get_archive_file_v2(UDWord id, File* archive_file) {
  archive_file->headerV2 = &archive_V2[id];
  archive_file->offset   =  archive_V2[id].offset     * SECTOR_SIZE;
  archive_file->size     =  archive_V2[id].streamsize * SECTOR_SIZE;
}

uint IMGArchive::open_archive_v2() {
  size_t ret;
  UDWord num_entries;

  ret     = fread(&num_entries, sizeof(UDWord), 1, img);
  CHECK_FREAD(img_path, img, ret, 1, FAIL);

  for (UDWord i = 0; i < num_entries; i++) {
    HeaderV2 entry;
    ret = fread(&entry, sizeof(UByte), HEADER_SIZE, img);
    CHECK_FREAD(img_path, img, ret, HEADER_SIZE, FAIL);
    if (entry.filesize != 0)
      ERR("[" + std::to_string(i) + "] DATA CORRUPTION: filesize should be 0, "
           "but is instead: " + std::to_string(entry.filesize) + "\n");
    archive_V2.push_back(entry);
  }

  version = v2;
  return SUCCESS;
}

bool IMGArchive::write_to_archive_v2(File* file, UDWord id) {
  size_t ret;
  size_t header_section;
  HeaderV2* header = new HeaderV2;

  header->offset     = file->offset / SECTOR_SIZE;
  header->streamsize = file->size   / SECTOR_SIZE;
  header->filesize   = 0;

  strncpy(header->filename, archive_V2[id].filename, 24);
  header->filename[23] = '\0';

  header_section  = IMG_HEADER_SIZE_V2 + num_files()*HEADER_SIZE;

  if (file->size % SECTOR_SIZE)
    ERR(String(header->filename) + ": The file size must be 2048 byte aligned.");
  if (file->offset < header_section)
    ERR(String(header->filename) + ": Attempted to write into the header "
        "section of the archive at offset: " + std::to_string(file->offset));

  FILE* img = fopen(img_path.c_str(), "r+");
  CHECK((img == nullptr), (String("failed opening image: ") + img_path), FAIL);

  // Write to .img:
  fseek(img, IMG_HEADER_SIZE_V2 + id*HEADER_SIZE, SEEK_SET);
  ret = fwrite(header, sizeof(HeaderV2), 1, img);
  CHECK_FWRITE(img_path, img, ret, 1, FAIL);
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

bool IMGArchive::replace_archive_files_v2(std::vector<String>  old_files,
                                          std::vector<String>  new_files) {
  UDWord file_idx;
  std::vector<Files<String, UDWord>> files_idxs;

  if (old_files.size() != new_files.size())
    ERR("There is an inequivalent amount of old and new files to be replaced.");

  // Sort the files that are going to get replaced by their ID, to keep the
  // file order in the archive intact.
  for (UDWord i = 0; i < old_files.size(); i++) {

    file_idx = get_file_idx<HeaderV2>(archive_V2, old_files[i]);
    CHECK((file_idx == archive_V2.size()),
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
      file->offset  = archive_V2[i].offset * SECTOR_SIZE;
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

    if(write_to_archive_v2(archive[i], idxs[i]))
      ERR("Failed writing to archive.");
  }

  files_idxs.clear();
  idxs.clear();
  DELETE_VEC(archive);
  archive_V2.clear();

  // Re-open the new archive.
  open_archive();

  return 0;
}
