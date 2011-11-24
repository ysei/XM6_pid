//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �C���v�b�g ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_inp_h)
#define mfc_inp_h

//===========================================================================
//
//	�C���v�b�g
//
//===========================================================================
class CInput : public CComponent
{
public:
	// ��{�t�@���N�V����
	CInput(CFrmWnd *pWnd);																// �R���X�g���N�^
	BOOL FASTCALL Init();																// ������
	void FASTCALL Cleanup();															// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);										// �ݒ�K�p
#if defined(_DEBUG)
	void AssertValid() const;															// �f�f
#endif	// _DEBUG

	// �Z�[�u�E���[�h
	BOOL FASTCALL Save(Fileio *pFio, int nVer);											// �Z�[�u
	BOOL FASTCALL Load(Fileio *pFio, int nVer);											// ���[�h

	// �O��API
	void FASTCALL Process(BOOL bRun);													// �i�s
	void FASTCALL Activate(BOOL bActivate);												// �A�N�e�B�u�ʒm
	BOOL FASTCALL IsActive() const		{ return m_bActive; }							// �A�N�e�B�u�󋵎擾
	void FASTCALL Menu(BOOL bMenu);														// ���j���[�ʒm
	BOOL FASTCALL IsMenu() const		{ return m_bMenu; }								// ���j���[�󋵎擾
	DWORD FASTCALL GetProcessCount() const	{ return m_dwProcessCount; }				// �i�s�J�E���^�擾

private:
	// ����
	LPDIRECTINPUT m_lpDI;									// DirectInput
	BOOL m_bActive;											// �A�N�e�B�u�t���O
	BOOL m_bMenu;											// ���j���[�t���O
	CRTC *m_pCRTC;											// CRTC
	DWORD m_dwDispCount;									// CRTC�\���J�E���g
	DWORD m_dwProcessCount;									// �i�s�J�E���g

	// �Z�[�u�E���[�h
	BOOL FASTCALL SaveMain(Fileio *pFio);					// �Z�[�u�{��
	BOOL FASTCALL Load200(Fileio *pFio);					// ���[�h�{�� (version2.00)
	BOOL FASTCALL Load201(Fileio *pFio);					// ���[�h�{�� (version2.01)

	// �L�[�{�[�h
	void FASTCALL InputKey(BOOL bEnable);					// �L�[�{�[�h����
	Keyboard *m_pKeyboard;									// �L�[�{�[�h

	// �}�E�X
	void FASTCALL InputMouse(BOOL bEnable);					// �}�E�X����
	Mouse *m_pMouse;										// �}�E�X

	// �W���C�X�e�B�b�N
	void FASTCALL InitJoy();								// �W���C�X�e�B�b�N������
	void FASTCALL InputJoy(BOOL bEnable);					// �W���C�X�e�B�b�N����
	PPI *m_pPPI;											// PPI
};

#endif	// mfc_inp_h
#endif	// _WIN32
