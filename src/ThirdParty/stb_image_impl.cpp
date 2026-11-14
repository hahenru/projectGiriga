#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#define STBI_ONLY_BMP
#define STBI_ONLY_PSD
#define STBI_ONLY_HDR
#include <stb/stb_image.h>

/**
 * 這個檔案的用途是編譯全部的stb_image符號
 * 你很有可能再也不用打開
 * 除非你打算新增更多stb系列的依賴
 */
