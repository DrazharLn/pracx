/**
 * Common loader for SMACX patches
 *
 * To be used by PRACX, Thinker, etc.
 *
 * Each function modifies SMACX's runtime memory, generally returning the old value. Loader checks
 * that patches do not overwrite the same locations in memory.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define DLL_EXPORT __declspec(dllexport)

typedef struct {
    char* name;
    HMODULE handle;
} Patch;

extern "C" {

/**
 * Find and call each patch dll.
 *
 * patch dlls are expected to export a function init() that calls the functions declared here.
 *
 * loader:init() will call each patch init() with:
 * 	loader_handle: the hModule for this dll for use with GetProcAddress,
 * 	len_patches,
 * 	patches: an array of Patch structs
 */
DLL_EXPORT int init();

/**
 * Replace the destination of an asm `call`.
 *
 * @param addr the argument to an asm call rel32 or jmp rel32.
 *
 * @return the old value of `addr` or a fail condition
 */
DLL_EXPORT int redirect_call(int addr, void* func_ptr);

/**
 * Replace the destination of an asm `extrn`.
 *
 * @return the old `extrn` value
 */
DLL_EXPORT int replace_extrn(int addr, void* func_ptr);

/**
 * Replace the contents of `addr` with the value `ptr`.
 *
 * @return the old value of `addr`
 */
DLL_EXPORT int replace_ptr(int addr, void* ptr);

/**
 * Write `bytes` to memory starting at `addr`.
 * 
 * TODO: is there a nice string library that will save users from having to specify their own lengths?
 */
DLL_EXPORT bool replace_bytes(int addr, char* bytes, int len);

}
/**
 * PRACX compat:
 *
 * astApi -> replace_extern
 * astHooks -> redirect_call
 * astChanges -> replace_ptr and replace_bytes
 * aiZoomDetailChanges -> replace_bytes
 */
