#define WINVER					0x500
#define _WIN32_WINNT			0x501
#define _CRT_SECURE_NO_WARNINGS
#define VC_EXTRALEAN

#define DIRECTSOUND_VERSION		0x500	// DirectSound5
#define DIRECTINPUT_VERSION		0x0800	// DirectInput8

// MFC
#if defined(_AFXDLL)
#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxconv.h>
#endif

// Win32API
#include <windows.h>
#include <imm.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <tchar.h>

// DirectX
#include <dsound.h>
#include <dinput.h>

// C Runtime
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
