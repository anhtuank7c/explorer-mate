#pragma once

namespace et::infra {

// Owns one COM single-threaded apartment for the current thread. IFileOperation and its
// progress UI require an STA with a message pump.
class ComApartment {
public:
    ComApartment();
    ~ComApartment();

    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;

    bool initialized() const { return initialized_; }

private:
    bool initialized_;
};

}  // namespace et::infra
