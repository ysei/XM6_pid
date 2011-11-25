//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �X�P�W���[�� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sch_h)
#define mfc_sch_h

//===========================================================================
//
//	�X�P�W���[��
//
//===========================================================================
class CScheduler	// : public CComponent
{
public:
	// ��{�t�@���N�V����
	CScheduler(CFrmWnd *pFrmWnd);			// �R���X�g���N�^
	BOOL FASTCALL Init();					// ������
	void FASTCALL Cleanup();				// �N���[���A�b�v

	BOOL IsEnable() const { return mm_bEnable; }
	void Enable(BOOL b) { mm_bEnable = b; }

	virtual BOOL FASTCALL Save(Fileio *pFio, int nVer) { return TRUE; }		// �Z�[�u
	virtual BOOL FASTCALL Load(Fileio *pFio, int nVer) { return TRUE; }		// ���[�h
	virtual void FASTCALL ApplyCfg(const Config *pConfig) {}				// �ݒ�K�p

private:
	void FASTCALL Run();					// ���s
	static UINT ThreadFunc(LPVOID pParam);	// �X���b�h�֐�
	CWinThread *m_pThread;					// �X���b�h�|�C���^
	BOOL m_bExitReq;						// �X���b�h�I���v��


	BOOL	mm_bEnable;
};

#endif	// mfc_sch_h
#endif	// _WIN32
