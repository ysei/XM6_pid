//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �A�v���P�[�V���� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "mfc_frm.h"
#include "mfc_asm.h"
#include "mfc_com.h"
#include "mfc_res.h"
#include "mfc_app.h"

#include "vm.h"
#include "cpu.h"
#include "memory.h"


//---------------------------------------------------------------------------
//
//	�A�v���P�[�V���� �C���X�^���X
//
//---------------------------------------------------------------------------
CApp theApp;

//---------------------------------------------------------------------------
//
//	�֐��|�C���^��`
//
//---------------------------------------------------------------------------
extern "C" {
typedef int (WINAPI *DRAWTEXTWIDE)(HDC, LPCWSTR, int, LPRECT, UINT);
}

//---------------------------------------------------------------------------
//
//	�O���[�o�� ���[�N
//
//---------------------------------------------------------------------------
VM *pVM;								// Virtual Machine

//---------------------------------------------------------------------------
//
//	�X�^�e�B�b�N ���[�N
//
//---------------------------------------------------------------------------
static CCriticalSection csect;			// VM���b�N�p�N���e�B�J���Z�N�V����
static BOOL bJapanese;					// ���{��E�p�ꔻ�ʃt���O
static BOOL bWinNT;						// WindowsNT�EWindows9x���ʃt���O
static BOOL bSupport932;				// CP932(SHIFT-JIS)�T�|�[�g�t���O
static BOOL bMMX;						// MMX���ʃt���O
static BOOL bCMOV;						// CMOV���ʃt���O
static LPSTR lpszInfoMsg;				// ��񃁃b�Z�[�W�o�b�t�@
static DRAWTEXTWIDE pDrawTextW;			// DrawTextW

//---------------------------------------------------------------------------
//
//	���{����̔���
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsJapanese(void)
{
	return bJapanese;
}

//---------------------------------------------------------------------------
//
//	WindowsNT�̔���
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsWinNT(void)
{
	return bWinNT;
}

//---------------------------------------------------------------------------
//
//	CP932�T�|�[�g�̔���
//
//---------------------------------------------------------------------------
BOOL FASTCALL Support932(void)
{
	return bSupport932;
}

//---------------------------------------------------------------------------
//
//	MMX�̔���
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsMMX(void)
{
	return bMMX;
}

//---------------------------------------------------------------------------
//
//	CMOV�̔���
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsCMOV(void)
{
	return bCMOV;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�擾
//
//---------------------------------------------------------------------------
void FASTCALL GetMsg(UINT uID, CString& string)
{
	// uID=0�Ŕ��ł���ꍇ������
	if (uID == 0) {
		string.Empty();
		return;
	}

	// ���{�ꂩ
	if (IsJapanese()) {
		if (!string.LoadString(uID)) {
#if defined(_DEBUG)
			TRACE(_T("GetMsg:�����񃍁[�h�Ɏ��s ID:%d\n"), uID);
#endif	// _DEBUG
			string.Empty();
		}
		return;
	}

	// �p��B+5000�Ŏ���
	if (string.LoadString(uID + 5000)) {
		return;
	}

	// +0�ł�����x
	if (!string.LoadString(uID)) {
#if defined(_DEBUG)
		TRACE(_T("GetMsg:�����񃍁[�h�Ɏ��s ID:%d\n"), uID);
#endif	// _DEBUG
		string.Empty();
	}
}

//---------------------------------------------------------------------------
//
//	���z�}�V�����擾
//
//---------------------------------------------------------------------------
VM* FASTCALL GetVM(void)
{
	ASSERT(pVM);
	return pVM;
}

//---------------------------------------------------------------------------
//
//	���z�}�V�������b�N
//
//---------------------------------------------------------------------------
void FASTCALL LockVM(void)
{
	csect.Lock();
}

//---------------------------------------------------------------------------
//
//	���z�}�V�����A�����b�N
//
//---------------------------------------------------------------------------
void FASTCALL UnlockVM(void)
{
	csect.Unlock();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���I�[�v���_�C�A���O
//	��lpszPath�͕K�����������ČĂяo������
//
//---------------------------------------------------------------------------
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPSTR lpszPath, UINT nFilterID)
{
	OPENFILENAME ofn;
	TCHAR szFilter[0x200];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	CString strFilter;
	int i;
	int nLen;
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	ASSERT(pParent);
	ASSERT(lpszPath);
	ASSERT(nFilterID);

	// �\���̂�ݒ�
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pParent->m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpszPath;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrInitialDir = Filepath::GetDefaultDir();
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

	// �t�B���^��ݒ�
	GetMsg(nFilterID, strFilter);
	_tcscpy(szFilter, (LPCTSTR)strFilter);
	nLen = (int)_tcslen(szFilter);
	for (i=0; i<nLen; i++) {
		if (szFilter[i] == _T('|')) {
			szFilter[i] = _T('\0');
		}
	}

	// �R�����_�C�A���O���s
	if (!GetOpenFileName(&ofn)) {
		return FALSE;
	}

	// �����ȃt�@�C�����𓾂�(FindFirstFile�œ�����̂̓t�@�C����+�g���q�̂�)
	hFind = FindFirstFile(lpszPath, &wfd);
	FindClose(hFind);
	_tsplitpath(lpszPath, szDrive, szDir, NULL, NULL);
	_tcscpy(lpszPath, szDrive);
	_tcscat(lpszPath, szDir);
	_tcscat(lpszPath, wfd.cFileName);

	// �f�t�H���g�f�B���N�g����ۑ�
	Filepath::SetDefaultDir(lpszPath);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���Z�[�u�_�C�A���O
//	��lpszPath�͕K�����������ČĂяo�����ƁBlpszExt�͐擪3�����̂ݗL��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FileSaveDlg(CWnd *pParent, LPSTR lpszPath, LPCTSTR lpszExt, UINT nFilterID)
{
	OPENFILENAME ofn;
	TCHAR szFilter[0x200];
	CString strFilter;
	int i;
	int nLen;

	ASSERT(pParent);
	ASSERT(lpszPath);
	ASSERT(nFilterID);

	// �\���̂�ݒ�
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pParent->m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpszPath;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrInitialDir = Filepath::GetDefaultDir();
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = lpszExt;

	// �t�B���^��ݒ�
	GetMsg(nFilterID, strFilter);
	_tcscpy(szFilter, (LPCTSTR)strFilter);
	nLen = (int)_tcslen(szFilter);
	for (i=0; i<nLen; i++) {
		if (szFilter[i] == _T('|')) {
			szFilter[i] = _T('\0');
		}
	}

	// �R�����_�C�A���O���s
	if (!GetSaveFileName(&ofn)) {
		return FALSE;
	}

	// �f�t�H���g�f�B���N�g����ۑ�
	Filepath::SetDefaultDir(lpszPath);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	��񃁃b�Z�[�W�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SetInfoMsg(LPCTSTR lpszMsg, BOOL bRec)
{
	// �L���t���O������
	if (bRec) {
		// �o�b�t�@�A�h���X���L��
		lpszInfoMsg = (LPSTR)lpszMsg;
		return;
	}

	// �o�b�t�@�A�h���X���^�����Ă��Ȃ���΁A����
	if (!lpszInfoMsg) {
		return;
	}
}

//---------------------------------------------------------------------------
//
//	DrawTextW
//	����T�|�[�gOS�ł́A�������Ȃ�
//
//---------------------------------------------------------------------------
int FASTCALL DrawTextWide(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	// �T�|�[�g����Ă��邩
	if (!pDrawTextW) {
		// �������Ȃ�
		return 1;
	}

	// ���C�h������Draw
	return pDrawTextW(hDC, lpString, nCount, lpRect, uFormat);
}

//===========================================================================
//
//	�A�v���P�[�V����
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CApp::CApp() : CWinApp(_T("XM6"))
{
	m_hMutex = NULL;
	m_hUser32 = NULL;
}

//---------------------------------------------------------------------------
//
//	�C���X�^���X������
//
//---------------------------------------------------------------------------
BOOL CApp::InitInstance()
{
	CFrmWnd *pFrmWnd;

	// �f�t�H���g�f�B���N�g�����N���A
	Filepath::ClearDefaultDir();

	// ������
	if (!CheckEnvironment()) {
		return FALSE;
	}

	// ��d�N���`�F�b�N
	if (!CheckMutex()) {
		// �R�}���h���C��������΁A�n��
		if (m_lpCmdLine[0] != _T('\0')) {
			SendCmd();
		}
		return FALSE;
	}

	// ���C���E�C���h�E�쐬(������m_pMainWnd�֑��)
	pFrmWnd = new CFrmWnd();
	m_pMainWnd = (CWnd*)pFrmWnd;

	// ������
	if (!pFrmWnd->Init()) {
		return FALSE;
	}

	// �\��
	pFrmWnd->ShowWindow(m_nCmdShow);
	pFrmWnd->UpdateWindow();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�C���X�^���X�I��
//
//---------------------------------------------------------------------------
BOOL CApp::ExitInstance()
{
	// Mutex�폜
	if (m_hMutex) {
		::CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}

	// USER32.DLL���
	if (m_hUser32) {
		::FreeLibrary(m_hUser32);
		m_hUser32 = NULL;
	}

	// ��{�N���X
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Mutex�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CApp::CheckMutex()
{
	HANDLE hMutex;

	ASSERT(this);

	// �L���ɂ�����炸�A�쐬
	hMutex = ::CreateMutex(NULL, TRUE, _T("XM6"));
	if (hMutex) {
		// ���ɋN���H
		if (::GetLastError() == ERROR_ALREADY_EXISTS) {
			return FALSE;
		}

		// OK
		m_hMutex = hMutex;
		return TRUE;
	}

	// �Ȃ������s
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���̔���
//
//---------------------------------------------------------------------------
BOOL FASTCALL CApp::CheckEnvironment()
{
	OSVERSIONINFO ovi;
	CString strError;

	ASSERT(this);

	//
	//	OS�̔���
	//

	// ���{����̔���
	::bJapanese = FALSE;
	if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_JAPANESE) {
		if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE) {
			// �V�X�e���f�t�H���g�E���[�U�f�t�H���g�̑o���Ŕ���
			::bJapanese = TRUE;
		}
	}

	// WindowsNT�̔���
	memset(&ovi, 0, sizeof(ovi));
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	VERIFY(::GetVersionEx(&ovi));
	if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		::bWinNT = TRUE;
	}
	else {
		::bWinNT = FALSE;
	}

	// �R�[�h�y�[�W932�T�|�[�g�̔���(UNICODE�T�|�[�g���O��)
	::bSupport932 = FALSE;
	::pDrawTextW = NULL;
	if (::bWinNT) {
		// UNICODE�T�|�[�gOS
		if (::IsValidCodePage(932)) {
			// USER32.DLL�����[�h
			m_hUser32 = ::LoadLibrary(_T("USER32.DLL"));
			if (m_hUser32) {
				// DrawTextW�̃A�h���X�𓾂�
				pDrawTextW = (DRAWTEXTWIDE)::GetProcAddress(m_hUser32, _T("DrawTextW"));
				if (pDrawTextW) {
					// CP932�ւ̕ϊ��ƕ\�����\
					::bSupport932 = TRUE;
				}
			}
		}
	}

	//
	//	�v���Z�b�T�̔���
	//

	// CMOV�̔���
	::bCMOV = FALSE;
	if (::IsCMOVSupport()) {
		::bCMOV = TRUE;
	}

	// MMX�̔���(Windows98�ȍ~�̂�)
	::bMMX = FALSE;
	if (ovi.dwMajorVersion >= 4) {
		// Windows95 or WindowsNT4 �ȍ~
		if ((ovi.dwMajorVersion == 4) && (ovi.dwMinorVersion == 0)) {
			// Windows95 or WindowsNT4
			::bMMX = FALSE;
		}
		else {
			// �v���Z�b�T�ɂ��
			::bMMX = ::IsMMXSupport();
		}
	}

	// version2.05����ACMOV,MMX�Ƃ��K�{
	if (!::bCMOV || !::bMMX) {
		::GetMsg(IDS_PROCESSOR ,strError);
		AfxMessageBox(strError, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// ���ׂ�OK
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�R�}���h���C�����M
//
//---------------------------------------------------------------------------
void FASTCALL CApp::SendCmd()
{
	HWND hWnd;
	COPYDATASTRUCT cds;

	ASSERT(this);

	// �E�B���h�E����
	hWnd = SearchXM6Wnd();
	if (!hWnd) {
		return;
	}

	// WM_COPYDATA�ő��M
	memset(&cds, 0, sizeof(cds));
	cds.dwData = WM_COPYDATA;
	cds.cbData = ((DWORD)_tcslen(m_lpCmdLine) + 1) * sizeof(TCHAR);
	cds.lpData = m_lpCmdLine;
	::SendMessage(hWnd, WM_COPYDATA, NULL, (LPARAM)&cds);
}

//---------------------------------------------------------------------------
//
//	XM6�E�B���h�E����
//
//---------------------------------------------------------------------------
HWND FASTCALL CApp::SearchXM6Wnd()
{
	HWND hWnd;

	// �E�B���h�E��NULL
	hWnd = NULL;

	// ����
	::EnumWindows(EnumXM6Proc, (LPARAM)&hWnd);

	// �R�[���o�b�N�֐��Ō��ʂ�����
	return hWnd;
}

//---------------------------------------------------------------------------
//
//	XM6�E�B���h�E�����R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL CALLBACK CApp::EnumXM6Proc(HWND hWnd, LPARAM lParam)
{
	HWND *phWnd;
	LONG lUser;

	// �p�����[�^�󂯎��
	phWnd = (HWND*)lParam;
	ASSERT(phWnd);
	ASSERT(*phWnd == NULL);

	// �Y���E�B���h�E�̃��[�U�f�[�^�𓾂�
	lUser = ::GetWindowLong(hWnd, GWL_USERDATA);

	// XM6�`�F�b�N���s��
	if (lUser == (LONG)MAKEID('X', 'M', '6', ' ')) {
		// XM6�t���[���E�B���h�E�Ɣ��肵�A�ł��؂�
		*phWnd = hWnd;
		return FALSE;
	}

	// ����Ă���̂ő�����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	cpudebug.c ���[�h�ǂݏo�� (mfc_cpu.cpp)
//
//---------------------------------------------------------------------------
extern "C" WORD cpudebug_fetch(DWORD addr)
{
	static Memory* cpudebug_memory = 0;

	WORD w;

	if(cpudebug_memory == 0) {
		cpudebug_memory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	}

	ASSERT(cpudebug_memory);

	addr &= 0xfffffe;
	w = (WORD) cpudebug_memory->ReadOnly(addr);
	w <<= 8;
	w |= cpudebug_memory->ReadOnly(addr + 1);

	return w;
}
#endif	// _WIN32
