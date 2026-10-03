#include <windows.h>

#include "ShellExtension/WrlModule.h"

using Microsoft::WRL::InProc;
using Microsoft::WRL::Module;

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

// The annotations repeat the SDK's declarations (combaseapi.h) so static analysis sees one
// consistent contract.
__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow() {
    return Module<InProc>::GetModule().Terminate() ? S_OK : S_FALSE;
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID classId, _In_ REFIID interfaceId, _Outptr_ LPVOID* object) {
    return Module<InProc>::GetModule().GetClassObject(classId, interfaceId, object);
}
