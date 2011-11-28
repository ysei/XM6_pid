//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スタティックRAM ]
//
//---------------------------------------------------------------------------

#if !defined(sram_h)
#define sram_h

#include "device.h"
#include "filepath.h"

//===========================================================================
//
//	スタティックRAM
//
//===========================================================================
class SRAM : public MemDevice
{
public:
	// 基本ファンクション
	SRAM(VM *p);
										// コンストラクタ
	int FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	int FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// メモリデバイス
	uint32_t FASTCALL ReadByte(uint32_t addr);
										// バイト読み込み
	uint32_t FASTCALL ReadWord(uint32_t addr);
										// ワード読み込み
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ

	// 外部API
	const uint8_t* FASTCALL GetSRAM() const;
										// SRAMエリア取得
	int FASTCALL GetSize() const;
										// SRAMサイズ取得
	void FASTCALL WriteEnable(int enable);
										// 書き込み許可
	void FASTCALL SetMemSw(uint32_t offset, uint32_t data);
										// メモリスイッチセット
	uint32_t FASTCALL GetMemSw(uint32_t offset) const;
										// メモリスイッチ取得
	void FASTCALL UpdateBoot();
										// 起動カウンタ更新

private:
	Filepath sram_path;
										// SRAMファイルパス
	int sram_size;
										// SRAMサイズ(16,32,48,64)
	uint8_t sram[0x10000];
										// SRAM (64KB)
	int write_en;
										// 書き込み許可フラグ
	int mem_sync;
										// メインRAMサイズ同期フラグ
	int changed;
										// 変更フラグ
};

#endif	// sram_h
