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
//	ファイルパス
//	※代入演算子を用意すること
//
//===========================================================================
class Filepath
{
public:
	Filepath();													// コンストラクタ
	virtual ~Filepath();										// デストラクタ
	void FASTCALL Clear();										// クリア
	void FASTCALL SysFile(XM6_pid::SysFileType sys);			// ファイル設定(システム)
	void FASTCALL SetPath(const XM6_pid::FiosPath* fiosPath);	// ファイル設定(ユーザ)
	void FASTCALL SetPath(const Filepath& path);				// ファイル設定
	const char* FASTCALL GetShort() const;						// ショート名取得(const char*)
	int FASTCALL CmpPath(const Filepath& path) const;			// パス比較
	int FASTCALL Save(Fileio *fio, int ver) const;				// セーブ
	int FASTCALL Load(Fileio *fio, int ver);					// ロード
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
