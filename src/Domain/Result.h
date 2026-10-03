#pragma once

#include <utility>
#include <variant>

#include "Domain/Error.h"

namespace et::domain {

// Value-or-error return type. std::expected needs C++23; the project is pinned to C++20.
template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value) : state_(std::move(value)) {}
    Result(Error error) : state_(std::move(error)) {}

    bool ok() const { return std::holds_alternative<T>(state_); }
    explicit operator bool() const { return ok(); }

    const T& value() const& { return std::get<T>(state_); }
    T&& value() && { return std::get<T>(std::move(state_)); }
    const Error& error() const { return std::get<Error>(state_); }

private:
    std::variant<T, Error> state_;
};

// For operations that either succeed or fail without producing a value.
struct Unit {};
using Status = Result<Unit>;

}  // namespace et::domain
