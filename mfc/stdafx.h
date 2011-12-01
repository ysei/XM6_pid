#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>

//#include <string.h>
extern "C" void* memset(void* dst, int c, size_t length);
extern "C" void* memcpy(void* dst, const void* src, size_t length);
extern "C" int memcmp(const void* s1, const void* s2, size_t length);

//#include <assert.h>
#ifdef  NDEBUG
#define assert(_Expression)     ((void)0)
#else
extern "C" _CRTIMP void __cdecl _wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);
#define assert(_Expression) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )
#endif

//#define ASSERT	assert
