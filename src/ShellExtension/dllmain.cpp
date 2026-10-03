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

STDAPI DllCanUnloadNow() {
    return Module<InProc>::GetModule().Terminate() ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID classId, REFIID interfaceId, LPVOID* object) {
    return Module<InProc>::GetModule().GetClassObject(classId, interfaceId, object);
}
