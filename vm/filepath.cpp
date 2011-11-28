//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ファイルパス ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"

#include <windows.h>
#include <tchar.h>

#ifndef _T
#if defined(UNICODE)
#define _T(x)	L x
#else
#define _T(x)	x
#endif
#endif

//===========================================================================
//
//	ファイルパス
//
//===========================================================================
namespace {
//---------------------------------------------------------------------------
//
//	システムファイルテーブル
//
//---------------------------------------------------------------------------
static LPCTSTR SystemFile[] = {					// システムファイル
	_T("IPLROM.DAT"),
	_T("IPLROMXV.DAT"),
	_T("IPLROMCO.DAT"),
	_T("IPLROM30.DAT"),
	_T("ROM30.DAT"),
	_T("CGROM.DAT"),
	_T("CGROM.TMP"),
	_T("SCSIINROM.DAT"),
	_T("SCSIEXROM.DAT"),
	_T("SRAM.DAT")
};

//---------------------------------------------------------------------------
//
//	ショート名
//
//---------------------------------------------------------------------------
static char ShortName[_MAX_FNAME + _MAX_DIR];	// ショート名(char)

//---------------------------------------------------------------------------
//
//	ファイル名＋拡張子
//
//---------------------------------------------------------------------------
static TCHAR FileExt[_MAX_FNAME + _MAX_DIR];	// ショート名(TCHAR)

//---------------------------------------------------------------------------
//
//	デフォルトディレクトリ
//
//---------------------------------------------------------------------------
static TCHAR DefaultDir[_MAX_PATH];				// デフォルトディレクトリ
}

struct Filepath::FilepathBuf {
	TCHAR m_szPath[_MAX_PATH];
										// ファイルパス
	TCHAR m_szDrive[_MAX_DRIVE];
										// ドライブ
	TCHAR m_szDir[_MAX_DIR];
										// ディレクトリ
	TCHAR m_szFile[_MAX_FNAME];
										// ファイル
	TCHAR m_szExt[_MAX_EXT];
										// 拡張子
	int m_bUpdate;
										// セーブ後の更新あり
	FILETIME m_SavedTime;
										// セーブ時の日付
	FILETIME m_CurrentTime;
										// 現在の日付
};

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Filepath::Filepath()
	: pfb(0)
{
	pfb = new FilepathBuf;

	// クリア
	Clear();

	// 更新なし
	pfb->m_bUpdate = FALSE;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Filepath::~Filepath()
{
	if(pfb) {
		delete pfb;
		pfb = 0;
	}
}

//---------------------------------------------------------------------------
//
//	代入演算子
//
//---------------------------------------------------------------------------
Filepath& Filepath::operator=(const Filepath& path)
{
	// パス設定(内部でSplitされる)
	SetPath(path.GetPath());

	// 日付及び更新情報を取得
	pfb->m_bUpdate = FALSE;
	if (path.IsUpdate()) {
		pfb->m_bUpdate = TRUE;
		path.GetUpdateTime(&pfb->m_SavedTime, &pfb->m_CurrentTime);
	}

	return *this;
}

//---------------------------------------------------------------------------
//
//	クリア
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Clear()
{
	ASSERT(this);

	// パスおよび各部分をクリア
	pfb->m_szPath[0] = _T('\0');
	pfb->m_szDrive[0] = _T('\0');
	pfb->m_szDir[0] = _T('\0');
	pfb->m_szFile[0] = _T('\0');
	pfb->m_szExt[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	ファイル設定(システム)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SysFile(SysFileType sys)
{
	int nFile;

	ASSERT(this);

	// キャスト
	nFile = (int)sys;

	// ファイル名コピー
	_tcscpy(pfb->m_szPath, SystemFile[nFile]);

	// 分離
	Split();

	// ベースディレクトリ設定
	SetBaseDir();
}

//---------------------------------------------------------------------------
//
//	ファイル設定(ユーザ)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetPath(LPCTSTR lpszPath)
{
	ASSERT(this);
	ASSERT(lpszPath);
	ASSERT(_tcslen(lpszPath) < _MAX_PATH);

	// パス名コピー
	_tcscpy(pfb->m_szPath, lpszPath);

	// 分離
	Split();

	// ドライブ又はディレクトリが入っていればOK
	if (_tcslen(pfb->m_szPath) > 0) {
		if (_tcslen(pfb->m_szDrive) == 0) {
			if (_tcslen(pfb->m_szDir) == 0) {
				// カレントディレクトリ設定
				SetCurDir();
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	パス分離
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Split()
{
	ASSERT(this);

	// パーツを初期化
	pfb->m_szDrive[0] = _T('\0');
	pfb->m_szDir[0] = _T('\0');
	pfb->m_szFile[0] = _T('\0');
	pfb->m_szExt[0] = _T('\0');

	// 分離
	_tsplitpath(pfb->m_szPath, pfb->m_szDrive, pfb->m_szDir, pfb->m_szFile, pfb->m_szExt);
}

//---------------------------------------------------------------------------
//
//	パス合成
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Make()
{
	ASSERT(this);

	// 合成
	_tmakepath(pfb->m_szPath, pfb->m_szDrive, pfb->m_szDir, pfb->m_szFile, pfb->m_szExt);
}

//---------------------------------------------------------------------------
//
//	ベースディレクトリ設定
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetBaseDir()
{
	TCHAR szModule[_MAX_PATH];

	ASSERT(this);

	// モジュールのパス名を得る
	::GetModuleFileName(NULL, szModule, _MAX_PATH);

	// 分離(ファイル名と拡張子は書き込まない)
	_tsplitpath(szModule, pfb->m_szDrive, pfb->m_szDir, NULL, NULL);

	// 合成
	Make();
}

//---------------------------------------------------------------------------
//
//	ベースファイル名設定
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetBaseFile()
{
	TCHAR szModule[_MAX_PATH];

	ASSERT(this);
	ASSERT(_tcslen(pfb->m_szPath) > 0);

	// モジュールのパス名を得る
	::GetModuleFileName(NULL, szModule, _MAX_PATH);

	// 分離(拡張子は書き込まない)
	_tsplitpath(szModule, pfb->m_szDrive, pfb->m_szDir, pfb->m_szFile, NULL);

	// 合成
	Make();
}

//---------------------------------------------------------------------------
//
//	カレントディレクトリ設定
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetCurDir()
{
	TCHAR szCurDir[_MAX_PATH];

	ASSERT(this);
	ASSERT(_tcslen(pfb->m_szPath) > 0);

	// カレントディレクトリ取得
	::GetCurrentDirectory(_MAX_PATH, szCurDir);

	// 分離(ファイル名と拡張子は無し)
	_tsplitpath(szCurDir, pfb->m_szDrive, pfb->m_szDir, NULL, NULL);

	// 合成
	Make();
}

//---------------------------------------------------------------------------
//
//	クリアされているか
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::IsClear() const
{
	// Clear()の逆
	if ((pfb->m_szPath[0] == _T('\0')) &&
		(pfb->m_szDrive[0] == _T('\0')) &&
		(pfb->m_szDir[0] == _T('\0')) &&
		(pfb->m_szFile[0] == _T('\0')) &&
		(pfb->m_szExt[0] == _T('\0'))) {
		// 確かに、クリアされている
		return TRUE;
	}

	// クリアされていない
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	ショート名取得
//	※返されるポインタは一時的なもの。すぐコピーすること
//	※FDIDiskのdisk.nameとの関係で、文字列は最大59文字+終端とすること
//
//---------------------------------------------------------------------------
const char* FASTCALL Filepath::GetShort() const
{
	ASSERT(this);

#if 0
	// TCHAR文字列からchar文字列へ変換
	char *lpszFile = T2A((LPTSTR)&pfb->m_szFile[0]);
	char *lpszExt = T2A((LPTSTR)&pfb->m_szExt[0]);

	// 固定バッファへ合成
	strcpy(ShortName, lpszFile);
	strcat(ShortName, lpszExt);
#else
	TCHAR buf[256+1];
	_tcscpy(buf, &pfb->m_szFile[0]);
	_tcscpy(buf, &pfb->m_szExt[0]);

#if !defined(_UNICODE)
	strcpy(ShortName, buf);
#else
#error	not implemented
#endif
#endif
	// strlenで調べたとき、最大59になるように細工
	ShortName[59] = '\0';

	// const charとして返す
	return (const char*)ShortName;
}

//---------------------------------------------------------------------------
//
//	ファイル名＋拡張子取得
//	※返されるポインタは一時的なもの。すぐコピーすること
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetFileExt() const
{
	ASSERT(this);

	// 固定バッファへ合成
	_tcscpy(FileExt, pfb->m_szFile);
	_tcscat(FileExt, pfb->m_szExt);

	// LPCTSTRとして返す
	return (LPCTSTR)FileExt;
}

//---------------------------------------------------------------------------
//
//	パス比較
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::CmpPath(const Filepath& path) const
{
	// パスが完全一致していればTRUE
	if (_tcscmp(path.GetPath(), GetPath()) == 0) {
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	デフォルトディレクトリ初期化
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::ClearDefaultDir()
{
	DefaultDir[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	デフォルトディレクトリ設定
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetDefaultDir(LPCTSTR lpszPath)
{
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];

	ASSERT(lpszPath);

	// 与えられたパスから、ドライブとディレクトリを生成
	_tsplitpath(lpszPath, szDrive, szDir, NULL, NULL);

	// ドライブとディレクトリをコピー
	_tcscpy(DefaultDir, szDrive);
	_tcscat(DefaultDir, szDir);
}

//---------------------------------------------------------------------------
//
//	デフォルトディレクトリ取得
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetDefaultDir()
{
	return (LPCTSTR)DefaultDir;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::Save(Fileio *fio, int /*ver*/)
{
	TCHAR szPath[_MAX_PATH];
	FILETIME ft;

	ASSERT(this);
	ASSERT(fio);

	// ゼロクリアして、ゴミを消したものを作る
	memset(szPath, 0, sizeof(szPath));
	_tcscpy(szPath, pfb->m_szPath);

	// ファイルパスを保存
	if (!fio->Write(szPath, sizeof(szPath))) {
		return FALSE;
	}

	// ファイル日付を取得(2038年問題を避けるため、Win32より取得)
	memset(&ft, 0, sizeof(ft));
#if 0
	CFile file;
	if (file.Open(szPath, CFile::modeRead)) {
		::GetFileTime((HANDLE)file.m_hFile, NULL, NULL, &ft);
		file.Close();
	}
#else
	{
		HANDLE h = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(h != INVALID_HANDLE_VALUE) {
			GetFileTime(h, 0, 0, &ft);
			CloseHandle(h);
		}
	}
#endif
	// 最終書き込み日付を保存
	if (!fio->Write(&ft, sizeof(ft))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::Load(Fileio *fio, int /*ver*/)
{
	TCHAR szPath[_MAX_PATH];
	ASSERT(this);
	ASSERT(fio);

	// フルパスを読み込み
	if (!fio->Read(szPath, sizeof(szPath))) {
		return FALSE;
	}

	// セット
	SetPath(szPath);

	// 最終書き込み日付を読み込む
	if (!fio->Read(&pfb->m_SavedTime, sizeof(pfb->m_SavedTime))) {
		return FALSE;
	}

	// ファイル日付を取得(2038年問題を避けるため、Win32より取得)
#if 0
	CFile file;
	if (!file.Open(szPath, CFile::modeRead)) {
		// ファイルが存在しなくても、エラーとはしない
		return TRUE;
	}
	if (!::GetFileTime((HANDLE)file.m_hFile, NULL, NULL, &pfb->m_CurrentTime)) {
		return FALSE;
	}
	file.Close();
#else
	{
		HANDLE h = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(h == INVALID_HANDLE_VALUE) {
			return TRUE;
		} else if(! GetFileTime(h, 0, 0, &pfb->m_CurrentTime)) {
			CloseHandle(h);
			return FALSE;
		}
		CloseHandle(h);
	}
#endif
	// ftの方が新しかった場合、更新フラグUp
	if (::CompareFileTime(&pfb->m_CurrentTime, &pfb->m_SavedTime) <= 0) {
		pfb->m_bUpdate = FALSE;
	}
	else {
		pfb->m_bUpdate = TRUE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	セーブ後に更新されたか
//
//---------------------------------------------------------------------------
int FASTCALL Filepath::IsUpdate() const
{
	ASSERT(this);

	return pfb->m_bUpdate;
}

//---------------------------------------------------------------------------
//
//	セーブ時間情報を取得
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::GetUpdateTime(FILETIME *pSaved, FILETIME *pCurrent) const
{
	ASSERT(this);
	ASSERT(pfb->m_bUpdate);

	// 時間情報を渡す
	*pSaved = pfb->m_SavedTime;
	*pCurrent = pfb->m_CurrentTime;
}
/*
//---------------------------------------------------------------------------
//
//	システムファイルテーブル
//
//---------------------------------------------------------------------------
LPCTSTR Filepath::SystemFile[] = {
	_T("IPLROM.DAT"),
	_T("IPLROMXV.DAT"),
	_T("IPLROMCO.DAT"),
	_T("IPLROM30.DAT"),
	_T("ROM30.DAT"),
	_T("CGROM.DAT"),
	_T("CGROM.TMP"),
	_T("SCSIINROM.DAT"),
	_T("SCSIEXROM.DAT"),
	_T("SRAM.DAT")
};

//---------------------------------------------------------------------------
//
//	ショート名
//
//---------------------------------------------------------------------------
char Filepath::ShortName[_MAX_FNAME + _MAX_DIR];

//---------------------------------------------------------------------------
//
//	ファイル名＋拡張子
//
//---------------------------------------------------------------------------
TCHAR Filepath::FileExt[_MAX_FNAME + _MAX_DIR];

//---------------------------------------------------------------------------
//
//	デフォルトディレクトリ
//
//---------------------------------------------------------------------------
TCHAR Filepath::DefaultDir[_MAX_PATH];
*/

//---------------------------------------------------------------------------
//
// パス名取得
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetPath() const {
	return pfb->m_szPath;
}

#endif	// WIN32
