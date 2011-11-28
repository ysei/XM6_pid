//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ エリアセット ]
//
//---------------------------------------------------------------------------

#if !defined(areaset_h)
#define areaset_h

#include "device.h"

//===========================================================================
//
//	エリアセット
//
//===========================================================================
class AreaSet : public MemDevice
{
public:
	// 基本ファンクション
	AreaSet(VM *p);
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

	// メモリデバイス
	uint32_t FASTCALL ReadByte(uint32_t addr);
										// バイト読み込み
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ

	// 外部API
	uint32_t FASTCALL GetArea() const;
										// 設定値取得

private:
	Memory *memory;
										// メモリ
	uint32_t area;
										// エリアセットレジスタ
};

#endif	// areaset_h
