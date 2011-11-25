//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_h)
#define mfc_h

//---------------------------------------------------------------------------
//
//	#include
//
//---------------------------------------------------------------------------
#define WINVER					0x500	// Windows98,Me,2000,XP以降
//VC2010//	#define _WIN32_WINNT			0x410	// Windows98,Me,2000,XP以降
#define _WIN32_WINNT			0x501	//VC2010//
#define _CRT_SECURE_NO_WARNINGS			//VC2010//
#define VC_EXTRALEAN

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
#define DIRECTSOUND_VERSION		0x500	// DirectX5を指定
#include <dsound.h>
//VC2010//#define DIRECTINPUT_VERSION		0x500	// DirectX5を指定
#define DIRECTINPUT_VERSION		0x0800	//VC2010//	// DirectX8を指定
#include <dinput.h>

// C Runtime
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

//---------------------------------------------------------------------------
//
//	#define
//
//---------------------------------------------------------------------------
#if defined(_DEBUG)
#define new		DEBUG_NEW
#endif	// _DEBUG

#if defined(_MSC_VER) && defined(_M_IX86)
#define FASTCALL	__fastcall
#else
#define FASTCALL
#endif	// _MSC_VER

//---------------------------------------------------------------------------
//
//	クラス宣言
//
//---------------------------------------------------------------------------
class VM;								// 仮想マシン
class CApp;								// アプリケーション
class CFrmWnd;							// フレームウィンドウ
class CDrawView;						// 描画ビュー

//---------------------------------------------------------------------------
//
//	グローバル
//
//---------------------------------------------------------------------------
void FASTCALL GetMsg(UINT uID, CString& string);													// メッセージ取得
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPTSTR lpszPath, UINT nFilterID);							// ファイルオープンダイアログ

VM* FASTCALL CreateVM(void);
void FASTCALL DestroyVM(void);
VM* FASTCALL GetVM(void);																			// 仮想マシンを取得
void FASTCALL LockVM(void);																			// 仮想マシンをロック
void FASTCALL UnlockVM(void);																		// 仮想マシンをアンロック

void schedulerInit();
BOOL schedulerIsEnable();
void schedulerSetEnable(BOOL b);

#endif	// mfc_h
#endif	// _WIN32
