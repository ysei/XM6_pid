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
#include "mfc_res.h"
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
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::Init(CWnd *pParent)
{
	return Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, CRect(0, 0, 0, 0), pParent, AFX_IDW_PANE_FIRST, NULL);
}
//---------------------------------------------------------------------------
//
//	�`��
//
//---------------------------------------------------------------------------
void CDrawView::OnDraw(CDC *pDC)
{
	// �������[�N��`
	struct DRAWINFO {
		BOOL bPower;					// �d��
		Render *pRender;				// �����_��
		Render::render_t *pWork;		// �����_�����[�N
        DWORD dwDrawCount;				// �`���

		// DIB�Z�N�V����
		HBITMAP hBitmap;				// DIB�Z�N�V����
		DWORD *pBits;					// �r�b�g�f�[�^
		int nBMPWidth;					// BMP��
		int nBMPHeight;					// BMP����

		// �����_���A�g
		int nRendWidth;					// �����_����
		int nRendHeight;				// �����_������
		int nRendHMul;					// �����_���������{��
		int nRendVMul;					// �����_���c�����{��
		int nLeft;						// ���}�[�W��
		int nTop;						// �c�}�[�W��
		int nWidth;						// BitBlt��
		int nHeight;					// BitBlt����

		// Blt�n
		int nBltTop;					// �`��J�nY
		int nBltBottom;					// �`��I��Y
		int nBltLeft;					// �`��J�nX
		int nBltRight;					// �`��I��X
		BOOL bBltAll;					// �S�\���t���O
	};

	static DRAWINFO m_Info;										// �������[�N

	m_Info.bBltAll	= TRUE;

	if(m_Info.hBitmap == 0) {
		CClientDC *pDC;
		BITMAPINFOHEADER *p;
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
		CRect rect;
		GetClientRect(&rect);
		if ((rect.Width() != 0) && (rect.Height() != 0)) {
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
		}
	}

	if(m_Info.hBitmap && m_Info.pRender == 0) {
		m_Info.pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
		ASSERT(m_Info.pRender);
		m_Info.pWork = m_Info.pRender->GetWorkAddr();
		ASSERT(m_Info.pWork);
		if (m_Info.pBits) {
			m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
		}
	}

	if(m_Info.hBitmap && m_Info.pRender) {
		// �r�b�g�}�b�v�̏������o���ĂȂ���Γh��Ԃ�
		CRect rect;
		GetClientRect(&rect);
		if (!m_Info.hBitmap || !m_Info.pWork) {
			pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		} else {
			// �Čv�Z
			// �����_�����[�N�A�r�b�g�}�b�v���Ȃ����return
			if(m_Info.pWork && m_Info.hBitmap) {
				// ��r
				BOOL flag = FALSE;
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
				if(flag) {
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
					int width = m_Info.nWidth * m_Info.nRendHMul;
				//	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if���Ǘv
				//		width = (width * 5) >> 2;
				//	}
					int height = m_Info.nHeight;
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
			}

			// �d��OFF�΍�
			if (::GetVM()->IsPower() != m_Info.bPower) {
				m_Info.bPower = ::GetVM()->IsPower();
				if (!m_Info.bPower) {
					// �r�b�g�}�b�v�����ׂď���
					memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
					m_Info.bBltAll = TRUE;
				}
			}

			// �ŏI�\���{�����m��
			int hmul;
			hmul = 1;
			if (m_Info.nRendHMul == 2) {
				// ��256�Ȃ�
				hmul = 2;
			}
			{
				// �A�X�y�N�g�䓯��͂��Ȃ��B���{�g��
				hmul <<= 2;
			}

			int vmul;
			vmul = 4;
			if (m_Info.nRendVMul == 2) {
				// �c256�Ȃ�
				vmul = 8;
			}

			// ������DC�쐬�A�Z���N�g
			HDC hMemDC = CreateCompatibleDC(pDC->m_hDC);
			if(hMemDC) {
				HBITMAP hDefBitmap = (HBITMAP)SelectObject(hMemDC, m_Info.hBitmap);
				if(hDefBitmap) {
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

					// �`��t���O���~�낷���Ƃ�Y�ꂸ��
					for (int i=0; i<m_Info.nHeight * 64; i++) {
						m_Info.pWork->drawflag[i] = FALSE;
					}
					m_Info.dwDrawCount++;
					m_Info.nBltLeft = 0;
					m_Info.nBltTop = 0;
					m_Info.nBltRight = m_Info.nWidth - 1;
					m_Info.nBltBottom = m_Info.nHeight - 1;
				}
				DeleteDC(hMemDC);
			}
		}
	}
}
#endif	// _WIN32
