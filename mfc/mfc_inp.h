//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC インプット ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_inp_h)
#define mfc_inp_h

//===========================================================================
//
//	インプット
//
//===========================================================================
class CInput : public CComponent
{
public:
	// 基本ファンクション
	CInput(CFrmWnd *pWnd);																// コンストラクタ
	BOOL FASTCALL Init();																// 初期化
	void FASTCALL Cleanup();															// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);										// 設定適用
#if defined(_DEBUG)
	void AssertValid() const;															// 診断
#endif	// _DEBUG

	// セーブ・ロード
	BOOL FASTCALL Save(Fileio *pFio, int nVer);											// セーブ
	BOOL FASTCALL Load(Fileio *pFio, int nVer);											// ロード

	// 外部API
	void FASTCALL Process(BOOL bRun);													// 進行
	void FASTCALL Activate(BOOL bActivate);												// アクティブ通知
	BOOL FASTCALL IsActive() const		{ return m_bActive; }							// アクティブ状況取得
	void FASTCALL Menu(BOOL bMenu);														// メニュー通知
	BOOL FASTCALL IsMenu() const		{ return m_bMenu; }								// メニュー状況取得
	DWORD FASTCALL GetProcessCount() const	{ return m_dwProcessCount; }				// 進行カウンタ取得
	DWORD FASTCALL GetAcquireCount(int nType) const;									// 獲得カウンタ取得

	// マウス
	void FASTCALL SetMouseMode(BOOL bMode);												// マウスモード設定
	BOOL FASTCALL GetMouseMode() const	{ return m_bMouseMode; }						// マウスモード取得
	void FASTCALL GetMouseInfo(int *pPos, BOOL *pBtn) const;							// マウス情報取得

private:
	// 共通
	LPDIRECTINPUT m_lpDI;									// DirectInput
	BOOL m_bActive;											// アクティブフラグ
	BOOL m_bMenu;											// メニューフラグ
	CRTC *m_pCRTC;											// CRTC
	DWORD m_dwDispCount;									// CRTC表示カウント
	DWORD m_dwProcessCount;									// 進行カウント

	// セーブ・ロード
	BOOL FASTCALL SaveMain(Fileio *pFio);					// セーブ本体
	BOOL FASTCALL Load200(Fileio *pFio);					// ロード本体 (version2.00)
	BOOL FASTCALL Load201(Fileio *pFio);					// ロード本体 (version2.01)

	// キーボード
	void FASTCALL InputKey(BOOL bEnable);					// キーボード入力
	Keyboard *m_pKeyboard;									// キーボード

	// マウス
	BOOL FASTCALL InitMouse();								// マウス初期化
	void FASTCALL InputMouse(BOOL bEnable);					// マウス入力
	Mouse *m_pMouse;										// マウス
	LPDIRECTINPUTDEVICE m_lpDIMouse;						// マウスデバイス
	DWORD m_dwMouseAcquire;									// マウス獲得カウンタ
	BOOL m_bMouseMode;										// マウスモードフラグ
	int m_nMouseX;											// マウスx座標
	int m_nMouseY;											// マウスy座標
	BOOL m_bMouseB[2];										// マウス左右ボタン
	DWORD m_dwMouseMid;										// マウス中央ボタンカウント
	BOOL m_bMouseMid;										// マウス中央ボタン使用フラグ

	// ジョイスティック
	void FASTCALL InitJoy();								// ジョイスティック初期化
	void FASTCALL InputJoy(BOOL bEnable);					// ジョイスティック入力
	PPI *m_pPPI;											// PPI
};

#endif	// mfc_inp_h
#endif	// _WIN32
