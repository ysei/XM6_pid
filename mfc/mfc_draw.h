//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Drawビュー ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_draw_h)
#define mfc_draw_h

//===========================================================================
//
//	Drawビュー
//
//===========================================================================
class CDrawView : public CView
{
public:
	// 基本ファンクション
	CDrawView();														// コンストラクタ
	BOOL FASTCALL Init(CWnd *pParent);									// 初期化
	void OnDraw(CDC *pDC);												// 描画
};

#endif	// mfc_draw_h
#endif	// _WIN32
