#include <filesystem>
#include <cassert>

using namespace std::literals;
namespace fs = std::filesystem;

// Возвращает true, если каталог p содержится внутри base_path.
bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

int main() {
    assert(IsSubPath("/root/path/sub-folder/file"s, "/root/path"s));
    assert(IsSubPath("/root/path/sub-folder/../file"s, "/root/path"s));
    assert(!IsSubPath("/root/path/../file"s, "/root/path"s));
}