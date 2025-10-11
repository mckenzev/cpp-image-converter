#include "ppm_image.h"

#include <fstream>
#include <vector>
#include <setjmp.h>
#include <stdio.h>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void) my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

FILE* OpenFileForWrite(const Path& file) {
#ifdef _MSC_VER
    return _wfopen(file.wstring().c_str(), "wb");
#else
    return fopen(file.string().c_str(), "wb");
#endif
}

FILE* OpenFileForRead(const Path& file) {
#ifdef _MSC_VER
    return _wfopen(file.wstring().c_str(), "rb");
#else
    return fopen(file.string().c_str(), "rb");
#endif
}

bool SaveJPEG(const Path& file, const Image& image) {
    FILE* outfile = OpenFileForWrite(file);

    if (!outfile) {
        return false;
    }

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = image.GetWidth();
    cinfo.image_height = image.GetHeight();
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    vector<unsigned char> buffer(3 * image.GetWidth());
    JSAMPROW row_pointer[1] = { buffer.data() };

    while (cinfo.next_scanline < cinfo.image_height) {
        int y = cinfo.next_scanline;
        const Color* line = image.GetLine(y);
        
        for (int i = 0; i < image.GetWidth(); ++i) {
            buffer[i * 3 + 0] = static_cast<unsigned char>(line[i].r);
            buffer[i * 3 + 1] = static_cast<unsigned char>(line[i].g);
            buffer[i * 3 + 2] = static_cast<unsigned char>(line[i].b);
        }

        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);

    return true;
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveScanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

Image LoadJPEG(const Path& file) {
    FILE* infile = OpenFileForRead(file);

    if (!infile) {
        return {};
    }

    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void) jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    (void) jpeg_start_decompress(&cinfo);
    
    int row_stride = cinfo.output_width * cinfo.output_components;
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);
        SaveScanlineToImage(buffer[0], y, result);
    }

    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib