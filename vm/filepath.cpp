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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#ifndef _T
#if defined(UNICODE)
#define _T(x)	L x
#else
#define _T(x)	x
#endif
#endif

static bool chk(const void* p) {
	int nbu = * (const int*) (((char*)p) - 32 + 0x14);
	return nbu >= 0;
}

#define	TR()	

/*
#define	TR()	\
		{		\
			char buf[256];	\
			sprintf(buf, __FUNCTION__" : this=%p, pfb=%p, bbb=%p\n", this, pfb, bbb);	\
			OutputDebugString(buf);	\
		}
*/

//===========================================================================
//
//	�t�@�C���p�X
//
//===========================================================================
/*
struct Filepath::FilepathBuf {
	XM6_pid::FiosPath	path;
};
*/

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::Filepath()
//	: pfb(0)
//	, bbb(0)
{
//	pfb = new FilepathBuf;
//	pfb->path.path[0] = 0;
//	bbb = pfb;
//	{
//		char buf[256];
//		sprintf(buf, "Filepath() : this=%p, pfb=%p, bbb=%p\n", this, pfb, bbb);
//		OutputDebugString(buf);
//	}

	// �N���A
	Clear();

//	ASSERT(chk(pfb) && pfb == bbb);
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::~Filepath()
{
	ASSERT(* (const unsigned int*) this != 0xfeeefeee);
//	ASSERT(pfb != 0);
//	ASSERT(pfb == bbb);

//	if(pfb) {
//		unsigned int nbu = * (const unsigned int*) (((char*)pfb) - 32 + 0x14);
//		{
//			char buf[256];
//			sprintf(buf, "~Filepath() : nBlockUse=%08x\n", nbu);
//			OutputDebugString(buf);
//		}
//		TR();
//		delete pfb;
//		pfb = 0;
//		bbb = 0;
//	}
}

void FASTCALL Filepath::SetPath(const Filepath& path) {
//	_tcscpy(pfb->path.path, path.pfb->path.path);
	_tcscpy(ffb.path.path, path.ffb.path.path);
}

const XM6_pid::FiosPath* FASTCALL Filepath::getFiosPath() const {
	return &ffb.path;
}

XM6_pid::FiosPath* FASTCALL Filepath::getFiosPath() {
	return &ffb.path;
}

//---------------------------------------------------------------------------
//
//	�N���A
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Clear()
{
	ASSERT(this);

//	ASSERT(chk(pfb) && pfb == bbb);
	// �p�X����ъe�������N���A
//	ffb.path.path[0] = _T('\0');
	ffb.path.clear();
//	ASSERT(chk(pfb) && pfb == bbb);
	TR();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(�V�X�e��)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SysFile(XM6_pid::SysFileType sys)
{
	ASSERT(this);
//	ASSERT(chk(pfb) && pfb == bbb);

	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	const XM6_pid::FiosPath* pFiosPath = 0;
	if(fios->getSystemFilePath(sys, &pFiosPath)) {
		ffb.path.set(pFiosPath);
	} else {
		ffb.path.clear();
	}
//	ASSERT(chk(pfb) && pfb == bbb);
	TR();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(���[�U)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetPath(const XM6_pid::FiosPath* pPath) {
	ASSERT(this);
	ASSERT(pPath);
//	ASSERT(chk(pfb) && pfb == bbb);

	// �p�X���R�s�[
	ffb.path.set(pPath);
//	ASSERT(chk(pfb) && pfb == bbb);
	TR();
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
//	ASSERT(chk(pfb) && pfb == bbb);

#if 1
//	const TCHAR* p = &pfb->m_szPath[0];
	const TCHAR* p = &ffb.path.path[0];
	const TCHAR* ret = 0;
	while(*p != 0) {
		if(*p == _T('\\')) {
			ret = p;
		}
		p += 1;
	}

	if(ret == 0) {
//		ret = &pfb->m_szPath[0];
		ret = &ffb.path.path[0];
	} else {
		ret = ret + 1;
	}

//	ASSERT(chk(pfb) && pfb == bbb);
	OutputDebugString(ret);
	TR();
	return ret;
#else
	TR();
	return ffb.path.getShort();
#endif
}

//---------------------------------------------------------------------------
//
//	�p�X��r
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::CmpPath(const Filepath& path) const
{
//	ASSERT(chk(pfb) && pfb == bbb);
	// �p�X�����S��v���Ă����TRUE
//	if (_tcscmp(path.pfb->m_szPath, pfb->m_szPath) == 0) {
	if (_tcscmp(path.ffb.path.path, ffb.path.path) == 0) {
//	ASSERT(chk(pfb) && pfb == bbb);
		TR();
		return TRUE;
	}
//	ASSERT(chk(pfb) && pfb == bbb);
	TR();
	return FALSE;
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
//	ASSERT(chk(pfb) && pfb == bbb);

	// �t�@�C���p�X��ۑ�
	const void* srcPtr = 0;
	int srcBytes = 0;
	getFiosPath()->getSaveInfo(srcPtr, srcBytes);

	if(!fio->Write(srcPtr, srcBytes)) {
//	ASSERT(chk(pfb) && pfb == bbb);
		TR();
		return FALSE;
	}

//	ASSERT(chk(pfb) && pfb == bbb);
	TR();
	return TRUE;
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
//	ASSERT(chk(pfb) && pfb == bbb);

	// �t���p�X��ǂݍ���
	void* dstPtr = 0;
	int dstBytes = 0;
	getFiosPath()->getLoadInfo(dstPtr, dstBytes);

	if(!fio->Write(dstPtr, dstBytes)) {
//	ASSERT(chk(pfb) && pfb == bbb);
		TR();
		return FALSE;
	}

//	ASSERT(chk(pfb) && pfb == bbb);
	TR();
	return TRUE;
}
#endif	// WIN32
