//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC スケジューラ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "mouse.h"
#include "render.h"
#include "config.h"
#include "mfc_com.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_snd.h"
#include "mfc_inp.h"
#include "mfc_sch.h"

//===========================================================================
//
//	スケジューラ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CScheduler::CScheduler(CFrmWnd *pFrmWnd) : CComponent(pFrmWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('S', 'C', 'H', 'E');
	m_strDesc = _T("Scheduler");

	// ワーク初期化
	m_pCPU = NULL;
	m_pRender = NULL;
	m_pThread = NULL;
	m_pSound = NULL;
	m_pInput = NULL;
	m_bExitReq = FALSE;
	m_dwExecTime = 0;
	m_nSubWndNum = 0;
	m_nSubWndDisp = -1;
	m_bMPUFull = FALSE;
	m_bVMFull = FALSE;
	m_dwDrawCount = 0;
	m_dwDrawTime = 0;
	m_bMenu = FALSE;
	m_bActivate = TRUE;
	m_bBackup = TRUE;
	m_bSavedValid = FALSE;
	m_bSavedEnable = FALSE;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CScheduler::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// CPU取得
	ASSERT(!m_pCPU);
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);

	// レンダラ取得
	ASSERT(!m_pRender);
	m_pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(m_pRender);

	// サウンドコンポーネントを検索
	ASSERT(!m_pSound);
	m_pSound = (CSound*)SearchComponent(MAKEID('S', 'N', 'D', ' '));
	ASSERT(m_pSound);

	// インプットコンポーネントを検索
	ASSERT(!m_pInput);
	m_pInput = (CInput*)SearchComponent(MAKEID('I', 'N', 'P', ' '));
	ASSERT(m_pInput);

	// マルチメディアタイマーの時間間隔を1msに設定
	::timeBeginPeriod(1);

	// スレッドを立てる
	m_pThread = AfxBeginThread(ThreadFunc, this);
	if (!m_pThread) {
		::timeEndPeriod(1);
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Cleanup()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// 停止
	Stop();

	// マルチメディアタイマーの時間間隔を戻す
	::timeEndPeriod(1);

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::ApplyCfg(const Config *pConfig)
{
	BOOL bFlag;

	ASSERT(this);
	ASSERT_VALID(this);
	ASSERT(pConfig);

	// 旧VMフルを保存
	bFlag = m_bVMFull;

	m_bMPUFull = pConfig->mpu_fullspeed;
	m_bVMFull = pConfig->vm_fullspeed;

	// 違ったらリセット
	if (bFlag != m_bVMFull) {
		Reset();
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void CScheduler::AssertValid() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pCPU);
	ASSERT(m_pCPU->GetID() == MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pRender);
	ASSERT(m_pRender->GetID() == MAKEID('R', 'E', 'N', 'D'));
	ASSERT(m_pSound);
	ASSERT(m_pSound->GetID() == MAKEID('S', 'N', 'D', ' '));
	ASSERT(m_pInput);
	ASSERT(m_pInput->GetID() == MAKEID('I', 'N', 'P', ' '));
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Reset()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// リセット
	m_dwExecTime = GetTime();
	m_dwDrawTime = m_dwExecTime;
	m_dwDrawBackup = 0;
	m_dwDrawCount = 0;
	m_dwDrawPrev = 0;
}

//---------------------------------------------------------------------------
//
//	停止
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Stop()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// スレッドが上がっている場合のみ終了処理
	if (m_pThread) {
		// 終了リクエストを立てる
		m_bExitReq = TRUE;

		// 停止まで待つ
		::WaitForSingleObject(m_pThread->m_hThread, INFINITE);

		// スレッドは終了した
		m_pThread = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CScheduler::Save(Fileio *pFio, int /*nVer*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// セーブ時のEnable状態を保存(version2.04)
	if (!pFio->Write(&m_bSavedEnable, sizeof(m_bSavedEnable))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL CScheduler::Load(Fileio *pFio, int nVer)
{
	ASSERT(this);
	ASSERT(nVer >= 0x0200);
	ASSERT_VALID(this);

	// セーブ時のEnable状態保存はされていないものと仮定
	m_bSavedValid = FALSE;

	// version2.04以降であれば、ロード
	if (nVer >= 0x0204) {
		if (!pFio->Read(&m_bSavedEnable, sizeof(m_bSavedEnable))) {
			return FALSE;
		}
		m_bSavedValid = TRUE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	スレッド関数
//
//---------------------------------------------------------------------------
UINT CScheduler::ThreadFunc(LPVOID pParam)
{
	CScheduler *pSch;

	// パラメータを受け取る
	pSch = (CScheduler*)pParam;
	ASSERT(pSch);
#if defined(_DEBUG)
	pSch->AssertValid();
#endif	// _DEBUG

	// 実行
	pSch->Run();

	// 終了コードを持ってスレッドを終了
	return 0;
}

//---------------------------------------------------------------------------
//
//	実行
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Run()
{
	VM *pVM;
	Scheduler *pScheduler;
	Render *pRender;
	DWORD dwExecCount;
	DWORD dwCycle;
	DWORD dwTime;

	ASSERT(this);
	ASSERT_VALID(this);

	// VM取得
	pVM = ::GetVM();
	ASSERT(pVM);
	pScheduler = (Scheduler*)pVM->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(pScheduler);
	pRender = (Render*)pVM->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(pRender);

	// 時間カウンタ
	m_dwExecTime = GetTime();
	dwExecCount = 0;
	m_dwDrawTime = m_dwExecTime;
	m_dwDrawBackup = 0;
	m_dwDrawCount = 0;
	m_dwDrawPrev = 0;
	m_bBackup = m_bEnable;

	// 終了リクエストが上がるまでループ
	while (!m_bExitReq) {
		// 常時診断
		ASSERT_VALID(this);

		// Sleep
		::Sleep(0);

		// ロック
		Lock();

		// 有効フラグが上がっていなければ、停止中
		if (!m_bEnable) {
			// ブレークポイント、電源停止などで有効→無効になったら、必ず再描画
			if (m_bBackup) {
				m_pFrmWnd->GetView()->Invalidate(FALSE);
				m_bBackup = FALSE;
			}

			// 描画
			Refresh();
			dwExecCount = 0;

			// 他コンポーネントの処理、時間あわせ
			m_pSound->Process(FALSE);
			m_pInput->Process(FALSE);
			m_dwExecTime = GetTime();
			Unlock();

			// Sleep
			::Sleep(10);
			continue;
		}

		// バックアップフラグ更新(直前まで実行中だった)
		m_bBackup = TRUE;

		// 時間が逆なら、表示する余裕がある
		dwTime = GetTime();
		if (m_dwExecTime > dwTime) {
			// 大幅にずれているなら合わせ(49日ループ対応)
			if ((m_dwExecTime - dwTime) > 1000) {
				m_dwExecTime = dwTime;
			}

			// 描画
			Refresh();
			dwExecCount = 0;

			// 高速モードの処理
			if (m_dwExecTime > GetTime()) {
				// 時間に余裕がある
				if (m_bVMFull) {
					// VM高速モード
					m_dwExecTime = GetTime();
				}
				else {
					if (!m_bMPUFull) {
						// 通常モード
						Unlock();
						::Sleep(1);
						continue;
					}
					// MPU高速モード
					dwCycle = pScheduler->GetCPUCycle();
					while (m_dwExecTime > GetTime()) {
						m_pCPU->Exec(pScheduler->GetCPUSpeed());
					}
					pScheduler->SetCPUCycle(dwCycle);
					Unlock();
					continue;
				}
			}
		}

		// レンダリング可否を判定(1or36)
		if (m_dwExecTime >= dwTime) {
			pRender->EnableAct(TRUE);
		}
		else {
			if ((dwTime - m_dwExecTime) <= 1) {
				// 順当
				pRender->EnableAct(TRUE);
			}
			else {
				pRender->EnableAct(FALSE);
			}
		}

		// VMを駆動
		if (!pVM->Exec(1000 * 2)) {
			// ブレークポイント
			SyncDisasm();
			dwExecCount++;
			m_bEnable = FALSE;
			Unlock();
			continue;
		}

		// 電源チェック
		if (!pVM->IsPower()) {
			// 電源オフ
			SyncDisasm();
			dwExecCount++;
			m_bEnable = FALSE;
			Unlock();
			continue;
		}
		dwExecCount++;
		m_dwExecTime++;

		// 他コンポーネントの処理
		m_pSound->Process(TRUE);
		m_pInput->Process(TRUE);

		// dwExecCountが規定数を超えたら、一度表示して強制時間合わせ
		if (dwExecCount > 400) {
			Refresh();
			dwExecCount = 0;
			m_dwExecTime = GetTime();
		}

		// 非アクティブまたはメニューONなら、8msごとに1度Sleep
		if (!m_bActivate || m_bMenu) {
			if ((m_dwExecTime & 0x07) == 0) {
				Unlock();
				::Sleep(1);
				continue;
			}
		}

		// アンロック
		Unlock();
	}
}

//---------------------------------------------------------------------------
//
//	リフレッシュ
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::Refresh()
{
//	int num;
	CDrawView *pView;

	ASSERT(this);
	ASSERT_VALID(this);
	ASSERT(m_pFrmWnd);

	// ビューを取得
	pView = m_pFrmWnd->GetView();
	ASSERT(pView);

//	// サブウィンドウの個数を取得
//	num = pView->GetSubWndNum();
//
//	// 個数が記憶値と違ったらリセット
//	if (m_nSubWndNum != num) {
//		m_nSubWndNum = num;
//		m_nSubWndDisp = -1;
//	}
//
	m_nSubWndDisp = -1;

	if (m_bEnable) {
		// 実行中でメイン画面の番か
		if (m_nSubWndDisp < 0) {
			// レンダラの準備ができていなければ描画しない
			if (!m_pRender->IsReady()) {
				return;
			}
			SyncDisasm();
		}
	}

	// 表示(一部)
	pView->Draw(m_nSubWndDisp);

	// メイン画面表示なら、カウントダウン
	if (m_nSubWndDisp < 0) {
		m_pRender->Complete();
		m_dwDrawCount++;
	}

	// サイクリックに表示
	m_nSubWndDisp++;
	if (m_nSubWndDisp >= m_nSubWndNum) {
		m_nSubWndDisp = -1;
	}
}

//---------------------------------------------------------------------------
//
//	逆アセンブラPCあわせ
//
//---------------------------------------------------------------------------
void FASTCALL CScheduler::SyncDisasm()
{
//	CDisasmWnd *pWnd;
	CDrawView *pView;
	DWORD dwID;
	int i;

	ASSERT(this);
	ASSERT_VALID(this);

	// ビューを取得
	pView = m_pFrmWnd->GetView();
	ASSERT(pView);

	// 最大8コまで
	for (i=0; i<8; i++) {
		dwID = MAKEID('D', 'I', 'S', ('A' + i));
//		pWnd = (CDisasmWnd*)pView->SearchSWnd(dwID);
//		if (pWnd) {
//			// 見つかった
//			pWnd->SetPC(m_pCPU->GetPC());
//		}
	}
}

//---------------------------------------------------------------------------
//
//	フレームレート取得
//
//---------------------------------------------------------------------------
int FASTCALL CScheduler::GetFrameRate()
{
	DWORD dwCount;
	DWORD dwTime;
    DWORD dwDiff;

	ASSERT(this);
	ASSERT_VALID(this);

	// 無効チェック
	if (!m_bEnable) {
		return 0;
	}

	// 時間取得(ループ対応、長い場合は初期化)
	dwTime = GetTime();
	if (dwTime <= m_dwDrawTime) {
		m_dwDrawTime = dwTime;
		m_dwDrawBackup = 0;
		m_dwDrawCount = 0;
		m_dwDrawPrev = 0;
		return 0;
	}
	dwDiff = dwTime - m_dwDrawTime;
	if (dwDiff > 3500) {
		m_dwDrawTime = dwTime;
		m_dwDrawBackup = 0;
		m_dwDrawCount = 0;
		m_dwDrawPrev = 0;
		return 0;
	}

	// 差が500ms以下なら普通に処理
	if (dwDiff < 500) {
		m_dwDrawBackup = 0;
		dwDiff /= 10;
		dwCount = m_dwDrawCount * 1000;
		if (dwDiff == 0) {
			return 0;
		}
		return (dwCount / dwDiff);
	}

	// 差が1000ms以下なら記憶
	if (dwDiff < 1000) {
		if (m_dwDrawBackup == 0) {
			m_dwDrawBackup = dwTime;
			m_dwDrawPrev = m_dwDrawCount;
		}
		dwDiff /= 10;
		dwCount = m_dwDrawCount * 1000;
		return (dwCount / dwDiff);
	}

	// それ以上なので、Prevに切り替え
	dwDiff /= 10;
	dwCount = m_dwDrawCount * 1000;
	m_dwDrawTime = m_dwDrawBackup;
	m_dwDrawBackup = 0;
	m_dwDrawCount -= m_dwDrawPrev;
	return (dwCount / dwDiff);
}

#endif	// _WIN32
