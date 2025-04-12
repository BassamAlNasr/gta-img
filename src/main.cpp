#include "img.hpp"
#include "utils.hpp"

int main(int argc, char* argv[]) {
  std::vector<String> file_paths;
  std::vector<String> files;
  char* dir = (char*)"";

  if (argc != 4 && argc != 5) {
    std::cout << "Usage:\n  " << argv[0] << " -e <file> <image> {directory}\n  "
                              << argv[0] << " -r <file|folder> <image> {directory}" << std::endl;
    return FAIL;
  }

  if (argc == 5)
    dir = argv[4];

  IMGArchive* img_archive  = new IMGArchive(argv[3], dir);
  File*       archive_file = nullptr;

  if (img_archive->version == vundef) {
    std::cout << "Initialization failed for the image: " << argv[3] << std::endl;
    DELETE_PTR(img_archive);
    return 1;
  }

  if (String(argv[1]) == "-e") {
    std::filesystem::path path(argv[2]);
    img_archive->copy_file_from_img(path.filename().string(), argv[2]);
  }
  else if (String(argv[1]) == "-r") {
    if (std::filesystem::is_directory(argv[2])) {
      for (const auto& entry : std::filesystem::directory_iterator(argv[2])) {
        if (std::filesystem::is_regular_file(entry)) {
          file_paths.push_back(entry.path().string());
          files.push_back(entry.path().filename().string());
          img_archive->replace_archive_files(files, file_paths);
        }
      }
    }
    else {
      std::filesystem::path path(argv[2]);
      file_paths.push_back(argv[2]);
      files.push_back(path.filename().string());
      img_archive->replace_archive_files(files, file_paths);
    }
  }
  else
    ERR("First argument: " + String(argv[1]) + " must either be `-e` or `-r`.");

  DELETE_PTR(archive_file);
  DELETE_PTR(img_archive);

  return 0;
}
