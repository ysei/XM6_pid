//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C���p�X ]
//
//---------------------------------------------------------------------------

#if !defined(filepath_h)
#define filepath_h

class Filepath;

#if defined(_WIN32)

//---------------------------------------------------------------------------
//
//	�萔��`
//
//---------------------------------------------------------------------------
#ifndef _MAX_PATH
#define	_MAX_PATH			260
#define	_MAX_DRIVE			3
#define	_MAX_EXT			256
#define	_MAX_FNAME			256
#define	_MAX_DIR			256
#endif
#define FILEPATH_MAX		_MAX_PATH

typedef char CHAR;
typedef const CHAR* LPCSTR;

#if defined(UNICODE)
typedef wchar_t WCHAR;
typedef WCHAR TCHAR;
typedef LPCWSTR LPCTSTR;
#else
typedef char TCHAR;
typedef LPCSTR LPCTSTR;
#endif

typedef struct _FILETIME FILETIME;

/*
class TCharBuf {
public:
	TCharBuf() : buf(0) {
	}

	TCharBuf(int n) : buf(0) {
		buf = new TCHAR [n];
	}

	~TCharBuf() {
		if(buf) {
			delete [] buf;
			buf = 0;
		}
	}

	operator const char*() const {
		return &buf[0];
	}

	operator const char*() {
		return &buf[0];
	}

	const char& operator[](const int i) const {
		return buf[i];
	}

	char& operator[](const int i) {
		return buf[i];
	}

protected:
	TCHAR* buf;
};
*/



//===========================================================================
//
//	�t�@�C���p�X
//	��������Z�q��p�ӂ��邱��
//
//===========================================================================
class Filepath
{
public:
	// �V�X�e���t�@�C�����
	enum SysFileType {
		IPL,							// IPL(version 1.00)
		IPLXVI,							// IPL(version 1.10)
		IPLCompact,						// IPL(version 1.20)
		IPL030,							// IPL(version 1.30)�㔼
		ROM030,							// IPL(version 1.30)�O��
		CG,								// CG
		CGTMP,							// CG(Win����)
		SCSIInt,						// SCSI(����)
		SCSIExt,						// SCSI(�O�t)
		SRAM							// SRAM
	};

public:
	Filepath();
										// �R���X�g���N�^
	virtual ~Filepath();
										// �f�X�g���N�^
	Filepath& operator=(const Filepath& path);
										// ���

	void FASTCALL Clear();
										// �N���A
	void FASTCALL SysFile(SysFileType sys);
										// �t�@�C���ݒ�(�V�X�e��)
	void FASTCALL SetPath(LPCTSTR lpszPath);
										// �t�@�C���ݒ�(���[�U)
	void FASTCALL SetBaseDir();
										// �x�[�X�f�B���N�g���ݒ�
	void FASTCALL SetBaseFile();
										// �x�[�X�f�B���N�g���{�t�@�C�����ݒ�

	int FASTCALL IsClear() const;
										// �N���A����Ă��邩
	LPCTSTR FASTCALL GetPath() const;
										// �p�X���擾
	const char* FASTCALL GetShort() const;
										// �V���[�g���擾(const char*)
	LPCTSTR FASTCALL GetFileExt() const;
										// �V���[�g���擾(LPCTSTR)
	int FASTCALL CmpPath(const Filepath& path) const;
										// �p�X��r

	static void FASTCALL ClearDefaultDir();
										// �f�t�H���f�B���N�g����������
	static void FASTCALL SetDefaultDir(LPCTSTR lpszPath);
										// �f�t�H���g�f�B���N�g���ɐݒ�
	static LPCTSTR FASTCALL GetDefaultDir();
										// �f�t�H���g�f�B���N�g���擾

	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

private:
	void FASTCALL Split();
										// �p�X����
	void FASTCALL Make();
										// �p�X����
	void FASTCALL SetCurDir();
										// �J�����g�f�B���N�g���ݒ�
	int FASTCALL IsUpdate() const;
										// �Z�[�u��̍X�V���肩
	void FASTCALL GetUpdateTime(FILETIME *pSaved, FILETIME *pCurrent ) const;
										// �Z�[�u��̎��ԏ����擾

	struct FilepathBuf;
	FilepathBuf* pfb;
};


#endif	// _WIN32
#endif	// filepath_h
