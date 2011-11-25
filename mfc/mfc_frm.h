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
	CFrmWnd();
										// �R���X�g���N�^
	BOOL Init();
										// ������

	// �擾
	CDrawView* FASTCALL GetView() const;				// �`��r���[�擾
	CComponent* FASTCALL GetFirstComponent() const;		// �ŏ��̃R���|�[�l���g���擾
	CScheduler* FASTCALL GetScheduler() const;			// �X�P�W���[���擾
	CConfig* FASTCALL GetConfig() const;				// �R���t�B�O�擾

	// �h���b�O���h���b�v�T�|�[�g
	BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath);	// �R�}���h���C������ �T�u

protected:
	// �I�[�o�[���C�h
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����
	void GetMessageString(UINT nID, CString& rMessage) const;
										// ���b�Z�[�W�������

	// WM���b�Z�[�W
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnClose();
										// �E�B���h�E�N���[�Y
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnMove(int x, int y);
										// �E�B���h�E�ړ�
	afx_msg LRESULT OnDisplayChange(UINT uParam, LONG lParam);
										// �f�B�X�v���C�ύX
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �E�B���h�E�w�i�`��
	afx_msg void OnPaint();
										// �E�B���h�E�`��
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// �A�N�e�B�x�[�g
#if _MFC_VER >= 0x700
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
#else
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
#endif
										// �^�X�N�؂�ւ�
	afx_msg void OnEnterMenuLoop(BOOL bTrackPopup);
										// ���j���[���[�v�J�n
	afx_msg void OnExitMenuLoop(BOOL bTrackPopup);
										// ���j���[���[�v�I��
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
										// �e�E�B���h�E�ʒm
	afx_msg LONG OnKick(UINT uParam, LONG lParam);
										// �L�b�N
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
										// �I�[�i�[�h���[
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
										// �R���e�L�X�g���j���[
	afx_msg LONG OnPowerBroadCast(UINT uParam, LONG lParam);
										// �d���ύX�ʒm
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
										// �V�X�e���R�}���h
#if _MFC_VER >= 0x700
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
#else
	afx_msg LONG OnCopyData(UINT uParam, LONG lParam);
										// �f�[�^�]��
#endif
	afx_msg void OnEndSession(BOOL bEnding);
										// �Z�b�V�����I��
	afx_msg LONG OnShellNotify(UINT uParam, LONG lParam);
										// �t�@�C���V�X�e����ԕω�

	// �R�}���h����
	afx_msg void OnOpen();
										// �J��
	afx_msg void OnOpenUI(CCmdUI *pCmdUI);
										// �J�� UI
	afx_msg void OnSave();
										// �㏑���ۑ�
	afx_msg void OnSaveUI(CCmdUI *pCmdUI);
										// �㏑���ۑ� UI
	afx_msg void OnSaveAs();
										// ���O��t���ĕۑ�
	afx_msg void OnSaveAsUI(CCmdUI *pCmdUI);
										// ���O��t���ĕۑ� UI
	afx_msg void OnMRU(UINT uID);
										// MRU
	afx_msg void OnMRUUI(CCmdUI *pCmdUI);
										// MRU UI
	afx_msg void OnReset();
										// ���Z�b�g
	afx_msg void OnResetUI(CCmdUI *pCmdUI);
										// ���Z�b�g UI
	afx_msg void OnInterrupt();
										// �C���^���v�g
	afx_msg void OnInterruptUI(CCmdUI *pCmdUI);
										// �C���^���v�g UI
	afx_msg void OnPower();
										// �d���X�C�b�`
	afx_msg void OnPowerUI(CCmdUI *pCmdUI);
										// �d���X�C�b�` UI
	afx_msg void OnExit();
										// �I��

	afx_msg void OnFD(UINT uID);
										// �t���b�s�[�f�B�X�N�R�}���h
	afx_msg void OnFDOpenUI(CCmdUI *pCmdUI);
										// �t���b�s�[�I�[�v�� UI
	afx_msg void OnFDEjectUI(CCmdUI *pCmdUI);
										// �t���b�s�[�C�W�F�N�g UI
	afx_msg void OnFDWritePUI(CCmdUI *pCmdUI);
										// �t���b�s�[�������ݕی� UI
	afx_msg void OnFDForceUI(CCmdUI *pCmdUI);
										// �t���b�s�[�����C�W�F�N�g UI
	afx_msg void OnFDInvalidUI(CCmdUI *pCmdUI);
										// �t���b�s�[��}�� UI
	afx_msg void OnFDMediaUI(CCmdUI *pCmdUI);
										// �t���b�s�[���f�B�A UI
	afx_msg void OnFDMRUUI(CCmdUI *pCmdUI);
										// �t���b�s�[MRU UI
	afx_msg void OnMouseMode();
										// �}�E�X���[�h
	afx_msg void OnSoftKey();
										// �\�t�g�E�F�A�L�[�{�[�h
	afx_msg void OnSoftKeyUI(CCmdUI *pCmdUI);
										// �\�t�g�E�F�A�L�[�{�[�h UI
	afx_msg void OnTimeAdj();
										// �������킹
	afx_msg void OnTrap();
										// trap#0
	afx_msg void OnTrapUI(CCmdUI *pCmdUI);
										// trap#0 UI
	afx_msg void OnSaveWav();
										// WAV�L���v�`��
	afx_msg void OnSaveWavUI(CCmdUI *pCmdUI);
										// WAV�L���v�`�� UI
	afx_msg void OnOptions();
										// �I�v�V����
private:
	// ������
	BOOL FASTCALL InitChild();
										// �`���C���h�E�B���h�E������
	void FASTCALL InitPos(BOOL bStart = TRUE);
										// �ʒu�E��`������
	void FASTCALL InitShell();
										// �V�F���A�g������
	BOOL FASTCALL InitVM();
										// VM������
	BOOL FASTCALL InitComponent();
										// �R���|�[�l���g������
	void FASTCALL InitVer();
										// �o�[�W����������
	void FASTCALL InitCmd(LPCTSTR lpszCmd);
										// �R�}���h���C������
	void FASTCALL ApplyCfg();
										// �ݒ�K�p
	void FASTCALL SizeStatus();
										// �X�e�[�^�X�o�[�T�C�Y�ύX
	void FASTCALL HideTaskBar(BOOL bHide, BOOL bFore);
										// �^�X�N�o�[�B��
	BOOL RestoreFrameWnd(BOOL bFullScreen);
										// �E�B���h�E����
	void RestoreDiskState();
										// �f�B�X�N�E�X�e�[�g����
	int m_nStatus;
										// �X�e�[�^�X�R�[�h
	static const DWORD SigTable[];
										// SRAM�V�O�l�`���e�[�u��

	// �I��
	void SaveFrameWnd();
										// �E�B���h�E�ۑ�
	void SaveDiskState();
										// �f�B�X�N�E�X�e�[�g�ۑ�
	void FASTCALL CleanSub();
										// �N���[���A�b�v
	BOOL m_bExit;
										// �I���t���O
	BOOL m_bSaved;
										// �t���[���E�f�B�X�N�E�X�e�[�g�ۑ��t���O

	// �Z�[�u�E���[�h
	BOOL FASTCALL SaveComponent(const Filepath& path, DWORD dwPos);
										// �Z�[�u
	BOOL FASTCALL LoadComponent(const Filepath& path, DWORD dwPos);
										// ���[�h

	// �R�}���h�n���h���T�u
	BOOL FASTCALL OnOpenSub(const Filepath& path);
										// �I�[�v���T�u
	BOOL FASTCALL OnOpenPrep(const Filepath& path, BOOL bWarning = TRUE);
										// �I�[�v���`�F�b�N
	void FASTCALL OnSaveSub(const Filepath& path);
										// �ۑ��T�u
	void FASTCALL OnFDOpen(int nDrive);
										// �t���b�s�[�I�[�v��
	void FASTCALL OnFDEject(int nDrive);
										// �t���b�s�[�C�W�F�N�g
	void FASTCALL OnFDWriteP(int nDrive);
										// �t���b�s�[�������ݕی�
	void FASTCALL OnFDForce(int nDrive);
										// �t���b�s�[�����C�W�F�N�g
	void FASTCALL OnFDInvalid(int nDrive);
										// �t���b�s�[��}��
	void FASTCALL OnFDMedia(int nDrive, int nMedia);
										// �t���b�s�[���f�B�A
	void FASTCALL OnFDMRU(int nDrive, int nMRU);
										// �t���b�s�[MRU
	int m_nFDDStatus[2];
										// �t���b�s�[�X�e�[�^�X

	// �f�o�C�X�E�r���[�E�R���|�[�l���g
	FDD *m_pFDD;
										// FDD
	SASI *m_pSASI;
										// SASI
	SCSI *m_pSCSI;
										// SCSI
	Scheduler *m_pScheduler;
										// Scheduler
	Keyboard *m_pKeyboard;
										// Keyboard
	Mouse *m_pMouse;
										// Mouse
	CDrawView *m_pDrawView;
										// �`��r���[
	CComponent *m_pFirstComponent;
										// �ŏ��̃R���|�[�l���g
	CScheduler *m_pSch;
										// �X�P�W���[��
	CConfig *m_pConfig;
										// �R���t�B�O

	// �t���X�N���[��
	BOOL m_bFullScreen;
										// �t���X�N���[���t���O
	DEVMODE m_DevMode;
										// �X�N���[���p�����[�^�L��
	HWND m_hTaskBar;
										// �^�X�N�o�[
	int m_nWndLeft;
										// �E�B���h�E���[�h��x
	int m_nWndTop;
										// �E�B���h�E���[�h��y

	// �T�u�E�B���h�E
	CString m_strWndClsName;
										// �E�B���h�E�N���X��

	// ���j���[
	void FASTCALL ShowMenu();
										// ���j���[�o�[�\��
	CMenu m_Menu;
										// ���C�����j���[
	BOOL m_bMenuBar;
										// ���j���[�o�[�\���t���O
	CMenu m_PopupMenu;
										// �|�b�v�A�b�v���j���[
	BOOL m_bPopupMenu;
										// �|�b�v�A�b�v���j���[���s��

	// �V�F���A�g
	ULONG m_uNotifyId;
										// �V�F���ʒmID
	SHChangeNotifyEntry m_fsne[1];
										// �V�F���ʒm�G���g��

	// �X�e�[�g�t�@�C��
	void FASTCALL UpdateExec();
										// �X�V(���s)
	DWORD m_dwExec;
										// �Z�[�u����s�J�E���^

	// �R���t�B�M�����[�V����
	BOOL m_bMouseMid;
										// �}�E�X���{�^���L��
	BOOL m_bPopup;
										// �|�b�v�A�b�v���[�h
	BOOL m_bAutoMouse;
										// �����}�E�X���[�h

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_frm_h
#endif	// _WIN32
