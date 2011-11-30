//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C��I/O ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"
#include "vm.h"

#if defined(_WIN32)

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Fileio::Fileio()
{
	// ���[�N������
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	handle = fios->getInvalidFd();
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Fileio::~Fileio()
{
	// Release�ł̈��S��
	Close();
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Load(const Filepath& path, void *buffer, int size)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle < 0);

	// �I�[�v��
	if (!Open(path, ReadOnly)) {
		return FALSE;
	}

	// �ǂݍ���
	if (!Read(buffer, size)) {
		Close();
		return FALSE;
	}

	// �N���[�Y
	Close();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Save(const Filepath& path, void *buffer, int size)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle < 0);

	// �I�[�v��
	if (!Open(path, WriteOnly)) {
		return FALSE;
	}

	// �ǂݍ���
	if (!Write(buffer, size)) {
		Close();
		return FALSE;
	}

	// �N���[�Y
	Close();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Open(const Filepath& path, OpenMode mode)
{
	ASSERT(this);

	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();

//	const void* fname = path.GetPath();
//	const void* fname = path.GetPathVoidPtr();
	const XM6_pid::FiosPath* fpath = path.getFiosPath();
	// ���[�h��
	switch (mode) {
	// �ǂݍ��݂̂�
	case ReadOnly:
//		handle = fios->open(fname, XM6_pid::XM6_FILEIO_SYSTEM::ReadOnly);
		handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::ReadOnly);
		break;

	// �������݂̂�
	case WriteOnly:
		handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::WriteOnly);
		break;

	// �ǂݏ�������
	case ReadWrite:
		// CD-ROM����̓ǂݍ��݂�RW���������Ă��܂�
		if (fios->access(fpath, 0x06) == 0) {
			handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::ReadWrite);
		}
		break;

	// �A�y���h
	case Append:
		handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::Append);
		break;

	// ����ȊO
	default:
		handle = fios->getInvalidFd();
		ASSERT(FALSE);
		break;
	}

	// ���ʕ]��
	return fios->isValid(handle);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ���
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Read(void *buffer, int size)
{
	int count;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle >= 0);

	// �ǂݍ���
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	count = fios->read(handle, buffer, size);
	if (count != size) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	��������
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Write(const void *buffer, int size)
{
	int count;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle >= 0);

	// �ǂݍ���
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	count = fios->write(handle, buffer, size);
	if (count != size) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Seek(long offset)
{
	ASSERT(this);
	ASSERT(handle >= 0);
	ASSERT(offset >= 0);

	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	if(fios->seekSet(handle, offset) != (uint32_t) offset) {
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���T�C�Y�擾
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Fileio::GetFileSize() const
{
	ASSERT(this);
	ASSERT(handle >= 0);
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();

	return fios->filelength(handle);
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ʒu�擾
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Fileio::GetFilePos() const
{
	ASSERT(this);
	ASSERT(handle >= 0);

	// �t�@�C���ʒu��32bit�Ŏ擾
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	return fios->tell(handle);
}

//---------------------------------------------------------------------------
//
//	�N���[�Y
//
//---------------------------------------------------------------------------
void FASTCALL Fileio::Close()
{
	ASSERT(this);

	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	if(fios->isValid(handle)) {
		fios->close(handle);
		handle = fios->getInvalidFd();
	}
}
#endif	// _WIN32
