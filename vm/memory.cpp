//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ メモリ ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "filepath.h"
#include "fileio.h"
#include "cpu.h"
#include "areaset.h"
#include "gvram.h"
#include "tvram.h"
#include "sram.h"
#include "config.h"
#include "core_asm.h"
#include "memory.h"

//---------------------------------------------------------------------------
//
//	スタティック ワーク
//
//---------------------------------------------------------------------------
static CPU *pCPU;

//---------------------------------------------------------------------------
//
//	バスエラー再現部(メインメモリ未実装エリアのみ)
//
//---------------------------------------------------------------------------
extern "C" {

//---------------------------------------------------------------------------
//
//	読み込みバスエラー
//
//---------------------------------------------------------------------------
void ReadBusErr(DWORD addr)
{
	pCPU->BusErr(addr, TRUE);
}

//---------------------------------------------------------------------------
//
//	書き込みバスエラー
//
//---------------------------------------------------------------------------
void WriteBusErr(DWORD addr)
{
	pCPU->BusErr(addr, FALSE);
}
}

//===========================================================================
//
//	メモリ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Memory::Memory(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('M', 'E', 'M', ' ');
	dev.desc = "Memory Ctrl (OHM2)";

	// 開始アドレス、終了アドレス
	memdev.first = 0;
	memdev.last = 0xffffff;

	// RAM/ROMバッファ
	mem.ram = NULL;
	mem.ipl = NULL;
	mem.cg = NULL;
	mem.scsi = NULL;

	// RAMは2MB
	mem.size = 2;
	mem.config = 0;
	mem.length = 0;

	// メモリタイプは未ロード
	mem.type = None;
	mem.now = None;

	// オブジェクト
	areaset = NULL;
	sram = NULL;

	// その他
	memset(mem.table, 0, sizeof(mem.table));
	mem.memsw = TRUE;

	// staticワーク
	::pCPU = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メインメモリ
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new BYTE[ mem.length ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ram) {
		return FALSE;
	}

	// メインメモリをゼロクリアする
	memset(mem.ram, 0x00, mem.length);

	// IPL ROM
	try {
		mem.ipl = new BYTE[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ipl) {
		return FALSE;
	}

	// CG ROM
	try {
		mem.cg = new BYTE[ 0xc0000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.cg) {
		return FALSE;
	}

	// SCSI ROM
	try {
		mem.scsi = new BYTE[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.scsi) {
		return FALSE;
	}

	// SASIのROMは必須なので、先にロードする
	if (!LoadROM(SASI)) {
		// IPLROM.DAT, CGROM.DATが存在しないパターン
		return FALSE;
	}

	// 他のROMがあれば、XVI→Compact→030の順で、先に見つかったものを優先する
	if (LoadROM(XVI)) {
		mem.now = XVI;
	}
	if (mem.type == None) {
		if (LoadROM(Compact)) {
			mem.now = Compact;
		}
	}
	if (mem.type == None) {
		if (LoadROM(X68030)) {
			mem.now = X68030;
		}
	}

	// XVI,Compact,030いずれも存在しなければ、再度SASIを読む
	if (mem.type == None) {
		LoadROM(SASI);
		mem.now = SASI;
	}

	// エリアセット取得
	areaset = (AreaSet*)vm->SearchDevice(MAKEID('A', 'R', 'E', 'A'));
	ASSERT(areaset);

	// SRAM取得
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// staticワーク
	::pCPU = cpu;

	// 初期化テーブル設定
	InitTable();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ROMロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::LoadROM(memtype target)
{
	Filepath path;
	Fileio fio;
	int i;
	BYTE data;
	BYTE *ptr;
	BOOL scsi_req;
	int scsi_size;

	ASSERT(this);

	// 一旦すべてのROMエリアを消去し、Noneに
	memset(mem.ipl, 0xff, 0x20000);
	memset(mem.cg, 0xff, 0xc0000);
	memset(mem.scsi, 0xff, 0x20000);
	mem.type = None;

	// IPL
	switch (target) {
		case SASI:
		case SCSIInt:
		case SCSIExt:
			path.SysFile(Filepath::IPL);
			break;
		case XVI:
			path.SysFile(Filepath::IPLXVI);
			break;
		case Compact:
			path.SysFile(Filepath::IPLCompact);
			break;
		case X68030:
			path.SysFile(Filepath::IPL030);
			break;
		default:
			ASSERT(FALSE);
			return FALSE;
	}
	if (!fio.Load(path, mem.ipl, 0x20000)) {
		return FALSE;
	}

	// IPLバイトスワップ
	ptr = mem.ipl;
	for (i=0; i<0x10000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// CG
	path.SysFile(Filepath::CG);
	if (!fio.Load(path, mem.cg, 0xc0000)) {
		// ファイルがなければ、CGTMPでリトライ
		path.SysFile(Filepath::CGTMP);
		if (!fio.Load(path, mem.cg, 0xc0000)) {
			return FALSE;
		}
	}

	// CGバイトスワップ
	ptr = mem.cg;
	for (i=0; i<0x60000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// SCSI
	scsi_req = FALSE;
	switch (target) {
		// 内蔵
		case SCSIInt:
		case XVI:
		case Compact:
			path.SysFile(Filepath::SCSIInt);
			scsi_req = TRUE;
			break;
		case X68030:
			path.SysFile(Filepath::ROM030);
			scsi_req = TRUE;
			break;
		// 外付
		case SCSIExt:
			path.SysFile(Filepath::SCSIExt);
			scsi_req = TRUE;
			break;
		// SASI(ROM必要なし)
		case SASI:
			break;
		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}
	if (scsi_req) {
		// X68030のみROM30.DAT(0x20000バイト)、その他は0x2000バイトでトライ
		if (target == X68030) {
			scsi_size = 0x20000;
		}
		else {
			scsi_size = 0x2000;
		}

		// 先にポインタを設定
		ptr = mem.scsi;

		// ロード
		if (!fio.Load(path, mem.scsi, scsi_size)) {
			// SCSIExtは0x1fe0バイトも許す(WinX68k高速版と互換をとる)
			if (target != SCSIExt) {
				return FALSE;
			}

			// 0x1fe0バイトで再トライ
			scsi_size = 0x1fe0;
			ptr = &mem.scsi[0x20];
			if (!fio.Load(path, &mem.scsi[0x0020], scsi_size)) {
				return FALSE;
			}
		}

		// SCSIバイトスワップ
		for (i=0; i<scsi_size; i+=2) {
			data = ptr[0];
			ptr[0] = ptr[1];
			ptr[1] = data;
			ptr += 2;
		}
	}

	// ターゲットをカレントにセットして、成功
	mem.type = target;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	初期化テーブル作成
//	※メモリデコーダに依存
//
//---------------------------------------------------------------------------
void FASTCALL Memory::InitTable()
{
#if defined(_WIN32)
#pragma pack(push, 1)
#endif	// _WIN32
	MemDevice* devarray[0x40];
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

	MemDevice *mdev;
	BYTE *table;
	DWORD ptr;
	int i;

	ASSERT(this);

	// ポインタ初期化
	mdev = this;
	i = 0;

	// Memory以降のデバイスを回って、ポインタを配列に落とす
	while (mdev) {
		devarray[i] = mdev;

		// 次へ
		i++;
		mdev = (MemDevice*)mdev->GetNextDevice();
	}

	// アセンブラルーチンを呼び出し、テーブルを引き渡す
	MemInitDecode(this, devarray);

	// アセンブラルーチンで出来たテーブルを逆に戻す(アラインメントに注意)
	table = (BYTE*) MemDecodeTable;
	for (i=0; i<0x180; i++) {
		// 4バイトごとにDWORD値を取り込み、ポインタにキャスト
		ptr = *(DWORD*)table;
		mem.table[i] = (MemDevice*)ptr;

		// 次へ
		table += 4;
	}
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Cleanup()
{
	ASSERT(this);

	// メモリ解放
	if (mem.ram) {
		delete[] mem.ram;
		mem.ram = NULL;
	}
	if (mem.ipl) {
		delete[] mem.ipl;
		mem.ipl = NULL;
	}
	if (mem.cg) {
		delete[] mem.cg;
		mem.cg = NULL;
	}
	if (mem.scsi) {
		delete[] mem.scsi;
		mem.scsi = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Reset()
{
	int size;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// メモリタイプが一致しているか
	if (mem.type != mem.now) {
		if (LoadROM(mem.type)) {
			// ROMが存在している。ロードできた
			mem.now = mem.type;
		}
		else {
			// ROMが存在しない。SASIタイプとして、設定もSASIに戻す
			LoadROM(SASI);
			mem.now = SASI;
			mem.type = SASI;
		}

		// コンテキストを作り直す(CPU::Resetは完了しているため、必ずFALSE)
		MakeContext(FALSE);
	}

	// メモリサイズが一致しているか
	if (mem.size == ((mem.config + 1) * 2)) {
		// 一致しているので、メモリスイッチ自動更新チェック
		if (mem.memsw) {
			// $ED0008 : メインRAMサイズ
			size = mem.size << 4;
			sram->SetMemSw(0x08, 0x00);
			sram->SetMemSw(0x09, size);
			sram->SetMemSw(0x0a, 0x00);
			sram->SetMemSw(0x0b, 0x00);
		}
		return;
	}

	// 変更
	mem.size = (mem.config + 1) * 2;

	// 再確保
	ASSERT(mem.ram);
	delete[] mem.ram;
	mem.ram = NULL;
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new BYTE[ mem.length ];
	}
	catch (...) {
		// メモリ不足の場合は2MBに固定
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new BYTE[ mem.length ];
	}
	if (!mem.ram) {
		// メモリ不足の場合は2MBに固定
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new BYTE[ mem.length ];
	}

	// メモリが確保できている場合のみ
	if (mem.ram) {
		memset(mem.ram, 0x00, mem.length);

		// コンテキストを作り直す(CPU::Resetは完了しているため、必ずFALSE)
		MakeContext(FALSE);
	}

	// メモリスイッチ自動更新
	if (mem.memsw) {
		// $ED0008 : メインRAMサイズ
		size = mem.size << 4;
		sram->SetMemSw(0x08, 0x00);
		sram->SetMemSw(0x09, size);
		sram->SetMemSw(0x0a, 0x00);
		sram->SetMemSw(0x0b, 0x00);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// タイプを書く
	if (!fio->Write(&mem.now, sizeof(mem.now))) {
		return FALSE;
	}

	// SCSI ROMの内容を書く (X68030以外)
	if (mem.now != X68030) {
		if (!fio->Write(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.sizeを書く
	if (!fio->Write(&mem.size, sizeof(mem.size))) {
		return FALSE;
	}

	// メモリを書く
	if (!fio->Write(mem.ram, mem.length)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Load(Fileio *fio, int /*ver*/)
{
	int size;
	BOOL context;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// コンテキストを作り直さない
	context = FALSE;

	// タイプを読む
	if (!fio->Read(&mem.type, sizeof(mem.type))) {
		return FALSE;
	}

	// タイプが現在のものと違っていれば
	if (mem.type != mem.now) {
		// ROMを読み直す
		if (!LoadROM(mem.type)) {
			// セーブ時に存在していたROMが、なくなっている
			LoadROM(mem.now);
			return FALSE;
		}

		// ROMの読み直しに成功した
		mem.now = mem.type;
		context = TRUE;
	}

	// SCSI ROMの内容を読む (X68030以外)
	if (mem.type != X68030) {
		if (!fio->Read(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.sizeを読む
	if (!fio->Read(&size, sizeof(size))) {
		return FALSE;
	}

	// mem.sizeと一致していなければ
	if (mem.size != size) {
		// 変更して
		mem.size = size;

		// 再確保
		delete[] mem.ram;
		mem.ram = NULL;
		mem.length = mem.size * 0x100000;
		try {
			mem.ram = new BYTE[ mem.length ];
		}
		catch (...) {
			mem.ram = NULL;
		}
		if (!mem.ram) {
			// メモリ不足の場合は2MBに固定
			mem.config = 0;
			mem.size = 2;
			mem.length = mem.size * 0x100000;
			mem.ram = new BYTE[ mem.length ];

			// ロード失敗
			return FALSE;
		}

		// コンテキスト再作成が必要
		context = TRUE;
	}

	// メモリを読む
	if (!fio->Read(mem.ram, mem.length)) {
		return FALSE;
	}

	// 必要であれば、コンテキストを作り直す
	if (context) {
		MakeContext(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Memory::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// メモリ種別(ROMロードは次回リセット時)
	mem.type = (memtype)config->mem_type;

	// RAMサイズ(メモリ確保は次回リセット時)
	mem.config = config->ram_size;
	ASSERT((mem.config >= 0) && (mem.config <= 5));

	// メモリスイッチ自動更新
	mem.memsw = config->ram_sramsync;
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadByte(DWORD addr)
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// メインRAM
	if (addr < mem.length) {
		return (DWORD)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (DWORD)mem.ipl[addr];
	}

	// IPLイメージ or SCSI内蔵
	if (addr >= 0xfc0000) {
		// IPLイメージか
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPLイメージ
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.ipl[addr];
		}
		// SCSI内蔵か(範囲チェック)
		if (addr < 0xfc2000) {
			// SCSI内蔵
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// X68030 IPL前半か
		if (mem.now == X68030) {
			// X68030 IPL前半
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// SCSI内蔵モデルで、ROM範囲外
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (DWORD)mem.cg[addr];
	}

	// SCSI外付
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadByte(addr);
		}
	}

	LOG1(Log::Warning, "未定義バイト読み込み $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadWord(DWORD addr)
{
	DWORD data;
	DWORD index;
	WORD *ptr;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// CPUからの場合は偶数保証されているが、DMACからの場合はチェック必要あり
	if (addr & 1) {
		// 一旦CPUへ渡す(CPU経由でDMAへ)
		cpu->AddrErr(addr, TRUE);
		return 0xffff;
	}

	// メインRAM
	if (addr < mem.length) {
		ptr = (WORD*)(&mem.ram[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		ptr = (WORD*)(&mem.ipl[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// IPLイメージ or SCSI内蔵
	if (addr >= 0xfc0000) {
		// IPLイメージか
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPLイメージ
			addr &= 0x1ffff;
			ptr = (WORD*)(&mem.ipl[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// SCSI内蔵か(範囲チェック)
		if (addr < 0xfc2000) {
			// SCSI内蔵
			addr &= 0x1fff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// X68030 IPL前半か
		if (mem.now == X68030) {
			// X68030 IPL前半
			addr &= 0x1ffff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// SCSI内蔵モデルで、ROM範囲外
		return 0xffff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		ptr = (WORD*)(&mem.cg[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// SCSI外付
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadWord(addr);
		}
	}

	// バスエラー
	LOG1(Log::Warning, "未定義ワード読み込み $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xffff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteByte(DWORD addr, DWORD data)
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);
	ASSERT(mem.now != None);

	// メインRAM
	if (addr < mem.length) {
		mem.ram[addr ^ 1] = (BYTE)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteByte(addr, data);
			return;
		}
	}

	// バスエラー
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "未定義バイト書き込み $%06X <- $%02X", addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteWord(DWORD addr, DWORD data)
{
	WORD *ptr;
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x10000);
	ASSERT(mem.now != None);

	// CPUからの場合は偶数保証されているが、DMACからの場合はチェック必要あり
	if (addr & 1) {
		// 一旦CPUへ渡す(CPU経由でDMAへ)
		cpu->AddrErr(addr, FALSE);
		return;
	}

	// メインRAM
	if (addr < mem.length) {
		ptr = (WORD*)(&mem.ram[addr]);
		*ptr = (WORD)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteWord(addr, data);
			return;
		}
	}

	// バスエラー
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "未定義ワード書き込み $%06X <- $%04X", addr, data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadOnly(DWORD addr) const
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// メインRAM
	if (addr < mem.length) {
		return (DWORD)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (DWORD)mem.ipl[addr];
	}

	// IPLイメージ or SCSI内蔵
	if (addr >= 0xfc0000) {
		// IPLイメージか
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPLイメージ
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.ipl[addr];
		}
		// SCSI内蔵か(範囲チェック)
		if (addr < 0xfc2000) {
			// SCSI内蔵
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// X68030 IPL前半か
		if (mem.now == X68030) {
			// X68030 IPL前半
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// SCSI内蔵モデルで、ROM範囲外
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (DWORD)mem.cg[addr];
	}

	// SCSI外付
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadOnly(addr);
		}
	}

	// マップされていない
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	コンテキスト作成
//
//---------------------------------------------------------------------------
void FASTCALL Memory::MakeContext(BOOL reset)
{
	ASSERT(this);

	// リセットか
	if (reset) {
		// エリアセットをリセット(CPU::ResetからMakeContextが呼ばれるため)
		ASSERT(areaset);
		areaset->Reset();

		// リセット専用コンテキスト($FF00000～が、$0000000～に見える)
		pCPU->BeginProgramRegion(TRUE);
		pCPU->AddProgramRegion(0x0000, 0xffff, ((DWORD)mem.ipl) + 0x10000);
		pCPU->EndProgramRegion();

		pCPU->BeginProgramRegion(FALSE);
		pCPU->AddProgramRegion(0x0000, 0xffff, ((DWORD)mem.ipl) + 0x10000);
		pCPU->EndProgramRegion();

		// データは全て無し
		pCPU->BeginDataRegion(FALSE, FALSE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  FALSE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(FALSE, FALSE, TRUE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  FALSE, TRUE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(FALSE, TRUE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  TRUE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(FALSE, TRUE, TRUE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  TRUE, TRUE);	pCPU->EndDataRegion();
	} else {
		{
			// 通常コンテキスト - プログラム(User)
			int area = areaset->GetArea();
			pCPU->BeginProgramRegion(FALSE);	// Program region (User)
			pCPU->AddProgramRegion((area + 1) << 13, mem.length - 1, (unsigned int)mem.ram);
			pCPU->EndProgramRegion();
		}

		{
			// 通常コンテキスト - プログラム(Super)
			pCPU->BeginProgramRegion(TRUE);	// Program region (Super)
			pCPU->AddProgramRegion(0, mem.length - 1, (unsigned int)mem.ram);
			pCPU->AddProgramRegion(0xfe0000, 0xffffff, ((unsigned int)mem.ipl) - 0xfe0000);	// IPL

			// SCSI外付
			if (mem.now == SCSIExt) {
				pCPU->AddProgramRegion(0xea0000, 0xea1fff, ((unsigned int)mem.scsi) - 0xea0000);
			}

			// IPLイメージ or SCSI内蔵
			if ((mem.now == SASI) || (mem.now == SCSIExt)) {
				pCPU->AddProgramRegion(0xfc0000, 0xfdffff, ((unsigned int)mem.ipl) - 0xfc0000);	//	IPL Image
			} else {
				// SCSI内蔵
				if(mem.now != X68030) {
					pCPU->AddProgramRegion(0xfc0000, 0xfc1fff, ((unsigned int)mem.scsi) - 0xfc0000);	// SCSI Internal
				} else {
					pCPU->AddProgramRegion(0xfc0000, 0xfdffff, ((unsigned int)mem.scsi) - 0xfc0000);	// X68030 first half
				}
			}

			// グラフィックVRAM
			GVRAM *gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
			ASSERT(gvram);
			pCPU->AddProgramRegion(0xc00000, 0xdfffff, ((unsigned int)gvram->GetGVRAM()) - 0xc00000);

			// テキストVRAM
			TVRAM* tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
			ASSERT(tvram);
			pCPU->AddProgramRegion(0xe00000, 0xe7ffff, ((unsigned int)tvram->GetTVRAM()) - 0xe00000);

			// SRAM
			ASSERT(sram);
			pCPU->AddProgramRegion(0xed0000, 0xed0000 + (sram->GetSize() << 10) - 1, ((unsigned int)sram->GetSRAM()) - 0xed0000);
			pCPU->EndProgramRegion();
		}

		{
			// 通常コンテキスト - 読み出し(User)
			int area = areaset->GetArea();

			pCPU->BeginDataRegion(FALSE, FALSE, FALSE);		// User, Read, Byte
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ユーザアクセス可能空間
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::ReadErrC, NULL);			// スーパバイザ空間
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::ReadErrC, NULL);			// メインメモリ未実装空間＋スーパーバイザI/O空間
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::ReadByteC, NULL);					// ユーザI/O空間($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::ReadErrC, NULL);					// スーパバイザ空間(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();

			pCPU->BeginDataRegion(FALSE, FALSE, TRUE);		// User, Read, Word
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ユーザアクセス可能空間
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::ReadErrC, NULL);			// スーパバイザ空間
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::ReadErrC, NULL);			// メインメモリ未実装空間＋スーパーバイザI/O空間
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::ReadWordC, NULL);					// ユーザI/O空間($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::ReadErrC, NULL);					// スーパバイザ空間(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();

			pCPU->BeginDataRegion(FALSE, TRUE, FALSE);		// User, Write, Byte
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ユーザアクセス可能空間
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::WriteErrC, NULL);			// スーパバイザ空間
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::WriteErrC, NULL);			// メインメモリ未実装空間＋スーパーバイザI/O空間
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::WriteByteC, NULL);				// ユーザI/O空間($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::WriteErrC, NULL);					// スーパバイザ空間(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();

			pCPU->BeginDataRegion(FALSE, TRUE, TRUE);		// User, Write, Word
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ユーザアクセス可能空間
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::WriteErrC, NULL);			// スーパバイザ空間
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::WriteErrC, NULL);			// メインメモリ未実装空間＋スーパーバイザI/O空間
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::WriteWordC, NULL);				// ユーザI/O空間($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::WriteErrC, NULL);					// スーパバイザ空間(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();
		}

		{
			// 通常コンテキスト - 読み出し(Super)
			for(int isWord = 0; isWord < 2; ++isWord) {
				pCPU->BeginDataRegion(TRUE, FALSE, (BOOL) isWord);		// Super, Read, {Byte|Word}
				pCPU->AddDataRegion(0, mem.length - 1, NULL, (void*)mem.ram);
				pCPU->AddDataRegion(0xf00000, 0xfbffff, NULL, (void*)mem.cg);			// CG
				pCPU->AddDataRegion(0xfe0000, 0xffffff, NULL, (void*)mem.ipl);			// IPL

				// SCSI外付
				if (mem.now == SCSIExt) {
					pCPU->AddDataRegion(0xea0020, 0xea1fff, NULL, (void*)(&mem.scsi[0x20]));
				}

				// IPLイメージ or SCSI内蔵
				if ((mem.now == SASI) || (mem.now == SCSIExt)) {
					// IPLイメージ
					pCPU->AddDataRegion(0xfc0000, 0xfdffff, NULL, (void*)mem.ipl);
				} else {
					// SCSI内蔵
					if (mem.now != X68030) {
						pCPU->AddDataRegion(0xfc0000, 0xfc1fff, NULL, (void*)mem.scsi);
					} else {
						// X68030 IPL前半
						pCPU->AddDataRegion(0xfc0000, 0xfdffff, NULL, (void*)mem.scsi);
					}
				}

				// それ以外(外部コール)
				if(!isWord) {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::ReadByteC, NULL);
				} else {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::ReadWordC, NULL);
				}
				pCPU->EndDataRegion();
			}
		}

		{
			// 通常コンテキスト - 書き込み(Super)
			for(int isWord = 0; isWord < 2; ++isWord) {
				pCPU->BeginDataRegion(TRUE, TRUE, (BOOL) isWord);		// Super, Write, {Byte|Word}

				pCPU->AddDataRegion(0, mem.length - 1, NULL, (void*)mem.ram);

				// それ以外(外部コール)
				if(!isWord) {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::WriteByteC, NULL);
				} else {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::WriteWordC, NULL);
				}
				pCPU->EndDataRegion();
			}
		}

		// cpu->Releaseを忘れずに
		cpu->Release();
	}
}

//---------------------------------------------------------------------------
//
//	IPLバージョンチェック
//	※IPLがversion1.00(87/05/07)であるか否かをチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::CheckIPL() const
{
	ASSERT(this);
	ASSERT(mem.now != None);

	// 存在チェック
	if (!mem.ipl) {
		return FALSE;
	}

	// SASIタイプの場合のみチェックする
	if (mem.now != SASI) {
		return TRUE;
	}

	// 日付(BCD)をチェック
	if (mem.ipl[0x1000a] != 0x87) {
		return FALSE;
	}
	if (mem.ipl[0x1000c] != 0x07) {
		return FALSE;
	}
	if (mem.ipl[0x1000d] != 0x05) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CGチェック
//	※8x8ドットフォント(全機種共通)のSum,Xorでチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::CheckCG() const
{
	BYTE add;
	BYTE eor;
	BYTE *ptr;
	int i;

	ASSERT(this);
	ASSERT(mem.now != None);

	// 存在チェック
	if (!mem.cg) {
		return FALSE;
	}

	// 初期設定
	add = 0;
	eor = 0;
	ptr = &mem.cg[0x3a800];

	// ADD, XORループ
	for (i=0; i<0x1000; i++) {
		add = (BYTE)(add + *ptr);
		eor ^= *ptr;
		ptr++;
	}

	// チェック(XVIでの実測値)
	if ((add != 0xec) || (eor != 0x84)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CG取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetCG() const
{
	ASSERT(this);
	ASSERT(mem.cg);

	return mem.cg;
}

//---------------------------------------------------------------------------
//
//	SCSI取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetSCSI() const
{
	ASSERT(this);
	ASSERT(mem.scsi);

	return mem.scsi;
}

//---------------------------------------------------------------------------
//
//	IPL取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetIPL() const
{
	ASSERT(this);
	ASSERT(mem.ipl);

	return mem.ipl;
}

extern "C" unsigned int MemDecodeTable[384] = { 0 };								// メモリデコードテーブル
extern "C" unsigned int EventTable[32] = { 0 };									
extern "C" unsigned int MemoryPtr =  0;										
extern "C" unsigned int EventNum = 0;										
/*
;
; データエリア (8KB単位)
;
; 0	MEMORY
; 1	GVRAM
; 2	TVRAM
; 3	CRTC
; 4	VC
; 5	DMAC
; 6	AREA
; 7	MFP
; 8	RTC
; 9	PRN
; 10	SYSPORT
; 11	OPM
; 12	ADPCM
; 13	FDC
; 14	SASI
; 15	SCC
; 16	PPI
; 17	IOSC
; 18	WINDRV
; 19	SCSI
; 20	MIDI
; 21	SPR
; 22	MERCURY
; 23	NEPTUNE
; 24	SRAM
;
*/
extern "C" DWORD MemDecodeData[] = {
// $C00000 (GVRAM)
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
// $E00000 (TVRAM)
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
// $E80000 (CRTC - IOSC)
			3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
// $E9E000 (WINDRV)
			18,
// $EA0000 (SCSI)
			19,
// $EA2000 (RESERVE)
			0,0,0,0,0,0,
// $EAE000 (MIDI)
			20,
// $EB0000 (SPRITE)
			21,21,21,21,21,21,21,21,
// $EC0000 (USER)
			0,0,0,0,0,0,
// $ECC000 (MERCURY)
			22,
// $ECE000 (NEPTUNE)
			23,
// $ED0000 (SRAM)
			24,24,24,24,24,24,24,24,
// $EE0000 (RESERVE)
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// メモリデコーダ初期化
extern "C" void MemInitDecode(Memory *mem, MemDevice* list[]) {
	unsigned int* ebx = (unsigned int*) &MemDecodeTable[0];
	const unsigned int* edx = (unsigned int*) &MemDecodeData[0];

	for(int i = 0; i < 384; ++i) {
		unsigned int eax = *edx;
		edx += 1;
		unsigned int edi = (unsigned int) list[eax];

		*ebx = edi;
		ebx += 1;
	}
}

extern "C" DWORD ReadByteC(DWORD addr);										// バイト読み込み
extern "C" DWORD ReadWordC(DWORD addr);										// ワード読み込み
extern "C" void WriteByteC(DWORD addr, DWORD data);							// バイト書き込み
extern "C" void WriteWordC(DWORD addr, DWORD data);							// ワード書き込み
extern "C" void ReadErrC(DWORD addr);										// バスエラー読み込み
extern "C" void WriteErrC(DWORD addr, DWORD data);							// バスエラー書き込み

// イベント群 指定
extern "C" void NotifyEvent(Event *first) {
	Event* esi = first;
	unsigned int ecx = 0;
	unsigned int* edi = &EventTable[0];

	while(esi != 0) {
		*edi = (unsigned int) esi;
		ecx += 1;

		esi = esi->GetNextEvent();
		edi += 1;
	}
	EventNum = ecx;
}

// イベント群 最小のものを探す
extern "C" DWORD GetMinEvent(DWORD hus) {
	unsigned int eax = hus;

	const unsigned int* esi = &EventTable[0];
	for(unsigned int i = 0, n = EventNum; i < n; ++i) {
		unsigned int edi = *esi;
		esi += 1;
		unsigned int edx = * (unsigned int*) (edi + 4);
		if(edx == 0) {
			edx = eax;
		}
		if(edx < eax) {
			eax = edx;
		}
	}

	return eax;
}

// イベント群 減算＆実行
extern "C" BOOL SubExecEvent(DWORD hus) {
	unsigned int edi = hus;
	const unsigned int* esi = &EventTable[0];

	for(unsigned i = 0, n = EventNum; i < n; ++i) {
	//	loop:
		unsigned int ebp = *esi;
		esi += 1;
		unsigned int eax = * (unsigned int*) (ebp + 8);		// Event::event_t.time
		if(eax != 0) {
			* (int*) (ebp+4) -= edi;		// Event::event_t.remain
			if(*(int*) (ebp+4) <= 0) {		// Event::event_t.remain
				// exec
				*(int*)(ebp+4) = (int) (eax);	// 	Event::event_t.remain

				Event*	pe = (Event*) ebp;
				Device* pd = pe->GetDevice();

				unsigned int eax = pd->Callback(pe);
				if(eax == 0) {
					// ; 無効化(timeおよびremainを0クリア)
					// .disable:
					* (unsigned int*) (ebp+8) = 0;		// Event::event_t.time
					* (unsigned int*) (ebp+4) = 0;		// Event::event_t.remain
				}
			}
		}
	//	next:
	}

	return FALSE;
}
