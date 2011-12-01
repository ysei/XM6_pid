//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ エリアセット ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "log.h"
#include "fileio.h"
#include "memory_xm6.h"
#include "areaset.h"

//===========================================================================
//
//	エリアセット
//
//===========================================================================
#if defined(AREASET_LOG)
#undef  AREASET_LOG
#define AREASET_LOG(...)	__VA_ARGS__
#else
#define AREASET_LOG(...)
#endif

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
AreaSet::AreaSet(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = XM6_MAKEID('A', 'R', 'E', 'A');
	dev.desc = "Area Set";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe86000;
	memdev.last = 0xe87fff;

	// オブジェクト
	memory = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
int FASTCALL AreaSet::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メモリ取得
	memory = (Memory*)vm->SearchDevice(XM6_MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//	※正規の順序でなく、Memory::MakeContextから呼ばれる
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	AREASET_LOG(LOG0(Log::Normal, "エリアセット設定 $00"));

	// エリア指定初期化
	area = 0;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL AreaSet::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(area);
	if (!fio->Write(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// エリア情報をセーブ
	if (!fio->Write(&area, (int)sizeof(area))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL AreaSet::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// サイズをロード
	if (!fio->Read(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// サイズを比較
	if (sz != sizeof(area)) {
		return FALSE;
	}

	// エリア情報をロード
	if (!fio->Read(&area, (int)sizeof(area))) {
		return FALSE;
	}

	// 適用
	memory->MakeContext(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL AreaSet::ReadByte(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 常にバスエラー
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::WriteByte(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 2バイトおきにマップ
	addr &= 1;

	// 奇数アドレスはエリアセット
	if (addr & 1) {
		LOG1(Log::Detail, "エリアセット設定 $%02X", data);

		// データ記憶
		area = data;

		// メモリマップ再構築
		memory->MakeContext(FALSE);
		return;
	}

	// 偶数アドレスはデコードされていない
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
uint32_t FASTCALL AreaSet::ReadOnly(uint32_t addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// EVENは0xff、ODDは設定値を返す
	if (addr & 1) {
		return area;
	}
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	エリアセット取得
//
//---------------------------------------------------------------------------
uint32_t FASTCALL AreaSet::GetArea() const
{
	ASSERT(this);
	return area;
}
