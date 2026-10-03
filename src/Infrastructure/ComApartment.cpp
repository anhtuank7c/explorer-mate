#include "Infrastructure/ComApartment.h"

#include <objbase.h>

namespace et::infra {

ComApartment::ComApartment()
    : initialized_(SUCCEEDED(
          CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {}

ComApartment::~ComApartment() {
    if (initialized_) {
        CoUninitialize();
    }
}

}  // namespace et::infra
