//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C��I/O ]
//
//---------------------------------------------------------------------------

#if !defined(fileio_h)
#define fileio_h
#include "filepath.h"

//===========================================================================
//
//	�t�@�C��I/O
//
//===========================================================================
class Fileio
{
public:
	enum OpenMode {
		ReadOnly,						// �ǂݍ��݂̂�
		WriteOnly,						// �������݂̂�
		ReadWrite,						// �ǂݏ�������
		Append							// �A�y���h
	};

public:
	Fileio();
										// �R���X�g���N�^
	virtual ~Fileio();
										// �f�X�g���N�^
	int FASTCALL Load(const Filepath& path, void *buffer, int size);
										// ROM,RAM���[�h
	int FASTCALL Save(const Filepath& path, void *buffer, int size);
										// RAM�Z�[�u

#if defined(_WIN32)
//	int FASTCALL Open(LPCTSTR fname, OpenMode mode);
//										// �I�[�v��
#endif	// _WIN32
	int FASTCALL Open(const Filepath& path, OpenMode mode);
										// �I�[�v��
	int FASTCALL Seek(long offset);
										// �V�[�N
	int FASTCALL Read(void *buffer, int size);
										// �ǂݍ���
	int FASTCALL Write(const void *buffer, int size);
										// ��������
	uint32_t FASTCALL GetFileSize() const;
										// �t�@�C���T�C�Y�擾
	uint32_t FASTCALL GetFilePos() const;
										// �t�@�C���ʒu�擾
	void FASTCALL Close();
										// �N���[�Y
	int FASTCALL IsValid() const		{ return (int)(handle != -1); }
										// �L���`�F�b�N
	int FASTCALL GetHandle() const		{ return handle; }
										// �n���h���擾

private:
	int handle;							// �t�@�C���n���h��
};

#endif	// fileio_h
