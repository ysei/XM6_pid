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
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#if defined(_WIN32)



#if defined(UNICODE)
#define _T(x)	L x
#else
#define _T(x)	x
#endif



#if defined(UNICODE)
#define _topen      _wopen
#else
#ifdef  _POSIX_
#define _topen      open
#define _taccess    access
#else
#define _topen      _open
#define _taccess    _access
#define _taccess_s  _access_s
#endif
#endif


//===========================================================================
//
//	�t�@�C��I/O
//
//===========================================================================

#if 0
static Vm* vm = 0;
static XM6_FILEIO_SYSTEM* fios = 0;

Fileio::Fileio()
	: handle(0)
{
	if(vm == 0) {
		vm = GetVM();
	}
	if(fios == 0) {
		if(vm) {
			fios = vm->GetHostFileSystem();
		}
	}
	if(fios) {
		fios->setInvalid(&handle);
	}
}

Fileio::~Fileio() {
	Close();
}

int FASTCALL Fileio::Open(const Filepath* path, OpenMode mode) {
	return fios->open(path, mode, &handle);
}


int FASTCALL Fileio::Seek(long offset) {
	return fios->seek(handle, path, offset);
}
									// �V�[�N
int FASTCALL Fileio::Read(void *buffer, int size) {
	return fios->read(handle, buffer, size);
}
									// �ǂݍ���
int FASTCALL Fileio::Write(const void *buffer, int size) {
	return fios->write(handle, buffer, size);
}

uint32_t FASTCALL Fileio::GetFileSize() const {
	uint32_t size = 0;
	fios->getFileSize(handle, buffer, &size);
	return size;
}

uint32_t FASTCALL Fileio::GetFilePos() const {
	uint32_t size = 0;
	fios->getFilePos(handle, buffer, &size);
	return size;
}

void FASTCALL Fileio::Close() {
	fios->close(handle);
	fios->setInvalid(&handle);
}

int FASTCALL Fileio::IsValid() const {
	return fios->isValid(handle);
}
#else
//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Fileio::Fileio()
{
	// ���[�N������
	handle = -1;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Fileio::~Fileio()
{
	ASSERT(handle == -1);

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
int FASTCALL Fileio::Open(LPCTSTR fname, OpenMode mode)
{
	ASSERT(this);
	ASSERT(fname);
	ASSERT(handle < 0);

	// �k�������񂩂�̓ǂݍ��݂͕K�����s������
	if (fname[0] == _T('\0')) {
		handle = -1;
		return FALSE;
	}

	// ���[�h��
	switch (mode) {
		// �ǂݍ��݂̂�
		case ReadOnly:
			handle = _topen(fname, _O_BINARY | _O_RDONLY);
			break;

		// �������݂̂�
		case WriteOnly:
			handle = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC,
						_S_IWRITE);
			break;

		// �ǂݏ�������
		case ReadWrite:
			// CD-ROM����̓ǂݍ��݂�RW���������Ă��܂�
			if (_taccess(fname, 0x06) != 0) {
				return FALSE;
			}
			handle = _topen(fname, _O_BINARY | _O_RDWR);
			break;

		// �A�y���h
		case Append:
			handle = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY | _O_APPEND,
						_S_IWRITE);
			break;

		// ����ȊO
		default:
			ASSERT(FALSE);
			break;
	}

	// ���ʕ]��
	if (handle == -1) {
		return FALSE;
	}
	ASSERT(handle >= 0);
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

	return Open(path.GetPath(), mode);
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
	count = _read(handle, buffer, size);
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
	count = _write(handle, buffer, size);
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

	if (_lseek(handle, offset, SEEK_SET) != offset) {
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
#if defined(_MSC_VER)
	__int64 len;

	ASSERT(this);
	ASSERT(handle >= 0);

	// �t�@�C���T�C�Y��64bit�Ŏ擾
	len = _filelengthi64(handle);

	// ��ʂ�����΁A0xffffffff�Ƃ��ĕԂ�
	if (len >= 0x100000000i64) {
		return 0xffffffff;
	}

	// ���ʂ̂�
	return (uint32_t)len;
#else
	ASSERT(this);
	ASSERT(handle >= 0);

	return (uint32_t)filelength(handle);
#endif	// _MSC_VER
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
	return _tell(handle);
}

//---------------------------------------------------------------------------
//
//	�N���[�Y
//
//---------------------------------------------------------------------------
void FASTCALL Fileio::Close()
{
	ASSERT(this);

	if (handle != -1) {
		_close(handle);
		handle = -1;
	}
}
#endif
#endif	// _WIN32
