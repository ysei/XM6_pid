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
#include "mfc_res.h"
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
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::Init(CWnd *pParent)
{
	return Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, CRect(0, 0, 0, 0), pParent, AFX_IDW_PANE_FIRST, NULL);
}
//---------------------------------------------------------------------------
//
//	描画
//
//---------------------------------------------------------------------------
void CDrawView::OnDraw(CDC *pDC)
{
	// 内部ワーク定義
	struct DRAWINFO {
		BOOL bPower;					// 電源
		Render *pRender;				// レンダラ
		Render::render_t *pWork;		// レンダラワーク
        DWORD dwDrawCount;				// 描画回数

		// DIBセクション
		HBITMAP hBitmap;				// DIBセクション
		DWORD *pBits;					// ビットデータ
		int nBMPWidth;					// BMP幅
		int nBMPHeight;					// BMP高さ

		// レンダラ連携
		int nRendWidth;					// レンダラ幅
		int nRendHeight;				// レンダラ高さ
		int nRendHMul;					// レンダラ横方向倍率
		int nRendVMul;					// レンダラ縦方向倍率
		int nLeft;						// 横マージン
		int nTop;						// 縦マージン
		int nWidth;						// BitBlt幅
		int nHeight;					// BitBlt高さ

		// Blt系
		int nBltTop;					// 描画開始Y
		int nBltBottom;					// 描画終了Y
		int nBltLeft;					// 描画開始X
		int nBltRight;					// 描画終了X
		BOOL bBltAll;					// 全表示フラグ
	};

	static DRAWINFO m_Info;										// 内部ワーク

	m_Info.bBltAll	= TRUE;

	if(m_Info.hBitmap == 0) {
		CClientDC *pDC;
		BITMAPINFOHEADER *p;
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
		CRect rect;
		GetClientRect(&rect);
		if ((rect.Width() != 0) && (rect.Height() != 0)) {
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
		// ビットマップの準備が出来てなければ塗りつぶし
		CRect rect;
		GetClientRect(&rect);
		if (!m_Info.hBitmap || !m_Info.pWork) {
			pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		} else {
			// 再計算
			// レンダラワーク、ビットマップがなければreturn
			if(m_Info.pWork && m_Info.hBitmap) {
				// 比較
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
					int width = m_Info.nWidth * m_Info.nRendHMul;
				//	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// if改良要
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

					// 領域描画を指定
					m_Info.bBltAll = TRUE;
				}
			}

			// 電源OFF対策
			if (::GetVM()->IsPower() != m_Info.bPower) {
				m_Info.bPower = ::GetVM()->IsPower();
				if (!m_Info.bPower) {
					// ビットマップをすべて消去
					memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
					m_Info.bBltAll = TRUE;
				}
			}

			// 最終表示倍率を確定
			int hmul;
			hmul = 1;
			if (m_Info.nRendHMul == 2) {
				// 横256など
				hmul = 2;
			}
			{
				// アスペクト比同一はしない。等倍拡大
				hmul <<= 2;
			}

			int vmul;
			vmul = 4;
			if (m_Info.nRendVMul == 2) {
				// 縦256など
				vmul = 8;
			}

			// メモリDC作成、セレクト
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

					// ビットマップ戻す
					SelectObject(hMemDC, hDefBitmap);

					// 描画フラグを降ろすことを忘れずに
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
