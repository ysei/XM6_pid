//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �A�v���P�[�V���� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

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
	CApp();
										// �R���X�g���N�^
	BOOL InitInstance();
										// �C���X�^���X������
	BOOL ExitInstance();
										// �C���X�^���X�I��

private:
	BOOL FASTCALL CheckMutex();
										// Mutex�`�F�b�N
	BOOL FASTCALL CheckEnvironment();
										// ���`�F�b�N
	void FASTCALL SendCmd();
										// �R�}���h���M
	HWND FASTCALL SearchXM6Wnd();
										// XM6�E�B���h�E����
	static BOOL CALLBACK EnumXM6Proc(HWND hWnd, LPARAM lParam);
										// �E�B���h�E�񋓃R�[���o�b�N
	HANDLE m_hMutex;
										// Mutex�n���h��
	HMODULE m_hUser32;
										// USER32.DLL�n���h��
};

#endif	// mfc_app_h
#endif	// _WIN32
