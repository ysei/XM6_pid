//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC スケジューラ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sch_h)
#define mfc_sch_h

//===========================================================================
//
//	スケジューラ
//
//===========================================================================
class CScheduler	// : public CComponent
{
public:
	// 基本ファンクション
	CScheduler(CFrmWnd *pFrmWnd);			// コンストラクタ
	BOOL FASTCALL Init();					// 初期化
	void FASTCALL Cleanup();				// クリーンアップ

	BOOL IsEnable() const { return mm_bEnable; }
	void Enable(BOOL b) { mm_bEnable = b; }

	virtual BOOL FASTCALL Save(Fileio *pFio, int nVer) { return TRUE; }		// セーブ
	virtual BOOL FASTCALL Load(Fileio *pFio, int nVer) { return TRUE; }		// ロード
	virtual void FASTCALL ApplyCfg(const Config *pConfig) {}				// 設定適用

private:
	void FASTCALL Run();					// 実行
	static UINT ThreadFunc(LPVOID pParam);	// スレッド関数
	CWinThread *m_pThread;					// スレッドポインタ
	BOOL m_bExitReq;						// スレッド終了要求


	BOOL	mm_bEnable;
};

#endif	// mfc_sch_h
#endif	// _WIN32
