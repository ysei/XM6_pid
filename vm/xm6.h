//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 共通定義 ]
//
//---------------------------------------------------------------------------

#if !defined(xm6_h)
#define xm6_h
#define _CRT_SECURE_NO_WARNINGS	//VC2010//
#pragma warning(disable : 4100)	//VC2010//

//---------------------------------------------------------------------------
//
//	基本定数
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
//	基本マクロ
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
//	基本型定義
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
		ReadOnly,						// 読み込みのみ
		WriteOnly,						// 書き込みのみ
		ReadWrite,						// 読み書き両方
		Append							// アペンド
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
	XM6_SYSTEM_FILEPATH_IPL030,							// IPL(version 1.30)後半
	XM6_SYSTEM_FILEPATH_ROM030,							// IPL(version 1.30)前半
	XM6_SYSTEM_FILEPATH_CG,								// CG
	XM6_SYSTEM_FILEPATH_CGTMP,							// CG(Win合成)
	XM6_SYSTEM_FILEPATH_SCSIInt,						// SCSI(内蔵)
	XM6_SYSTEM_FILEPATH_SCSIExt,						// SCSI(外付)
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
//	IDマクロ
//
//---------------------------------------------------------------------------
#define MAKEID(a, b, c, d)	((uint32_t)((a<<24) | (b<<16) | (c<<8) | d))

//---------------------------------------------------------------------------
//
//	クラス宣言
//
//---------------------------------------------------------------------------
class VM;					// 仮想マシン
class Config;				// コンフィギュレーション
class Device;				// デバイス汎用
class MemDevice;			// メモリマップドデバイス汎用
class Log;					// ログ
class Event;				// イベント
class Scheduler;			// スケジューラ
class CPU;					// CPU MC68000
class Memory;				// アドレス空間 OHM2
class Fileio;				// ファイル入出力
class SRAM;					// スタティックRAM
class SysPort;				// システムポート MESSIAH
class TVRAM;				// テキストVRAM
class VC;					// ビデオコントローラ VIPS&CATHY
class CRTC;					// CRTC VICON
class RTC;					// RTC RP5C15
class PPI;					// PPI i8255A
class DMAC;					// DMAC HD63450
class MFP;					// MFP MC68901
class FDC;					// FDC uPD72065
class FDD;					// FDD FD55GFR
class IOSC;					// I/Oコントローラ IOSC-2
class SASI;					// SASI
class Sync;					// 同期オブジェクト
class OPMIF;				// OPM YM2151
class Keyboard;				// キーボード
class ADPCM;				// ADPCM MSM6258V
class GVRAM;				// グラフィックVRAM
class Sprite;				// スプライトRAM
class SCC;					// SCC Z8530
class Mouse;				// マウス
class Printer;				// プリンタ
class AreaSet;				// エリアセット
class Render;				// レンダラ
class Windrv;				// Windrv
class FDI;					// フロッピーディスクイメージ
class Disk;					// SASI/SCSIディスク
class MIDI;					// MIDI YM3802
class Filepath;				// ファイルパス
class JoyDevice;			// ジョイスティックデバイス
class FileSys;				// ファイルシステム
class SCSI;					// SCSI MB89352
class Mercury;				// Mercury-Unit

VM* getCurrentVm();			// どうしても vm にアクセスしたい人むけ

#endif	// xm6_h
