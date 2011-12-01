//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ デバイス ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "device.h"

//===========================================================================
//
//	デバイス
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Device::Device(VM *p)
{
	// ワーク初期化
	dev.next = NULL;
	dev.id = 0;
	dev.desc = NULL;

	// ワーク記憶、デバイス追加
	vm = p;
	vm->AddDevice(this);
#if defined(XM6_USE_LOG)
	log = &(vm->log);
#endif
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Device::~Device()
{
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
int FASTCALL Device::Init()
{
	ASSERT(this);

	// ここで、メモリ確保・ファイルロード等を行う
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Device::Cleanup()
{
	ASSERT(this);

	// VMに対し、デバイス削除を依頼
	ASSERT(vm);
	vm->DelDevice(this);

	// 自分自身を削除
	delete this;
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Device::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL Device::Save(Fileio*, int)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL Device::Load(Fileio*, int)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Device::ApplyCfg(const Config*)
{
	ASSERT(this);
	ASSERT_DIAG();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL Device::AssertDiag() const
{
	ASSERT(this);
	ASSERT(dev.id != 0);
	ASSERT(dev.desc);
	ASSERT(vm);
#if defined(XM6_USE_LOG)
	ASSERT(log);
	ASSERT(log == &(vm->log));
#endif
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
int FASTCALL Device::Callback(Event*)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//===========================================================================
//
//	メモリマップドデバイス
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
MemDevice::MemDevice(VM *p) : Device(p)
{
	// メモリアドレス割り当てなし
	memdev.first = 0;
	memdev.last = 0;

	// ポインタ初期化
	cpu = NULL;
	scheduler = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
int FASTCALL MemDevice::Init()
{
	ASSERT(this);
	ASSERT((memdev.first != 0) || (memdev.last != 0));
	ASSERT(memdev.first <= 0xffffff);
	ASSERT(memdev.last <= 0xffffff);
	ASSERT(memdev.first <= memdev.last);

	// デバイスの初期化を先に行う
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU、スケジューラ取得
	cpu = (CPU*)vm->SearchDevice(XM6_MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);
	scheduler = (Scheduler*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL MemDevice::ReadByte(uint32_t)
{
	ASSERT(this);
	ASSERT_DIAG();

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL MemDevice::ReadWord(uint32_t addr)
{
	uint32_t data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// 上位→下位の順でRead
	data = ReadByte(addr);
	data <<= 8;
	data |= ReadByte(addr + 1);

	return data;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::WriteByte(uint32_t, uint32_t)
{
	ASSERT(this);
	ASSERT_DIAG();
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::WriteWord(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// 上位→下位の順でWrite
	WriteByte(addr, (uint8_t)(data >> 8));
	WriteByte(addr + 1, (uint8_t)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
uint32_t FASTCALL MemDevice::ReadOnly(uint32_t) const
{
	ASSERT(this);
	ASSERT_DIAG();

	return 0xff;
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::AssertDiag() const
{
	// 基本クラス
	Device::AssertDiag();

	ASSERT(this);
	ASSERT((memdev.first != 0) || (memdev.last != 0));
	ASSERT(memdev.first <= 0xffffff);
	ASSERT(memdev.last <= 0xffffff);
	ASSERT(memdev.first <= memdev.last);
	ASSERT(cpu);
	ASSERT(cpu->GetID() == XM6_MAKEID('C', 'P', 'U', ' '));
	ASSERT(scheduler);
	ASSERT(scheduler->GetID() == XM6_MAKEID('S', 'C', 'H', 'E'));
}
#endif	// NDEBUG
