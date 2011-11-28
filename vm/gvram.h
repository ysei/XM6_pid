//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ グラフィックVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(gvram_h)
#define gvram_h

#include "device.h"
#include "crtc.h"

//===========================================================================
//
//	グラフィックVRAMハンドラ
//
//===========================================================================
class GVRAMHandler
{
public:
	GVRAMHandler(Render *rend, uint8_t *mem, CPU *p);
										// コンストラクタ
	virtual uint32_t FASTCALL ReadByte(uint32_t addr) = 0;
										// バイト読み込み
	virtual uint32_t FASTCALL ReadWord(uint32_t addr) = 0;
										// ワード読み込み
	virtual void FASTCALL WriteByte(uint32_t addr, uint32_t data) = 0;
										// バイト書き込み
	virtual void FASTCALL WriteWord(uint32_t addr, uint32_t data) = 0;
										// ワード書き込み
	virtual uint32_t FASTCALL ReadOnly(uint32_t addr) const = 0;
										// 読み込みのみ

protected:
	Render *render;
										// レンダラ
	uint8_t *gvram;
										// グラフィックVRAM
	CPU *cpu;
										// CPU
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(1024)
//
//===========================================================================
class GVRAM1024 : public GVRAMHandler
{
public:
	GVRAM1024(Render *render, uint8_t *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(16色)
//
//===========================================================================
class GVRAM16 : public GVRAMHandler
{
public:
	GVRAM16(Render *render, uint8_t *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(256色)
//
//===========================================================================
class GVRAM256 : public GVRAMHandler
{
public:
	GVRAM256(Render *render, uint8_t *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(無効)
//
//===========================================================================
class GVRAMNDef : public GVRAMHandler
{
public:
	GVRAMNDef(Render *render, uint8_t *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(65536色)
//
//===========================================================================
class GVRAM64K : public GVRAMHandler
{
public:
	GVRAM64K(Render *render, uint8_t *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAM
//
//===========================================================================
class GVRAM : public MemDevice
{
public:
	// 内部ワーク定義
	typedef struct {
		int mem;						// 512KB単純メモリフラグ
		uint32_t siz;						// 1024×1024フラグ
		uint32_t col;						// 16, 256, 未定義, 65536
		int type;						// ハンドラタイプ(0～4)
		uint32_t mask[4];					// 高速クリア マスク
		int plane[4];					// 高速クリア プレーン
	} gvram_t;

public:
	// 基本ファンクション
	GVRAM(VM *p);
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
	void FASTCALL SetType(uint32_t type);
										// GVRAMタイプ設定
	void FASTCALL FastSet(uint32_t mask);
										// 高速クリア設定
	void FASTCALL FastClr(const CRTC::crtc_t *p);
										// 高速クリア
	const uint8_t* FASTCALL GetGVRAM() const;
										// GVRAM取得

private:
	void FASTCALL FastClr768(const CRTC::crtc_t *p);
										// 高速クリア 1024x1024 512/768
	void FASTCALL FastClr256(const CRTC::crtc_t *p);
										// 高速クリア 1024x1024 256
	void FASTCALL FastClr512(const CRTC::crtc_t *p);
										// 高速クリア 512x512
	Render *render;
										// レンダラ
	uint8_t *gvram;
										// グラフィックVRAM
	GVRAMHandler *handler;
										// メモリハンドラ(カレント)
	GVRAM1024 *hand1024;
										// メモリハンドラ(1024)
	GVRAM16 *hand16;
										// メモリハンドラ(16色)
	GVRAM256 *hand256;
										// メモリハンドラ(256色)
	GVRAMNDef *handNDef;
										// メモリハンドラ(無効)
	GVRAM64K *hand64K;
										// メモリハンドラ(64K色)
	gvram_t gvdata;
										// 内部ワーク
	uint32_t gvcount;
										// GVRAMアクセスカウント(version2.04以降)
};

#endif	// gvram_h
