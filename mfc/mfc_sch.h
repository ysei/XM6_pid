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

private:
	void FASTCALL Run();					// ���s
	static UINT ThreadFunc(LPVOID pParam);	// �X���b�h�֐�
	CWinThread *m_pThread;					// �X���b�h�|�C���^
	BOOL m_bExitReq;						// �X���b�h�I���v��

};

BOOL schedulerInit();
BOOL schedulerIsEnable();
void schedulerSetEnable(BOOL b);

#endif	// mfc_sch_h
#endif	// _WIN32
