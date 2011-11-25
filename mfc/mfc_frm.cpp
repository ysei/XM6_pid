//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �t���[���E�B���h�E ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "memory.h"
#include "sasi.h"
#include "scsi.h"
#include "fdd.h"
#include "fdc.h"
#include "fdi.h"
#include "render.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_res.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_host.h"
#include "mfc_cfg.h"

//===========================================================================
//
//	�t���[���E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�V�F���萔��`
//	��include�t�@�C���ł͂Ȃ��A�A�v���P�[�V�������Œ�`����悤��߂��Ă���
//
//---------------------------------------------------------------------------
#define SHCNRF_InterruptLevel			0x0001
#define SHCNRF_ShellLevel				0x0002
#define SHCNRF_NewDelivery				0x8000

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CFrmWnd::CFrmWnd()
{
	// VM�E�X�e�[�^�X�R�[�h
	::pVM = NULL;
	m_nStatus = -1;

	// �f�o�C�X
	m_pFDD = NULL;
	m_pSASI = NULL;
	m_pSCSI = NULL;
	m_pScheduler = NULL;
	m_pKeyboard = NULL;
	m_pMouse = NULL;

	// �R���|�[�l���g
	m_pFirstComponent = NULL;
	m_pDrawView = NULL;
	m_pSch = NULL;
//	m_pSound = NULL;
//	m_pInput = NULL;
	m_pHost = NULL;
	m_pConfig = NULL;

	// �t���X�N���[��
	m_bFullScreen = FALSE;
	m_hTaskBar = NULL;
	memset(&m_DevMode, 0, sizeof(m_DevMode));
	m_nWndLeft = 0;
	m_nWndTop = 0;

	// �T�u�E�B���h�E
	m_strWndClsName.Empty();

	// �X�e�[�^�X�o�[�E���j���[�E�L���v�V����
	m_bMenuBar = TRUE;

	// �V�F���ʒm
	m_uNotifyId = NULL;

	// �R���t�B�M�����[�V����
	m_bMouseMid = TRUE;
	m_bPopup = FALSE;
	m_bAutoMouse = TRUE;

	// ���̑��ϐ�
	m_bExit = FALSE;
	m_bSaved = FALSE;
	m_nFDDStatus[0] = 0;
	m_nFDDStatus[1] = 0;
	m_dwExec = 0;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CFrmWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOVE()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
	ON_WM_ENTERMENULOOP()
	ON_WM_EXITMENULOOP()
	ON_WM_PARENTNOTIFY()
	ON_MESSAGE(WM_KICK, OnKick)
	ON_WM_DRAWITEM()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_POWERBROADCAST, OnPowerBroadCast)
	ON_WM_SYSCOMMAND()
#if _MFC_VER >= 0x700
	ON_WM_COPYDATA()
#else
	ON_MESSAGE(WM_COPYDATA, OnCopyData)
#endif
	ON_WM_ENDSESSION()
	ON_MESSAGE(WM_SHELLNOTIFY, OnShellNotify)

	ON_COMMAND(IDM_OPEN, OnOpen)
	ON_UPDATE_COMMAND_UI(IDM_OPEN, OnOpenUI)
	ON_COMMAND(IDM_SAVE, OnSave)
	ON_UPDATE_COMMAND_UI(IDM_SAVE, OnSaveUI)
	ON_COMMAND(IDM_SAVEAS, OnSaveAs)
	ON_UPDATE_COMMAND_UI(IDM_SAVEAS, OnSaveAsUI)
	ON_COMMAND(IDM_RESET, OnReset)
	ON_UPDATE_COMMAND_UI(IDM_RESET, OnResetUI)
	ON_COMMAND(IDM_INTERRUPT, OnInterrupt)
	ON_UPDATE_COMMAND_UI(IDM_INTERRUPT, OnInterruptUI)
	ON_COMMAND(IDM_POWER, OnPower)
	ON_UPDATE_COMMAND_UI(IDM_POWER, OnPowerUI)
	ON_COMMAND_RANGE(IDM_XM6_MRU0, IDM_XM6_MRU8, OnMRU)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_XM6_MRU0, IDM_XM6_MRU8, OnMRUUI)
	ON_COMMAND(IDM_EXIT, OnExit)

	ON_COMMAND_RANGE(IDM_D0OPEN, IDM_D1_MRU8, OnFD)
	ON_UPDATE_COMMAND_UI(IDM_D0OPEN, OnFDOpenUI)
	ON_UPDATE_COMMAND_UI(IDM_D1OPEN, OnFDOpenUI)
	ON_UPDATE_COMMAND_UI(IDM_D0EJECT, OnFDEjectUI)
	ON_UPDATE_COMMAND_UI(IDM_D1EJECT, OnFDEjectUI)
	ON_UPDATE_COMMAND_UI(IDM_D0WRITEP, OnFDWritePUI)
	ON_UPDATE_COMMAND_UI(IDM_D1WRITEP, OnFDWritePUI)
	ON_UPDATE_COMMAND_UI(IDM_D0FORCE, OnFDForceUI)
	ON_UPDATE_COMMAND_UI(IDM_D1FORCE, OnFDForceUI)
	ON_UPDATE_COMMAND_UI(IDM_D0INVALID, OnFDInvalidUI)
	ON_UPDATE_COMMAND_UI(IDM_D1INVALID, OnFDInvalidUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MEDIA0, IDM_D0_MEDIAF, OnFDMediaUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MEDIA0, IDM_D1_MEDIAF, OnFDMediaUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D0_MRU0, IDM_D0_MRU8, OnFDMRUUI)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_D1_MRU0, IDM_D1_MRU8, OnFDMRUUI)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::Init()
{
	// �E�B���h�E�쐬
	if (!Create(NULL, _T("XM6"),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
			WS_BORDER | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			rectDefault, NULL, NULL, 0, NULL)) {
		return FALSE;
	}

	// ����ȊO�̏�������OnCrate�ɔC����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬����
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	// ��{�N���X
	if (!CFrameWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// �N���C�A���g�G�b�W���O��
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CFrmWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	LONG lUser;
	CMenu *pSysMenu;
	UINT nCount;
	CString string;

	// ��{�N���X
	if (CFrameWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// ���[�U�f�[�^�w��
	lUser = (LONG)MAKEID('X', 'M', '6', ' ');
	::SetWindowLong(m_hWnd, GWL_USERDATA, lUser);

	// �A�N�Z�����[�^�w��A�A�C�R���w��AIMM�w��
	LoadAccelTable(MAKEINTRESOURCE(IDR_ACCELERATOR));
	SetIcon(AfxGetApp()->LoadIcon(IDI_APPICON), TRUE);
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// ���j���[(�E�B���h�E)
	if (::IsJapanese()) {
		// ���{�ꃁ�j���[
		m_Menu.LoadMenu(IDR_MENU);
		m_PopupMenu.LoadMenu(IDR_MENUPOPUP);
	}
	else {
		// �p�ꃁ�j���[
		m_Menu.LoadMenu(IDR_US_MENU);
		m_PopupMenu.LoadMenu(IDR_US_MENUPOPUP);
	}
	SetMenu(&m_Menu);
	m_bMenuBar = TRUE;
	m_bPopupMenu = FALSE;

	// ���j���[(�V�X�e��)
	::GetMsg(IDS_STDWIN, string);
	pSysMenu = GetSystemMenu(FALSE);
	ASSERT(pSysMenu);
	nCount = pSysMenu->GetMenuItemCount();

	// �u�E�B���h�E�W���ʒu�v��}��
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_STRING, IDM_STDWIN, string);
	pSysMenu->InsertMenu(nCount - 2, MF_BYPOSITION | MF_SEPARATOR);

	// �`���C���h�E�B���h�E������
	if (!InitChild()) {
		return -1;
	}

	// �E�B���h�E�ʒu�A��`������
	InitPos();

	// �V�F���ʒm������
	InitShell();

	// VM������
	if (!InitVM()) {
		// VM�������G���[
		m_nStatus = 1;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// �o�[�W�������\�[�X����VM�փo�[�W������`�B
	InitVer();

	// �f�o�C�X�L��
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);
	m_pMouse = (Mouse*)::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pMouse);

	// �R���|�[�l���g�쐬�A������
	if (!InitComponent()) {
		// �R���|�[�l���g�������G���[
		m_nStatus = 2;
		PostMessage(WM_KICK, 0, 0);
		return 0;
	}

	// �ݒ�K�p(OnOption�Ɠ��l�AVM���b�N����)
	::LockVM();
	ApplyCfg();
	::UnlockVM();

	// ���Z�b�g
	::GetVM()->Reset();

	// �E�C���h�E�ʒu�����W���[��(m_nStatus != 0�ɗ���)
	ASSERT(m_nStatus != 0);
	RestoreFrameWnd(FALSE);

	// ���b�Z�[�W���|�X�g���ďI��
	m_nStatus = 0;
	PostMessage(WM_KICK, 0, 0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	�`���C���h�E�B���h�E������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitChild()
{
	HDC hDC;
	HFONT hFont;
	HFONT hDefFont;
	TEXTMETRIC tm;
	int i;
	int nWidth;
	UINT uIndicator[6];

	// �r���[�쐬
	m_pDrawView = new CDrawView;
	if (!m_pDrawView->Init(this)) {
		return FALSE;
	}

	uIndicator[0] = ID_SEPARATOR;
	for (i=1; i<6; i++) {
		uIndicator[i] = (UINT)i;
	}

	// �e�L�X�g���g���b�N���擾
	hDC = ::GetDC(m_hWnd);
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(hDC, hFont);
	ASSERT(hDefFont);
	::GetTextMetrics(hDC, &tm);
	::SelectObject(hDC, hDefFont);
	::ReleaseDC(m_hWnd, hDC);

	// �T�C�Y�ݒ胋�[�v
	nWidth = 0;
	for (i=1; i<6; i++) {
		switch (i) {
			// FD0, FD1
			case 1:
			case 2:
				nWidth = tm.tmAveCharWidth * 32;
				break;

			// HD BUSY
			case 3:
				nWidth = tm.tmAveCharWidth * 10;
				break;

			// TIMER
			case 4:
				nWidth = tm.tmAveCharWidth * 9;
				break;

			// POWER
			case 5:
				nWidth = tm.tmAveCharWidth * 9;
				break;
		}
	}

	// �ă��C�A�E�g
	RecalcLayout();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ʒu�E��`������
//	��bStart=FALSE�̏ꍇ�AbFullScreen=FALSE�̎��Ɉʒu�𕜌����邱��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitPos(BOOL bStart)
{
	int cx;
	int cy;
	CRect rect;
	CRect rectStatus;
	CRect rectWnd;

	ASSERT(this);

	// �X�N���[���T�C�Y�A�E�B���h�E��`���擾
	cx = ::GetSystemMetrics(SM_CXSCREEN);
	cy = ::GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(&rectWnd);

	// 800x600�ȉ��̓X�N���[���T�C�Y�����ς��ɍL����
	if ((cx <= 800) || (cy <= 600)) {
		if ((rectWnd.left != 0) || (rectWnd.top != 0)) {
			SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOZORDER);
			return;
		}
		if ((rectWnd.Width() != cx) || (rectWnd.Height() != cy)) {
			SetWindowPos(&wndTop, 0, 0, cx, cy, SWP_NOZORDER);
			return;
		}
		return;
	}

	// 824x560(DDP2)���m���C���^�[���[�X�̍ő�T�C�Y�ƔF��
	rect.left = 0;
	rect.top = 0;
	rect.right = 824;
	rect.bottom = 560;
	::AdjustWindowRectEx(&rect, GetView()->GetStyle(), FALSE, GetView()->GetExStyle());
//	m_StatusBar.GetWindowRect(&rectStatus);
	rect.bottom += rectStatus.Height();
	::AdjustWindowRectEx(&rect, GetStyle(), TRUE, GetExStyle());

	// rect.left, rect.bottom�����ɂȂ�炵��(����ȍ~�Aright,bottom��cx,cy������)
	rect.right -= rect.left;
	rect.left = 0;
	rect.bottom -= rect.top;
	rect.top = 0;

	// �]�T������΁A�Z���^�����O
	if (rect.right < cx) {
		rect.left = (cx - rect.right) / 2;
	}
	if (rect.bottom < cy) {
		rect.top = (cy - rect.bottom) / 2;
	}

	// bStart�ŕ�����(�����J�n���A�E�B���h�E�|�t���X�N���[���̐؂�ւ���)
	if (bStart) {
		// �E�B���h�E�ʒu����U�ۑ�(���̌�A�ēxRestoreFrameWnd�̃`�����X����)
		m_nWndLeft = rect.left;
		m_nWndTop = rect.top;
	}
	else {
		// �E�B���h�E���[�h�̎��Ɍ���A�ʒu��␳
		if (!m_bFullScreen) {
			if ((rect.left == 0) && (rect.top == 0)) {
				// WM_DISPLAYCHANGE���b�Z�[�W�����āA�E�B���h�E���������Ȃ����ꍇ
				m_nWndLeft = rect.left;
				m_nWndTop = rect.top;
			}
			else {
				// ����ȊO(�t���X�N���[�����E�B���h�E�ւ̏�ԑJ�ڂ��܂�)
				rect.left = m_nWndLeft;
				rect.top = m_nWndTop;
			}
		}
	}

	// �ݒ�
	if ((rect.left != rectWnd.left) || (rect.top != rectWnd.top)) {
		SetWindowPos(&wndTop, rect.left, rect.top, rect.right, rect.bottom, 0);
		return;
	}
	if ((rect.right != rectWnd.Width()) || (rect.bottom != rectWnd.Height())) {
		SetWindowPos(&wndTop, rect.left, rect.top, rect.right, rect.bottom, 0);
		return;
	}
}

//---------------------------------------------------------------------------
//
//	�V�F���A�g������
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitShell()
{
	int nSources;

	// �ʒm�v����ݒ�
	if (::IsWinNT()) {
		// Windows2000/XP:shared memory�𗘗p����t���O��ǉ�
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel | SHCNRF_NewDelivery;
	}
	else {
		// Windows9x:shared memory�͎g�p���Ȃ�
		nSources = SHCNRF_InterruptLevel | SHCNRF_ShellLevel;
	}

	// �G���g����������
	m_fsne[0].pidl = NULL;
	m_fsne[0].fRecursive = FALSE;

	// �V�F���ʒm���b�Z�[�W��o�^
	m_uNotifyId = ::SHChangeNotifyRegister(m_hWnd,
							nSources,
							SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED,
							WM_SHELLNOTIFY,
							sizeof(m_fsne)/sizeof(m_fsne[0]),
							m_fsne);
	ASSERT(m_uNotifyId);
}

//---------------------------------------------------------------------------
//
//	VM������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitVM()
{
	::pVM = new VM;
	if (!::GetVM()->Init()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitComponent()
{
	BOOL bSuccess;
	CComponent *pComponent;

	ASSERT(!m_pFirstComponent);
	ASSERT(!m_pSch);
//	ASSERT(!m_pSound);
//	ASSERT(!m_pInput);
	ASSERT(!m_pHost);
	ASSERT(!m_pConfig);

	// �R���X�g���N�g(���Ԃ��l������K�v����B�ŏ�Config�A�Ō�Scheduler)
	m_pConfig = new CConfig(this);
	m_pFirstComponent = m_pConfig;
//	m_pSound = new CSound(this);
//	m_pFirstComponent->AddComponent(m_pSound);
//	m_pInput = new CInput(this);
//	m_pFirstComponent->AddComponent(m_pInput);
	m_pHost = new CHost(this);
	m_pFirstComponent->AddComponent(m_pHost);
	m_pSch = new CScheduler(this);
	m_pFirstComponent->AddComponent(m_pSch);

	// ������
	pComponent = m_pFirstComponent;
	bSuccess = TRUE;

	// ���[�v
	while (pComponent) {
		if (!pComponent->Init()) {
			bSuccess = FALSE;
		}
		pComponent = pComponent->GetNextComponent();
	}

	return bSuccess;
}

//---------------------------------------------------------------------------
//
//	�o�[�W����������
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitVer()
{
	TCHAR szPath[_MAX_PATH];
	DWORD dwHandle;
	DWORD dwLength;
	BYTE *pVerInfo;
	VS_FIXEDFILEINFO *pFileInfo;
	UINT uLength;
	DWORD dwMajor;
	DWORD dwMinor;

	ASSERT(this);

	// �p�X���擾
	::GetModuleFileName(NULL, szPath, _MAX_PATH);

	// �o�[�W��������ǂݎ��
	dwLength = GetFileVersionInfoSize(szPath, &dwHandle);
	if (dwLength == 0) {
		return;
	}

	pVerInfo = new BYTE[ dwLength ];
	if (::GetFileVersionInfo(szPath, dwHandle, dwLength, pVerInfo) == 0) {
		return;
	}

	// �o�[�W�����������o��
	if (::VerQueryValue(pVerInfo, _T("\\"), (LPVOID*)&pFileInfo, &uLength) == 0) {
		delete[] pVerInfo;
		return;
	}

	// �o�[�W�����𕪗��AVM�֒ʒm
	dwMajor = (DWORD)HIWORD(pFileInfo->dwProductVersionMS);
	dwMinor = (DWORD)(LOWORD(pFileInfo->dwProductVersionMS) * 16
					+ HIWORD(pFileInfo->dwProductVersionLS));
	::GetVM()->SetVersion(dwMajor, dwMinor);

	// �I��
	delete[] pVerInfo;
}

//---------------------------------------------------------------------------
//
//	�R�}���h���C������
//	���R�}���h���C���AWM_COPYDATA�ŋ���
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::InitCmd(LPCTSTR lpszCmd)
{
	LPCTSTR lpszCurrent;
	LPCTSTR lpszNext;
	TCHAR szPath[_MAX_PATH];
	int nLen;
	int i;
	BOOL bReset;

	ASSERT(this);
	ASSERT(lpszCmd);

	// �|�C���^�A�t���O������
	lpszCurrent = lpszCmd;
	bReset = FALSE;

	// ���[�v
	for (i=0; i<2; i++) {
		// �X�y�[�X�A�^�u�̓X�L�b�v
		while (lpszCurrent[0] <= _T(0x20)) {
			if (lpszCurrent[0] == _T('\0')) {
				break;
			}
			lpszCurrent++;
		}
		if (lpszCurrent[0] == _T('\0')) {
			break;
		}

		// �ŏ����_�u���N�H�[�g�Ȃ�A���̃N�H�[�g��T��
		if (lpszCurrent[0] == _T('\x22')) {
			lpszNext = _tcschr(lpszCurrent + 1, _T('\x22'));
			if (!lpszNext) {
				// �Ή�����_�u���N�H�[�g��������Ȃ�
				return;
			}
			nLen = (int)(lpszNext - (lpszCurrent + 1));
			if (nLen >= _MAX_PATH) {
				// ��������
				return;
			}

			// �N�H�[�g���ꂽ�������R�s�[
			_tcsnccpy(szPath, &lpszCurrent[1], nLen);
			szPath[nLen] = _T('\0');

			// �N�H�[�g�̎����w��
			lpszCurrent = &lpszNext[1];
		}
		else {
			// ���̃X�y�[�X��T��
			lpszNext = _tcschr(lpszCurrent + 1, _T(' '));
			if (lpszNext) {
				// �X�y�[�X�܂�
				nLen = (int)(lpszNext - lpszCurrent);
				if (nLen >= _MAX_PATH) {
					// ��������
					return;
				}

				// �X�y�[�X�܂ł̕������R�s�[
				_tcsnccpy(szPath, lpszCurrent, nLen);
				szPath[nLen] = _T('\0');

				// �X�y�[�X�̎����w��
				lpszCurrent = &lpszNext[1];
			}
			else {
				// �I�[�܂�
				_tcscpy(szPath, lpszCurrent);
				lpszCurrent = NULL;
			}
		}

		// �I�[�v�������݂�
		bReset = InitCmdSub(i, szPath);

		// �I�[�Ȃ�I��
		if (!lpszCurrent) {
			break;
		}
	}

	// ���Z�b�g�v��������΁A���Z�b�g
	if (bReset) {
		OnReset();
	}
}

//---------------------------------------------------------------------------
//
//	�R�}���h���C������ �T�u
//	���R�}���h���C���AWM_COPYDATA�A�h���b�O&�h���b�v�ŋ���
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::InitCmdSub(int nDrive, LPCTSTR lpszPath)
{
	Filepath path;
	Fileio fio;
	LPTSTR lpszFile;
	DWORD dwSize;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;
	CString strMsg;

	ASSERT(this);
	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(lpszPath);

	// pFDI������
	pFDI = NULL;

	// �t�@�C���I�[�v���`�F�b�N
	path.SetPath(lpszPath);
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	dwSize = fio.GetFileSize();
	fio.Close();

	// �t���p�X��
	::GetFullPathName(lpszPath, _MAX_PATH, szPath, &lpszFile);
	path.SetPath(szPath);

	// VM���b�N
	::LockVM();

	// 128MO or 230MO or 540MO or 640MO
	if ((dwSize == 0x797f400) || (dwSize == 0xd9eea00) ||
		(dwSize == 0x1fc8b800) || (dwSize == 0x25e28000)) {
		// MO�̊��蓖�Ă����݂�
		nDrive = 2;

		if (!m_pSASI->Open(path)) {
			// MO���蓖�Ď��s
			GetScheduler()->Reset();
//			ResetCaption();
			::UnlockVM();
			return FALSE;
		}
	}
	else {
		if (dwSize >= 0x200000) {
			// VM�̊��蓖�Ă����݂�
			nDrive = 4;

			// �I�[�v���O����
			if (!OnOpenPrep(path, FALSE)) {
				// �t�@�C�����Ȃ����A�o�[�W�����Ȃǂ��������Ȃ�
				GetScheduler()->Reset();
//				ResetCaption();
				::UnlockVM();
				return FALSE;
			}

			// ���[�h���s(OnOpenSub�ɔC����)
			::UnlockVM();
			if (OnOpenSub(path)) {
				Filepath::SetDefaultDir(szPath);
			}
			// ���Z�b�g�͍s��Ȃ�
			return FALSE;
		}
		else {
			// FD�̊��蓖�Ă����݂�
			if (!m_pFDD->Open(nDrive, path)) {
				// FD���蓖�Ď��s
				GetScheduler()->Reset();
//				ResetCaption();
				::UnlockVM();
				return FALSE;
			}
			pFDI = m_pFDD->GetFDI(nDrive);
		}
	}

	// VM���Z�b�g�A���b�N����
	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// �����B�f�B���N�g���ۑ���MRU�ǉ�
	Filepath::SetDefaultDir(szPath);
	GetConfig()->SetMRUFile(nDrive, szPath);

	// �t���b�s�[�Ȃ�ABAD�C���[�W�x��
	if (pFDI) {
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}

		// �t���b�s�[�����蓖�Ă��Ƃ������A���Z�b�g����
		return TRUE;
	}

	// �I��
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g���Z�[�u
//	���X�P�W���[���͒�~���Ă��邪�ACSound,CInput�͓��쒆
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::SaveComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	CComponent *pComponent;
	DWORD dwMajor;
	DWORD dwMinor;
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// �o�[�W�������쐬
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nVer = (int)((dwMajor << 8) | dwMinor);

	// �t�@�C���I�[�v���ƃV�[�N
	if (!fio.Open(path, Fileio::Append)) {
		return FALSE;
	}
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// ���C���R���|�[�l���g����ۑ�
	dwID = MAKEID('M', 'A', 'I', 'N');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// �R���|�[�l���g���[�v
	pComponent = m_pFirstComponent;
	while (pComponent) {
		// ID��ۑ�
		dwID = pComponent->GetID();
		if (!fio.Write(&dwID, sizeof(dwID))) {
			fio.Close();
			return FALSE;
		}

		// �R���|�[�l���g�ŗL
		if (!pComponent->Save(&fio, nVer)) {
			fio.Close();
			return FALSE;
		}

		// ����
		pComponent = pComponent->GetNextComponent();
	}

	// �I�[��������
	dwID = MAKEID('E', 'N', 'D', ' ');
	if (!fio.Write(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}

	// �I��
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g�����[�h
//	���X�P�W���[���͒�~���Ă��邪�ACSound,CInput�͓��쒆
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::LoadComponent(const Filepath& path, DWORD dwPos)
{
	Fileio fio;
	DWORD dwID;
	CComponent *pComponent;
	char cHeader[0x10];
	int nVer;

	ASSERT(this);
	ASSERT(dwPos > 0);

	// �t�@�C���I�[�v��
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// �w�b�_�ǂݎ��
	if (!fio.Read(cHeader, sizeof(cHeader))) {
		fio.Close();
		return FALSE;
	}

	// �w�b�_�`�F�b�N�A�o�[�W�������ǂݎ��
	cHeader[0x0a] = '\0';
	nVer = ::strtoul(&cHeader[0x09], NULL, 16);
	nVer <<= 8;
	cHeader[0x0d] = '\0';
	nVer |= ::strtoul(&cHeader[0x0b], NULL, 16);
	cHeader[0x09] = '\0';
	if (strcmp(cHeader, "XM6 DATA ") != 0) {
		fio.Close();
		return FALSE;
	}

	// �V�[�N
	if (!fio.Seek(dwPos)) {
		fio.Close();
		return FALSE;
	}

	// ���C���R���|�[�l���g�ǂݎ��
	if (!fio.Read(&dwID, sizeof(dwID))) {
		fio.Close();
		return FALSE;
	}
	if (dwID != MAKEID('M', 'A', 'I', 'N')) {
		fio.Close();
		return FALSE;
	}

	// �R���|�[�l���g���[�v
	for (;;) {
		// ID�ǂݎ��
		if (!fio.Read(&dwID, sizeof(dwID))) {
			fio.Close();
			return FALSE;
		}

		// �I���`�F�b�N
		if (dwID == MAKEID('E', 'N', 'D', ' ')) {
			break;
		}

		// �R���|�[�l���g��T��
		pComponent = m_pFirstComponent->SearchComponent(dwID);
		if (!pComponent) {
			// �Z�[�u���̓R���|�[�l���g�����݂������A���͌�����Ȃ�
			fio.Close();
			return FALSE;
		}

		// �R���|�[�l���g�ŗL
		if (!pComponent->Load(&fio, nVer)) {
			fio.Close();
			return FALSE;
		}
	}

	// �N���[�Y
	fio.Close();

	// �ݒ�K�p(VM���b�N���čs��)
	if (GetConfig()->IsApply()) {
		::LockVM();
		ApplyCfg();
		::UnlockVM();
	}

	// �E�B���h�E�ĕ`��
	GetView()->Invalidate(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ApplyCfg()
{
	Config config;
	CComponent *pComponent;

	// �ݒ�擾
	GetConfig()->GetConfig(&config);

	// �܂�VM�ɓK�p
	::GetVM()->ApplyCfg(&config);

	// ���ɃR���|�[�l���g�ɓK�p
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pComponent->ApplyCfg(&config);
		pComponent = pComponent->GetNextComponent();
	}

	// ���Ƀr���[�ɓK�p
	GetView()->ApplyCfg(&config);

	// �t���[���E�B���h�E(�|�b�v�A�b�v)
	if (config.popup_swnd != m_bPopup) {
//		// �T�u�E�B���h�E�����ׂăN���A
//		GetView()->ClrSWnd();

		// �ύX
		m_bPopup = config.popup_swnd;
	}

	// �t���[���E�B���h�E(�}�E�X)
	m_bMouseMid = config.mouse_mid;
	m_bAutoMouse = config.auto_mouse;
	if (config.mouse_port == 0) {
		// �}�E�X�ڑ��Ȃ��Ȃ�A�}�E�X���[�hOFF
//		if (GetInput()->GetMouseMode()) {
//			OnMouseMode();
//		}
	}
}

//---------------------------------------------------------------------------
//
//	�L�b�N
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnKick(UINT , LONG )
{
	CComponent *pComponent;
//	CInfo *pInfo;
	Config config;
	CString strMsg;
	MSG msg;
	Memory *pMemory;
	int nIdle;
	LPSTR lpszCmd;
	LPCTSTR lpszCommand;
	BOOL bFullScreen;

	// �G���[�������ɍs��
	switch (m_nStatus) {
		// VM�G���[
		case 1:
			::GetMsg(IDS_INIT_VMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;

		// �R���|�[�l���g�G���[
		case 2:
			::GetMsg(IDS_INIT_COMERR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
	}
	// ����̏ꍇ
	ASSERT(m_nStatus == 0);

	// ROM�`�F�b�N
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(pMemory);
	if (!pMemory->CheckIPL()) {
		::GetMsg(IDS_INIT_IPLERR, strMsg);
		if (MessageBox(strMsg, NULL, MB_ICONSTOP | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}
	}
	if (!pMemory->CheckCG()) {
		::GetMsg(IDS_INIT_CGERR, strMsg);
		if (MessageBox(strMsg, NULL, MB_ICONSTOP | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
			PostMessage(WM_CLOSE, 0, 0);
			return 0;
		}
	}

	// �ݒ�擾(power_off�ݒ�̂���)
	GetConfig()->GetConfig(&config);
	if (config.power_off) {
		// �d��OFF�ŋN��
		::GetVM()->SetPower(FALSE);
		::GetVM()->PowerSW(FALSE);
	}

	// �T�u�E�B���h�E�̏���
	m_strWndClsName = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS);

	// �R���|�[�l���g���C�l�[�u���B������Scheduler�͐ݒ�ɂ��
	GetView()->Enable(TRUE);
	pComponent = m_pFirstComponent;
	while (pComponent) {
		// �X�P�W���[����
		if (pComponent->GetID() == MAKEID('S', 'C', 'H', 'E')) {
			if (config.power_off) {
				// �d��OFF�ŋN��
				pComponent->Enable(FALSE);
				pComponent = pComponent->GetNextComponent();
				continue;
			}
		}

		// �C�l�[�u��
		pComponent->Enable(TRUE);
		pComponent = pComponent->GetNextComponent();
	}

	// ���Z�b�g(�X�e�[�^�X�o�[�̂���)
	if (!config.power_off) {
		OnReset();
	}

	// �R�}���h���C������
	lpszCmd = AfxGetApp()->m_lpCmdLine;
	lpszCommand = A2T(lpszCmd);
	if (_tcslen(lpszCommand) > 0) {
		InitCmd(lpszCommand);
	}

	// �ő剻�w��ł���΁A�߂�����ɁA�t���X�N���[��
	bFullScreen = FALSE;
	if (IsZoomed()) {
		ShowWindow(SW_RESTORE);
		bFullScreen = TRUE;
	}

	// �E�C���h�E�ʒu�����W���[��
	bFullScreen = RestoreFrameWnd(bFullScreen);
	if (bFullScreen) {
		// �ő剻�w�肩�A�O����s���Ƀt���X�N���[��
		PostMessage(WM_COMMAND, IDM_FULLSCREEN);
	}

	// �f�B�X�N�E�X�e�[�g�����W���[��
	RestoreDiskState();

	// �������[�v
	nIdle = 0;
	while (!m_bExit) {
		// ���b�Z�[�W�`�F�b�N���|���v
		if (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!AfxGetApp()->PumpMessage()) {
				::PostQuitMessage(0);
				return 0;
			}
			// continue���邱�ƂŁAWM_DESTROY�����m_bExit�`�F�b�N��ۏ�
			continue;
		}

		// �X���[�v
		if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			Sleep(20);

			// �X�V�J�E���^Up
			nIdle++;

			// �X�e�[�^�X�E���s��20ms
			UpdateExec();

			if ((nIdle & 1) == 0) {
				// �r���[��40ms
				GetView()->Update();
			}
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�N���[�Y
//
//---------------------------------------------------------------------------
void CFrmWnd::OnClose()
{
	CString strFormat;
	CString strText;
	Filepath path;
	int nResult;

	ASSERT(this);
	ASSERT(!m_bSaved);

	// �L���ȃX�e�[�g�t�@�C��������΁A�Z�[�u��₤
	::LockVM();
	::GetVM()->GetPath(path);
	::UnlockVM();

	// �L���ȃX�e�[�g�t�@�C����������
	if (!path.IsClear()) {
		// Windows�T�C�h��20ms�ȏ�̎��s���т������
		if (m_dwExec >= 2) {
			// �m�F
			::GetMsg(IDS_SAVECLOSE, strFormat);
			strText.Format(strFormat, path.GetFileExt());
			nResult = MessageBox(strText, NULL, MB_ICONQUESTION | MB_YESNOCANCEL);

			// �m�F���ʂɂ��
			switch (nResult) {
				// YES
				case IDYES:
					// �ۑ�
					OnSaveSub(path);
					break;

				// NO
				case IDNO:
					// �p�X���N���A(�X�e�[�g�Ȃ�)
					::GetVM()->Clear();
					break;

				// �L�����Z��
				case IDCANCEL:
					// �N���[�Y����Ȃ��������Ƃɂ���
					return;
			}
		}
	}

	// �������ς݂Ȃ�
	if ((m_nStatus == 0) && !m_bSaved) {
		// �E�B���h�E��ԁE�f�B�X�N�E�X�e�[�g��ۑ�
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

	// ��{�N���X
	CFrameWnd::OnClose();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDestroy()
{
	ASSERT(this);

	// �������ς݂Ȃ�
	if ((m_nStatus == 0) && !m_bSaved) {
		// �E�B���h�E��ԁE�f�B�X�N�E�X�e�[�g��ۑ�
		SaveFrameWnd();
		SaveDiskState();
		m_bSaved = TRUE;
	}

	// �N���[���A�b�v(WM_ENDSESSION�Ƌ���)
	CleanSub();

	// ��{�N���X��
	CFrameWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�Z�b�V�����I��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEndSession(BOOL bEnding)
{
	ASSERT(this);

	// �I�����́A�N���[���A�b�v���s��
	if (bEnding) {
		// �������ς݂Ȃ�
		if (m_nStatus == 0) {
			// �E�B���h�E��ԁE�f�B�X�N�E�X�e�[�g��ۑ�
			if (!m_bSaved) {
				SaveFrameWnd();
				SaveDiskState();
				m_bSaved = TRUE;
			}

			// �N���[���A�b�v
			CleanSub();
		}
	}

	// ��{�N���X
	CFrameWnd::OnEndSession(bEnding);
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v����
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::CleanSub()
{
	CComponent *pComponent;
	CComponent *pNext;
	int i;

	// �I���t���O���グ��
	m_bExit = TRUE;

	// �R���|�[�l���g���~�߂�
	GetView()->Enable(FALSE);
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pComponent->Enable(FALSE);
		pComponent = pComponent->GetNextComponent();
	}

	// �X�P�W���[�������s����߂�܂ő҂�
	for (i=0; i<8; i++) {
		::LockVM();
		::UnlockVM();
	}

	// �X�P�W���[�����~(CScheduler)
	if (m_nStatus == 0) {
		GetScheduler()->Stop();
	}

	// �R���|�[�l���g���폜
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pComponent->Cleanup();
		pComponent = pComponent->GetNextComponent();
	}
	pComponent = m_pFirstComponent;
	while (pComponent) {
		pNext = pComponent->GetNextComponent();
		delete pComponent;
		pComponent = pNext;
	}

	// ���z�}�V�����폜
	if (::pVM) {
		::LockVM();
		::GetVM()->Cleanup();
		delete ::pVM;
		::pVM = NULL;
		::UnlockVM();
	}

	// �V�F���ʒm���폜
	if (m_uNotifyId) {
		 VERIFY(::SHChangeNotifyDeregister(m_uNotifyId));
		 m_uNotifyId = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E��Ԃ�ۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveFrameWnd()
{
	CRect rectWnd;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// �ݒ�擾
	GetConfig()->GetConfig(&config);

	// �L���v�V�����E���j���[�E�X�e�[�^�X�o�[
	config.menu_bar = m_bMenuBar;

	// �E�B���h�E��`
	if (m_bFullScreen) {
		// �t���X�N���[�����́A�E�B���h�E���̈ʒu��ۑ�����
		config.window_left = m_nWndLeft;
		config.window_top = m_nWndTop;
	}
	else {
		// �E�B���h�E���́A���݂̈ʒu��ۑ�����
		GetWindowRect(&rectWnd);
		config.window_left = rectWnd.left;
		config.window_top = rectWnd.top;
	}

	// �t���X�N���[��
	config.window_full = m_bFullScreen;

	// �ݒ�ύX
	GetConfig()->SetConfig(&config);
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�E�X�e�[�g��ۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::SaveDiskState()
{
	int nDrive;
	Filepath path;
	Config config;

	ASSERT(this);
	ASSERT_VALID(this);

	// ���b�N
	::LockVM();

	// �ݒ�擾
	GetConfig()->GetConfig(&config);

	// �t���b�s�[�f�B�X�N
	for (nDrive=0; nDrive<2; nDrive++) {
		// ���f�B
		config.resume_fdi[nDrive] = m_pFDD->IsReady(nDrive, FALSE);

		// ���f�B�łȂ���΁A����
		if (!config.resume_fdi[nDrive]) {
			continue;
		}

		// ���f�B�A
		config.resume_fdm[nDrive]  = m_pFDD->GetMedia(nDrive);

		// ���C�g�v���e�N�g
		config.resume_fdw[nDrive] = m_pFDD->IsWriteP(nDrive);
	}

	// MO�f�B�X�N
	config.resume_mos = m_pSASI->IsReady();
	if (config.resume_mos) {
		config.resume_mow = m_pSASI->IsWriteP();
	}

	// CD-ROM
	config.resume_iso = m_pSCSI->IsReady(FALSE);

	// �X�e�[�g
	::GetVM()->GetPath(path);
	config.resume_xm6 = !path.IsClear();

	// �f�t�H���g�f�B���N�g��
	_tcscpy(config.resume_path, Filepath::GetDefaultDir());

	// �ݒ�ύX
	GetConfig()->SetConfig(&config);

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E��Ԃ𕜌�
//	��OnCreate��OnKick�ƁA2��Ă΂��
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::RestoreFrameWnd(BOOL bFullScreen)
{
	int nWidth;
	int nHeight;
	int nLeft;
	int nTop;
	CRect rectWnd;
	BOOL bValid;
	Config config;

	ASSERT(this);

	// �ݒ�擾
	GetConfig()->GetConfig(&config);

	// �E�B���h�E�ʒu�̕������w�肳��Ă��Ȃ���΁A�f�t�H���g��Ԃœ���
	if (!config.resume_screen) {
		return bFullScreen;
	}

	// ���j���[
	m_bMenuBar = config.menu_bar;
	ShowMenu();

	// ���z��ʂ̃T�C�Y�ƌ��_���擾
	nWidth = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
	nHeight = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
	nLeft = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
	nTop = ::GetSystemMetrics(SM_YVIRTUALSCREEN);

	// �E�B���h�E��`���擾
	GetWindowRect(&rectWnd);

	// ��̓͂��Ƃ���ɂ���΁A�E�B���h�E�ʒu���ړ�����B�܂��̓`�F�b�N
	bValid = TRUE;
	if (config.window_left < nLeft) {
		if (config.window_left < nLeft - rectWnd.Width()) {
			bValid = FALSE;
		}
	}
	else {
		if (config.window_left >= (nLeft + nWidth)) {
			bValid = FALSE;
		}
	}
	if (config.window_top < nTop) {
		if (config.window_top < nTop - rectWnd.Height()) {
			bValid = FALSE;
		}
	}
	else {
		if (config.window_top >= (nTop + nHeight)) {
			bValid = FALSE;
		}
	}

	// �E�B���h�E�ʒu���ړ�
	if (bValid) {
		SetWindowPos(&wndTop, config.window_left, config.window_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// ���[�N�G���A�������ɕύX����
		m_nWndLeft = config.window_left;
		m_nWndTop = config.window_top;
	}

	// VM���������Ȃ�A�����܂�
	if (m_nStatus != 0) {
		return FALSE;
	}

	// �t���X�N���[����
	if (bFullScreen || config.window_full) {
		// �ő剻�N�����A�O��t���X�N���[��������
		return TRUE;
	}
	else {
		// �ő剻�N���łȂ��A���A�O��ʏ�\��������
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�E�X�e�[�g�𕜌�
//
//---------------------------------------------------------------------------
void CFrmWnd::RestoreDiskState()
{
	int nDrive;
	TCHAR szMRU[_MAX_PATH];
	BOOL bResult;
	Filepath path;
	Config config;

	ASSERT(this);

	// �ݒ�擾
	GetConfig()->GetConfig(&config);

	// �X�e�[�g���w�肳��Ă���΁A������ɍs��
	if (config.resume_state) {
		// �X�e�[�g��������
		if (config.resume_xm6) {
			// �p�X�擾
			GetConfig()->GetMRUFile(4, 0, szMRU);
			path.SetPath(szMRU);

			// �I�[�v���O����
			if (OnOpenPrep(path)) {
				// �I�[�v���T�u
				if (OnOpenSub(path)) {
					// �����Ȃ̂ŁA�f�t�H���g�f�B���N�g����������
					if (config.resume_dir) {
						Filepath::SetDefaultDir(config.resume_path);
					}

					// ����ȍ~�͏������Ȃ�(FD, MO, CD�̃A�N�Z�X���ɃZ�[�u�����ꍇ)
					return;
				}
			}
		}
	}

	// �t���b�s�[�f�B�X�N
	if (config.resume_fd) {
		for (nDrive=0; nDrive<2; nDrive++) {
			// �f�B�X�N�}������Ă�����
			if (!config.resume_fdi[nDrive]) {
				// �f�B�X�N�}������Ă��Ȃ��B�X�L�b�v
				continue;
			}

			// �f�B�X�N�}��
			GetConfig()->GetMRUFile(nDrive, 0, szMRU);
			ASSERT(szMRU[0] != _T('\0'));
			path.SetPath(szMRU);

			// VM���b�N���s���A�f�B�X�N���蓖�Ă����݂�
			::LockVM();
			bResult = m_pFDD->Open(nDrive, path, config.resume_fdm[nDrive]);
			::UnlockVM();

			// ���蓖�Ăł��Ȃ���΃X�L�b�v
			if (!bResult) {
				continue;
			}

			// �������݋֎~
			if (config.resume_fdw[nDrive]) {
				::LockVM();
				m_pFDD->WriteP(nDrive, TRUE);
				::UnlockVM();
			}
		}
	}

	// MO�f�B�X�N
	if (config.resume_mo) {
		// �f�B�X�N�}������Ă�����
		if (config.resume_mos) {
			// �f�B�X�N�}��
			GetConfig()->GetMRUFile(2, 0, szMRU);
			ASSERT(szMRU[0] != _T('\0'));
			path.SetPath(szMRU);

			// VM���b�N���s���A�f�B�X�N���蓖�Ă����݂�
			::LockVM();
			bResult = m_pSASI->Open(path);
			::UnlockVM();

			// ���蓖�Ăł����
			if (bResult) {
				// �������݋֎~
				if (config.resume_mow) {
					::LockVM();
					m_pSASI->WriteP(TRUE);
					::UnlockVM();
				}
			}
		}
	}

	// CD-ROM
	if (config.resume_cd) {
		// �f�B�X�N�}������Ă�����
		if (config.resume_iso) {
			// �f�B�X�N�}��
			GetConfig()->GetMRUFile(3, 0, szMRU);
			ASSERT(szMRU[0] != _T('\0'));
			path.SetPath(szMRU);

			// VM���b�N���s���A�f�B�X�N���蓖�Ă����݂�
			::LockVM();
			m_pSCSI->Open(path, FALSE);
			::UnlockVM();
		}
	}

	// �f�t�H���g�f�B���N�g��
	if (config.resume_dir) {
		Filepath::SetDefaultDir(config.resume_path);
	}
}

//---------------------------------------------------------------------------
//
//	�f�B�X�v���C�ύX
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnDisplayChange(UINT uParam, LONG lParam)
{
	LRESULT lResult;

	// ��{�N���X
	lResult = CFrameWnd::OnDisplayChange(uParam, lParam);

	// �ŏ����͉������Ȃ�
	if (IsIconic()) {
		return lResult;
	}

	// �|�W�V�����ݒ�
	InitPos(FALSE);

	return lResult;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�w�i�`��
//
//---------------------------------------------------------------------------
BOOL CFrmWnd::OnEraseBkgnd(CDC * )
{
	// �w�i�`���}��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�`��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaint()
{
	// �K�����b�N���čs��
	::LockVM();

	PAINTSTRUCT ps;
	BeginPaint(&ps);
	EndPaint(&ps);

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�ړ�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMove(int x, int y)
{
//	CRect rect;
//
//	// �������ς݂Ȃ�
//	if (m_nStatus == 0) {
//	}
//
	// ��{�N���X
	CFrameWnd::OnMove(x, y);
}

//---------------------------------------------------------------------------
//
//	�A�N�e�B�x�[�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized)
{
//	CInput *pInput;
	CScheduler *pScheduler;

	// �������ς݂Ȃ�
	if (m_nStatus == 0) {
		// �C���v�b�g�A�X�P�W���[���֒ʒm
//		pInput = GetInput();
		pScheduler = GetScheduler();
		if (pScheduler) {
			// WA_INACTIVE���ŏ����Ȃ�A�f�B�Z�[�u��
			if ((nState == WA_INACTIVE) || bMinimized) {
				// ���͎󂯕t���Ȃ��A�ᑬ���s
				pScheduler->Activate(FALSE);
			}
			else {
				// ���͎󂯕t����A�ʏ���s
				pScheduler->Activate(TRUE);
			}
		}
	}

	// ��{�N���X��
	CFrameWnd::OnActivate(nState, pWnd, bMinimized);
}

//---------------------------------------------------------------------------
//
//	�A�N�e�B�x�[�g�A�v���P�[�V����
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CFrmWnd::OnActivateApp(BOOL bActive, DWORD dwThreadID)
#else
void CFrmWnd::OnActivateApp(BOOL bActive, HTASK hTask)
#endif
{
	// �������ς݂Ȃ�
	if (m_nStatus == 0) {
		// �t���X�N���[����p
		if (m_bFullScreen) {
			if (bActive) {
				// ���ꂩ��A�N�e�B�u�ɂȂ�
				HideTaskBar(TRUE, TRUE);
			}
			else {
				// �A�N�e�B�u����O�ꂽ
				HideTaskBar(FALSE, FALSE);
			}
		}
	}

	// ��{�N���X
#if _MFC_VER >= 0x700
	CFrameWnd::OnActivateApp(bActive, dwThreadID);
#else
	CFrameWnd::OnActivateApp(bActive, hTask);
#endif
}

//---------------------------------------------------------------------------
//
//	���j���[���[�v�J�n
//
//---------------------------------------------------------------------------
void CFrmWnd::OnEnterMenuLoop(BOOL bTrackPopup)
{
	::LockVM();

	// �X�P�W���[���֒ʒm
	CScheduler *pScheduler = GetScheduler();
	if (pScheduler) {
		pScheduler->Menu(TRUE);
	}

	::UnlockVM();

	// ��{�N���X��
	CFrameWnd::OnEnterMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	���j���[���[�v�I��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExitMenuLoop(BOOL bTrackPopup)
{
	::LockVM();

	// �X�P�W���[���֒ʒm
	CScheduler *pScheduler = GetScheduler();
	if (pScheduler) {
		pScheduler->Menu(FALSE);
	}

	::UnlockVM();

	// ��{�N���X��
	CFrameWnd::OnExitMenuLoop(bTrackPopup);
}

//---------------------------------------------------------------------------
//
//	�e�E�B���h�E�ʒm
//
//---------------------------------------------------------------------------
void CFrmWnd::OnParentNotify(UINT message, LPARAM lParam)
{
	// ��{�N���X��
	CFrameWnd::OnParentNotify(message, lParam);
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnContextMenu(CWnd * , CPoint pos)
{
	// �|�b�v�A�b�v���j���[
	m_bPopupMenu = TRUE;

	CMenu *pMenu = m_PopupMenu.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_CENTERALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							pos.x, pos.y, this, 0);
	m_bPopupMenu = FALSE;
}

//---------------------------------------------------------------------------
//
//	�d�͕ύX�ʒm
//
//---------------------------------------------------------------------------
LONG CFrmWnd::OnPowerBroadCast(UINT , LONG )
{
	// �������ς݂Ȃ�
	if (m_nStatus == 0) {
		// VM���b�N�A���ԍĐݒ�
		::LockVM();
		timeEndPeriod(1);
		timeBeginPeriod(1);
		::UnlockVM();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�X�e���R�}���h
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	// �W���E�B���h�E�ʒu���T�|�[�g
	if ((nID & 0xfff0) == IDM_STDWIN) {
		InitPos(TRUE);
		return;
	}

	// �ő剻�̓t���X�N���[��
	if ((nID & 0xfff0) == SC_MAXIMIZE) {
		if (!m_bFullScreen) {
			PostMessage(WM_COMMAND, IDM_FULLSCREEN);
		}
		return;
	}

	// ��{�N���X
	CFrameWnd::OnSysCommand(nID, lParam);
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]��
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
afx_msg BOOL CFrmWnd::OnCopyData(CWnd* , COPYDATASTRUCT* pCopyDataStruct)
#else
LONG CFrmWnd::OnCopyData(UINT , LONG pCopyDataStruct)
#endif
{
	PCOPYDATASTRUCT pCDS;

	// �p�����[�^�󂯎��
	pCDS = (PCOPYDATASTRUCT)pCopyDataStruct;

	// �R�}���h���C��������
	InitCmd((LPSTR)pCDS->lpData);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�F���ʒm
//
//---------------------------------------------------------------------------
LRESULT CFrmWnd::OnShellNotify(UINT uParam, LONG lParam)
{
	HANDLE hMemoryMap;
	DWORD dwProcessId;
	LPITEMIDLIST *pidls;
	HANDLE hLock;
	LONG nEvent;
	TCHAR szPath[_MAX_PATH];
	CHost *pHost;

	// Windows NT��
	if (::IsWinNT()) {
		// Windows2000/XP�̏ꍇ�ASHChangeNotification_Lock�Ń��b�N����
		hMemoryMap = (HANDLE)uParam;
		dwProcessId = (DWORD)lParam;
		hLock = ::SHChangeNotification_Lock(hMemoryMap, dwProcessId, &pidls, &nEvent);
		if (hLock == NULL) {
			return 0;
		}
	}
	else {
		// Windows9x�̏ꍇ�Apidls��nEvent��uParam,lParam���璼�ړ���
		pidls = (LPITEMIDLIST*)uParam;
		nEvent = lParam;
		hLock = NULL;
	}

	// ���s���ŁACHost������΁A�ʒm
	if (m_nStatus == 0) {
		pHost = GetHost();

#if 1
		// Windrv���܂��s����̂��߁A���ۂ�Enable�ɂ���Ă��Ȃ��ꍇ�͉������Ȃ�(version2.04)
		{
			Config config;
			GetConfig()->GetConfig(&config);
			if ((config.windrv_enable <= 0) || (config.windrv_enable > 3)) {
				pHost = NULL;
			}
		}
#endif

		if (pHost) {
			// �p�X�擾
			::SHGetPathFromIDList(pidls[0], szPath);

			// �ʒm
			pHost->ShellNotify(nEvent, szPath);
		}
	}

	// NT�̏ꍇ�ASHCnangeNotifcation_Unlock�ŃA�����b�N����
	if (::IsWinNT()) {
		ASSERT(hLock);
		::SHChangeNotification_Unlock(hLock);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	�X�V(���s)
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::UpdateExec()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// �X�P�W���[�����L���Ȃ�A���s�J�E���^���グ��(�Z�[�u���ɃN���A�����)
	if (GetScheduler()->IsEnable()) {
		m_dwExec++;
		if (m_dwExec == 0) {
			m_dwExec--;
		}
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�������
//
//---------------------------------------------------------------------------
void CFrmWnd::GetMessageString(UINT nID, CString& rMessage) const
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];
	TCHAR szName[60];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	int nMRU;
	int nDisk;
	BOOL bValid;
//	CInfo *pInfo;

	// �t���OFALSE
	bValid = FALSE;

	// ���j���[��������ɍs��(�p���+MRU���l��)
	if ((nID >= IDM_OPEN) && (nID <= IDM_ABOUT)) {
		// �p�����
		if (!::IsJapanese()) {
			// +5000�Ŏ���
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// ���j���[���������(IDM_STDWIN)
	if (nID == IDM_STDWIN) {
		// �p�����
		if (!::IsJapanese()) {
			// +5000�Ŏ���
			if (rMessage.LoadString(nID + 5000)) {
				bValid = TRUE;
			}
		}
	}

	// MRU0
	if ((nID >= IDM_D0_MRU0) && (nID <= IDM_D0_MRU8)) {
		nMRU = nID - IDM_D0_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(0, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU1
	if ((nID >= IDM_D1_MRU0) && (nID <= IDM_D1_MRU8)) {
		nMRU = nID - IDM_D1_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(1, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU2
	if ((nID >= IDM_MO_MRU0) && (nID <= IDM_MO_MRU8)) {
		nMRU = nID - IDM_MO_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(2, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU3
	if ((nID >= IDM_CD_MRU0) && (nID <= IDM_CD_MRU8)) {
		nMRU = nID - IDM_CD_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(3, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// MRU4
	if ((nID >= IDM_XM6_MRU0) && (nID <= IDM_XM6_MRU8)) {
		nMRU = nID - IDM_XM6_MRU0;
		ASSERT((nMRU >= 0) && (nMRU <= 8));
		GetConfig()->GetMRUFile(4, nMRU, szPath);
		szPath[60] = _T('\0');
		rMessage = szPath;
		bValid = TRUE;
	}

	// �f�B�X�N��0
	if ((nID >= IDM_D0_MEDIA0) && (nID <= IDM_D0_MEDIAF)) {
		nDisk = nID - IDM_D0_MEDIA0;
		ASSERT((nDisk >= 0) && (nDisk <= 15));
		::LockVM();
		m_pFDD->GetName(0, szName, nDisk);
		m_pFDD->GetPath(0, path);
		::UnlockVM();
		_tsplitpath(path.GetPath(), szDrive, szDir, szFile, szExt);
		rMessage = szName;
		rMessage += _T(" (");
		rMessage += szFile;
		rMessage += szExt;
		rMessage += _T(")");
		bValid = TRUE;
	}

	// �f�B�X�N��1
	if ((nID >= IDM_D1_MEDIA0) && (nID <= IDM_D1_MEDIAF)) {
		nDisk = nID - IDM_D1_MEDIA0;
		ASSERT((nDisk >= 0) && (nDisk <= 15));
		::LockVM();
		m_pFDD->GetName(1, szName, nDisk);
		m_pFDD->GetPath(1, path);
		::UnlockVM();
		_tsplitpath(path.GetPath(), szDrive, szDir, szFile, szExt);
		rMessage = szName;
		rMessage += _T(" (");
		rMessage += szFile;
		rMessage += szExt;
		rMessage += _T(")");
		bValid = TRUE;
	}

	// �����܂łŊm�肵�Ă��Ȃ���΁A��{�N���X
	if (!bValid) {
		CFrameWnd::GetMessageString(nID, rMessage);
	}
}

//---------------------------------------------------------------------------
//
//	�^�X�N�o�[�B��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::HideTaskBar(BOOL bHide, BOOL bFore)
{
	if (bHide) {
		// "��ɑO��"
		m_hTaskBar = ::FindWindow(_T("Shell_TrayWnd"), NULL);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_HIDE);
		}
		ModifyStyleEx(0, WS_EX_TOPMOST, 0);
	}
	else {
		// "�ʏ�"
		ModifyStyleEx(WS_EX_TOPMOST, 0, 0);
		if (m_hTaskBar) {
			::ShowWindow(m_hTaskBar, SW_SHOWNA);
		}
	}

	// �O�ʃI�v�V�����������
	if (bFore) {
		SetForegroundWindow();
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�i�[�h���[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS)
{
}

//---------------------------------------------------------------------------
//
//	���j���[�o�[�\��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::ShowMenu()
{
	HMENU hMenu;

	ASSERT(this);

	// �K�v�ł����VM�����b�N
	if (m_nStatus == 0) {
		::LockVM();
	}

	// ���݂̃��j���[���擾
	hMenu = ::GetMenu(m_hWnd);

	// ���j���[���s�K�v�ȏꍇ
	if (m_bFullScreen || !m_bMenuBar) {
		// ���j���[�����݂��邩
		if (hMenu != NULL) {
			// ���j���[������
			SetMenu(NULL);
		}
		if (m_nStatus == 0) {
			::UnlockVM();
		}
		return;
	}

	// ���j���[���K�v�ȏꍇ
	if (hMenu != NULL) {
		// �Z�b�g���������j���[�Ɠ�����
		if (m_Menu.GetSafeHmenu() == hMenu) {
			// �ύX�̕K�v�͂Ȃ�
			if (m_nStatus == 0) {
				::UnlockVM();
			}
			return;
		}
	}

	// ���j���[���Z�b�g
	SetMenu(&m_Menu);

	// �K�v�Ȃ�VM���A�����b�N
	if (m_nStatus == 0) {
		::UnlockVM();
	}
}

//---------------------------------------------------------------------------
//
//	�`��r���[�擾
//
//---------------------------------------------------------------------------
CDrawView* FASTCALL CFrmWnd::GetView() const
{
	ASSERT(this);
	ASSERT(m_pDrawView);
	ASSERT(m_pDrawView->m_hWnd);
	return m_pDrawView;
}

//---------------------------------------------------------------------------
//
//	�ŏ��̃R���|�[�l���g���擾
//
//---------------------------------------------------------------------------
CComponent* FASTCALL CFrmWnd::GetFirstComponent() const
{
	ASSERT(this);
	return m_pFirstComponent;
}

//---------------------------------------------------------------------------
//
//	�X�P�W���[���擾
//
//---------------------------------------------------------------------------
CScheduler* FASTCALL CFrmWnd::GetScheduler() const
{
	ASSERT(this);
	ASSERT(m_pSch);
	return m_pSch;
}
/*
//---------------------------------------------------------------------------
//
//	�C���v�b�g�擾
//
//---------------------------------------------------------------------------
CInput* FASTCALL CFrmWnd::GetInput() const
{
	ASSERT(this);
	ASSERT(m_pInput);
	return m_pInput;
}
*/
//---------------------------------------------------------------------------
//
//	Host�擾
//
//---------------------------------------------------------------------------
CHost* FASTCALL CFrmWnd::GetHost() const
{
	ASSERT(this);
	ASSERT(m_pHost);
	return m_pHost;
}

//---------------------------------------------------------------------------
//
//	�R���t�B�O�擾
//
//---------------------------------------------------------------------------
CConfig* FASTCALL CFrmWnd::GetConfig() const
{
	ASSERT(this);
	ASSERT(m_pConfig);
	return m_pConfig;
}

#endif	// _WIN32
