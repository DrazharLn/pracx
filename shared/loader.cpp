/**
 * Public API documented in header.
 */

#include "loader.h"
#include <set>
#include <iostream>
#include <fstream>

#define DEBUG 1
#define log(msg) \
	do { if (DEBUG) {\
		std::ofstream logfile;\
		logfile.open("loader.log", std::ios::app);\
		logfile << GetTickCount() << ":" << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\t" << msg << std::endl;\
		logfile.close();\
	} } while (0)

std::set<int> PATCHED_ADDRESSES;

bool is_patched(int start, int len) {
    for (int i = start; i < start + len; i++) {
        if (PATCHED_ADDRESSES.count(i) == 1)
            return true;
    }
    return false;
}

/**
 * False iff a patch cannot be registered because another has been registered for that address
 */
bool register_patch(int start, int len) {
    if (is_patched(start, len))
        return false;

    for (int i = start; i < start + len; i++)
        PATCHED_ADDRESSES.insert(i);
    return true;
}

extern "C" {

typedef void (*patch_init_func_type)(HMODULE self);

DLL_EXPORT int init() {
    log("");

    // Load PRACX and call init
    HMODULE pracx = LoadLibraryA("prax");
    patch_init_func_type patch_init =
        (patch_init_func_type)(GetProcAddress(pracx, (LPCSTR)1));
    patch_init(pracx);

    return false;
}

DLL_EXPORT int redirect_call(int addr, void* func_ptr) {
    // Override relative JMP or CALLs by calculating the offset to our functions and replacing
    // the operand.
    int offset = (int)func_ptr - (addr + 4); // +4 because displacement relative to next instruction.
    return replace_ptr(addr, (void*)offset);
}

DLL_EXPORT int replace_extrn(int addr, void* func_ptr) {
    // Override API references (`extrn`s) by replacing the addresses.
    return replace_ptr(addr, func_ptr);
}

DLL_EXPORT int replace_ptr(int addr, void* ptr) {
    if (!register_patch(addr, 4))
        return 0;

    int old = *(int*)addr;

    DWORD oldProtection;
    VirtualProtect((LPVOID)addr, 4, PAGE_READWRITE, &oldProtection);
    *(int*)addr = (int)ptr;
    VirtualProtect((LPVOID)addr, 4, oldProtection, &oldProtection);
    return old;
}

DLL_EXPORT bool replace_bytes(int addr, char* bytes, int len) {
    // Destination and source must not overlap
    if ((int)bytes > addr && (int)bytes < addr+len)
        return false;

    if (!register_patch(addr, len))
        return false;

    DWORD oldProtection;
    VirtualProtect((LPVOID)addr, len, PAGE_READWRITE, &oldProtection);
    memcpy((LPVOID)addr, bytes, len);
    VirtualProtect((LPVOID)addr, len, oldProtection, &oldProtection);

    return true;
}

}
