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
//#define	XM6_USE_EVENT_DESC

namespace XM6_pid {
	// システムファイル種別
	enum SysFileType {
		SYS_FILE_TYPE_IPL = 0,		// IPL(version 1.00)
		SYS_FILE_TYPE_IPLXVI,		// IPL(version 1.10)
		SYS_FILE_TYPE_IPLCompact,	// IPL(version 1.20)
		SYS_FILE_TYPE_IPL030,		// IPL(version 1.30)後半
		SYS_FILE_TYPE_ROM030,		// IPL(version 1.30)前半
		SYS_FILE_TYPE_CG,			// CG
		SYS_FILE_TYPE_CGTMP,		// CG(Win合成)
		SYS_FILE_TYPE_SCSIInt,		// SCSI(内蔵)
		SYS_FILE_TYPE_SCSIExt,		// SCSI(外付)
		SYS_FILE_TYPE_SRAM,			// SRAM
		SYS_FILE_TYPE_MAX
	};

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

	class FiosPath {
	protected:
							FiosPath() {}						// Do not use 'new'. see create()
				void		operator=(const FiosPath&);		// Do not use '=' operator. see set()
	public:
		virtual				~FiosPath() {}
		virtual	int			getSaveInfo(const void*& srcPtr, int& srcMaxBytes) const = 0;
		virtual	int			getLoadInfo(void*& dstPtr, int& dstMaxBytes) = 0;
		virtual	void		clear() = 0;
		virtual	void		set(const FiosPath* p) = 0;
		virtual	const char*	getLongPath() const = 0;
		virtual	const char*	getShort() const = 0;
		virtual	int			cmpPath(const FiosPath* p) = 0;

		static	FiosPath*	create();
	};

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

		virtual int			close				(int fd) = 0;

		virtual int			isValid				(int fd) const = 0;
		virtual int			getInvalidFd		() const = 0;

		virtual uint32_t	read				(int fd, void* buffer, unsigned int count) = 0;
		virtual uint32_t	write				(int fd, const void* buffer, unsigned int count) = 0;
		virtual uint32_t	seekSet				(int fd, int offset) = 0;
		virtual uint32_t	filelength			(int fd) = 0;
		virtual uint32_t	tell				(int fd) = 0;
		virtual int			getSystemFilePath	(SysFileType sysFileType, const FiosPath** ppFiosPath) = 0;

		virtual int			open				(const FiosPath* fiosPath, OpenMode mode) = 0;
		virtual int			access				(const FiosPath* filename, int mode) = 0;
	};
} // namespace XM6_pid



//---------------------------------------------------------------------------
//
//	IDマクロ
//
//---------------------------------------------------------------------------
#define XM6_MAKEID(a, b, c, d)	((uint32_t)((a<<24) | (b<<16) | (c<<8) | d))

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
