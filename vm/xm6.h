//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ���ʒ�` ]
//
//---------------------------------------------------------------------------

#if !defined(xm6_h)
#define xm6_h
#define _CRT_SECURE_NO_WARNINGS	//VC2010//
#pragma warning(disable : 4100)	//VC2010//

//---------------------------------------------------------------------------
//
//	��{�萔
//
//---------------------------------------------------------------------------
#if !defined(FALSE)
#define FALSE		0
#define TRUE		1
#endif	// FALSE
#if !defined(NULL)
#define NULL		0
#endif	// NULL

//---------------------------------------------------------------------------
//
//	��{�}�N��
//
//---------------------------------------------------------------------------
#if !defined(ASSERT)
#if !defined(NDEBUG)
#define ASSERT(cond)	assert(cond)
#else
#define ASSERT(cond)	((void)0)
#endif	// NDEBUG
#endif	// ASSERT

#if !defined(ASSERT_DIAG)
#if !defined(NDEBUG)
#define ASSERT_DIAG()	AssertDiag()
#else
#define ASSERT_DIAG()	((void)0)
#endif	// NDEBUG
#endif	// ASSERT_DIAG

//---------------------------------------------------------------------------
//
//	��{�^��`
//
//---------------------------------------------------------------------------
//	typedef uint8_t uint8_t;
//	typedef uint16_t uint16_t;
//	typedef uint32_t uint32_t;
//	typedef int int;

//#define	XM6_USE_EVENT_DESC

struct XM6_RTC {
	int32_t	sec;		// [0,61]
	int32_t	min;		// [0,59]
	int32_t	hour;		// [0,23]
	int32_t	mday;		// [1,31]
	int32_t	mon;		// [0,11]
	int32_t	year;		// [0,119]	years since 1900
	int32_t	wday;		// [0,6]
};

typedef int (__stdcall *XM6_RTC_CALLBACK)(XM6_RTC*);

class XM6_FILEIO_SYSTEM {
public:
	enum OpenMode {
		ReadOnly,						// �ǂݍ��݂̂�
		WriteOnly,						// �������݂̂�
		ReadWrite,						// �ǂݏ�������
		Append							// �A�y���h
	};

	XM6_FILEIO_SYSTEM() {}
	virtual ~XM6_FILEIO_SYSTEM() {}

	virtual int open			(const void* filename, OpenMode mode) = 0;
	virtual int close			(int fd) = 0;

	virtual int isValid			(int fd) const = 0;
	virtual int getInvalidFd	() const = 0;

	virtual uint32_t read		(int fd, void* buffer, unsigned int count) = 0;
	virtual uint32_t write		(int fd, const void* buffer, unsigned int count) = 0;
	virtual uint32_t seekSet	(int fd, int offset) = 0;
	virtual uint32_t filelength	(int fd) = 0;
	virtual uint32_t tell		(int fd) = 0;
	virtual int		 access		(const void* path, int mode) = 0;
};

/*
enum OpenMode {
	OPEN_MODE_READ_ONLY = 0,
	OPEN_MODE_WRITE_ONLY,
	OPEN_MODE_READ_WRITE,
	OPEN_MODE_APPEND,
	OPEN_MODE_MAX,
};

typedef enum XM6_SYSTEM_FILEPATH_TYPE {
	XM6_SYSTEM_FILEPATH_IPL,							// IPL(version 1.00)
	XM6_SYSTEM_FILEPATH_IPLXVI,							// IPL(version 1.10)
	XM6_SYSTEM_FILEPATH_IPLCompact,						// IPL(version 1.20)
	XM6_SYSTEM_FILEPATH_IPL030,							// IPL(version 1.30)�㔼
	XM6_SYSTEM_FILEPATH_ROM030,							// IPL(version 1.30)�O��
	XM6_SYSTEM_FILEPATH_CG,								// CG
	XM6_SYSTEM_FILEPATH_CGTMP,							// CG(Win����)
	XM6_SYSTEM_FILEPATH_SCSIInt,						// SCSI(����)
	XM6_SYSTEM_FILEPATH_SCSIExt,						// SCSI(�O�t)
	XM6_SYSTEM_FILEPATH_SRAM							// SRAM
} XM6_SYSTEM_FILEPATH_TYPE;

typedef int XM6_FILE_HANDLE;

class Filepath;

class XM6_FILEIO_SYSTEM {
public:
	XM6_FILEIO_SYSTEM() {}
	virtual ~XM6_FILEIO_SYSTEM() = 0;

	virtual int open		(const Filepath* path, OpenMode openMode, XM6_FILE_HANDLE* out) = 0;
	virtual int	close		(XM6_FILE_HANDLE fh) = 0;
	virtual int	seek		(XM6_FILE_HANDLE fh, uint32_t offset) = 0;
	virtual int	read		(XM6_FILE_HANDLE fh, void* buffer, uint32_t size) = 0;
	virtual int	write		(XM6_FILE_HANDLE fh, const void* buffer, uint32_t size) = 0;
	virtual int	getFileSize	(XM6_FILE_HANDLE fh, uint32_t* size) = 0;
	virtual int	getFilePos	(XM6_FILE_HANDLE fh, uint32_t* pos) = 0;
	virtual int	isValid		(XM6_FILE_HANDLE fh) = 0;
	virtual int setInvalid	(XM6_FILE_HANDLE* out) = 0;
};

const Filepath* getSystemFilepath(XM6_SYSTEM_FILEPATH_TYPE sft);
*/



//---------------------------------------------------------------------------
//
//	ID�}�N��
//
//---------------------------------------------------------------------------
#define MAKEID(a, b, c, d)	((uint32_t)((a<<24) | (b<<16) | (c<<8) | d))

//---------------------------------------------------------------------------
//
//	�N���X�錾
//
//---------------------------------------------------------------------------
class VM;					// ���z�}�V��
class Config;				// �R���t�B�M�����[�V����
class Device;				// �f�o�C�X�ėp
class MemDevice;			// �������}�b�v�h�f�o�C�X�ėp
class Log;					// ���O
class Event;				// �C�x���g
class Scheduler;			// �X�P�W���[��
class CPU;					// CPU MC68000
class Memory;				// �A�h���X��� OHM2
class Fileio;				// �t�@�C�����o��
class SRAM;					// �X�^�e�B�b�NRAM
class SysPort;				// �V�X�e���|�[�g MESSIAH
class TVRAM;				// �e�L�X�gVRAM
class VC;					// �r�f�I�R���g���[�� VIPS&CATHY
class CRTC;					// CRTC VICON
class RTC;					// RTC RP5C15
class PPI;					// PPI i8255A
class DMAC;					// DMAC HD63450
class MFP;					// MFP MC68901
class FDC;					// FDC uPD72065
class FDD;					// FDD FD55GFR
class IOSC;					// I/O�R���g���[�� IOSC-2
class SASI;					// SASI
class Sync;					// �����I�u�W�F�N�g
class OPMIF;				// OPM YM2151
class Keyboard;				// �L�[�{�[�h
class ADPCM;				// ADPCM MSM6258V
class GVRAM;				// �O���t�B�b�NVRAM
class Sprite;				// �X�v���C�gRAM
class SCC;					// SCC Z8530
class Mouse;				// �}�E�X
class Printer;				// �v�����^
class AreaSet;				// �G���A�Z�b�g
class Render;				// �����_��
class Windrv;				// Windrv
class FDI;					// �t���b�s�[�f�B�X�N�C���[�W
class Disk;					// SASI/SCSI�f�B�X�N
class MIDI;					// MIDI YM3802
class Filepath;				// �t�@�C���p�X
class JoyDevice;			// �W���C�X�e�B�b�N�f�o�C�X
class FileSys;				// �t�@�C���V�X�e��
class SCSI;					// SCSI MB89352
class Mercury;				// Mercury-Unit

VM* getCurrentVm();			// �ǂ����Ă� vm �ɃA�N�Z�X�������l�ނ�

#endif	// xm6_h
