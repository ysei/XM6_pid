//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ファイルパス ]
//
//---------------------------------------------------------------------------

#if !defined(filepath_h)
#define filepath_h

class Filepath;

#if defined(_WIN32)

//---------------------------------------------------------------------------
//
//	定数定義
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
//	ファイルパス
//	※代入演算子を用意すること
//
//===========================================================================
class Filepath
{
public:
	// システムファイル種別
	enum SysFileType {
		IPL,							// IPL(version 1.00)
		IPLXVI,							// IPL(version 1.10)
		IPLCompact,						// IPL(version 1.20)
		IPL030,							// IPL(version 1.30)後半
		ROM030,							// IPL(version 1.30)前半
		CG,								// CG
		CGTMP,							// CG(Win合成)
		SCSIInt,						// SCSI(内蔵)
		SCSIExt,						// SCSI(外付)
		SRAM							// SRAM
	};

public:
	Filepath();
										// コンストラクタ
	virtual ~Filepath();
										// デストラクタ
	Filepath& operator=(const Filepath& path);
										// 代入

	void FASTCALL Clear();
										// クリア
	void FASTCALL SysFile(SysFileType sys);
										// ファイル設定(システム)
	void FASTCALL SetPath(LPCTSTR lpszPath);
										// ファイル設定(ユーザ)
	void FASTCALL SetBaseDir();
										// ベースディレクトリ設定
	void FASTCALL SetBaseFile();
										// ベースディレクトリ＋ファイル名設定

	int FASTCALL IsClear() const;
										// クリアされているか
	LPCTSTR FASTCALL GetPath() const;
										// パス名取得
	const char* FASTCALL GetShort() const;
										// ショート名取得(const char*)
	LPCTSTR FASTCALL GetFileExt() const;
										// ショート名取得(LPCTSTR)
	int FASTCALL CmpPath(const Filepath& path) const;
										// パス比較

	static void FASTCALL ClearDefaultDir();
										// デフォルディレクトリを初期化
	static void FASTCALL SetDefaultDir(LPCTSTR lpszPath);
										// デフォルトディレクトリに設定
	static LPCTSTR FASTCALL GetDefaultDir();
										// デフォルトディレクトリ取得

	int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	int FASTCALL Load(Fileio *fio, int ver);
										// ロード

private:
	void FASTCALL Split();
										// パス分割
	void FASTCALL Make();
										// パス合成
	void FASTCALL SetCurDir();
										// カレントディレクトリ設定
	int FASTCALL IsUpdate() const;
										// セーブ後の更新ありか
	void FASTCALL GetUpdateTime(FILETIME *pSaved, FILETIME *pCurrent ) const;
										// セーブ後の時間情報を取得

	struct FilepathBuf;
	FilepathBuf* pfb;
};


#endif	// _WIN32
#endif	// filepath_h
