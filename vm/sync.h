//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 同期オブジェクト ]
//
//---------------------------------------------------------------------------

#if !defined(sync_h)
#define sync_h
#if defined(_WIN32)

//---------------------------------------------------------------------------
//
//	MFCクラス定義
//
//---------------------------------------------------------------------------
class CCriticalSection;
extern "C" typedef struct _RTL_CRITICAL_SECTION CRITICAL_SECTION;

//===========================================================================
//
//	同期オブジェクト
//
//===========================================================================
class Sync
{
public:
	Sync() {}
										// コンストラクタ
	virtual ~Sync() {}
										// デストラクタ
	void FASTCALL Lock() {}
										// ロック
	void FASTCALL Unlock() {}
										// アンロック

private:
//	CCriticalSection *csect;
//										// クリティカルセクション
//	CRITICAL_SECTION*	pCriticalSection;
};

#endif	// _WIN32
#endif	// sync_h
