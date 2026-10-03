#include <CppUnitTest.h>

#include <thread>

#include "Infrastructure/ComApartment.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

TEST_CLASS(ComApartmentTests) {
public:
    // Runs on a fresh thread: the test host thread may already belong to another apartment.
    TEST_METHOD(InitializesStaOnFreshThread) {
        bool initialized = false;
        std::thread worker([&initialized] {
            const infra::ComApartment apartment;
            initialized = apartment.initialized();
        });
        worker.join();
        Assert::IsTrue(initialized);
    }
};

}  // namespace et::tests
