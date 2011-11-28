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
//	ファイルI/O
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
									// シーク
int FASTCALL Fileio::Read(void *buffer, int size) {
	return fios->read(handle, buffer, size);
}
									// 読み込み
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
//	コンストラクタ
//
//---------------------------------------------------------------------------
Fileio::Fileio()
{
	// ワーク初期化
	handle = -1;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Fileio::~Fileio()
{
	ASSERT(handle == -1);

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
int FASTCALL Fileio::Open(LPCTSTR fname, OpenMode mode)
{
	ASSERT(this);
	ASSERT(fname);
	ASSERT(handle < 0);

	// ヌル文字列からの読み込みは必ず失敗させる
	if (fname[0] == _T('\0')) {
		handle = -1;
		return FALSE;
	}

	// モード別
	switch (mode) {
		// 読み込みのみ
		case ReadOnly:
			handle = _topen(fname, _O_BINARY | _O_RDONLY);
			break;

		// 書き込みのみ
		case WriteOnly:
			handle = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC,
						_S_IWRITE);
			break;

		// 読み書き両方
		case ReadWrite:
			// CD-ROMからの読み込みはRWが成功してしまう
			if (_taccess(fname, 0x06) != 0) {
				return FALSE;
			}
			handle = _topen(fname, _O_BINARY | _O_RDWR);
			break;

		// アペンド
		case Append:
			handle = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY | _O_APPEND,
						_S_IWRITE);
			break;

		// それ以外
		default:
			ASSERT(FALSE);
			break;
	}

	// 結果評価
	if (handle == -1) {
		return FALSE;
	}
	ASSERT(handle >= 0);
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

	return Open(path.GetPath(), mode);
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
	count = _read(handle, buffer, size);
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
	count = _write(handle, buffer, size);
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

	if (_lseek(handle, offset, SEEK_SET) != offset) {
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
#if defined(_MSC_VER)
	__int64 len;

	ASSERT(this);
	ASSERT(handle >= 0);

	// ファイルサイズを64bitで取得
	len = _filelengthi64(handle);

	// 上位があれば、0xffffffffとして返す
	if (len >= 0x100000000i64) {
		return 0xffffffff;
	}

	// 下位のみ
	return (uint32_t)len;
#else
	ASSERT(this);
	ASSERT(handle >= 0);

	return (uint32_t)filelength(handle);
#endif	// _MSC_VER
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
	return _tell(handle);
}

//---------------------------------------------------------------------------
//
//	クローズ
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
