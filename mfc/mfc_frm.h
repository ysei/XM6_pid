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
	CFrmWnd();
										// コンストラクタ
	BOOL Init();
										// 初期化

	// 取得
	CDrawView* FASTCALL GetView() const;				// 描画ビュー取得
	CComponent* FASTCALL GetFirstComponent() const;		// 最初のコンポーネントを取得
	CScheduler* FASTCALL GetScheduler() const;			// スケジューラ取得
	CConfig* FASTCALL GetConfig() const;				// コンフィグ取得

	// ドラッグ＆ドロップサポート
	BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath);	// コマンドライン処理 サブ

protected:
	// オーバーライド
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	void GetMessageString(UINT nID, CString& rMessage) const;
										// メッセージ文字列提供

	// WMメッセージ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnClose();
										// ウィンドウクローズ
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnMove(int x, int y);
										// ウィンドウ移動
	afx_msg LRESULT OnDisplayChange(UINT uParam, LONG lParam);
										// ディスプレイ変更
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// ウィンドウ背景描画
	afx_msg void OnPaint();
										// ウィンドウ描画
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// アクティベート
#if _MFC_VER >= 0x700
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
#else
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
#endif
										// タスク切り替え
	afx_msg void OnEnterMenuLoop(BOOL bTrackPopup);
										// メニューループ開始
	afx_msg void OnExitMenuLoop(BOOL bTrackPopup);
										// メニューループ終了
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
										// 親ウィンドウ通知
	afx_msg LONG OnKick(UINT uParam, LONG lParam);
										// キック
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
										// オーナードロー
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
										// コンテキストメニュー
	afx_msg LONG OnPowerBroadCast(UINT uParam, LONG lParam);
										// 電源変更通知
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
										// システムコマンド
#if _MFC_VER >= 0x700
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
#else
	afx_msg LONG OnCopyData(UINT uParam, LONG lParam);
										// データ転送
#endif
	afx_msg void OnEndSession(BOOL bEnding);
										// セッション終了
	afx_msg LONG OnShellNotify(UINT uParam, LONG lParam);
										// ファイルシステム状態変化

	// コマンド処理
	afx_msg void OnOpen();
										// 開く
	afx_msg void OnOpenUI(CCmdUI *pCmdUI);
										// 開く UI
	afx_msg void OnSave();
										// 上書き保存
	afx_msg void OnSaveUI(CCmdUI *pCmdUI);
										// 上書き保存 UI
	afx_msg void OnSaveAs();
										// 名前を付けて保存
	afx_msg void OnSaveAsUI(CCmdUI *pCmdUI);
										// 名前を付けて保存 UI
	afx_msg void OnMRU(UINT uID);
										// MRU
	afx_msg void OnMRUUI(CCmdUI *pCmdUI);
										// MRU UI
	afx_msg void OnReset();
										// リセット
	afx_msg void OnResetUI(CCmdUI *pCmdUI);
										// リセット UI
	afx_msg void OnInterrupt();
										// インタラプト
	afx_msg void OnInterruptUI(CCmdUI *pCmdUI);
										// インタラプト UI
	afx_msg void OnPower();
										// 電源スイッチ
	afx_msg void OnPowerUI(CCmdUI *pCmdUI);
										// 電源スイッチ UI
	afx_msg void OnExit();
										// 終了

	afx_msg void OnFD(UINT uID);
										// フロッピーディスクコマンド
	afx_msg void OnFDOpenUI(CCmdUI *pCmdUI);
										// フロッピーオープン UI
	afx_msg void OnFDEjectUI(CCmdUI *pCmdUI);
										// フロッピーイジェクト UI
	afx_msg void OnFDWritePUI(CCmdUI *pCmdUI);
										// フロッピー書き込み保護 UI
	afx_msg void OnFDForceUI(CCmdUI *pCmdUI);
										// フロッピー強制イジェクト UI
	afx_msg void OnFDInvalidUI(CCmdUI *pCmdUI);
										// フロッピー誤挿入 UI
	afx_msg void OnFDMediaUI(CCmdUI *pCmdUI);
										// フロッピーメディア UI
	afx_msg void OnFDMRUUI(CCmdUI *pCmdUI);
										// フロッピーMRU UI
	afx_msg void OnMouseMode();
										// マウスモード
	afx_msg void OnSoftKey();
										// ソフトウェアキーボード
	afx_msg void OnSoftKeyUI(CCmdUI *pCmdUI);
										// ソフトウェアキーボード UI
	afx_msg void OnTimeAdj();
										// 時刻合わせ
	afx_msg void OnTrap();
										// trap#0
	afx_msg void OnTrapUI(CCmdUI *pCmdUI);
										// trap#0 UI
	afx_msg void OnSaveWav();
										// WAVキャプチャ
	afx_msg void OnSaveWavUI(CCmdUI *pCmdUI);
										// WAVキャプチャ UI
	afx_msg void OnOptions();
										// オプション
private:
	// 初期化
	BOOL FASTCALL InitChild();
										// チャイルドウィンドウ初期化
	void FASTCALL InitPos(BOOL bStart = TRUE);
										// 位置・矩形初期化
	void FASTCALL InitShell();
										// シェル連携初期化
	BOOL FASTCALL InitVM();
										// VM初期化
	BOOL FASTCALL InitComponent();
										// コンポーネント初期化
	void FASTCALL InitVer();
										// バージョン初期化
	void FASTCALL InitCmd(LPCTSTR lpszCmd);
										// コマンドライン処理
	void FASTCALL ApplyCfg();
										// 設定適用
	void FASTCALL SizeStatus();
										// ステータスバーサイズ変更
	void FASTCALL HideTaskBar(BOOL bHide, BOOL bFore);
										// タスクバー隠す
	BOOL RestoreFrameWnd(BOOL bFullScreen);
										// ウィンドウ復元
	void RestoreDiskState();
										// ディスク・ステート復元
	int m_nStatus;
										// ステータスコード
	static const DWORD SigTable[];
										// SRAMシグネチャテーブル

	// 終了
	void SaveFrameWnd();
										// ウィンドウ保存
	void SaveDiskState();
										// ディスク・ステート保存
	void FASTCALL CleanSub();
										// クリーンアップ
	BOOL m_bExit;
										// 終了フラグ
	BOOL m_bSaved;
										// フレーム・ディスク・ステート保存フラグ

	// セーブ・ロード
	BOOL FASTCALL SaveComponent(const Filepath& path, DWORD dwPos);
										// セーブ
	BOOL FASTCALL LoadComponent(const Filepath& path, DWORD dwPos);
										// ロード

	// コマンドハンドラサブ
	BOOL FASTCALL OnOpenSub(const Filepath& path);
										// オープンサブ
	BOOL FASTCALL OnOpenPrep(const Filepath& path, BOOL bWarning = TRUE);
										// オープンチェック
	void FASTCALL OnSaveSub(const Filepath& path);
										// 保存サブ
	void FASTCALL OnFDOpen(int nDrive);
										// フロッピーオープン
	void FASTCALL OnFDEject(int nDrive);
										// フロッピーイジェクト
	void FASTCALL OnFDWriteP(int nDrive);
										// フロッピー書き込み保護
	void FASTCALL OnFDForce(int nDrive);
										// フロッピー強制イジェクト
	void FASTCALL OnFDInvalid(int nDrive);
										// フロッピー誤挿入
	void FASTCALL OnFDMedia(int nDrive, int nMedia);
										// フロッピーメディア
	void FASTCALL OnFDMRU(int nDrive, int nMRU);
										// フロッピーMRU
	int m_nFDDStatus[2];
										// フロッピーステータス

	// デバイス・ビュー・コンポーネント
	FDD *m_pFDD;
										// FDD
	SASI *m_pSASI;
										// SASI
	SCSI *m_pSCSI;
										// SCSI
	Scheduler *m_pScheduler;
										// Scheduler
	Keyboard *m_pKeyboard;
										// Keyboard
	Mouse *m_pMouse;
										// Mouse
	CDrawView *m_pDrawView;
										// 描画ビュー
	CComponent *m_pFirstComponent;
										// 最初のコンポーネント
	CScheduler *m_pSch;
										// スケジューラ
	CConfig *m_pConfig;
										// コンフィグ

	// フルスクリーン
	BOOL m_bFullScreen;
										// フルスクリーンフラグ
	DEVMODE m_DevMode;
										// スクリーンパラメータ記憶
	HWND m_hTaskBar;
										// タスクバー
	int m_nWndLeft;
										// ウィンドウモード時x
	int m_nWndTop;
										// ウィンドウモード時y

	// サブウィンドウ
	CString m_strWndClsName;
										// ウィンドウクラス名

	// メニュー
	void FASTCALL ShowMenu();
										// メニューバー表示
	CMenu m_Menu;
										// メインメニュー
	BOOL m_bMenuBar;
										// メニューバー表示フラグ
	CMenu m_PopupMenu;
										// ポップアップメニュー
	BOOL m_bPopupMenu;
										// ポップアップメニュー実行中

	// シェル連携
	ULONG m_uNotifyId;
										// シェル通知ID
	SHChangeNotifyEntry m_fsne[1];
										// シェル通知エントリ

	// ステートファイル
	void FASTCALL UpdateExec();
										// 更新(実行)
	DWORD m_dwExec;
										// セーブ後実行カウンタ

	// コンフィギュレーション
	BOOL m_bMouseMid;
										// マウス中ボタン有効
	BOOL m_bPopup;
										// ポップアップモード
	BOOL m_bAutoMouse;
										// 自動マウスモード

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_frm_h
#endif	// _WIN32
