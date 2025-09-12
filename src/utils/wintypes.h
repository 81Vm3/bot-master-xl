#ifndef WINTYPES_H
#define WINTYPES_H

#ifndef _WIN32

#include <stdint.h>

// Windows type definitions for Linux compatibility
typedef uint8_t BYTE;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;

typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;

typedef char* PCHAR;
typedef const char* PCCHAR;
typedef void* PVOID;
typedef const void* PCVOID;

typedef unsigned long ULONG;
typedef long LONG;

// Boolean definitions
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef BOOL
typedef int BOOL;
#endif

// Handle definitions
typedef void* HANDLE;

// Common constants
#ifndef NULL
#define NULL 0
#endif

// Include RakNet enums - remove our conflicting definitions
// RakNet will provide the proper enum values for:
// - PacketPriority (SYSTEM_PRIORITY, HIGH_PRIORITY, etc.)
// - PacketReliability (UNRELIABLE, RELIABLE, etc.)
// - NetworkID
#else

#include <windows.h>
#endif

#endif // WINTYPES_H
