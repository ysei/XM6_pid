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

//typedef struct _FILETIME FILETIME;

//===========================================================================
//
//	�t�@�C���p�X
//	��������Z�q��p�ӂ��邱��
//
//===========================================================================
class Filepath
{
public:
	Filepath();													// �R���X�g���N�^
	virtual ~Filepath();										// �f�X�g���N�^
	void FASTCALL Clear();										// �N���A
	void FASTCALL SysFile(XM6_pid::SysFileType sys);			// �t�@�C���ݒ�(�V�X�e��)
	void FASTCALL SetPath(const XM6_pid::FiosPath* fiosPath);	// �t�@�C���ݒ�(���[�U)
	void FASTCALL SetPath(const Filepath& path);				// �t�@�C���ݒ�
	const char* FASTCALL GetShort() const;						// �V���[�g���擾(const char*)
	int FASTCALL CmpPath(const Filepath& path) const;			// �p�X��r
	int FASTCALL Save(Fileio *fio, int ver) const;				// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);					// ���[�h
//	const void* FASTCALL GetPathVoidPtr() const;
	const XM6_pid::FiosPath* FASTCALL getFiosPath() const;
	XM6_pid::FiosPath* FASTCALL getFiosPath();

protected:
	struct FilepathBuf;
//	FilepathBuf* pfb;
//	FilepathBuf* bbb;

	struct FilepathBuf {
		XM6_pid::FiosPath	path;
	};
	FilepathBuf ffb;
};


#endif	// _WIN32
#endif	// filepath_h
