#pragma once

#include <string>
#include <utility>

namespace et::domain {

enum class ErrorCode {
    InvalidArgument,
    EmptySelection,
    MixedParents,
    UnsupportedLocation,
    ItemNotFound,
    InvalidName,
    NameCollision,
    Cancelled,
    AccessDenied,
    OperationFailed,
};

struct Error {
    ErrorCode code;
    std::wstring message;

    Error(ErrorCode errorCode, std::wstring errorMessage)
        : code(errorCode), message(std::move(errorMessage)) {}
};

}  // namespace et::domain
