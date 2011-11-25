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
class CScheduler : public CComponent
{
public:
	// 基本ファンクション
	CScheduler(CFrmWnd *pFrmWnd);			// コンストラクタ
	BOOL FASTCALL Init();					// 初期化
	void FASTCALL Cleanup();				// クリーンアップ
#if defined(_DEBUG)
	void AssertValid() const;				// 診断
#endif	// _DEBUG

	// 実行制御
	void FASTCALL Reset();					// 時間をリセット

private:
	void FASTCALL Run();					// 実行
	static UINT ThreadFunc(LPVOID pParam);	// スレッド関数
	CWinThread *m_pThread;					// スレッドポインタ
	BOOL m_bExitReq;						// スレッド終了要求
};

#endif	// mfc_sch_h
#endif	// _WIN32
