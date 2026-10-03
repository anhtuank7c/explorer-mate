#pragma once

#include <string>

#include "Application/ActionRequest.h"
#include "Domain/Result.h"

namespace et::infra {

// Hands a request from the shell extension to the worker process through a UTF-8 file in a
// per-user folder. Only the file path travels on the command line, so the selection size is
// not limited by command-line length and never needs shell quoting.
class RequestFileStore {
public:
    // `directory` is where request files live, normally <ProductDataDirectory>\requests.
    explicit RequestFileStore(std::wstring directory);

    // Writes a new request file and returns its full path.
    domain::Result<std::wstring> Put(const app::ActionRequest& request) const;

    // Reads, deletes and parses a request file. Refuses paths outside the store's directory,
    // oversized files and anything that does not parse.
    domain::Result<app::ActionRequest> Take(const std::wstring& path) const;

    // Deletes request files older than `maxAgeSeconds`, left behind by a worker that never ran.
    void RemoveStale(unsigned maxAgeSeconds) const;

private:
    std::wstring directory_;
};

}  // namespace et::infra
