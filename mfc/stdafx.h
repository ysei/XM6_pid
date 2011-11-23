#define WINVER					0x500
#define _WIN32_WINNT			0x501
#define _CRT_SECURE_NO_WARNINGS
#define VC_EXTRALEAN

#define DIRECTSOUND_VERSION		0x500	// DirectSound5
#define DIRECTINPUT_VERSION		0x0800	// DirectInput8

// MFC
#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxconv.h>

// Win32API
#include <imm.h>
#include <mmsystem.h>
#include <shlobj.h>

// DirectX
#include <dsound.h>
#include <dinput.h>

// C Runtime
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
