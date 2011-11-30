//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ファイルI/O ]
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
//	コンストラクタ
//
//---------------------------------------------------------------------------
Fileio::Fileio()
{
	// ワーク初期化
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	handle = fios->getInvalidFd();
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Fileio::~Fileio()
{
	// Releaseでの安全策
	Close();
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Load(const Filepath& path, void *buffer, int size)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle < 0);

	// オープン
	if (!Open(path, ReadOnly)) {
		return FALSE;
	}

	// 読み込み
	if (!Read(buffer, size)) {
		Close();
		return FALSE;
	}

	// クローズ
	Close();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Save(const Filepath& path, void *buffer, int size)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle < 0);

	// オープン
	if (!Open(path, WriteOnly)) {
		return FALSE;
	}

	// 読み込み
	if (!Write(buffer, size)) {
		Close();
		return FALSE;
	}

	// クローズ
	Close();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	オープン
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
	// モード別
	switch (mode) {
	// 読み込みのみ
	case ReadOnly:
//		handle = fios->open(fname, XM6_pid::XM6_FILEIO_SYSTEM::ReadOnly);
		handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::ReadOnly);
		break;

	// 書き込みのみ
	case WriteOnly:
		handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::WriteOnly);
		break;

	// 読み書き両方
	case ReadWrite:
		// CD-ROMからの読み込みはRWが成功してしまう
		if (fios->access(fpath, 0x06) == 0) {
			handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::ReadWrite);
		}
		break;

	// アペンド
	case Append:
		handle = fios->open(fpath, XM6_pid::XM6_FILEIO_SYSTEM::Append);
		break;

	// それ以外
	default:
		handle = fios->getInvalidFd();
		ASSERT(FALSE);
		break;
	}

	// 結果評価
	return fios->isValid(handle);
}

//---------------------------------------------------------------------------
//
//	読み込み
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Read(void *buffer, int size)
{
	int count;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle >= 0);

	// 読み込み
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
//	書き込み
//
//---------------------------------------------------------------------------
int FASTCALL Fileio::Write(const void *buffer, int size)
{
	int count;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle >= 0);

	// 読み込み
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
//	シーク
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
//	ファイルサイズ取得
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
//	ファイル位置取得
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Fileio::GetFilePos() const
{
	ASSERT(this);
	ASSERT(handle >= 0);

	// ファイル位置を32bitで取得
	VM* vm = getCurrentVm();
	XM6_pid::XM6_FILEIO_SYSTEM* fios = vm->GetHostFileSystem();
	return fios->tell(handle);
}

//---------------------------------------------------------------------------
//
//	クローズ
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
