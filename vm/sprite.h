//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スプライト(CYNTHIA) ]
//
//---------------------------------------------------------------------------

#if !defined(sprite_h)
#define sprite_h

#include "device.h"

//===========================================================================
//
//	スプライト
//
//===========================================================================
class Sprite : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		int connect;					// アクセス可能フラグ
		int disp;						// 表示(ウェイト)フラグ
		uint8_t *mem;						// スプライトメモリ
		uint8_t *pcg;						// スプライトPCGエリア

		int bg_on[2];					// BG表示ON
		uint32_t bg_area[2];				// BGデータエリア
		uint32_t bg_scrlx[2];				// BGスクロールX
		uint32_t bg_scrly[2];				// BGスクロールY
		int bg_size;					// BGサイズ

		uint32_t h_total;					// 水平トータル期間
		uint32_t h_disp;					// 水平表示期間
		uint32_t v_disp;					// 垂直表示期間
		int lowres;					// 15kHzモード
		uint32_t h_res;					// 水平解像度
		uint32_t v_res;					// 垂直解像度
	} sprite_t;

public:
	// 基本ファンクション
	Sprite(VM *p);
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
	uint32_t FASTCALL ReadWord(uint32_t addr);
										// ワード読み込み
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL Connect(int con)		{ spr.connect = con; }
										// 接続
	int FASTCALL IsConnect() const		{ return spr.connect; }
										// 接続状況取得
	int FASTCALL IsDisplay() const		{ return spr.disp; }
										// 表示状況取得
	void FASTCALL GetSprite(sprite_t *buffer) const;
										// 内部データ取得
	const uint8_t* FASTCALL GetMem() const;
										// メモリエリア取得
	const uint8_t* FASTCALL GetPCG() const;
										// PCGエリア取得 

private:
	void FASTCALL Control(uint32_t addr, uint32_t ctrl);
										// コントロール
	sprite_t spr;
										// 内部データ
	Render *render;
										// レンダラ
	uint8_t *sprite;
										// スプライトRAM(64KB)
};

#endif	// sprite_h
