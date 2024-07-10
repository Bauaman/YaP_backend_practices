#include "aux.h"

namespace http_handler {

    const std::unordered_map<std::string, std::string_view> ContentType::DICT = {
    {".htm", "text/html"},
    {".html", "text/html"},
    {"html", "text/html"},
    {".css", "text/css"},
    {".txt", "text/plain"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {"json", "application/json"},  
    {".xml", "application/xml"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpe", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/vnd.microsoft.icon"},
    {".tiff", "image/tiff"},
    {".tif", "image/tiff"},
    {".svg", "image/svg+xml"},
    {".svgz", "image/svg+xml"},
    {".mp3", "audio/mpeg"},
    };

}

namespace auxillary {

std::string UrlDecode(const std::string& str) {
    std::ostringstream decoded;
    std::istringstream encoded(str);
    encoded >> std::noskipws;

    char current;
    while (encoded >> current) {
        if (current == '%') {
            int hexCode;
            if (!(encoded >> std::hex >> hexCode)) {
                decoded << '%';
            } else {
                decoded << static_cast<char>(hexCode);
            }
        } else if (current == '+') {
            decoded << ' ';
        } else {
            decoded << current;
        }
    }
    return decoded.str();
}

bool IsSubPath(fs::path base, fs::path path) {
    fs::path combined_path = base / path;
    fs::path canonical_path = fs::weakly_canonical(path);

    return canonical_path.string().find(base.string()) == 0;
}

}