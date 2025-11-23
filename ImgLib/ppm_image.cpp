#include <fstream>
#include <string_view>
#include <sstream>

#include "ppm_image.h"

using namespace std;

namespace img_lib {

static const string_view PPM_SIG = "P6"sv;
static const int PPM_MAX = 255;

bool SavePPM(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);

    if (!out) return false;
    
    int w = image.GetWidth();
    int h = image.GetHeight();

    // Чтобы минимизировать кол-во записей в файл, заголовок сначала формируется в виде строки в ostringstream
    ostringstream header;
    header << PPM_SIG << '\n' << w << ' ' << h << '\n' << PPM_MAX << '\n';
    
    out << header.str();

    if (!out) return false;

    vector<unsigned char> buffer(w * 3);
    for (int i = 0; i < h; ++i) {
        auto line = image.GetLine(i);

        for (int j = 0; j < w; ++j) {
            buffer[j * 3 + 0] = static_cast<unsigned char>(line[j].r);
            buffer[j * 3 + 1] = static_cast<unsigned char>(line[j].g);
            buffer[j * 3 + 2] = static_cast<unsigned char>(line[j].b);
        }

        out.write(reinterpret_cast<char*>(buffer.data()), buffer.size());

        if (!out) return false;
    }

    return true;
}

Image LoadPPM(const Path& file) {
    ifstream ifs(file, ios::binary);
    
    if (!ifs) return {};

    std::string sign;
    int w, h, color_max;

    // читаем заголовок: он содержит формат, размеры изображения
    // и максимальное значение цвета
    ifs >> sign >> w >> h >> color_max;

    if (!ifs) return {};

    // мы поддерживаем изображения только формата P6
    // с максимальным значением цвета 255
    if (sign != PPM_SIG || color_max != PPM_MAX) {
        return {};
    }

    // пропускаем один байт - это конец строки
    const char next = ifs.get();
    if (next != '\n') return {};

    Image result(w, h, Color::Black());
    std::vector<char> buff(w * 3);

    for (int y = 0; y < h; ++y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), w * 3);

        if (!ifs) return {};

        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].b = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}
}  // namespace img_lib