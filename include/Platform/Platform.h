#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    // ========================================================
    // Linux / Non-Windows Compatibility Definitions
    // ========================================================
    #include <cstdint>
    #include <cstddef>
    #include <cstdlib>
    #include <cstring>
    #include <strings.h>
    #include <cassert>
    #include <cstdarg>
    #include <unistd.h>

    // Basic Win32 Scalar Types
    typedef uint32_t      DWORD;
    typedef uint16_t      WORD;
    typedef uint8_t       BYTE;
    typedef int32_t       BOOL;
    typedef int32_t       LONG;
    typedef uint32_t      ULONG;
    typedef uint32_t      UINT;
    typedef int32_t       INT;
    typedef int64_t       LONGLONG;
    typedef uint64_t      ULONGLONG;
    typedef int64_t       __int64;
    typedef void          VOID;

    typedef BYTE*         LPBYTE;
    typedef WORD*         LPWORD;
    typedef DWORD*        LPDWORD;

    #ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3) \
        ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
        ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
    #endif

    // Handles & Pointers
    typedef void*         HANDLE;
    typedef void*         HWND;
    typedef void*         HINSTANCE;
    typedef void*         HMODULE;
    typedef void*         HDC;
    typedef void*         HFONT;
    typedef void*         HBITMAP;
    typedef void*         HRGN;
    typedef void*         HMENU;
    typedef void*         LPVOID;
    typedef const void*   LPCVOID;

    // Strings
    typedef char*         LPSTR;
    typedef const char*   LPCSTR;
    typedef wchar_t*      LPWSTR;
    typedef const wchar_t* LPCWSTR;

    // Win32 COM / HRESULT
    typedef int32_t       HRESULT;
    #ifndef S_OK
        #define S_OK ((HRESULT)0L)
    #endif
    #ifndef S_FALSE
        #define S_FALSE ((HRESULT)1L)
    #endif
    #ifndef E_FAIL
        #define E_FAIL ((HRESULT)0x80004005L)
    #endif
    #ifndef SUCCEEDED
        #define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
    #endif
    #ifndef FAILED
        #define FAILED(hr) (((HRESULT)(hr)) < 0)
    #endif

    // Win32 Constants
    #ifndef TRUE
        #define TRUE 1
    #endif
    #ifndef FALSE
        #define FALSE 0
    #endif
    #ifndef MAX_PATH
        #define MAX_PATH 260
    #endif
    #ifndef _MAX_PATH
        #define _MAX_PATH MAX_PATH
    #endif
    #ifndef INVALID_HANDLE_VALUE
        #define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
    #endif

    #ifndef MB_OK
        #define MB_OK 0x00000000L
    #endif
    #ifndef IDOK
        #define IDOK 1
    #endif

    inline int MessageBoxA(HWND, LPCSTR, LPCSTR, UINT) { return IDOK; }
    #ifndef MessageBox
        #define MessageBox MessageBoxA
    #endif
    inline void OutputDebugStringA(LPCSTR) {}
    #ifndef OutputDebugString
        #define OutputDebugString OutputDebugStringA
    #endif

    inline BOOL DeleteFileA(LPCSTR lpFileName) { return unlink(lpFileName) == 0; }
    #ifndef DeleteFile
        #define DeleteFile DeleteFileA
    #endif

    // Calling Conventions & Compiler Attributes
    #define CALLBACK
    #define WINAPI
    #define __stdcall
    #define __cdecl
    #ifndef __forceinline
        #define __forceinline inline __attribute__((always_inline))
    #endif

    // Memory Allocation Helpers
    #define GMEM_FIXED    0x0000
    #define GMEM_MOVEABLE 0x0002
    #define GMEM_ZEROINIT 0x0040
    inline void* GlobalAlloc(UINT uFlags, size_t dwBytes) { (void)uFlags; return malloc(dwBytes); }
    inline void* GlobalFree(void* hMem) { free(hMem); return NULL; }
    inline void* GlobalLock(void* hMem) { return hMem; }
    inline BOOL  GlobalUnlock(void* hMem) { (void)hMem; return FALSE; }

    #ifndef CopyMemory
        #define CopyMemory(dest, src, count) memcpy((dest), (src), (count))
    #endif
    #ifndef ZeroMemory
        #define ZeroMemory(dest, count) memset((dest), 0, (count))
    #endif
    #ifndef MoveMemory
        #define MoveMemory(dest, src, count) memmove((dest), (src), (count))
    #endif

    typedef char          CHAR;
    typedef char*         PCHAR;
    typedef unsigned char* PUCHAR;
    typedef void*         PVOID;
    typedef int           BOOLEAN;

    // String Case-Insensitive Helpers
    #ifndef _stricmp
        #define _stricmp strcasecmp
    #endif
    #ifndef stricmp
        #define stricmp strcasecmp
    #endif
    #ifndef _strnicmp
        #define _strnicmp strncasecmp
    #endif
    #ifndef strnicmp
        #define strnicmp strncasecmp
    #endif
    #ifndef _snprintf
        #define _snprintf snprintf
    #endif
    #ifndef _vsnprintf
        #define _vsnprintf vsnprintf
    #endif
    #ifndef _getcwd
        #define _getcwd getcwd
    #endif
    #ifndef _access
        #define _access access
    #endif

    // Structs
    typedef struct tagRECT {
        LONG left;
        LONG top;
        LONG right;
        LONG bottom;
    } RECT, *PRECT, *LPRECT;

    typedef struct tagPOINT {
        LONG x;
        LONG y;
    } POINT, *PPOINT, *LPPOINT;

    typedef struct tagSIZE {
        LONG cx;
        LONG cy;
    } SIZE, *PSIZE, *LPSIZE;

    #ifndef FILE_ATTRIBUTE_DIRECTORY
        #define FILE_ATTRIBUTE_DIRECTORY 0x00000010
    #endif
    #ifndef FILE_ATTRIBUTE_NORMAL
        #define FILE_ATTRIBUTE_NORMAL 0x00000080
    #endif

    typedef struct _WIN32_FIND_DATAA {
        DWORD dwFileAttributes;
        char cFileName[MAX_PATH];
    } WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
    typedef WIN32_FIND_DATAA WIN32_FIND_DATA;

#endif
