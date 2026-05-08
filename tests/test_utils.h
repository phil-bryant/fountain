#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <sstream>
#include <string>

inline std::string TestDbPath(const std::string &prefix) {
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937_64 gen(static_cast<std::uint64_t>(now));
    std::uniform_int_distribution<std::uint64_t> dist;
    std::ostringstream oss;
    oss << "/tmp/" << prefix << "_" << now << "_" << dist(gen) << ".sqlite3";
    return oss.str();
}

inline void RemoveFileIfExists(const std::string &path) {
    std::remove(path.c_str());
}
