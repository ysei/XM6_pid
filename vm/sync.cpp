//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 同期オブジェクト ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "sync.h"

#if defined(_WIN32)

//===========================================================================
//
//	同期オブジェクト
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Sync::Sync()
{
	// クリティカルセクション作成
//	csect = new CCriticalSection;
	pCriticalSection = new CRITICAL_SECTION;
	InitializeCriticalSection(pCriticalSection);
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Sync::~Sync()
{
//	// ロックして
//	Lock();
//
//	// クリティカルセクション削除
//	ASSERT(csect);
//	delete csect;
//	csect = NULL;
	DeleteCriticalSection(pCriticalSection);
	delete pCriticalSection;
	pCriticalSection = 0;
}

//---------------------------------------------------------------------------
//
//	ロック
//
//---------------------------------------------------------------------------
void FASTCALL Sync::Lock()
{
//	ASSERT(csect);
//	csect->Lock();
	EnterCriticalSection(pCriticalSection);
}

//---------------------------------------------------------------------------
//
//	アンロック
//
//---------------------------------------------------------------------------
void FASTCALL Sync::Unlock()
{
//	ASSERT(csect);
//	csect->Unlock();
	LeaveCriticalSection(pCriticalSection);
}

#endif	// _WIN32
