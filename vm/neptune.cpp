//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ Neptune-X ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "neptune.h"

//===========================================================================
//
//	Neptune-X
//
//===========================================================================
//#define NEPTUNE_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Neptune::Neptune(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = XM6_MAKEID('N', 'E', 'P', 'X');
	dev.desc = "Neptune-X (DP8390)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xece000;
	memdev.last = 0xecffff;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
int FASTCALL Neptune::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL Neptune::Save(Fileio* /*fio*/, int /*ver*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL Neptune::Load(Fileio* /*fio*/, int /*ver*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == XM6_MAKEID('N', 'E', 'P', 'X'));
	ASSERT(memdev.first == 0xece000);
	ASSERT(memdev.last == 0xecffff);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Neptune::ReadByte(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// バスエラー
	cpu->BusErr(addr, TRUE);

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Neptune::ReadWord(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::WriteByte(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// バスエラー
	cpu->BusErr(addr, FALSE);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::WriteWord(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	WriteByte(addr + 1, (uint8_t)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Neptune::ReadOnly(uint32_t addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	return 0xff;
}
