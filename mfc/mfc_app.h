//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC アプリケーション ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32) && defined(_AFXDLL)

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

//---------------------------------------------------------------------------
//
//	グローバル
//
//---------------------------------------------------------------------------
void FASTCALL GetMsg(UINT uID, CString& string);													// メッセージ取得
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPTSTR lpszPath, UINT nFilterID);							// ファイルオープンダイアログ

VM* FASTCALL CreateVM(void);
void FASTCALL DestroyVM(void);
VM* FASTCALL GetVM(void);
void FASTCALL LockVM(void);
void FASTCALL UnlockVM(void);

#endif	// mfc_app_h
#endif	// _WIN32
