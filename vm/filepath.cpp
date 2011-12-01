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
#include "vm.h"
#include "filepath.h"
#include "fileio.h"

//===========================================================================
//
//	�t�@�C���p�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::Filepath()
{
	ffb.path = XM6_pid::FiosPath::create();
	Clear();
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::~Filepath()
{
}

const XM6_pid::FiosPath* FASTCALL Filepath::getFiosPath() const {
	return ffb.path;
}

XM6_pid::FiosPath* FASTCALL Filepath::getFiosPath() {
	return ffb.path;
}

void FASTCALL Filepath::set(const Filepath* fp) {
	if(fp) {
		ffb.path->set(fp->ffb.path);
	} else {
		Clear();
	}
}

//---------------------------------------------------------------------------
//
//	�N���A
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Clear()
{
	ASSERT(this);
	ffb.path->clear();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(�V�X�e��)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SysFile(XM6_pid::SysFileType sys)
{
	ASSERT(this);

	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();

	const XM6_pid::FiosPath* pFiosPath = 0;
	if(fios->getSystemFilePath(sys, &pFiosPath)) {
		ffb.path->set(pFiosPath);
	} else {
		ffb.path->clear();
	}
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(���[�U)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetPath(const XM6_pid::FiosPath* pPath) {
	ASSERT(this);

	if(pPath) {
		ffb.path->set(pPath);
	} else {
		Clear();
	}
}

//---------------------------------------------------------------------------
//
//	�V���[�g���擾
//	���Ԃ����|�C���^�͈ꎞ�I�Ȃ��́B�����R�s�[���邱��
//	��FDIDisk��disk.name�Ƃ̊֌W�ŁA������͍ő�59����+�I�[�Ƃ��邱��
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::getName(char* dst, int dstBytes) const {
	return ffb.path->getName(dst, dstBytes);
}

//---------------------------------------------------------------------------
//
//	�p�X��r
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::CmpPath(const Filepath& rhs) const
{
	return ffb.path->cmpPath(rhs.ffb.path);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::Save(Fileio *fio, int) const
{
	ASSERT(this);
	ASSERT(fio);

	const void* srcPtr = 0;
	int srcBytes = 0;
	getFiosPath()->getSaveInfo(srcPtr, srcBytes);
	return fio->Write(srcPtr, srcBytes);
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::Load(Fileio *fio, int)
{
	ASSERT(this);
	ASSERT(fio);

	void* dstPtr = 0;
	int dstBytes = 0;
	getFiosPath()->getLoadInfo(dstPtr, dstBytes);
	return fio->Read(dstPtr, dstBytes);
}
#endif	// WIN32
