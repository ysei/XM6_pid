//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R�}���h���� ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "fdd.h"
#include "fdi.h"
#include "rtc.h"
#include "keyboard.h"
#include "sasi.h"
#include "sram.h"
#include "memory.h"
#include "render.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_res.h"
#include "mfc_draw.h"

#include "mfp.h"
#include "dmac.h"
#include "scc.h"
#include "fdc.h"
#include "midi.h"
#include "sasi.h"
#include "scsi.h"

#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_cfg.h"

//---------------------------------------------------------------------------
//
//	�J��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];

	// �R�����_�C�A���O���s
	::GetVM()->GetPath(path);
	_tcscpy(szPath, path.GetPath());
	if (!::FileOpenDlg(this, szPath, IDS_XM6OPEN)) {
//		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// �I�[�v���O����
	if (!OnOpenPrep(path)) {
		return;
	}

	// �I�[�v���T�u
	OnOpenSub(path);
}

//---------------------------------------------------------------------------
//
//	�J�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOpenUI(CCmdUI *pCmdUI)
{
	BOOL bPower;
	BOOL bSW;
	Filepath path;
	CMenu *pMenu;
	CMenu *pSubMenu;
	CString strExit;
	TCHAR szMRU[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	int nEnable;
	int i;

	// �d����Ԃ��擾�A�t�@�C���p�X���擾(VM���b�N���čs��)
	::LockVM();
	bPower = ::GetVM()->IsPower();
	bSW = ::GetVM()->IsPowerSW();
	::GetVM()->GetPath(path);
	::UnlockVM();

	// �I�[�v��
	pCmdUI->Enable(bPower);

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);
	// �t�@�C�����j���[�͍ŏ�
	pSubMenu = pMenu->GetSubMenu(0);
	ASSERT(pSubMenu);

	// �㏑���ۑ�UI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if (bPower && (_tcslen(path.GetPath()) > 0)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// ���O��t���ĕۑ�UI
	if (bPower) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// ���Z�b�gUI
	if (bPower) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// �C���^���v�gUI
	if (bPower) {
		pSubMenu->EnableMenuItem(6, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(6, MF_BYPOSITION | MF_GRAYED);
	}

	// �d���X�C�b�`UI
	if (bSW) {
		pSubMenu->EnableMenuItem(7, MF_BYPOSITION | MF_CHECKED);
	}
	else {
		pSubMenu->EnableMenuItem(7, MF_BYPOSITION | MF_UNCHECKED);
	}

	// �Z�p���[�^�������āA����ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 9) {
		pSubMenu->RemoveMenu(9, MF_BYPOSITION);
	}

	// MRU���Ȃ���΁A�I�����j���[��ǉ����ďI���
	if (GetConfig()->GetMRUNum(4) == 0) {
		::GetMsg(IDS_EXIT, strExit);
		pSubMenu->AppendMenu(MF_STRING, IDM_EXIT, strExit);
		return;
	}

	// �L���E�����萔�ݒ�
	if (bPower) {
		nEnable = MF_BYCOMMAND | MF_GRAYED;
	}
	else {
		nEnable = MF_BYCOMMAND | MF_ENABLED;
	}

	// MRU���� - �ǉ�
	for (i=0; i<9; i++) {
		// �擾���Ă݂�
		GetConfig()->GetMRUFile(4, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ����΃��j���[�ɒǉ�
		_tsplitpath(szMRU, szDrive, szDir, szFile, szExt);
		if (_tcslen(szDir) > 1) {
			_tcscpy(szDir, _T("\\...\\"));
		}
		_stprintf(szMRU, _T("&%d "), i + 1);
		_tcscat(szMRU, szDrive);
		_tcscat(szMRU, szDir);
		_tcscat(szMRU, szFile);
		_tcscat(szMRU, szExt);

		pSubMenu->AppendMenu(MF_STRING, IDM_XM6_MRU0 + i, szMRU);
		pSubMenu->EnableMenuItem(IDM_XM6_MRU0 + i, nEnable);
	}

	// �Z�p���[�^��ǉ�
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// �I�����j���[��ǉ�
	::GetMsg(IDS_EXIT, strExit);
	pSubMenu->AppendMenu(MF_STRING, IDM_EXIT, strExit);
}

//---------------------------------------------------------------------------
//
//	�I�[�v���O�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::OnOpenPrep(const Filepath& path, BOOL bWarning)
{
	Fileio fio;
	CString strMsg;
	CString strFmt;
	char cHeader[0x10];
	int nRecVer;
	int nNowVer;
	DWORD dwMajor;
	DWORD dwMinor;

	ASSERT(this);

	// �t�@�C�����݃`�F�b�N
	if (!fio.Open(path, Fileio::ReadOnly)) {
		if (bWarning) {
			::GetMsg(IDS_XM6LOADFILE, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
		return FALSE;
	}

	// �w�b�_�ǂݍ���
	memset(cHeader, 0, sizeof(cHeader));
	fio.Read(cHeader, sizeof(cHeader));
	fio.Close();

	// �L�^�o�[�W�����擾
	cHeader[0x0a] = '\0';
	nRecVer = ::strtoul(&cHeader[0x09], NULL, 16);
	nRecVer <<= 8;
	cHeader[0x0d] = '\0';
	nRecVer |= ::strtoul(&cHeader[0x0b], NULL, 16);

	// ���s�o�[�W�����擾
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nNowVer = (int)((dwMajor << 8) | dwMinor);

	// �w�b�_�`�F�b�N
	cHeader[0x09] = '\0';
	if (strcmp(cHeader, "XM6 DATA ") != 0) {
		if (bWarning) {
			::GetMsg(IDS_XM6LOADHDR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
		return FALSE;
	}

	// �o�[�W�����`�F�b�N
	if (nNowVer < nRecVer) {
		// �L�^����Ă���o�[�W�����̂ق����V����(�m��Ȃ��`��)
		::GetMsg(IDS_XM6LOADVER, strMsg);
		strFmt.Format(strMsg,
						nNowVer >> 8, nNowVer & 0xff,
						nRecVer >> 8, nRecVer & 0xff);
		MessageBox(strFmt, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// �p��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�I�[�v���T�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::OnOpenSub(const Filepath& path)
{
	BOOL bRun;
	CString strMsg;
	DWORD dwPos;
	Filepath diskpath;
	int nDrive;

	// �X�P�W���[����~�A�T�E���h��~
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
//	BOOL bSound = GetSound()->IsEnable();
//	GetSound()->Enable(FALSE);

	// ���[�h
	AfxGetApp()->BeginWaitCursor();

	// VM
	dwPos = ::GetVM()->Load(path);
	if (dwPos == 0) {
		AfxGetApp()->EndWaitCursor();

		// ���s�͓r�����f�Ŋ댯�Ȃ��߁A�K�����Z�b�g����
		::GetVM()->Reset();
//		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
//		ResetCaption();

		// ���[�h�G���[
		::GetMsg(IDS_XM6LOADERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// MFC
	if (!LoadComponent(path, dwPos)) {
		AfxGetApp()->EndWaitCursor();

		// ���s�͓r�����f�Ŋ댯�Ȃ��߁A�K�����Z�b�g����
		::GetVM()->Reset();
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);

		// ���[�h�G���[
		::GetMsg(IDS_XM6LOADERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// ���[�h�I��
	AfxGetApp()->EndWaitCursor();

	// FD, MO, CD��MRU�֒ǉ�(version2.04�ȍ~�̃��W���[���΍�)
	for (nDrive=0; nDrive<2; nDrive++) {
		if (m_pFDD->IsReady(nDrive, FALSE)) {
			m_pFDD->GetPath(nDrive, diskpath);
			GetConfig()->SetMRUFile(nDrive, diskpath.GetPath());
		}
	}
	if (m_pSASI->IsReady()) {
		m_pSASI->GetPath(diskpath);
		GetConfig()->SetMRUFile(2, diskpath.GetPath());
	}
	// ���s�J�E���^���N���A
	m_dwExec = 0;

	// ����
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(4, path.GetPath());

	// ��񃁃b�Z�[�W��\��
	::GetMsg(IDS_XM6LOADOK, strMsg);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�㏑���ۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSave()
{
	Filepath path;

	// VM����J�����g�p�X���󂯎��
	::GetVM()->GetPath(path);

	// �N���A����Ă���ΏI��
	if (path.IsClear()) {
		return;
	}

	// �ۑ��T�u
	OnSaveSub(path);
}

//---------------------------------------------------------------------------
//
//	�㏑���ۑ� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveUI(CCmdUI *pCmdUI)
{
	Filepath path;

	// �d��OFF�ł���΋֎~
	if (!::GetVM()->IsPower()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// VM����J�����g�p�X���󂯎��
	::GetVM()->GetPath(path);

	// �N���A����Ă���Ύg�p�֎~
	if (path.IsClear()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// �g�p����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	���O��t���ĕۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveAs()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];

	// �R�����_�C�A���O���s
	::GetVM()->GetPath(path);
	_tcscpy(szPath, path.GetPath());
	if (!::FileSaveDlg(this, szPath, _T("xm6"), IDS_XM6OPEN)) {
//		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// �ۑ��T�u
	OnSaveSub(path);
}

//---------------------------------------------------------------------------
//
//	���O��t���ĕۑ� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveAsUI(CCmdUI *pCmdUI)
{
	// �d��ON�̏ꍇ�̂�
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	�ۑ��T�u
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnSaveSub(const Filepath& path)
{
	BOOL bRun;
	CString strMsg;
	DWORD dwPos;

	// �X�P�W���[����~�A�T�E���h��~
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
//	BOOL bSound = GetSound()->IsEnable();
//	GetSound()->Enable(FALSE);

	AfxGetApp()->BeginWaitCursor();

	// �X�P�W���[���ɑ΂��āA�Z�[�u���̏�Ԃ�ʒm(version2.04)
//	GetScheduler()->SetSavedEnable(bRun);

	// VM
	dwPos = ::GetVM()->Save(path);
	if (dwPos== 0) {
		AfxGetApp()->EndWaitCursor();

		// �Z�[�u���s
//		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
//		ResetCaption();

		// �Z�[�u�G���[
		::GetMsg(IDS_XM6SAVEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return;
	}

	// MFC
	if (!SaveComponent(path, dwPos)) {
		AfxGetApp()->EndWaitCursor();

		// �Z�[�u���s
//		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
//		ResetCaption();

		// �Z�[�u�G���[
		::GetMsg(IDS_XM6SAVEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return;
	}

	// ���s�J�E���^���N���A
	m_dwExec = 0;

	AfxGetApp()->EndWaitCursor();

	// ����
//	GetSound()->Enable(bSound);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
//	ResetCaption();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(4, path.GetPath());

	// ��񃁃b�Z�[�W��\��
	::GetMsg(IDS_XM6SAVEOK, strMsg);
//	SetInfo(strMsg);
}

//---------------------------------------------------------------------------
//
//	MRU
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMRU(UINT uID)
{
	TCHAR szMRU[_MAX_PATH];
	Filepath path;

	ASSERT(uID >= IDM_XM6_MRU0);

	// uID�ϊ�
	uID -= IDM_XM6_MRU0;
	ASSERT(uID <= 8);

	// MRU�擾�A�p�X�쐬
	GetConfig()->GetMRUFile(4, (int)uID, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// �I�[�v���O����
	if (!OnOpenPrep(path)) {
		return;
	}

	// �I�[�v������
	if (OnOpenSub(path)) {
		// �f�t�H���g�f�B���N�g���X�V
		Filepath::SetDefaultDir(szMRU);
	}
}

//---------------------------------------------------------------------------
//
//	MRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMRUUI(CCmdUI *pCmdUI)
{
	// �d��ON�̏ꍇ�̂�
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnReset()
{
	SRAM *pSRAM;
	DWORD Sw[0x100];
	DWORD dwDevice;
	DWORD dwAddr;
	CString strReset;
	CString strSub;
	BOOL bFlag;
	int i;

	// �d��OFF�Ȃ瑀��s��
	if (!::GetVM()->IsPower()) {
		return;
	}

	::LockVM();

	// ���Z�b�g���ĕ`��
	::GetVM()->Reset();
	GetView()->Refresh();
//	ResetCaption();

	// �������X�C�b�`�擾���s��
	pSRAM = (SRAM*)::GetVM()->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(pSRAM);
	for (i=0; i<0x100; i++) {
		Sw[i] = pSRAM->ReadOnly(0xed0000 + i);
	}

	::UnlockVM();

	// ���Z�b�g���b�Z�[�W�����[�h
	::GetMsg(IDS_RESET, strReset);

	// �������X�C�b�`�̐擪���r
	if (memcmp(Sw, SigTable, sizeof(DWORD) * 7) != 0) {
//		SetInfo(strReset);
		return;
	}

	// �u�[�g�f�o�C�X���擾
	dwDevice = Sw[0x18];
	dwDevice <<= 8;
	dwDevice |= Sw[0x19];

	// �u�[�g�f�o�C�X����
	bFlag = FALSE;
	if (dwDevice == 0x0000) {
		// STD
		strSub = _T("STD)");
		bFlag = TRUE;
	}
	if (dwDevice == 0xa000) {
		// ROM
		dwAddr = Sw[0x0c];
		dwAddr = (dwAddr << 8) | Sw[0x0d];
		dwAddr = (dwAddr << 8) | Sw[0x0e];
		dwAddr = (dwAddr << 8) | Sw[0x0f];

		// FC0000�`FC001C�ƁAEA0020�`EA003C��SCSI#
		strSub.Format(_T("ROM $%06X)"), dwAddr);
		if ((dwAddr >= 0xfc0000) && (dwAddr < 0xfc0020)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		if ((dwAddr >= 0xea0020) && (dwAddr < 0xea0040)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		bFlag = TRUE;
	}
	if (dwDevice == 0xb000) {
		// RAM
		dwAddr = Sw[0x10];
		dwAddr = (dwAddr << 8) | Sw[0x11];
		dwAddr = (dwAddr << 8) | Sw[0x12];
		dwAddr = (dwAddr << 8) | Sw[0x13];
		strSub.Format(_T("RAM $%06X)"), dwAddr);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x9070) {
		strSub.Format(_T("2HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x8000) {
		strSub.Format(_T("HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if (!bFlag) {
		strSub = _T("Unknown)");
	}

	// �\��
	strReset += _T(" (");
	strReset += strSub;
//	SetInfo(strReset);
}

//---------------------------------------------------------------------------
//
//	���Z�b�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnResetUI(CCmdUI *pCmdUI)
{
	// �d��ON�Ȃ瑀��ł���
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	SRAM�V�O�l�`���e�[�u��
//
//---------------------------------------------------------------------------
const DWORD CFrmWnd::SigTable[] = {
	0x82, 0x77, 0x36, 0x38, 0x30, 0x30, 0x30
};

//---------------------------------------------------------------------------
//
//	�C���^���v�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInterrupt()
{
	CString strIntr;

	// �d��ON�Ȃ瑀��ł���
	if (::GetVM()->IsPower()) {
		// NMI���荞��
		::LockVM();
		::GetVM()->Interrupt();
		::UnlockVM();

		// ���b�Z�[�W
		::GetMsg(IDS_INTERRUPT, strIntr);
//		SetInfo(strIntr);
	}
}

//---------------------------------------------------------------------------
//
//	�C���^���v�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInterruptUI(CCmdUI *pCmdUI)
{
	// �d��ON�Ȃ瑀��ł���
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	�d���X�C�b�`
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPower()
{
	BOOL bPower;

	::LockVM();

	if (::GetVM()->IsPowerSW()) {
		// �I���Ȃ�I�t
		::GetVM()->PowerSW(FALSE);
		::UnlockVM();
		return;
	}

	// ���݂̓d���̏�Ԃ�ۑ����āA�d��ON
	bPower = ::GetVM()->IsPower();
	::GetVM()->PowerSW(TRUE);

	// �d�����؂�Ă��ăX�P�W���[�����~�܂��Ă���΁A������
	if (!bPower && !GetScheduler()->IsEnable()) {
		GetScheduler()->Enable(TRUE);
	}

	::UnlockVM();

	// ���Z�b�g(�X�e�[�^�X�o�[�\���̂���)
	OnReset();
}

//---------------------------------------------------------------------------
//
//	�d���X�C�b�` UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPowerUI(CCmdUI *pCmdUI)
{
	// �Ƃ肠�����A�I���Ȃ�`�F�b�N���Ă���
	pCmdUI->SetCheck(::GetVM()->IsPowerSW());
}

//---------------------------------------------------------------------------
//
//	�I��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExit()
{
	PostMessage(WM_CLOSE, 0, 0);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�f�B�X�N����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFD(UINT uID)
{
	int nDrive;

	// �h���C�u����
	nDrive = 0;
	if (uID >= IDM_D1OPEN) {
		nDrive = 1;
		uID -= (IDM_D1OPEN - IDM_D0OPEN);
	}

	switch (uID) {
		// �I�[�v��
		case IDM_D0OPEN:
			OnFDOpen(nDrive);
			break;

		// �C�W�F�N�g
		case IDM_D0EJECT:
			OnFDEject(nDrive);
			break;

		// �������ݕی�
		case IDM_D0WRITEP:
			OnFDWriteP(nDrive);
			break;

		// �����C�W�F�N�g
		case IDM_D0FORCE:
			OnFDForce(nDrive);
			break;

		// ��}��
		case IDM_D0INVALID:
			OnFDInvalid(nDrive);
			break;

		// ����ȊO
		default:
			if (uID >= IDM_D0_MRU0) {
				// MRU
				uID -= IDM_D0_MRU0;
				ASSERT(uID <= 8);
				OnFDMRU(nDrive, (int)uID);
			}
			else {
				// Media
				uID -= IDM_D0_MEDIA0;
				ASSERT(uID <= 15);
				OnFDMedia(nDrive, (int)uID);
			}
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�I�[�v��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDOpen(int nDrive)
{
	Filepath path;
	CString strMsg;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(m_pFDD);

	// �R�����_�C�A���O���s
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_FDOPEN)) {
//		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VM���b�N
	::LockVM();

	// �f�B�X�N���蓖��
	if (!m_pFDD->Open(nDrive, path)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// �I�[�v���G���[
		::GetMsg(IDS_FDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
//		ResetCaption();
		return;
	}

	// VM�����X�^�[�g������O�ɁAFDI���擾���Ă���
	pFDI = m_pFDD->GetFDI(nDrive);

	// ����
	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(nDrive, szPath);

	// �����Ȃ�ABAD�C���[�W�x��
	if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
		::GetMsg(IDS_BADFDI_WARNING, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDEject(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM�����b�N���čs��
	::LockVM();
	m_pFDD->Eject(nDrive, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�������ݕی�
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDWriteP(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// �C���[�W�𑀍�
	::LockVM();
	m_pFDD->WriteP(nDrive, !m_pFDD->IsWriteP(nDrive));
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�����C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDForce(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM�����b�N���čs��
	::LockVM();
	m_pFDD->Eject(nDrive, TRUE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[��}��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDInvalid(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM�����b�N���čs��
	::LockVM();
	m_pFDD->Invalid(nDrive);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[���f�B�A
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDMedia(int nDrive, int nMedia)
{
	Filepath path;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT((nMedia >= 0) && (nMedia <= 15));

	// VM���b�N
	::LockVM();

	// �O�̂��ߊm�F
	if (nMedia < m_pFDD->GetDisks(nDrive)) {
		m_pFDD->GetPath(nDrive, path);

		// �ăI�[�v��
		m_pFDD->Open(nDrive, path, nMedia);
	}

	// VM�A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[MRU
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDMRU(int nDrive, int nMRU)
{
	TCHAR szMRU[_MAX_PATH];
	Filepath path;
	BOOL bResult;
	FDI *pFDI;
	CString strMsg;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT((nMRU >= 0) && (nMRU <= 8));

	// MRU�擾�A�p�X�쐬
	GetConfig()->GetMRUFile(nDrive, nMRU, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// VM���b�N
	::LockVM();

	// �f�B�X�N���蓖�Ă����݂�
	bResult = m_pFDD->Open(nDrive, path);
	pFDI = m_pFDD->GetFDI(nDrive);
	GetScheduler()->Reset();
//	ResetCaption();

	// VM�A�����b�N
	::UnlockVM();

	// ��������΁A�f�B���N�g���X�V��MRU�ǉ�
	if (bResult) {
		// �f�t�H���g�f�B���N�g���X�V
		Filepath::SetDefaultDir(szMRU);

		// MRU�ɒǉ�
		GetConfig()->SetMRUFile(nDrive, szMRU);

		// BAD�C���[�W�x��
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�I�[�v�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDOpenUI(CCmdUI *pCmdUI)
{
	CMenu *pMenu;
	CMenu *pSubMenu;
	UINT nEnable;
	int nDrive;
	int nStat;
	int nDisks;
	int nMedia;
	char szShort[_MAX_PATH];
	LPTSTR lpszShort;
	int i;
	TCHAR szMRU[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	ASSERT(this);
	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	::LockVM();
	nStat = m_pFDD->GetStatus(nDrive);
	m_nFDDStatus[nDrive] = nStat;
	nDisks = m_pFDD->GetDisks(nDrive);
	nMedia = m_pFDD->GetMedia(nDrive);
	::UnlockVM();
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	// �t�@�C��(F)�̎��Ƀt���b�s�[0�A�t���b�s�[1�ƕ���
	pSubMenu = pMenu->GetSubMenu(nDrive + 1);

	// �C�W�F�N�gUI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// �������ݕی�UI
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// �����C�W�F�N�gUI
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// ��}��UI
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_GRAYED);
	}

	// �ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->RemoveMenu(6, MF_BYPOSITION);
	}

	// �}���`�f�B�X�N����
	if (nDisks > 1) {
		// �L���E�����萔�ݒ�
		if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
		else {
			nEnable = MF_BYCOMMAND | MF_GRAYED;
		}

		// �Z�p���[�^��}��
		pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

		// ���f�B�A���[�v
		ASSERT(nDisks <= 16);
		for (i=0; i<nDisks; i++) {
			// �f�B�X�N����char*�Ŋi�[����Ă���ׁATCHAR�֕ϊ�
			m_pFDD->GetName(nDrive, szShort, i);
			lpszShort = A2T(szShort);

			// �ǉ�
			if (nDrive == 0) {
				pSubMenu->AppendMenu(MF_STRING, IDM_D0_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D0_MEDIA0 + i, nEnable);
			}
			else {
				pSubMenu->AppendMenu(MF_STRING, IDM_D1_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D1_MEDIA0 + i, nEnable);
			}
		}

		// ���W�I�{�^���ݒ�
		if (nDrive == 0) {
			pSubMenu->CheckMenuRadioItem(IDM_D0_MEDIA0, IDM_D0_MEDIAF,
										IDM_D0_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
		else {
			pSubMenu->CheckMenuRadioItem(IDM_D1_MEDIA0, IDM_D1_MEDIAF,
										IDM_D1_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
	}

	// MRU���� - �Z�p���[�^
	if (GetConfig()->GetMRUNum(nDrive) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// �L���E�����萔�ݒ�
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		nEnable = MF_BYCOMMAND | MF_GRAYED;
	}
	else {
		nEnable = MF_BYCOMMAND | MF_ENABLED;
	}

	// MRU���� - �ǉ�
	for (i=0; i<9; i++) {
		// �擾���Ă݂�
		GetConfig()->GetMRUFile(nDrive, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ����΃��j���[�ɒǉ�
		_tsplitpath(szMRU, szDrive, szDir, szFile, szExt);
		if (_tcslen(szDir) > 1) {
			_tcscpy(szDir, _T("\\...\\"));
		}
		_stprintf(szMRU, _T("&%d "), i + 1);
		_tcscat(szMRU, szDrive);
		_tcscat(szMRU, szDir);
		_tcscat(szMRU, szFile);
		_tcscat(szMRU, szExt);
		if (nDrive == 0) {
			pSubMenu->AppendMenu(MF_STRING, IDM_D0_MRU0 + i, szMRU);
			pSubMenu->EnableMenuItem(IDM_D0_MRU0 + i, nEnable);
		}
		else {
			pSubMenu->AppendMenu(MF_STRING, IDM_D1_MRU0 + i, szMRU);
			pSubMenu->EnableMenuItem(IDM_D1_MRU0 + i, nEnable);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDEjectUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C���T�[�g���ŁA�C�W�F�N�g�֎~�łȂ���΃C�W�F�N�g�ł���
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�������ݕی� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDWritePUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �������ݕی�ɏ]���`�F�b�N
	pCmdUI->SetCheck(m_pFDD->IsWriteP(nDrive));

	// ���[�h�I�����[���A�C���T�[�g����Ă��Ȃ���Ζ���
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
		return;
	}
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�����C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDForceUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C�W�F�N�g�֎~�̎��̂ݗL��
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[��}�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDInvalidUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �}������Ă��Ȃ����̂ݗL��
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[���f�B�A UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDMediaUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[MRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDMRUUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�I�v�V����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOptions()
{
	Config config;
	CConfigSheet sheet(this);

	// �ݒ�f�[�^���擾
	GetConfig()->GetConfig(&config);

	// �v���p�e�B�V�[�g�����s
	sheet.m_pConfig = &config;
	if (sheet.DoModal() != IDOK) {
		return;
	}

	// �f�[�^�]��
	GetConfig()->SetConfig(&config);

	// �K�p(VM���b�N���čs��)
	::LockVM();
	ApplyCfg();
	GetScheduler()->Reset();
//	ResetCaption();
	::UnlockVM();
}

#endif	// _WIN32
