//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ テキストVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(tvram_h)
#define tvram_h

#include "device.h"

//===========================================================================
//
//	テキストVRAMハンドラ
//
//===========================================================================
class TVRAMHandler
{
public:
	TVRAMHandler(Render *rend, uint8_t *mem);
										// コンストラクタ
	virtual void FASTCALL WriteByte(uint32_t addr, uint32_t data) = 0;
										// バイト書き込み
	virtual void FASTCALL WriteWord(uint32_t addr, uint32_t data) = 0;
										// ワード書き込み

	// TVRAMワークのコピー
	uint32_t multi;
										// 同時アクセス(bit0-bit3)
	uint32_t mask;
										// アクセスマスク(1で変更なし)
	uint32_t rev;
										// アクセスマスク反転
	uint32_t maskh;
										// アクセスマスク上位バイト
	uint32_t revh;
										// アクセスマスク上位反転

protected:
	Render *render;
										// レンダラ
	uint8_t *tvram;
										// テキストVRAM
};

//===========================================================================
//
//	テキストVRAMハンドラ(通常)
//
//===========================================================================
class TVRAMNormal : public TVRAMHandler
{
public:
	TVRAMNormal(Render *rend, uint8_t *mem);
										// コンストラクタ
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAMハンドラ(マスク)
//
//===========================================================================
class TVRAMMask : public TVRAMHandler
{
public:
	TVRAMMask(Render *rend, uint8_t *mem);
										// コンストラクタ
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAMハンドラ(マルチ)
//
//===========================================================================
class TVRAMMulti : public TVRAMHandler
{
public:
	TVRAMMulti(Render *rend, uint8_t *mem);
										// コンストラクタ
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAMハンドラ(マスク＋マルチ)
//
//===========================================================================
class TVRAMBoth : public TVRAMHandler
{
public:
	TVRAMBoth(Render *rend, uint8_t *mem);
										// コンストラクタ
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAM
//
//===========================================================================
class TVRAM : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t multi;					// 同時アクセス(bit0-bit3)
		uint32_t mask;						// アクセスマスク(1で変更なし)
		uint32_t rev;						// アクセスマスク反転
		uint32_t maskh;					// アクセスマスク上位バイト
		uint32_t revh;						// アクセスマスク上位反転
		uint32_t src;						// ラスタコピー 元ラスタ
		uint32_t dst;						// ラスタコピー 先ラスタ
		uint32_t plane;					// ラスタコピー 対象プレーン
	} tvram_t;

public:
	// 基本ファンクション
	TVRAM(VM *p);
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
	const uint8_t* FASTCALL GetTVRAM() const;
										// TVRAM取得
	void FASTCALL SetMulti(uint32_t data);
										// 同時書き込み設定
	void FASTCALL SetMask(uint32_t data);
										// アクセスマスク設定
	void FASTCALL SetCopyRaster(uint32_t src, uint32_t dst, uint32_t plane);
										// コピーラスタ指定
	void FASTCALL RasterCopy();
										// ラスタコピー動作

private:
	void FASTCALL SelectHandler();
										// ハンドラ選択
	TVRAMNormal *normal;
										// ハンドラ(通常)
	TVRAMMask *mask;
										// ハンドラ(マスク)
	TVRAMMulti *multi;
										// ハンドラ(マルチ)
	TVRAMBoth *both;
										// ハンドラ(両方)
	TVRAMHandler *handler;
										// ハンドラ(現在選択中)
	Render *render;
										// レンダラ
	uint8_t *tvram;
										// テキストVRAM (512KB)
	tvram_t tvdata;
										// 内部データ
	uint32_t tvcount;
										// TVRAMアクセスカウント(version2.04以降)
};

#endif	// tvram_h
