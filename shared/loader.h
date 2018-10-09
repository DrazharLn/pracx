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

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LOADER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LOADER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LOADER_EXPORTS
#define LOADER_API __declspec(dllexport)
#else
#define LOADER_API __declspec(dllimport)
#endif

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
LOADER_API int init();

/**
 * Replace the destination of an asm `call`.
 *
 * @param addr the argument to an asm call rel32 or jmp rel32.
 *
 * @return the old value of `addr` or a fail condition
 */
LOADER_API int redirect_call(int addr, void* func_ptr);

/**
 * Replace the destination of an asm `extrn`.
 *
 * @return the old `extrn` value
 */
LOADER_API int replace_extrn(int addr, void* func_ptr);

/**
 * Replace the contents of `addr` with the value `ptr`.
 *
 * @return the old value of `addr`
 */
LOADER_API int replace_ptr(int addr, void* ptr);

/**
 * Write `bytes` to memory starting at `addr`.
 * 
 * TODO: is there a nice string library that will save users from having to specify their own lengths?
 */
LOADER_API bool replace_bytes(int addr, char* bytes, int len);

}
/**
 * PRACX compat:
 *
 * astApi -> replace_extern
 * astHooks -> redirect_call
 * astChanges -> replace_ptr and replace_bytes
 * aiZoomDetailChanges -> replace_bytes
 */
