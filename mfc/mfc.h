//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_h)
#define mfc_h

#pragma warning(disable : 4127)
#pragma warning(disable : 4702)

//---------------------------------------------------------------------------
//
//	#include
//
//---------------------------------------------------------------------------
#define WINVER					0x500	// Windows98,Me,2000,XP�ȍ~
//VC2010//	#define _WIN32_WINNT			0x410	// Windows98,Me,2000,XP�ȍ~
#define _WIN32_WINNT			0x501	//VC2010//
#define _CRT_SECURE_NO_WARNINGS			//VC2010//
#define VC_EXTRALEAN

// MFC
#if defined(_AFXDLL)
#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxconv.h>
#else
#include <assert.h>
#endif

// Win32API
#include <windows.h>
#include <imm.h>
#include <mmsystem.h>
#include <shlobj.h>

// DirectX
#define DIRECTSOUND_VERSION		0x500	// DirectX5���w��
#include <dsound.h>
//VC2010//#define DIRECTINPUT_VERSION		0x500	// DirectX5���w��
#define DIRECTINPUT_VERSION		0x0800	//VC2010//	// DirectX8���w��
#include <dinput.h>

// C Runtime
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

//---------------------------------------------------------------------------
//
//	#define
//
//---------------------------------------------------------------------------
//#if defined(_DEBUG)
//#define new		DEBUG_NEW
//#endif	// _DEBUG

#if defined(_MSC_VER) && defined(_M_IX86)
#define FASTCALL	__fastcall
#else
#define FASTCALL
#endif	// _MSC_VER

//---------------------------------------------------------------------------
//
//	�N���X�錾
//
//---------------------------------------------------------------------------
class VM;								// ���z�}�V��
class CApp;								// �A�v���P�[�V����
class CFrmWnd;							// �t���[���E�B���h�E

void FASTCALL LockVM(void);																			// ���z�}�V�������b�N
void FASTCALL UnlockVM(void);																		// ���z�}�V�����A�����b�N

#endif	// mfc_h
#endif	// _WIN32
