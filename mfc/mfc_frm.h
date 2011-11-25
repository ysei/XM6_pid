//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC フレームウィンドウ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_frm_h)
#define mfc_frm_h

//---------------------------------------------------------------------------
//
//	ウィンドウメッセージ
//
//---------------------------------------------------------------------------
#define WM_KICK			WM_APP				// エミュレータスタート
#define WM_SHELLNOTIFY	(WM_USER + 5)		// ファイルシステム状態変化

//===========================================================================
//
//	フレームウィンドウ
//
//===========================================================================
class CFrmWnd : public CFrameWnd
{
public:
	// 初期化
	CFrmWnd();												// コンストラクタ
	BOOL Init();											// 初期化

	// ドラッグ＆ドロップサポート
	BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath);	// コマンドライン処理 サブ
	void OnDraw(CDC *pDC);

protected:
	// WMメッセージ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);						// ウィンドウ作成
	afx_msg void OnDestroy();													// ウィンドウ削除
	afx_msg void OnPaint();														// ウィンドウ描画

	afx_msg LONG OnKick(UINT uParam, LONG lParam);		// キック
	afx_msg void OnReset();								// リセット
	afx_msg void OnFD(UINT uID);						// フロッピーディスクコマンド

private:
	void FASTCALL	InitCmd(LPCTSTR lpszCmd);			// コマンドライン処理
	void FASTCALL	OnFDOpen(int nDrive);				// フロッピーオープン
	void FASTCALL	ShowMenu();							// メニューバー表示

	BOOL			m_bExit;							// 終了フラグ
	FDD*			m_pFDD;								// FDD
	CMenu			m_Menu;								// メインメニュー

	DECLARE_MESSAGE_MAP()								// メッセージ マップあり
};

#endif	// mfc_frm_h
#endif	// _WIN32
