#include "bmp_image.h"
#include "pack_defines.h"

#include <cinttypes>
#include <fstream>
#include <vector>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    char signature[2] = {'B', 'M'};
    uint32_t file_size;
    uint32_t reserved = 0u;
    uint32_t pixel_array_offset = 54u;
}
PACKED_STRUCT_END

// Заголовок BitmapFileHeader должен занимать 14 байт
static_assert(sizeof(BitmapFileHeader) == 14u);

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size = 40u;
    int32_t width;
    int32_t height;
    uint16_t planes = 1u;
    uint16_t bits_per_pixel = 24u;
    uint32_t compression = 0u;
    uint32_t image_size;
    int32_t h_resolution = 11811u;
    int32_t v_resolution = 11811u;
    int32_t colors_used = 0;
    int32_t colors_important = 0x1000000;
}
PACKED_STRUCT_END

// Заголовок BitmapInfoHeader должен занимать 40 байт
static_assert(sizeof(BitmapInfoHeader) == 40u);

// Функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}


bool SaveBMP(const Path& file, const Image& image) {
    ofstream fstream(file, ios::binary);

    if (!fstream) return false;

    int w = image.GetWidth();
    int h = image.GetHeight();

    BitmapInfoHeader info_header;
    info_header.width = w;
    info_header.height = h;
    info_header.image_size = GetBMPStride(w) * h;

    BitmapFileHeader file_header;
    file_header.file_size = info_header.image_size + file_header.pixel_array_offset;

    fstream.write(reinterpret_cast<char*>(&file_header), 14);

    if (!fstream) return false;

    fstream.write(reinterpret_cast<char*>(&info_header), 40);

    if (!fstream) return false;

    vector<unsigned char> buffer(GetBMPStride(w));

    for (int i = h; i-- > 0;) {
        // т.к. в bmp строки идут снизу вверх, строки Image будут так же заполняться снизу вверх
        const Color* colors_line = image.GetLine(i);

        for (int j = 0; j < w; ++j) {
            int buf_idx = j * 3;
            const Color& pixel = colors_line[j];
            
            // формат пикселя в bmp - BGR
            buffer[buf_idx + 2] = static_cast<unsigned char>(pixel.r);
            buffer[buf_idx + 1] = static_cast<unsigned char>(pixel.g);
            buffer[buf_idx + 0] = static_cast<unsigned char>(pixel.b);
        }

        fstream.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

        if (!fstream) return false;
    }

    return true;
}

namespace {
    bool IsCorrectFileHeader(BitmapFileHeader header) {
        return string_view(header.signature, sizeof(header.signature)) == "BM"sv
            && header.pixel_array_offset == 54u;
    }

    bool IsCorrectInfoHeader(const BitmapInfoHeader& header) {
        return header.header_size == 40u
            && header.width > 0
            && header.height > 0
            && header.planes == 1u
            && header.bits_per_pixel == 24u
            && header.compression == 0u;
    }

}

Image LoadBMP(const Path& file) {
    ifstream fstream(file, ios::binary);
    if (!fstream) return {};

    BitmapFileHeader file_header;
    fstream.read(reinterpret_cast<char*>(&file_header), 14);

    if (!fstream || !IsCorrectFileHeader(file_header)) {
        return {};
    }

    BitmapInfoHeader info_header;
    fstream.read(reinterpret_cast<char*>(&info_header), 40);

    if (!fstream || !IsCorrectInfoHeader(info_header)) {
        return {};
    }

    int w = info_header.width;
    int h = info_header.height;
    Image result(w, h, Color::Black());

    vector<unsigned char> buffer(GetBMPStride(w));

    for (int i = h; i-- > 0;) {
        // т.к. в bmp строки идут снизу вверх, строки Image будут так же заполняться снизу вверх
        Color* colors_line = result.GetLine(i);

        fstream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

        if (!fstream) return {};

        for (int j = 0; j < w; ++j) {
            int buf_idx = j * 3;
            // формат пикселя в bmp - BGR
            colors_line[j] = Color{
                .r = byte{buffer[buf_idx + 2]},
                .g = byte{buffer[buf_idx + 1]},
                .b = byte{buffer[buf_idx + 0]}
            };
        }
    }
    
    return result;
}

}  // namespace img_lib