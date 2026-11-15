// Shared helpers for the C++ tests (the counterpart of tests/python/conftest.py).
#ifndef URLPARSER_TEST_CONFTEST_H
#define URLPARSER_TEST_CONFTEST_H
#include <filesystem>
#include <ostream>
#include <string>

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

#endif // URLPARSER_TEST_CONFTEST_H