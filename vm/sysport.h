//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ システムポート ]
//
//---------------------------------------------------------------------------

#if !defined(sysport_h)
#define sysport_h

#include "device.h"

//===========================================================================
//
//	システムポート
//
//===========================================================================
class SysPort : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t contrast;					// コントラスト
		uint32_t scope_3d;					// 3Dスコープ制御
		uint32_t image_unit;				// イメージユニット制御
		uint32_t power_count;				// 電源制御カウンタ
		uint32_t ver_count;				// バージョン管理カウンタ
	} sysport_t;

public:
	// 基本ファンクション
	SysPort(VM *p);
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

private:
	uint32_t FASTCALL GetVR();
										// バージョンレジスタ読み出し
	sysport_t sysport;
										// 内部ワーク
	Memory *memory;
										// メモリ
	SRAM *sram;
										// スタティックRAM
	Keyboard *keyboard;
										// キーボード
	CRTC *crtc;
										// CRTC
	Render *render;
										// レンダラ
};

#endif	// sysport_h
