//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C���p�X ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"

#include <windows.h>
#include <tchar.h>

#ifndef _T
#if defined(UNICODE)
#define _T(x)	L x
#else
#define _T(x)	x
#endif
#endif

//===========================================================================
//
//	�t�@�C���p�X
//
//===========================================================================
namespace {
//---------------------------------------------------------------------------
//
//	�V�X�e���t�@�C���e�[�u��
//
//---------------------------------------------------------------------------
static LPCTSTR SystemFile[] = {					// �V�X�e���t�@�C��
	_T("IPLROM.DAT"),
	_T("IPLROMXV.DAT"),
	_T("IPLROMCO.DAT"),
	_T("IPLROM30.DAT"),
	_T("ROM30.DAT"),
	_T("CGROM.DAT"),
	_T("CGROM.TMP"),
	_T("SCSIINROM.DAT"),
	_T("SCSIEXROM.DAT"),
	_T("SRAM.DAT")
};

//---------------------------------------------------------------------------
//
//	�V���[�g��
//
//---------------------------------------------------------------------------
static char ShortName[_MAX_FNAME + _MAX_DIR];	// �V���[�g��(char)

//---------------------------------------------------------------------------
//
//	�t�@�C�����{�g���q
//
//---------------------------------------------------------------------------
static TCHAR FileExt[_MAX_FNAME + _MAX_DIR];	// �V���[�g��(TCHAR)

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g��
//
//---------------------------------------------------------------------------
static TCHAR DefaultDir[_MAX_PATH];				// �f�t�H���g�f�B���N�g��
}

struct Filepath::FilepathBuf {
	TCHAR m_szPath[_MAX_PATH];
										// �t�@�C���p�X
	TCHAR m_szDrive[_MAX_DRIVE];
										// �h���C�u
	TCHAR m_szDir[_MAX_DIR];
										// �f�B���N�g��
	TCHAR m_szFile[_MAX_FNAME];
										// �t�@�C��
	TCHAR m_szExt[_MAX_EXT];
										// �g���q
	int m_bUpdate;
										// �Z�[�u��̍X�V����
	FILETIME m_SavedTime;
										// �Z�[�u���̓��t
	FILETIME m_CurrentTime;
										// ���݂̓��t
};

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::Filepath()
	: pfb(0)
{
	pfb = new FilepathBuf;

	// �N���A
	Clear();

	// �X�V�Ȃ�
	pfb->m_bUpdate = FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::~Filepath()
{
	if(pfb) {
		delete pfb;
		pfb = 0;
	}
}

//---------------------------------------------------------------------------
//
//	������Z�q
//
//---------------------------------------------------------------------------
Filepath& Filepath::operator=(const Filepath& path)
{
	// �p�X�ݒ�(������Split�����)
	SetPath(path.GetPath());

	// ���t�y�эX�V�����擾
	pfb->m_bUpdate = FALSE;
	if (path.IsUpdate()) {
		pfb->m_bUpdate = TRUE;
		path.GetUpdateTime(&pfb->m_SavedTime, &pfb->m_CurrentTime);
	}

	return *this;
}

//---------------------------------------------------------------------------
//
//	�N���A
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Clear()
{
	ASSERT(this);

	// �p�X����ъe�������N���A
	pfb->m_szPath[0] = _T('\0');
	pfb->m_szDrive[0] = _T('\0');
	pfb->m_szDir[0] = _T('\0');
	pfb->m_szFile[0] = _T('\0');
	pfb->m_szExt[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(�V�X�e��)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SysFile(SysFileType sys)
{
	int nFile;

	ASSERT(this);

	// �L���X�g
	nFile = (int)sys;

	// �t�@�C�����R�s�[
	_tcscpy(pfb->m_szPath, SystemFile[nFile]);

	// ����
	Split();

	// �x�[�X�f�B���N�g���ݒ�
	SetBaseDir();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(���[�U)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetPath(LPCTSTR lpszPath)
{
	ASSERT(this);
	ASSERT(lpszPath);
	ASSERT(_tcslen(lpszPath) < _MAX_PATH);

	// �p�X���R�s�[
	_tcscpy(pfb->m_szPath, lpszPath);

	// ����
	Split();

	// �h���C�u���̓f�B���N�g���������Ă����OK
	if (_tcslen(pfb->m_szPath) > 0) {
		if (_tcslen(pfb->m_szDrive) == 0) {
			if (_tcslen(pfb->m_szDir) == 0) {
				// �J�����g�f�B���N�g���ݒ�
				SetCurDir();
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	�p�X����
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Split()
{
	ASSERT(this);

	// �p�[�c��������
	pfb->m_szDrive[0] = _T('\0');
	pfb->m_szDir[0] = _T('\0');
	pfb->m_szFile[0] = _T('\0');
	pfb->m_szExt[0] = _T('\0');

	// ����
	_tsplitpath(pfb->m_szPath, pfb->m_szDrive, pfb->m_szDir, pfb->m_szFile, pfb->m_szExt);
}

//---------------------------------------------------------------------------
//
//	�p�X����
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Make()
{
	ASSERT(this);

	// ����
	_tmakepath(pfb->m_szPath, pfb->m_szDrive, pfb->m_szDir, pfb->m_szFile, pfb->m_szExt);
}

//---------------------------------------------------------------------------
//
//	�x�[�X�f�B���N�g���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetBaseDir()
{
	TCHAR szModule[_MAX_PATH];

	ASSERT(this);

	// ���W���[���̃p�X���𓾂�
	::GetModuleFileName(NULL, szModule, _MAX_PATH);

	// ����(�t�@�C�����Ɗg���q�͏������܂Ȃ�)
	_tsplitpath(szModule, pfb->m_szDrive, pfb->m_szDir, NULL, NULL);

	// ����
	Make();
}

//---------------------------------------------------------------------------
//
//	�x�[�X�t�@�C�����ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetBaseFile()
{
	TCHAR szModule[_MAX_PATH];

	ASSERT(this);
	ASSERT(_tcslen(pfb->m_szPath) > 0);

	// ���W���[���̃p�X���𓾂�
	::GetModuleFileName(NULL, szModule, _MAX_PATH);

	// ����(�g���q�͏������܂Ȃ�)
	_tsplitpath(szModule, pfb->m_szDrive, pfb->m_szDir, pfb->m_szFile, NULL);

	// ����
	Make();
}

//---------------------------------------------------------------------------
//
//	�J�����g�f�B���N�g���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetCurDir()
{
	TCHAR szCurDir[_MAX_PATH];

	ASSERT(this);
	ASSERT(_tcslen(pfb->m_szPath) > 0);

	// �J�����g�f�B���N�g���擾
	::GetCurrentDirectory(_MAX_PATH, szCurDir);

	// ����(�t�@�C�����Ɗg���q�͖���)
	_tsplitpath(szCurDir, pfb->m_szDrive, pfb->m_szDir, NULL, NULL);

	// ����
	Make();
}

//---------------------------------------------------------------------------
//
//	�N���A����Ă��邩
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::IsClear() const
{
	// Clear()�̋t
	if ((pfb->m_szPath[0] == _T('\0')) &&
		(pfb->m_szDrive[0] == _T('\0')) &&
		(pfb->m_szDir[0] == _T('\0')) &&
		(pfb->m_szFile[0] == _T('\0')) &&
		(pfb->m_szExt[0] == _T('\0'))) {
		// �m���ɁA�N���A����Ă���
		return TRUE;
	}

	// �N���A����Ă��Ȃ�
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�V���[�g���擾
//	���Ԃ����|�C���^�͈ꎞ�I�Ȃ��́B�����R�s�[���邱��
//	��FDIDisk��disk.name�Ƃ̊֌W�ŁA������͍ő�59����+�I�[�Ƃ��邱��
//
//---------------------------------------------------------------------------
const char* FASTCALL Filepath::GetShort() const
{
	ASSERT(this);

#if 0
	// TCHAR�����񂩂�char������֕ϊ�
	char *lpszFile = T2A((LPTSTR)&pfb->m_szFile[0]);
	char *lpszExt = T2A((LPTSTR)&pfb->m_szExt[0]);

	// �Œ�o�b�t�@�֍���
	strcpy(ShortName, lpszFile);
	strcat(ShortName, lpszExt);
#else
	TCHAR buf[256+1];
	_tcscpy(buf, &pfb->m_szFile[0]);
	_tcscpy(buf, &pfb->m_szExt[0]);

#if !defined(_UNICODE)
	strcpy(ShortName, buf);
#else
#error	not implemented
#endif
#endif
	// strlen�Œ��ׂ��Ƃ��A�ő�59�ɂȂ�悤�ɍ׍H
	ShortName[59] = '\0';

	// const char�Ƃ��ĕԂ�
	return (const char*)ShortName;
}

//---------------------------------------------------------------------------
//
//	�t�@�C�����{�g���q�擾
//	���Ԃ����|�C���^�͈ꎞ�I�Ȃ��́B�����R�s�[���邱��
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetFileExt() const
{
	ASSERT(this);

	// �Œ�o�b�t�@�֍���
	_tcscpy(FileExt, pfb->m_szFile);
	_tcscat(FileExt, pfb->m_szExt);

	// LPCTSTR�Ƃ��ĕԂ�
	return (LPCTSTR)FileExt;
}

//---------------------------------------------------------------------------
//
//	�p�X��r
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::CmpPath(const Filepath& path) const
{
	// �p�X�����S��v���Ă����TRUE
	if (_tcscmp(path.GetPath(), GetPath()) == 0) {
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g��������
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::ClearDefaultDir()
{
	DefaultDir[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetDefaultDir(LPCTSTR lpszPath)
{
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];

	ASSERT(lpszPath);

	// �^����ꂽ�p�X����A�h���C�u�ƃf�B���N�g���𐶐�
	_tsplitpath(lpszPath, szDrive, szDir, NULL, NULL);

	// �h���C�u�ƃf�B���N�g�����R�s�[
	_tcscpy(DefaultDir, szDrive);
	_tcscat(DefaultDir, szDir);
}

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g���擾
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetDefaultDir()
{
	return (LPCTSTR)DefaultDir;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::Save(Fileio *fio, int /*ver*/)
{
	TCHAR szPath[_MAX_PATH];
	FILETIME ft;

	ASSERT(this);
	ASSERT(fio);

	// �[���N���A���āA�S�~�����������̂����
	memset(szPath, 0, sizeof(szPath));
	_tcscpy(szPath, pfb->m_szPath);

	// �t�@�C���p�X��ۑ�
	if (!fio->Write(szPath, sizeof(szPath))) {
		return FALSE;
	}

	// �t�@�C�����t���擾(2038�N��������邽�߁AWin32���擾)
	memset(&ft, 0, sizeof(ft));
#if 0
	CFile file;
	if (file.Open(szPath, CFile::modeRead)) {
		::GetFileTime((HANDLE)file.m_hFile, NULL, NULL, &ft);
		file.Close();
	}
#else
	{
		HANDLE h = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(h != INVALID_HANDLE_VALUE) {
			GetFileTime(h, 0, 0, &ft);
			CloseHandle(h);
		}
	}
#endif
	// �ŏI�������ݓ��t��ۑ�
	if (!fio->Write(&ft, sizeof(ft))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::Load(Fileio *fio, int /*ver*/)
{
	TCHAR szPath[_MAX_PATH];
	ASSERT(this);
	ASSERT(fio);

	// �t���p�X��ǂݍ���
	if (!fio->Read(szPath, sizeof(szPath))) {
		return FALSE;
	}

	// �Z�b�g
	SetPath(szPath);

	// �ŏI�������ݓ��t��ǂݍ���
	if (!fio->Read(&pfb->m_SavedTime, sizeof(pfb->m_SavedTime))) {
		return FALSE;
	}

	// �t�@�C�����t���擾(2038�N��������邽�߁AWin32���擾)
#if 0
	CFile file;
	if (!file.Open(szPath, CFile::modeRead)) {
		// �t�@�C�������݂��Ȃ��Ă��A�G���[�Ƃ͂��Ȃ�
		return TRUE;
	}
	if (!::GetFileTime((HANDLE)file.m_hFile, NULL, NULL, &pfb->m_CurrentTime)) {
		return FALSE;
	}
	file.Close();
#else
	{
		HANDLE h = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(h == INVALID_HANDLE_VALUE) {
			return TRUE;
		} else if(! GetFileTime(h, 0, 0, &pfb->m_CurrentTime)) {
			CloseHandle(h);
			return FALSE;
		}
		CloseHandle(h);
	}
#endif
	// ft�̕����V���������ꍇ�A�X�V�t���OUp
	if (::CompareFileTime(&pfb->m_CurrentTime, &pfb->m_SavedTime) <= 0) {
		pfb->m_bUpdate = FALSE;
	}
	else {
		pfb->m_bUpdate = TRUE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u��ɍX�V���ꂽ��
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::IsUpdate() const
{
	ASSERT(this);

	return pfb->m_bUpdate;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u���ԏ����擾
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::GetUpdateTime(FILETIME *pSaved, FILETIME *pCurrent) const
{
	ASSERT(this);
	ASSERT(pfb->m_bUpdate);

	// ���ԏ���n��
	*pSaved = pfb->m_SavedTime;
	*pCurrent = pfb->m_CurrentTime;
}
/*
//---------------------------------------------------------------------------
//
//	�V�X�e���t�@�C���e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR Filepath::SystemFile[] = {
	_T("IPLROM.DAT"),
	_T("IPLROMXV.DAT"),
	_T("IPLROMCO.DAT"),
	_T("IPLROM30.DAT"),
	_T("ROM30.DAT"),
	_T("CGROM.DAT"),
	_T("CGROM.TMP"),
	_T("SCSIINROM.DAT"),
	_T("SCSIEXROM.DAT"),
	_T("SRAM.DAT")
};

//---------------------------------------------------------------------------
//
//	�V���[�g��
//
//---------------------------------------------------------------------------
char Filepath::ShortName[_MAX_FNAME + _MAX_DIR];

//---------------------------------------------------------------------------
//
//	�t�@�C�����{�g���q
//
//---------------------------------------------------------------------------
TCHAR Filepath::FileExt[_MAX_FNAME + _MAX_DIR];

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g��
//
//---------------------------------------------------------------------------
TCHAR Filepath::DefaultDir[_MAX_PATH];
*/

//---------------------------------------------------------------------------
//
// �p�X���擾
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetPath() const {
	return pfb->m_szPath;
}

#endif	// WIN32
