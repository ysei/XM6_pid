//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ メモリ ]
//
//---------------------------------------------------------------------------

#if !defined(memory_h)
#define memory_h

#include "device.h"

//===========================================================================
//
//	外部関数
//
//===========================================================================
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus
void ReadBusErr(DWORD addr);
										// 読み込みバスエラー
void WriteBusErr(DWORD addr);
										// 書き込みバスエラー
#if defined(__cplusplus)
}
#endif	// __cplusplus

//===========================================================================
//
//	メモリ
//
//===========================================================================
class Memory : public MemDevice
{
public:
	// メモリ種別(=システム種別)
	enum memtype {
		None,							// ロードされていない
		SASI,							// v1.00-SASI(初代/ACE/EXPERT/PRO)
		SCSIInt,						// v1.00-SCSI内蔵(SUPER)
		SCSIExt,						// v1.00-SCSI外付ボード(初代/ACE/EXPERT/PRO)
		XVI,							// v1.10-SCSI内蔵(XVI)
		Compact,						// v1.20-SCSI内蔵(Compact)
		X68030							// v1.50-SCSI内蔵(X68030)
	};

protected:
	// 内部データ定義
	typedef struct {
		MemDevice* table[0x180];		// ジャンプテーブル
		int size;						// RAMサイズ(2,4,6,8,10,12)
		int config;						// RAM設定値(0～5)
		DWORD length;					// RAM最終バイト+1
		BYTE *ram;						// メインRAM
		BYTE *ipl;						// IPL ROM (128KB)
		BYTE *cg;						// CG ROM(768KB)
		BYTE *scsi;						// SCSI ROM (8KB)
		memtype type;					// メモリ種別(リセット後)
		memtype now;					// メモリ種別(カレント)
		BOOL memsw;						// メモリスイッチ自動更新
	} memory_t;

public:
	// 基本ファンクション
	Memory(VM *p);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用

	// メモリデバイス
	DWORD FASTCALL ReadByte(DWORD addr);
										// バイト読み込み
	DWORD FASTCALL ReadWord(DWORD addr);
										// ワード読み込み
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL MakeContext(BOOL reset);
										// メモリコンテキスト作成
	BOOL FASTCALL CheckIPL() const;
										// IPLバージョンチェック
	BOOL FASTCALL CheckCG() const;
										// CGチェック
	const BYTE* FASTCALL GetCG() const;
										// CG取得
	const BYTE* FASTCALL GetSCSI() const;
										// SCSI取得
	const BYTE* FASTCALL GetIPL() const;
										// IPL取得
	memtype FASTCALL GetMemType() const { return mem.now; }
										// メモリ種別取得

private:
	BOOL FASTCALL LoadROM(memtype target);
										// ROMロード
	void FASTCALL InitTable();
										// デコードテーブル初期化
	AreaSet *areaset;
										// エリアセット
	SRAM *sram;
										// SRAM
	memory_t mem;
										// 内部データ
};

#endif	// memory_h
