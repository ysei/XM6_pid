//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �A�v���P�[�V���� ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "mfc_frm.h"
#include "mfc_asm.h"
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
//	�O���[�o�� ���[�N
//
//---------------------------------------------------------------------------
static VM *pVM = 0;								// Virtual Machine

//---------------------------------------------------------------------------
//
//	�X�^�e�B�b�N ���[�N
//
//---------------------------------------------------------------------------
static CCriticalSection csect;			// VM���b�N�p�N���e�B�J���Z�N�V����
static LPSTR lpszInfoMsg;				// ��񃁃b�Z�[�W�o�b�t�@
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

VM* FASTCALL CreateVM(void) {
	if(pVM == 0) {
		pVM = new VM;
	}
	return pVM;
}

void FASTCALL DestroyVM(void) {
	if(pVM) {
		pVM->Cleanup();
		delete pVM;
		pVM = 0;
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
}

//---------------------------------------------------------------------------
//
//	�C���X�^���X������
//
//---------------------------------------------------------------------------
BOOL CApp::InitInstance()
{
	// �f�t�H���g�f�B���N�g�����N���A
	Filepath::ClearDefaultDir();

	// ���C���E�C���h�E�쐬(������m_pMainWnd�֑��)
	CFrmWnd *pFrmWnd = new CFrmWnd();
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
