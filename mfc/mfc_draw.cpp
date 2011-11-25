//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC Draw�r���[ ]
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
//	Draw�r���[
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CDrawView::CDrawView()
{
	// ���[�N������(��{)
	m_bEnable = FALSE;
//	m_pSubWnd = NULL;
	m_pFrmWnd = NULL;

	// �R���|�[�l���g
	m_pScheduler = NULL;
//	m_pInput = NULL;

	// ���[�N������(�`��S��)
	m_Info.bPower = FALSE;
	m_Info.pRender = NULL;
	m_Info.pWork = NULL;
	m_Info.dwDrawCount = 0;

	// ���[�N������(DIB�Z�N�V����)
	m_Info.hBitmap = NULL;
	m_Info.pBits = NULL;
	m_Info.nBMPWidth = 0;
	m_Info.nBMPHeight = 0;

	// ���[�N������(�T�C�Y����)
	m_Info.nRendWidth = 0;
	m_Info.nRendHeight = 0;
	m_Info.nRendHMul = 0;
	m_Info.nRendVMul = 0;
	m_Info.nLeft = 0;
	m_Info.nTop = 0;
	m_Info.nWidth = 0;
	m_Info.nHeight = 0;

	// ���[�N������(Blt)
	m_Info.nBltTop = 0;
	m_Info.nBltBottom = 0;
	m_Info.nBltLeft = 0;
	m_Info.nBltRight = 0;
	m_Info.bBltAll = TRUE;
	m_Info.bBltStretch = FALSE;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
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
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::Init(CWnd *pParent)
{
	ASSERT(pParent);

	// �t���[���E�B���h�E�L��
	m_pFrmWnd = (CFrmWnd*)pParent;

	// �ŏ��̃r���[�Ƃ��č쐬
	if (!Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
				CRect(0, 0, 0, 0), pParent, AFX_IDW_PANE_FIRST, NULL)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬����
//
//---------------------------------------------------------------------------
BOOL CDrawView::PreCreateWindow(CREATESTRUCT& cs)
{
	// ��{�N���X
	if (!CView::PreCreateWindow(cs)) {
		return FALSE;
	}

	// WS_CLIPCHILDREN��ǉ�
	cs.style |= WS_CLIPCHILDREN;

	// �N���C�A���g�G�b�W��ǉ�
	cs.dwExStyle |= WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CDrawView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// ��{�N���X
	if (CView::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// IME�I�t
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// �e�L�X�g�t�H���g�쐬
	if (IsJapanese()) {
		// ���{���
		m_TextFont.CreateFont(14, 0, 0, 0,
							FW_NORMAL, 0, 0, 0,
							SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							FIXED_PITCH, NULL);
	}
	else {
		// �p���
		m_TextFont.CreateFont(14, 0, 0, 0,
							FW_NORMAL, 0, 0, 0,
							DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							FIXED_PITCH, NULL);
	}

	// �h���b�O���h���b�v����
	DragAcceptFiles(TRUE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CDrawView::OnDestroy()
{
	// �����~
	Enable(FALSE);

	// �r�b�g�}�b�v���폜
	if (m_Info.hBitmap) {
		::DeleteObject(m_Info.hBitmap);
		m_Info.hBitmap = NULL;
		m_Info.pBits = NULL;
	}

	// �e�L�X�g�t�H���g�폜
	m_TextFont.DeleteObject();

	// ��{�N���X��
	CView::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�T�C�Y�ύX
//
//---------------------------------------------------------------------------
void CDrawView::OnSize(UINT nType, int cx, int cy)
{
	// �r�b�g�}�b�v�̍X�V
	SetupBitmap();

	// ��{�N���X
	CView::OnSize(nType, cx, cy);
}

//---------------------------------------------------------------------------
//
//	�`��
//
//---------------------------------------------------------------------------
void CDrawView::OnPaint()
{
	Render *pRender;
	CRTC *pCRTC;
	const CRTC::crtc_t *p;
	CFrmWnd *pFrmWnd;
	PAINTSTRUCT ps;

	// VM���b�N
	::LockVM();

	// �S�`��t���OON
	m_Info.bBltAll = TRUE;

	// �C�l�[�u���ŃX�P�W���[����OFF�Ȃ�AMix�o�b�t�@�ɍ쐬(���Ȃ苭��)
	if (m_bEnable) {
		pFrmWnd = (CFrmWnd*)GetParent();
		ASSERT(pFrmWnd);
		if (!schedulerIsEnable()) {
			// �C�l�[�u���Ȃ�K��Render,CRTC�͑���
			pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
			ASSERT(pRender);
			pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
			ASSERT(pCRTC);
			p = pCRTC->GetWorkAddr();

			// �쐬
			m_Info.bPower = ::GetVM()->IsPower();
			if (m_Info.bPower) {
				pRender->Complete();
				pRender->EnableAct(TRUE);
				pRender->StartFrame();
				pRender->HSync(p->v_dots);
				pRender->EndFrame();
			}
			else {
				// �r�b�g�}�b�v�����ׂď���
				memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
			}

			// �`��(CDrawView::OnDraw�ւȂ���)
			CView::OnPaint();

			// VM�A�����b�N
			::UnlockVM();
			return;
		}
	}

	// DC�������Ă���(�_�~�[)
	BeginPaint(&ps);
	EndPaint(&ps);

	// VM�A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�w�i�`��
//
//---------------------------------------------------------------------------
BOOL CDrawView::OnEraseBkgnd(CDC *pDC)
{
	CRect rect;

	// �C�l�[�u���łȂ���΁A���œh��Ԃ�
	if (!m_bEnable) {
		GetClientRect(&rect);
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�\�����ύX
//
//---------------------------------------------------------------------------
LRESULT CDrawView::OnDisplayChange(WPARAM , LPARAM )
{
	// �r�b�g�}�b�v����
	SetupBitmap();

	return 0;
}
//---------------------------------------------------------------------------
//
//	�L�[����
//
//---------------------------------------------------------------------------
void CDrawView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// ���O�������L�[�𔻕�
//	if (!KeyUpDown(nChar, nFlags, TRUE)) {
//		return;
//	}

	// ��{�N���X�֗���
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	�V�X�e���L�[����
//
//---------------------------------------------------------------------------
void CDrawView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// ���O�������L�[�𔻕�
//	if (!KeyUpDown(nChar, nFlags, TRUE)) {
//		return;
//	}

	// ��{�N���X�֗���
	CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	�L�[������
//
//---------------------------------------------------------------------------
void CDrawView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// ���O�������L�[�𔻕�
//	if (!KeyUpDown(nChar, nFlags, FALSE)) {
//		return;
//	}

	// ��{�N���X�֗���
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	�V�X�e���L�[������
//
//---------------------------------------------------------------------------
void CDrawView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// ���O�������L�[�𔻕�
//	if (!KeyUpDown(nChar, nFlags, FALSE)) {
//		return;
//	}

	// ��{�N���X�֗���
	CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}
//---------------------------------------------------------------------------
//
//	�E�B���h�E�ړ�
//
//---------------------------------------------------------------------------
void CDrawView::OnMove(int x, int y)
{
	ASSERT(m_pFrmWnd);

	// ��{�N���X
	CView::OnMove(x, y);

	// �t���[���E�B���h�E�̍Ĕz�u���Ă�
//	m_pFrmWnd->RecalcStatusView();
}

//---------------------------------------------------------------------------
//
//	�r�b�g�}�b�v����
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::SetupBitmap()
{
	CClientDC *pDC;
	BITMAPINFOHEADER *p;
	CRect rect;

	// �r�b�g�}�b�v������΁A��U���
	if (m_Info.hBitmap) {
		if (m_Info.pRender) {
			m_Info.pRender->SetMixBuf(NULL, 0, 0);
		}
		::DeleteObject(m_Info.hBitmap);
		m_Info.hBitmap = NULL;
		m_Info.pBits = NULL;
	}

	// �ŏ����͓��ʈ���
	GetClientRect(&rect);
	if ((rect.Width() == 0) || (rect.Height() == 0)) {
		return;
	}

	// �r�b�g�}�b�v�w�b�_�̂��߂̃��������m��
	p = (BITMAPINFOHEADER*) new BYTE[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)];
	memset(p, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD));

	// �r�b�g�}�b�v���쐬
	m_Info.nBMPWidth = rect.Width();
	m_Info.nBMPHeight = rect.Height();
	p->biSize = sizeof(BITMAPINFOHEADER);
	p->biWidth = m_Info.nBMPWidth;
	p->biHeight = -m_Info.nBMPHeight;
	p->biPlanes = 1;
	p->biBitCount = 32;
	p->biCompression = BI_RGB;
	p->biSizeImage = m_Info.nBMPWidth * m_Info.nBMPHeight * (32 >> 3);

	// DC�擾�ADIB�Z�N�V�����쐬
	pDC = new CClientDC(this);
	m_Info.hBitmap = ::CreateDIBSection(pDC->m_hDC, (BITMAPINFO*)p, DIB_RGB_COLORS,
								(void**)&(m_Info.pBits), NULL, 0);
	// ����������A�����_���ɓ`����
	if (m_Info.hBitmap && m_Info.pRender) {
		m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
	}
	delete pDC;
	delete[] p;

	// �Čv�Z
	m_Info.nRendHMul = -1;
	m_Info.nRendVMul = -1;
	ReCalc(rect);
}

//---------------------------------------------------------------------------
//
//	���쐧��
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Enable(BOOL bEnable)
{
	// �t���O�L��
	m_bEnable = bEnable;

	// �L���Ȃ烌���_���L��
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

	// �T�u�E�B���h�E�ɑ΂��A�w��
//	CSubWnd *pWnd;
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->Enable(bEnable);
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	����t���O�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::IsEnable() const
{
	return m_bEnable;
}

//---------------------------------------------------------------------------
//
//	���t���b�V���`��
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Refresh()
{
	CClientDC dc(this);

	// Draw�r���[���ĕ`��
	OnDraw(&dc);

	// �T�u�E�B���h�E���ĕ`��
//	CSubWnd *pWnd;
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->Refresh();
//
//		// ���̃T�u�E�B���h�E
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	�`��(�X�P�W���[�����)
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Draw(int nChildWnd)
{
	CClientDC *pDC;

	ASSERT(nChildWnd >= -1);

	// -1��Draw�r���[
	if (nChildWnd < 0) {
		pDC = new CClientDC(this);
		OnDraw(pDC);
		delete pDC;
		return;
	}

	// 0�ȍ~�̓T�u�E�B���h�E
//	CSubWnd *pSubWnd;
//	pSubWnd = m_pSubWnd;
//
//	while (nChildWnd > 0) {
//		// ���̃T�u�E�B���h�E
//		pSubWnd = pSubWnd->m_pNextWnd;
//		ASSERT(pSubWnd);
//		nChildWnd--;
//	}
//
//	// ���t���b�V��
//	pSubWnd->Refresh();
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Update()
{
//	CSubWnd *pWnd;
//
//	// �T�u�E�B���h�E�ɑ΂��A�w��
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->Update();
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ApplyCfg(const Config *pConfig)
{
	ASSERT(pConfig);

	// �X�g���b�`
	Stretch(pConfig->aspect_stretch);

	// �T�u�E�B���h�E�ɑ΂��A�w��
//	CSubWnd *pWnd;
//	pWnd = m_pSubWnd;
//	while (pWnd) {
//		pWnd->ApplyCfg(pConfig);
//		pWnd = pWnd->m_pNextWnd;
//	}
}

//---------------------------------------------------------------------------
//
//	�`����擾
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::GetDrawInfo(LPDRAWINFO pDrawInfo) const
{
	ASSERT(this);
	ASSERT(pDrawInfo);

	// �������[�N���R�s�[
	*pDrawInfo = m_Info;
}

//---------------------------------------------------------------------------
//
//	�`��
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

	// �r�b�g�}�b�v�̏������o���ĂȂ���Γh��Ԃ�
	GetClientRect(&rect);
	if (!m_Info.hBitmap || !m_bEnable || !m_Info.pWork) {
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		return;
	}

	// �Čv�Z
	ReCalc(rect);

	// �d��OFF�΍�
	if (::GetVM()->IsPower() != m_Info.bPower) {
		m_Info.bPower = ::GetVM()->IsPower();
		if (!m_Info.bPower) {
			// �r�b�g�}�b�v�����ׂď���
			memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
			m_Info.bBltAll = TRUE;
		}
	}

	// ����`��
	if (m_Info.bBltAll) {
		DrawRect(pDC);
	}

	// �ŏI�\���{�����m��
	hmul = 1;
	if (m_Info.nRendHMul == 2) {
		// ��256�Ȃ�
		hmul = 2;
	}
	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if�����Ǘv
		// 768�~512�ȊO�ŁA�A�X�y�N�g���ꃂ�[�h�w��
		hmul *= 5;
	}
	else {
		// �A�X�y�N�g�䓯��͂��Ȃ��B���{�g��
		hmul <<= 2;
	}
	vmul = 4;
	if (m_Info.nRendVMul == 2) {
		// �c256�Ȃ�
		vmul = 8;
	}

	// bBltAll�̎��͑S�̈�Ō��܂�
	if (m_Info.bBltAll) {
		// ������DC�쐬�A�Z���N�g
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

		// �r�b�g�}�b�v�߂�
		SelectObject(hMemDC, hDefBitmap);
		DeleteDC(hMemDC);

		// �`��t���O���~�낷���Ƃ�Y�ꂸ��
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

	// �`��̈�𒲂ׂ�
	if (!CalcRect()) {
		return;
	}
	ASSERT(m_Info.nBltTop <= m_Info.nBltBottom);
	ASSERT(m_Info.nBltLeft <= m_Info.nBltRight);

	// ������DC�쐬�A�Z���N�g
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

	// �ꕔ�̈�̂ݕ`��
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

	// �r�b�g�}�b�v�߂�
	SelectObject(hMemDC, hDefBitmap);
	DeleteDC(hMemDC);

	// �t���O��CalcRect�ō~�낵�Ă���
	m_Info.dwDrawCount++;
}

//---------------------------------------------------------------------------
//
//	�Čv�Z
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ReCalc(CRect& rect)
{
	int width;
	int height;
	BOOL flag;

	// �����_�����[�N�A�r�b�g�}�b�v���Ȃ����return
	if (!m_Info.pWork || !m_Info.hBitmap) {
		return;
	}

	// ��r
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

	// �����_���A�r�b�g�}�b�v�̂����������ق����Ƃ�
	m_Info.nWidth = m_Info.nRendWidth;
	if (m_Info.nBMPWidth < m_Info.nWidth) {
		m_Info.nWidth = m_Info.nBMPWidth;
	}
	m_Info.nHeight = m_Info.nRendHeight;
	if (m_Info.nRendVMul == 0) {
		// 15k�C���^���[�X�̂��߂̏���
		m_Info.nHeight <<= 1;
	}
	if (m_Info.nBMPHeight < m_Info.nRendHeight) {
		m_Info.nHeight = m_Info.nBMPHeight;
	}

	// �{�����l�����ăZ���^�����O���A�]�����Z�o
	width = m_Info.nWidth * m_Info.nRendHMul;
	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if���Ǘv
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

	// �̈�`����w��
	m_Info.bBltAll = TRUE;
}

//---------------------------------------------------------------------------
//
//	�g�僂�[�h
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Stretch(BOOL bStretch)
{
	CRect rect;

	ASSERT(this);

	// ��v�Ȃ牽�����Ȃ�
	if (bStretch == m_Info.bBltStretch) {
		return;
	}
	m_Info.bBltStretch = bStretch;

	// 768�~512�łȂ���΁A�Čv�Z
	if ((m_Info.nRendWidth > 0) && (m_Info.nRendWidth < 600)) {		// if�����Ǘv
		m_Info.nRendWidth = m_Info.pWork->width + 1;
		GetClientRect(&rect);
		ReCalc(rect);
	}
}

//---------------------------------------------------------------------------
//
//	����`��
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::DrawRect(CDC *pDC)
{
	CRect crect;
	CRect brect;

	ASSERT(m_Info.bBltAll);
	ASSERT(pDC);

	// �S���g���悤�Ȃ�K�v����
	if ((m_Info.nLeft == 0) && (m_Info.nTop == 0)) {
		return;
	}

	// �N���C�A���g��`�𓾂�
	GetClientRect(&crect);

	if (m_Info.nLeft > 0) {
		// ������
		brect.left = 0;
		brect.top = 0;
		brect.right = m_Info.nLeft;
		brect.bottom = crect.bottom;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));

		// �E����
		brect.right = crect.right;
		brect.left = brect.right - m_Info.nLeft - 1;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));
	}

	if (m_Info.nTop > 0) {
		// �㔼��
		brect.left = 0;
		brect.top = 0;
		brect.right = crect.right;
		brect.bottom = m_Info.nTop;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));

		// �E����
		brect.bottom = crect.bottom;
		brect.top = brect.bottom - m_Info.nTop - 1;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));
	}
}

//---------------------------------------------------------------------------
//
//	�`��͈͂𒲂ׂ�
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

	// ������
	left = 64;
	top = 2048;
	right = -1;
	bottom = -1;
	p = m_Info.pWork->drawflag;

	// y���[�v
	for (i=0; i<m_Info.nHeight; i++) {
		flag = FALSE;

		// x���[�v
		for(j=0; j<64; j++) {
			if (*p) {
				// ����
				*p = FALSE;

				// ����16dot�͕`�悪�K�v
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
			// ���̃��C���͕`�悪�K�v
			if (top > i) {
				top = i;
			}
			if (bottom < i) {
				bottom = i;
			}
		}
	}

	// y���ω��Ȃ���ΕK�v����
	if (bottom < top) {
		return FALSE;
	}

	// x��␳(x16)
	left <<= 4;
	right = ((right + 1) << 4) - 1;
	if (right >= m_Info.nWidth) {
		right = m_Info.nWidth - 1;
	}

	// �R�s�[
	m_Info.nBltLeft = left;
	m_Info.nBltTop = top;
	m_Info.nBltRight = right;
	m_Info.nBltBottom = bottom;

	return TRUE;
}
#endif	// _WIN32
