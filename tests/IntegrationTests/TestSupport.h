#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include "Infrastructure/ComApartment.h"

namespace et::tests {

// Runs `body` on a fresh STA thread and returns its result. IFileOperation needs an STA, and
// the test host thread's apartment is not ours to choose. Assertions must stay on the
// calling thread: the test framework reports failures by throwing.
template <typename Body>
auto RunInSta(Body body) -> std::invoke_result_t<Body> {
    std::invoke_result_t<Body> result{};
    std::thread worker([&] {
        const infra::ComApartment apartment;
        result = body();
    });
    worker.join();
    return result;
}

inline void WriteText(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << content;
}

inline std::string ReadText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

}  // namespace et::tests
