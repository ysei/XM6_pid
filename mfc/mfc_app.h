//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �A�v���P�[�V���� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32) && defined(_AFXDLL)

#if !defined(mfc_app_h)
#define mfc_app_h

//===========================================================================
//
//	�A�v���P�[�V����
//
//===========================================================================
class CApp : public CWinApp
{
public:
	CApp();														// �R���X�g���N�^
	BOOL InitInstance();										// �C���X�^���X������
};

//---------------------------------------------------------------------------
//
//	�O���[�o��
//
//---------------------------------------------------------------------------
void FASTCALL GetMsg(UINT uID, CString& string);													// ���b�Z�[�W�擾
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPTSTR lpszPath, UINT nFilterID);							// �t�@�C���I�[�v���_�C�A���O

#endif	// mfc_app_h
#endif	// _WIN32
