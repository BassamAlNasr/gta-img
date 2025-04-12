/*
Architecture reverse engineered and partially documented by:
https://gtamods.com/wiki/IMG_archive

Rockstar Games used a .img CD image format as archive files for GTA III, GTA
Vice City, GTA San Andreas and GTA 4.

The file format is based on CD-ROM disk sectors. Every file in the image
archive must be sector aligned where the size of each sector is 2048 bytes.
Thus, file offsets and file sizes must be multiplied by 2048 bytes.

Each file in the archive must be accompanied with a header that stores
information about the respective file such as its name and size. Thus, the file
header is the entry data section of the files stored in the image. The file
content is the set of bytes that are subsequent to the entry data section.
Image archives vary; some image versions contain image headers at the entry of
the image archive (.img) and some images store file headers in a separate file,
referenced as the directory structure archive data (.dir).

Files are usually stored unsorted, uncompressed and linear (no directory tree).
There may not be any empty blocks between files in the archive, hence, files
are stored consecutively. This is because the game might read more than one
file with a single function call.

The game will look for the exact file names that it expects has been preserved
in its executable, thus, renaming archive files is disallowed.

VERSION 1 - GTA III & GTA Vice City (+ Bully: Scholarship Edition for PC)

The directory structure archive data and the file data is stored in gta3.dir
and gta3.img respectively. The directory archive (.dir) must have the same name
as the image archive (.img).

The directory archive is a set headers that store information about their
respective image archive file, in a sequence of 32-byte headers:

4  bytes - DWORD    - File offset (in sectors).
4  bytes - DWORD    - File size   (in sectors).
24 bytes - CHAR[24] - File name   (NULL terminated).

The total amount of headers can be calculated by dividing the total size of the
directory archive with 32.

The image archive solely stores the archive files in a purely raw format.

VERSION 2 - GTA San Andreas

The directory structure archive data (.dir) has in version 2 been merged with
the image archive (.img) as opposed to the seperation of the two in version 1.

8-byte image headers:

Image header:

4 bytes - CHAR[4] - "VER2"
4 bytes - DWORD   - Number of files

Followed by the image header is the set of files and their 32-byte file
headers:

File header:

4  bytes - DWORD    - File offset (in sectors).
2  bytes - WORD     - Stream size (in sectors).
2  bytes - WORD     - File size   (in sectors) (always 0).
24 bytes - CHAR[24] - File name   (NULL terminated).

The stream size is the actual file size, that is, the amount of sectors that
it extents, since the actual file size may not fit in the 2 byte memory chunk
that has been reserved for it, the game developers have compressed storing the
actual file size by dividing it with 2048 prior to storing it in the file
header.
The file size field is unused and hence always 0. It is believed that this
field would be used for compression of files, but never made its way into
production.

VERSION 3 - GTA IV

The games internal image parser does still deal with 2 kB buffers (sectors),
but images may now be encrypted. The image is encrypted if the initial 4 bytes
of the image seem invalid. The image can be decrypted by 16 repetitions of
AES-128 in ECB mode.

The image header of an unencrypted file has a size of 20 bytes:

4 bytes - DWORD - Identifier (0xA94E2A52 if the archive is unencrypted)
4 bytes - DWORD - Version (always 3)
4 bytes - DWORD - Number of items.
4 bytes - DWORD - Table size in bytes.
2 bytes - WORD  - Size of table items (always 16)
2 bytes - WORD  - Unknown

Each item header has a size of 16 bytes:

4 byte - DWORD - Item size in bytes.
4 byte - DWORD - Resource type.
4 byte - DWORD - Offset (in sectors).
2 byte - WORD  - Used Blocks.
2 byte - WORD  - Padding.

The resource type opcodes have the following meanings:

0x01: Generic.
0x08: Texture archive.
0x20: Bounds.
0x6E: Model file.
0x24: xpfl.

Decryption of GTA 4 files is further described here:
https://gtamods.com/wiki/Cryptography#GTA_IV

Archives and other files have their content stored as the byte-order
represented in the systems memory, thus when writing the following 4 byte
value: 0xA9 0x4E 0x2A 0x52 which is stored in virtual memory as:
0x52 0x2A 0x4E 0xA9 (least significant byte-order - little endianness), the
byte sequence in the file will then be written in the same order as the byte
order in memory: 0x52 0x2A 0x4E 0xA9. Vice versa, the byte sequence:
0x52 0x2A 0x4E 0xA9 will be stored in memory as 0xA9 0x4E 0x2A 0x52 if directly
read as raw bytes or assigned to a 4-byte sized type.
*/

#ifndef GTA_IMG_H
#define GTA_IMG_H

#include <iostream>
#include <string>
#include <vector>
#include <stdint.h>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <string.h>

#define SECTOR_SIZE 2048 // The size of each sector is 2048 bytes
#define HEADER_SIZE 32   // The size of each header is 32 bytes

#define IMG_HEADER_SIZE_V2 8

typedef uint8_t  UByte;
typedef uint16_t UWord;
typedef uint32_t UDWord;
typedef uint64_t UQWord;

typedef std::string String;

enum Version {
  vundef, v1, v2, v3
};

struct  HeaderV1 {
  UDWord offset;       // Offset   (in sectors)
  UDWord filesize;     // Size     (in sectors)
  char   filename[24]; // Filename (null terminated)
};

struct HeaderV2 {
  UDWord offset;       // Offset          (in sectors)
  UWord  streamsize;   // Streaming size  (in sectors)
  UWord  filesize;     // Size in archive (in sectors) (always 0)
  char   filename[24]; // Filename        (null terminated)
};

struct HeaderV3 {
  UDWord size;    // Item size in bytes
  UDWord type;    // Resource type
  UDWord ofset;   // Offset (in sectors)
  UWord  blocks;  // Number of used blocks
  UWord  padding; // Padding size
};

struct File {
  union {
    HeaderV1* headerV1;
    HeaderV2* headerV2;
    HeaderV3* headerV3;
  };
  UQWord offset;  // Actual file offset
  UQWord size;    // Actual file size
  UByte* content; // File content
  ~File();
};

class IMGArchive {
  public:
    IMGArchive(String path, String dir_path = "");
    ~IMGArchive();

    Version version = vundef;

    UDWord num_files();
    File* get_archive_file(UDWord id);
    File* get_archive_file(String filename);
    uint  copy_file_from_img(String filename, String dest);
    uint  replace_archive_files(std::vector<String> old_file, std::vector<String> new_file);
  private:
    uint open_archive();
    uint open_archive_v1();
    uint open_archive_v2();
    void get_archive_file_v1(UDWord id, File* archive_file);
    void get_archive_file_v2(UDWord id, File* archive_file);
    template <typename T>
    static UDWord get_file_idx(std::vector<T>& v, const String& file);
    bool replace_archive_files_v1(std::vector<String> old_files, std::vector<String> new_files);
    bool replace_archive_files_v2(std::vector<String> old_files, std::vector<String> new_files);
    bool read_from_file(const char* filename, long off, UByte* dst, size_t size);
    bool write_to_archive_v1(File* file, UDWord id);
    bool write_to_archive_v2(File* file, UDWord id);
    FILE*   img;
    String  img_path;
    String  dir_path;
    union {
      std::vector<HeaderV1> archive_V1;
      std::vector<HeaderV2> archive_V2;
      std::vector<HeaderV3> archive_V3;
    };
};

#endif

