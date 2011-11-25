//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �t���[���E�B���h�E ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_frm_h)
#define mfc_frm_h

//---------------------------------------------------------------------------
//
//	�E�B���h�E���b�Z�[�W
//
//---------------------------------------------------------------------------
#define WM_KICK			WM_APP				// �G�~�����[�^�X�^�[�g
#define WM_SHELLNOTIFY	(WM_USER + 5)		// �t�@�C���V�X�e����ԕω�

//===========================================================================
//
//	�t���[���E�B���h�E
//
//===========================================================================
class CFrmWnd : public CFrameWnd
{
public:
	// ������
	CFrmWnd();												// �R���X�g���N�^
	BOOL Init();											// ������

	// �h���b�O���h���b�v�T�|�[�g
	BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath);	// �R�}���h���C������ �T�u
	void OnDraw(CDC *pDC);

protected:
	// WM���b�Z�[�W
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);						// �E�B���h�E�쐬
	afx_msg void OnDestroy();													// �E�B���h�E�폜
	afx_msg void OnPaint();														// �E�B���h�E�`��

	afx_msg LONG OnKick(UINT uParam, LONG lParam);		// �L�b�N
	afx_msg void OnReset();								// ���Z�b�g
	afx_msg void OnFD(UINT uID);						// �t���b�s�[�f�B�X�N�R�}���h

private:
	void FASTCALL	InitCmd(LPCTSTR lpszCmd);			// �R�}���h���C������
	void FASTCALL	OnFDOpen(int nDrive);				// �t���b�s�[�I�[�v��
	void FASTCALL	ShowMenu();							// ���j���[�o�[�\��

	BOOL			m_bExit;							// �I���t���O
	FDD*			m_pFDD;								// FDD
	CMenu			m_Menu;								// ���C�����j���[

	DECLARE_MESSAGE_MAP()								// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_frm_h
#endif	// _WIN32
