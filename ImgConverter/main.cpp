
#include <filesystem>
#include <iostream>
#include <string_view>

#include <bmp_image.h>
#include <jpeg_image.h>
#include <img_lib.h>
#include <ppm_image.h>

using namespace std;

enum class Format {JPEG, PPM, BMP, UNKNOWN};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }

    if (ext == ".ppm"sv) {
        return Format::PPM;
    }

    if (ext == ".bmp"sv) {
        return Format::BMP;
    }

    return Format::UNKNOWN;
}

class ImageFormatInterface {
public:
    virtual ~ImageFormatInterface() = default;
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

class FormatPPM : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class FormatJPEG : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};

class FormatBMP : public ImageFormatInterface {
public:
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }

    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadBMP(file);
    }
};


ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    Format format = GetFormatByExtension(path);

    static FormatPPM kFormatPPM;
    static FormatJPEG kFormatJPEG;
    static FormatBMP kFormatBMP;

    switch (format) {
        case Format::JPEG : return &kFormatJPEG;
        case Format::PPM : return &kFormatPPM;
        case Format::BMP : return &kFormatBMP;
    }

    return nullptr;
}

int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    ImageFormatInterface* in_format = GetFormatInterface(in_path);
    if (!in_format) {
        cerr << "Unknown format of the input file" << endl;
        return 2;
    }

    ImageFormatInterface* out_format = GetFormatInterface(out_path);
    if (!out_format) {
        cerr << "Unknown format of the output file" << endl;
        return 3;
    }
  
    img_lib::Image image = in_format->LoadImage(in_path);

    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    if (!out_format->SaveImage(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
}