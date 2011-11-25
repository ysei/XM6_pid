//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC コンフィグ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_cfg_h)
#define mfc_cfg_h

#include "config.h"
#include "ppi.h"

//===========================================================================
//
//	コンフィグ
//
//===========================================================================
class CConfig : public CComponent
{
public:
	// 基本ファンクション
	CConfig(CFrmWnd *pWnd);													// コンストラクタ
	BOOL FASTCALL Init();													// 初期化
	void FASTCALL Cleanup();												// クリーンアップ

	// 設定データ(全体)
	void FASTCALL GetConfig(Config *pConfigBuf) const;						// 設定データ取得
	void FASTCALL SetConfig(Config *pConfigBuf);							// 設定データ設定

	// 設定データ(個別)
	void FASTCALL SetStretch(BOOL bStretch);								// 画面拡大設定
	void FASTCALL SetMIDIDevice(int nDevice, BOOL bIn);						// MIDIデバイス設定

	// MRU
	void FASTCALL SetMRUFile(int nType, LPCTSTR pszFile);					// MRUファイル設定(最も新しい)
	void FASTCALL GetMRUFile(int nType, int nIndex, LPTSTR pszFile) const;	// MRUファイル取得
	int FASTCALL GetMRUNum(int nType) const;								// MRUファイル個数取得

	// セーブ・ロード
	BOOL FASTCALL Save(Fileio *pFio, int nVer);								// セーブ
	BOOL FASTCALL Load(Fileio *pFio, int nVer);								// ロード
	BOOL FASTCALL IsApply();												// 適用するか

private:
	// 設定データ
	typedef struct _INIKEY {
		void *pBuf;						// ポインタ
		LPCTSTR pszSection;				// セクション名
		LPCTSTR pszKey;					// キー名
		int nType;						// 型
		int nDef;						// デフォルト値
		int nMin;						// 最小値(一部タイプのみ)
		int nMax;						// 最大値(一部のみ)
	} INIKEY, *PINIKEY;

	// INIファイル
	TCHAR m_IniFile[FILEPATH_MAX];								// INIファイル名

	// 設定データ
	void FASTCALL LoadConfig();									// 設定データロード
	void FASTCALL SaveConfig() const;							// 設定データセーブ
	static const INIKEY IniTable[];								// 設定データINIテーブル
	static Config m_Config;										// 設定データ

	// バージョン互換
	void FASTCALL ResetSASI();									// SASI再設定
	void FASTCALL ResetCDROM();									// CD-ROM再設定
	static BOOL m_bCDROM;										// CD-ROM有効

	// MRU
	enum {
		MruTypes = 5											// MRUタイプ数
	};
	void FASTCALL ClearMRU(int nType);							// MRUクリア
	void FASTCALL LoadMRU(int nType);							// MRUロード
	void FASTCALL SaveMRU(int nType) const;						// MRUセーブ
	TCHAR m_MRUFile[MruTypes][9][FILEPATH_MAX];					// MRUファイル
	int m_MRUNum[MruTypes];										// MRU数

	// キー
	void FASTCALL LoadKey() const;								// キーロード
	void FASTCALL SaveKey() const;								// キーセーブ

	// TrueKey
	void FASTCALL LoadTKey() const;								// TrueKeyロード
	void FASTCALL SaveTKey() const;								// TrueKeyセーブ

	// バージョン互換
	BOOL FASTCALL Load200(Fileio *pFio);						// version2.00 or version2.01
	BOOL FASTCALL Load202(Fileio *pFio);						// version2.02 or version2.03

	// ロード・セーブ
	BOOL m_bApply;												// ロード後ApplyCfgを要求
};

//---------------------------------------------------------------------------
//
//	クラス先行定義
//
//---------------------------------------------------------------------------
class CConfigSheet;

//===========================================================================
//
//	コンフィグプロパティページ
//
//===========================================================================
class CConfigPage : public CPropertyPage
{
public:
	CConfigPage();													// コンストラクタ
	void FASTCALL Init(CConfigSheet *pSheet);						// 初期化
	virtual BOOL OnInitDialog();									// ダイアログ初期化
	virtual BOOL OnSetActive();										// ページアクティブ
	DWORD FASTCALL GetID() const		{ return m_dwID; }			// ID取得

protected:
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT nMsg);	// マウスカーソル設定
	Config *m_pConfig;												// 設定データ
	DWORD m_dwID;													// ページID
	int m_nTemplate;												// テンプレートID
	UINT m_uHelpID;													// ヘルプID
	UINT m_uMsgID;													// ヘルプメッセージID
	CConfigSheet *m_pSheet;											// プロパティシート
	DECLARE_MESSAGE_MAP()											// メッセージ マップあり
};
//===========================================================================
//
//	コンフィグプロパティシート
//
//===========================================================================
class CConfigSheet : public CPropertySheet
{
public:
	CConfigSheet(CWnd *pParent);							// コンストラクタ
	Config *m_pConfig;										// 設定データ
	CConfigPage* FASTCALL SearchPage(DWORD dwID) const;		// ページ検索

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);	// ウィンドウ作成
	afx_msg void OnDestroy();								// ウィンドウ削除
	afx_msg void OnTimer(UINT_PTR nTimerID);				// タイマ

private:
	CFrmWnd *m_pFrmWnd;										// フレームウィンドウ
	UINT_PTR m_nTimerID;
	DECLARE_MESSAGE_MAP()									// メッセージ マップあり
};

#endif	// mfc_cfg_h
#endif	// _WIN32
