//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Drawビュー ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "render.h"
#include "crtc.h"
#include "config.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_cfg.h"
#include "mfc_res.h"
#include "mfc_sch.h"
#include "mfc_draw.h"

//===========================================================================
//
//	Drawビュー
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CDrawView::CDrawView()
{
	// ワーク初期化(基本)
	m_bEnable = FALSE;
//	m_pSubWnd = NULL;
	m_pFrmWnd = NULL;

	// コンポーネント
	m_pScheduler = NULL;
//	m_pInput = NULL;

	// ワーク初期化(描画全般)
	m_Info.bPower = FALSE;
	m_Info.pRender = NULL;
	m_Info.pWork = NULL;
	m_Info.dwDrawCount = 0;

	// ワーク初期化(DIBセクション)
	m_Info.hBitmap = NULL;
	m_Info.pBits = NULL;
	m_Info.nBMPWidth = 0;
	m_Info.nBMPHeight = 0;

	// ワーク初期化(サイズ整合)
	m_Info.nRendWidth = 0;
	m_Info.nRendHeight = 0;
	m_Info.nRendHMul = 0;
	m_Info.nRendVMul = 0;
	m_Info.nLeft = 0;
	m_Info.nTop = 0;
	m_Info.nWidth = 0;
	m_Info.nHeight = 0;

	// ワーク初期化(Blt)
	m_Info.nBltTop = 0;
	m_Info.nBltBottom = 0;
	m_Info.nBltLeft = 0;
	m_Info.nBltRight = 0;
	m_Info.bBltAll = TRUE;
	m_Info.bBltStretch = FALSE;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CDrawView, CView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_WM_DROPFILES()
#if _MFC_VER >= 0x600
	ON_WM_MOUSEWHEEL()
#endif	// _MFC_VER
	ON_WM_KEYDOWN()
	ON_WM_SYSKEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SYSKEYUP()
	ON_WM_MOVE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::Init(CWnd *pParent)
{
	ASSERT(pParent);

	// フレームウィンドウ記憶
	m_pFrmWnd = (CFrmWnd*)pParent;

	// 最初のビューとして作成
	if (!Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
				CRect(0, 0, 0, 0), pParent, AFX_IDW_PANE_FIRST, NULL)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成準備
//
//---------------------------------------------------------------------------
BOOL CDrawView::PreCreateWindow(CREATESTRUCT& cs)
{
	// 基本クラス
	if (!CView::PreCreateWindow(cs)) {
		return FALSE;
	}

	// WS_CLIPCHILDRENを追加
	cs.style |= WS_CLIPCHILDREN;

	// クライアントエッジを追加
	cs.dwExStyle |= WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CDrawView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// 基本クラス
	if (CView::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// IMEオフ
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// テキストフォント作成
	if (IsJapanese()) {
		// 日本語環境
		m_TextFont.CreateFont(14, 0, 0, 0,
							FW_NORMAL, 0, 0, 0,
							SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							FIXED_PITCH, NULL);
	}
	else {
		// 英語環境
		m_TextFont.CreateFont(14, 0, 0, 0,
							FW_NORMAL, 0, 0, 0,
							DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							FIXED_PITCH, NULL);
	}

	// ドラッグ＆ドロップ許可
	DragAcceptFiles(TRUE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ削除
//
//---------------------------------------------------------------------------
void CDrawView::OnDestroy()
{
	// 動作停止
	Enable(FALSE);

	// ビットマップを削除
	if (m_Info.hBitmap) {
		::DeleteObject(m_Info.hBitmap);
		m_Info.hBitmap = NULL;
		m_Info.pBits = NULL;
	}

	// テキストフォント削除
	m_TextFont.DeleteObject();

	// 基本クラスへ
	CView::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	サイズ変更
//
//---------------------------------------------------------------------------
void CDrawView::OnSize(UINT nType, int cx, int cy)
{
	// ビットマップの更新
	SetupBitmap();

	// 基本クラス
	CView::OnSize(nType, cx, cy);
}

//---------------------------------------------------------------------------
//
//	描画
//
//---------------------------------------------------------------------------
void CDrawView::OnPaint()
{
	Render *pRender;
	CRTC *pCRTC;
	const CRTC::crtc_t *p;
	CFrmWnd *pFrmWnd;
	PAINTSTRUCT ps;

	// VMロック
	::LockVM();

	// 全描画フラグON
	m_Info.bBltAll = TRUE;

	// イネーブルでスケジューラがOFFなら、Mixバッファに作成(かなり強引)
	if (m_bEnable) {
		pFrmWnd = (CFrmWnd*)GetParent();
		ASSERT(pFrmWnd);
		if (!schedulerIsEnable()) {
			// イネーブルなら必ずRender,CRTCは存在
			pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
			ASSERT(pRender);
			pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
			ASSERT(pCRTC);
			p = pCRTC->GetWorkAddr();

			// 作成
			m_Info.bPower = ::GetVM()->IsPower();
			if (m_Info.bPower) {
				pRender->Complete();
				pRender->EnableAct(TRUE);
				pRender->StartFrame();
				pRender->HSync(p->v_dots);
				pRender->EndFrame();
			}
			else {
				// ビットマップをすべて消去
				memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
			}

			// 描画(CDrawView::OnDrawへつなげる)
			CView::OnPaint();

			// VMアンロック
			::UnlockVM();
			return;
		}
	}

	// DCだけ得ておく(ダミー)
	BeginPaint(&ps);
	EndPaint(&ps);

	// VMアンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	背景描画
//
//---------------------------------------------------------------------------
BOOL CDrawView::OnEraseBkgnd(CDC *pDC)
{
	CRect rect;

	// イネーブルでなければ、黒で塗りつぶす
	if (!m_bEnable) {
		GetClientRect(&rect);
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	表示環境変更
//
//---------------------------------------------------------------------------
LRESULT CDrawView::OnDisplayChange(WPARAM , LPARAM )
{
	// ビットマップ準備
	SetupBitmap();

	return 0;
}
//---------------------------------------------------------------------------
//
//	キー押下
//
//---------------------------------------------------------------------------
void CDrawView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 除外したいキーを判別
//	if (!KeyUpDown(nChar, nFlags, TRUE)) {
//		return;
//	}

	// 基本クラスへ流す
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	システムキー押下
//
//---------------------------------------------------------------------------
void CDrawView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 除外したいキーを判別
//	if (!KeyUpDown(nChar, nFlags, TRUE)) {
//		return;
//	}

	// 基本クラスへ流す
	CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	キー離した
//
//---------------------------------------------------------------------------
void CDrawView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 除外したいキーを判別
//	if (!KeyUpDown(nChar, nFlags, FALSE)) {
//		return;
//	}

	// 基本クラスへ流す
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	システムキー離した
//
//---------------------------------------------------------------------------
void CDrawView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 除外したいキーを判別
//	if (!KeyUpDown(nChar, nFlags, FALSE)) {
//		return;
//	}

	// 基本クラスへ流す
	CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}
//---------------------------------------------------------------------------
//
//	ウィンドウ移動
//
//---------------------------------------------------------------------------
void CDrawView::OnMove(int x, int y)
{
	ASSERT(m_pFrmWnd);

	// 基本クラス
	CView::OnMove(x, y);

	// フレームウィンドウの再配置を呼ぶ
//	m_pFrmWnd->RecalcStatusView();
}

//---------------------------------------------------------------------------
//
//	ビットマップ準備
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::SetupBitmap()
{
	CClientDC *pDC;
	BITMAPINFOHEADER *p;
	CRect rect;

	// ビットマップがあれば、一旦解放
	if (m_Info.hBitmap) {
		if (m_Info.pRender) {
			m_Info.pRender->SetMixBuf(NULL, 0, 0);
		}
		::DeleteObject(m_Info.hBitmap);
		m_Info.hBitmap = NULL;
		m_Info.pBits = NULL;
	}

	// 最小化は特別扱い
	GetClientRect(&rect);
	if ((rect.Width() == 0) || (rect.Height() == 0)) {
		return;
	}

	// ビットマップヘッダのためのメモリを確保
	p = (BITMAPINFOHEADER*) new BYTE[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)];
	memset(p, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD));

	// ビットマップ情報作成
	m_Info.nBMPWidth = rect.Width();
	m_Info.nBMPHeight = rect.Height();
	p->biSize = sizeof(BITMAPINFOHEADER);
	p->biWidth = m_Info.nBMPWidth;
	p->biHeight = -m_Info.nBMPHeight;
	p->biPlanes = 1;
	p->biBitCount = 32;
	p->biCompression = BI_RGB;
	p->biSizeImage = m_Info.nBMPWidth * m_Info.nBMPHeight * (32 >> 3);

	// DC取得、DIBセクション作成
	pDC = new CClientDC(this);
	m_Info.hBitmap = ::CreateDIBSection(pDC->m_hDC, (BITMAPINFO*)p, DIB_RGB_COLORS,
								(void**)&(m_Info.pBits), NULL, 0);
	// 成功したら、レンダラに伝える
	if (m_Info.hBitmap && m_Info.pRender) {
		m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
	}
	delete pDC;
	delete[] p;

	// 再計算
	m_Info.nRendHMul = -1;
	m_Info.nRendVMul = -1;
	ReCalc(rect);
}

//---------------------------------------------------------------------------
//
//	動作制御
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Enable(BOOL bEnable)
{
	// フラグ記憶
	m_bEnable = bEnable;

	// 有効ならレンダラ記憶
	if (m_bEnable) {
		if (!m_Info.pRender) {
			m_Info.pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
			ASSERT(m_Info.pRender);
			m_Info.pWork = m_Info.pRender->GetWorkAddr();
			ASSERT(m_Info.pWork);
			if (m_Info.pBits) {
				m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
			}
		}
	}

	// サブウィンドウに対し、指示
//	CSubWnd *pWnd;
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->Enable(bEnable);
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	動作フラグ取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::IsEnable() const
{
	return m_bEnable;
}

//---------------------------------------------------------------------------
//
//	リフレッシュ描画
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Refresh()
{
	CClientDC dc(this);

	// Drawビューを再描画
	OnDraw(&dc);

	// サブウィンドウを再描画
//	CSubWnd *pWnd;
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->Refresh();
//
//		// 次のサブウィンドウ
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	描画(スケジューラより)
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Draw(int nChildWnd)
{
	CClientDC *pDC;

	ASSERT(nChildWnd >= -1);

	// -1はDrawビュー
	if (nChildWnd < 0) {
		pDC = new CClientDC(this);
		OnDraw(pDC);
		delete pDC;
		return;
	}

	// 0以降はサブウィンドウ
//	CSubWnd *pSubWnd;
//	pSubWnd = m_pSubWnd;
//
//	while (nChildWnd > 0) {
//		// 次のサブウィンドウ
//		pSubWnd = pSubWnd->m_pNextWnd;
//		ASSERT(pSubWnd);
//		nChildWnd--;
//	}
//
//	// リフレッシュ
//	pSubWnd->Refresh();
}

//---------------------------------------------------------------------------
//
//	メッセージスレッドからの更新
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Update()
{
//	CSubWnd *pWnd;
//
//	// サブウィンドウに対し、指示
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->Update();
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ApplyCfg(const Config *pConfig)
{
	ASSERT(pConfig);

	// ストレッチ
	Stretch(pConfig->aspect_stretch);

	// サブウィンドウに対し、指示
//	CSubWnd *pWnd;
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->ApplyCfg(pConfig);
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	描画情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::GetDrawInfo(LPDRAWINFO pDrawInfo) const
{
	ASSERT(this);
	ASSERT(pDrawInfo);

	// 内部ワークをコピー
	*pDrawInfo = m_Info;
}

//---------------------------------------------------------------------------
//
//	描画
//
//---------------------------------------------------------------------------
void CDrawView::OnDraw(CDC *pDC)
{
	CRect rect;
	HDC hMemDC;
	HBITMAP hDefBitmap;
	int i;
	int vmul;
	int hmul;

	// ビットマップの準備が出来てなければ塗りつぶし
	GetClientRect(&rect);
	if (!m_Info.hBitmap || !m_bEnable || !m_Info.pWork) {
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		return;
	}

	// 再計算
	ReCalc(rect);

	// 電源OFF対策
	if (::GetVM()->IsPower() != m_Info.bPower) {
		m_Info.bPower = ::GetVM()->IsPower();
		if (!m_Info.bPower) {
			// ビットマップをすべて消去
			memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
			m_Info.bBltAll = TRUE;
		}
	}

	// 隅を描画
	if (m_Info.bBltAll) {
		DrawRect(pDC);
	}

	// 最終表示倍率を確定
	hmul = 1;
	if (m_Info.nRendHMul == 2) {
		// 横256など
		hmul = 2;
	}
	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if文改良要
		// 768×512以外で、アスペクト同一モード指定
		hmul *= 5;
	}
	else {
		// アスペクト比同一はしない。等倍拡大
		hmul <<= 2;
	}
	vmul = 4;
	if (m_Info.nRendVMul == 2) {
		// 縦256など
		vmul = 8;
	}

	// bBltAllの時は全領域で決まり
	if (m_Info.bBltAll) {
		// メモリDC作成、セレクト
		hMemDC = CreateCompatibleDC(pDC->m_hDC);
		if (!hMemDC) {
			return;
		}
		hDefBitmap = (HBITMAP)SelectObject(hMemDC, m_Info.hBitmap);
		if (!hDefBitmap) {
			DeleteDC(hMemDC);
			return;
		}

		// Blt
		if ((hmul == 4) && (vmul == 4)) {
			::BitBlt(pDC->m_hDC,
				m_Info.nLeft, m_Info.nTop,
				m_Info.nWidth, m_Info.nHeight,
				hMemDC, 0, 0,
				SRCCOPY);
		}
		else {
			::StretchBlt(pDC->m_hDC,
				m_Info.nLeft, m_Info.nTop,
				(m_Info.nWidth * hmul) >> 2,
				(m_Info.nHeight * vmul) >> 2,
				hMemDC, 0, 0,
				m_Info.nWidth, m_Info.nHeight,
				SRCCOPY);
		}
		::GdiFlush();
		m_Info.bBltAll = FALSE;

		// ビットマップ戻す
		SelectObject(hMemDC, hDefBitmap);
		DeleteDC(hMemDC);

		// 描画フラグを降ろすことを忘れずに
		for (i=0; i<m_Info.nHeight * 64; i++) {
			m_Info.pWork->drawflag[i] = FALSE;
		}
		m_Info.dwDrawCount++;
		m_Info.nBltLeft = 0;
		m_Info.nBltTop = 0;
		m_Info.nBltRight = m_Info.nWidth - 1;
		m_Info.nBltBottom = m_Info.nHeight - 1;
		return;
	}

	// 描画領域を調べる
	if (!CalcRect()) {
		return;
	}
	ASSERT(m_Info.nBltTop <= m_Info.nBltBottom);
	ASSERT(m_Info.nBltLeft <= m_Info.nBltRight);

	// メモリDC作成、セレクト
	hMemDC = CreateCompatibleDC(pDC->m_hDC);
	if (!hMemDC) {
		m_Info.bBltAll = TRUE;
		return;
	}
	hDefBitmap = (HBITMAP)SelectObject(hMemDC, m_Info.hBitmap);
	if (!hDefBitmap) {
		DeleteDC(hMemDC);
		m_Info.bBltAll = TRUE;
		return;
	}

	// 一部領域のみ描画
	if ((hmul == 4) && (vmul == 4)) {
		::BitBlt(pDC->m_hDC,
			m_Info.nLeft + m_Info.nBltLeft,
			m_Info.nTop + m_Info.nBltTop,
			m_Info.nBltRight - m_Info.nBltLeft + 1,
			m_Info.nBltBottom - m_Info.nBltTop + 1,
			hMemDC,
			m_Info.nBltLeft,
			m_Info.nBltTop,
			SRCCOPY);
	}
	else {
		::StretchBlt(pDC->m_hDC,
			m_Info.nLeft + ((m_Info.nBltLeft * hmul) >> 2),
			m_Info.nTop + ((m_Info.nBltTop * vmul) >> 2),
			((m_Info.nBltRight - m_Info.nBltLeft + 1) * hmul) >> 2,
			((m_Info.nBltBottom - m_Info.nBltTop + 1) * vmul) >> 2,
			hMemDC,
			m_Info.nBltLeft,
			m_Info.nBltTop,
			m_Info.nBltRight - m_Info.nBltLeft + 1,
			m_Info.nBltBottom - m_Info.nBltTop + 1,
			SRCCOPY);
	}
	::GdiFlush();

	// ビットマップ戻す
	SelectObject(hMemDC, hDefBitmap);
	DeleteDC(hMemDC);

	// フラグはCalcRectで降ろしている
	m_Info.dwDrawCount++;
}

//---------------------------------------------------------------------------
//
//	再計算
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ReCalc(CRect& rect)
{
	int width;
	int height;
	BOOL flag;

	// レンダラワーク、ビットマップがなければreturn
	if (!m_Info.pWork || !m_Info.hBitmap) {
		return;
	}

	// 比較
	flag = FALSE;
	if (m_Info.nRendWidth != m_Info.pWork->width) {
		m_Info.nRendWidth = m_Info.pWork->width;
		flag = TRUE;
	}
	if (m_Info.nRendHeight != m_Info.pWork->height) {
		m_Info.nRendHeight = m_Info.pWork->height;
		flag = TRUE;
	}
	if (m_Info.nRendHMul != m_Info.pWork->h_mul) {
		m_Info.nRendHMul = m_Info.pWork->h_mul;
		flag = TRUE;
	}
	if (m_Info.nRendVMul != m_Info.pWork->v_mul) {
		m_Info.nRendVMul = m_Info.pWork->v_mul;
		flag = TRUE;
	}
	if (!flag) {
		return;
	}

	// レンダラ、ビットマップのうち小さいほうをとる
	m_Info.nWidth = m_Info.nRendWidth;
	if (m_Info.nBMPWidth < m_Info.nWidth) {
		m_Info.nWidth = m_Info.nBMPWidth;
	}
	m_Info.nHeight = m_Info.nRendHeight;
	if (m_Info.nRendVMul == 0) {
		// 15kインタレースのための処理
		m_Info.nHeight <<= 1;
	}
	if (m_Info.nBMPHeight < m_Info.nRendHeight) {
		m_Info.nHeight = m_Info.nBMPHeight;
	}

	// 倍率を考慮してセンタリングし、余白を算出
	width = m_Info.nWidth * m_Info.nRendHMul;
	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if改良要
		width = (width * 5) >> 2;
	}
	height = m_Info.nHeight;
	if (m_Info.nRendVMul == 2) {
		height <<= 1;
	}

	m_Info.nLeft = 0;
	if (width < rect.Width()) {
		m_Info.nLeft = (rect.Width() - width) >> 1;
	}
	m_Info.nTop = 0;
	if (height < rect.Height()) {
		m_Info.nTop = (rect.Height() - height) >> 1;
	}

	// 領域描画を指定
	m_Info.bBltAll = TRUE;
}

//---------------------------------------------------------------------------
//
//	拡大モード
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Stretch(BOOL bStretch)
{
	CRect rect;

	ASSERT(this);

	// 一致なら何もしない
	if (bStretch == m_Info.bBltStretch) {
		return;
	}
	m_Info.bBltStretch = bStretch;

	// 768×512でなければ、再計算
	if ((m_Info.nRendWidth > 0) && (m_Info.nRendWidth < 600)) {		// if文改良要
		m_Info.nRendWidth = m_Info.pWork->width + 1;
		GetClientRect(&rect);
		ReCalc(rect);
	}
}

//---------------------------------------------------------------------------
//
//	隅を描画
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::DrawRect(CDC *pDC)
{
	CRect crect;
	CRect brect;

	ASSERT(m_Info.bBltAll);
	ASSERT(pDC);

	// 全部使うようなら必要無い
	if ((m_Info.nLeft == 0) && (m_Info.nTop == 0)) {
		return;
	}

	// クライアント矩形を得る
	GetClientRect(&crect);

	if (m_Info.nLeft > 0) {
		// 左半分
		brect.left = 0;
		brect.top = 0;
		brect.right = m_Info.nLeft;
		brect.bottom = crect.bottom;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));

		// 右半分
		brect.right = crect.right;
		brect.left = brect.right - m_Info.nLeft - 1;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));
	}

	if (m_Info.nTop > 0) {
		// 上半分
		brect.left = 0;
		brect.top = 0;
		brect.right = crect.right;
		brect.bottom = m_Info.nTop;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));

		// 右半分
		brect.bottom = crect.bottom;
		brect.top = brect.bottom - m_Info.nTop - 1;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));
	}
}

//---------------------------------------------------------------------------
//
//	描画範囲を調べる
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::CalcRect()
{
	int i;
	int j;
	int left;
	int top;
	int right;
	int bottom;
	BOOL *p;
	BOOL flag;

	// 初期化
	left = 64;
	top = 2048;
	right = -1;
	bottom = -1;
	p = m_Info.pWork->drawflag;

	// yループ
	for (i=0; i<m_Info.nHeight; i++) {
		flag = FALSE;

		// xループ
		for(j=0; j<64; j++) {
			if (*p) {
				// 消す
				*p = FALSE;

				// この16dotは描画が必要
				if (left > j) {
					left = j;
				}
				if (right < j) {
					right = j;
				}
				flag = TRUE;
			}
			p++;
		}

		if (flag) {
			// このラインは描画が必要
			if (top > i) {
				top = i;
			}
			if (bottom < i) {
				bottom = i;
			}
		}
	}

	// yが変化なければ必要無し
	if (bottom < top) {
		return FALSE;
	}

	// xを補正(x16)
	left <<= 4;
	right = ((right + 1) << 4) - 1;
	if (right >= m_Info.nWidth) {
		right = m_Info.nWidth - 1;
	}

	// コピー
	m_Info.nBltLeft = left;
	m_Info.nBltTop = top;
	m_Info.nBltRight = right;
	m_Info.nBltBottom = bottom;

	return TRUE;
}
#endif	// _WIN32
