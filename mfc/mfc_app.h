//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC アプリケーション ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_app_h)
#define mfc_app_h

//===========================================================================
//
//	アプリケーション
//
//===========================================================================
class CApp : public CWinApp
{
public:
	CApp();														// コンストラクタ
	BOOL InitInstance();										// インスタンス初期化
};

#endif	// mfc_app_h
#endif	// _WIN32
