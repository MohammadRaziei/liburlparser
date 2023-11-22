#ifndef MYTEST_COMMON_H
#define MYTEST_COMMON_H
#include <filesystem>

template <typename=void>
std::string makeAbsolutePath(const std::string& relativePath) {
    std::filesystem::path currentPath(__FILE__);
    std::filesystem::path absolutePath = currentPath.parent_path().parent_path() / relativePath;
    return absolutePath.string();
}

struct BaseData {
    virtual std::string toString() const = 0;
    friend std::ostream& operator<<(std::ostream& os, const BaseData& data) {
        return os << data.toString();
    }
};

#endif // MYTEST_COMMON_H