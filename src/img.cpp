#include "img.hpp"
#include "utils.hpp"

IMGArchive::IMGArchive(String path1, String path2) {
  img      = nullptr;
  img_path = path1;
  dir_path = path2;
  open_archive();
}

IMGArchive::~IMGArchive() {
  if      (version == v1) archive_V1.clear();
  else if (version == v2) archive_V2.clear();
  else if (version == v3) archive_V3.clear();
}

File::~File() {
  delete content;
  content = nullptr;
}

uint IMGArchive::open_archive() {
  size_t ret;
  img = fopen(&img_path[0], "rb");
  CHECK((img == nullptr), ("Failed opening file: " + img_path).c_str(), FAIL);

  char ver[4];
  ret = fread(ver, sizeof(char), 4, img);
  CHECK_FREAD(img_path, img, ret, 4, FAIL);

  if (ver[0] == 'V' && ver[1] == 'E' && ver[2] == 'R' && ver[3] == '2') {
    CHECK(open_archive_v2(),  "Failed opening V2 archive " + img_path, FAIL); }
  else if (*reinterpret_cast<int*>(ver) == 0x67A3A1CE) {
    ERR("The XBOX version is unsupported."); }
  else {
    CHECK(open_archive_v1(), "Failed opening V1 archive: " + img_path, FAIL); }

  fclose(img);
  return 0;
}

UDWord IMGArchive::num_files() {
  if      (version == vundef) return 0;
  else if (version == v1)     return archive_V1.size();
  return archive_V2.size();
}

File* IMGArchive::get_archive_file(UDWord id) {
  size_t ret;

  if (id >= num_files() || version == vundef)
    return nullptr;

  img = nullptr;
  img = fopen(&img_path[0], "rb");
  CHECK((img == nullptr), ("Failed opening file: " + img_path).c_str(), nullptr)

  File* archive_file = new File;
  if (version == v1)
    get_archive_file_v1(id, archive_file);
  else
    get_archive_file_v2(id, archive_file);

  archive_file->content = new UByte[archive_file->size];

  fseek(img, archive_file->offset, SEEK_SET);
  ret = fread(archive_file->content, sizeof(char), archive_file->size, img);
  CHECK_FREAD(img_path, img, ret, archive_file->size, nullptr);
  fclose(img);

  return archive_file;
}

File* IMGArchive::get_archive_file(String filename) {
  size_t ret;
  String entry_filename;

  UDWord num_entries = num_files();

  for (UDWord i = 0; i < num_entries; i++) {
    if (version == v1)
      entry_filename = archive_V1[i].filename;
    else if (version == v2)
      entry_filename = archive_V2[i].filename;
    else
      std::cout << "get_archive_file: Unimplemented code" << std::endl;

    if (entry_filename == filename) {
      img = nullptr;
      img = fopen(&img_path[0], "rb");
      CHECK((img == nullptr), ("Failed opening file: " + img_path).c_str(), nullptr);

      File* archive_file = new File;
      if (version == v1)
        get_archive_file_v1(i, archive_file);
      else
        get_archive_file_v2(i, archive_file);

      archive_file->content = new UByte[archive_file->size];

      fseek(img, archive_file->offset, SEEK_SET);
      ret = fread(archive_file->content, sizeof(char), archive_file->size, img);
      CHECK_FREAD(img_path, img, ret, archive_file->size, nullptr);
      fclose(img);

      return archive_file;
    }
  }

  return nullptr;
}

uint IMGArchive::copy_file_from_img(String filename, String dest) {
  size_t ret;
  File* archive_file = get_archive_file(filename);
  CHECK((archive_file == nullptr), ("failed retrieving file: " + filename).c_str(), FAIL);

  FILE* dest_file = fopen((dest).c_str(), "w");
  CHECK((dest_file == nullptr), ("failed opening file: " + (dest)).c_str(), FAIL);

  ret = fwrite(archive_file->content, sizeof(char), archive_file->size, dest_file);
  CHECK_FWRITE(dest, dest_file, ret, archive_file->size, FAIL);

  DELETE_PTR(archive_file);
  fclose(dest_file);
  return 0;
}

template UDWord IMGArchive::get_file_idx<HeaderV1>(std::vector<HeaderV1>&, const String&);
template UDWord IMGArchive::get_file_idx<HeaderV2>(std::vector<HeaderV2>&, const String&);

template <typename T>
UDWord IMGArchive::get_file_idx(std::vector<T>& v, const String& file) {
  auto it = std::find_if(v.begin(), v.end(), [&file](const T& header) {
      return header.filename == file;
  });

  if (it != v.end())
    return std::distance(v.begin(), it);

  return v.size();
}

bool IMGArchive::read_from_file(const char* filename, long off, UByte* dst, size_t size) {
  size_t ret;
  FILE* file = fopen(filename, "rb");
  CHECK((file == nullptr), (String("failed opening file: ") + filename), FAIL);
  fseek(file, off, SEEK_SET);
  ret = fread(dst, sizeof(UByte), size, file);
  CHECK_FREAD(filename, file, ret, size, FAIL);
  fclose(file);
  return 0;
}

uint IMGArchive::replace_archive_files(std::vector<String> old_files, std::vector<String> new_files) {

  if (old_files.size() != new_files.size())
    ERR("The amount of files to be replaced must be equivalent to the "
                 "amount of replacing files.");

  for (String s : new_files) {
    if (!std::filesystem::exists(s))
      ERR(s + ": The file to replace an archive file does not exist.");

    if (std::filesystem::file_size(s) > (pow(2, 16) * SECTOR_SIZE))
      ERR(s + ": The maximum file size may not be larger than 134.21 MB.");

    if (std::filesystem::file_size(s) < 32)
      ERR(s + ": The minimum file size must be larger than 32 bytes.");
  }

  switch (version) {
    case v1:
      return replace_archive_files_v1(old_files, new_files);
    break;

    case v2:
      return replace_archive_files_v2(old_files, new_files);
    break;

    case v3:
      ERR("Unimplemented.");
    break;

    case vundef:
      ERR("Undetermined image archive version. Could not replace file(s).");
    break;

    default:
      ERR("replace: Logic Flaw: Unreachable code reached.");
    break;
  }

  return SUCCESS;
}
