//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC Draw�r���[ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_draw_h)
#define mfc_draw_h

//===========================================================================
//
//	Draw�r���[
//
//===========================================================================
class CDrawView : public CView
{
public:
	// ��{�t�@���N�V����
	CDrawView();														// �R���X�g���N�^
	BOOL FASTCALL Init(CWnd *pParent);									// ������
	void OnDraw(CDC *pDC);												// �`��
};

#endif	// mfc_draw_h
#endif	// _WIN32
