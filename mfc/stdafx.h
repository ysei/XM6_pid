#define WINVER					0x500
#define _WIN32_WINNT			0x501
#define _CRT_SECURE_NO_WARNINGS
#define VC_EXTRALEAN

#define DIRECTSOUND_VERSION		0x500	// DirectSound5
#define DIRECTINPUT_VERSION		0x0800	// DirectInput8

#include <stdint.h>

//#include <string.h>
extern "C" void* memset(void* dst, int c, size_t length);
extern "C" int memcmp(const void* s1, const void* s2, size_t length);
extern "C" void* memcpy(void* dst, const void* src, size_t length);

extern "C" char* strcpy(char* dst, const char* src);
extern "C" int strcmp(const char* s1, const char* s2);
extern "C" size_t strlen(const char* s);
//extern "C" int _strnicmp(const char* s1, const char* s2, size_t length);	// string.h

//#include <assert.h>
#ifdef  NDEBUG
#define assert(_Expression)     ((void)0)
#else
extern "C" _CRTIMP void __cdecl _wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);
#define assert(_Expression) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )
#endif

#define ASSERT	assert
