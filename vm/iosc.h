//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ I/Oコントローラ(IOSC-2) ]
//
//---------------------------------------------------------------------------

#if !defined(iosc_h)
#define iosc_h

#include "device.h"

//===========================================================================
//
//	I/Oコントローラ
//
//===========================================================================
class IOSC : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		int prt_int;					// プリンタ割り込み要求
		int prt_en;					// プリンタ割り込み許可
		int fdd_int;					// FDD割り込み要求
		int fdd_en;					// FDD割り込み許可
		int fdc_int;					// FDC割り込み要求
		int fdc_en;					// FDC割り込み許可
		int hdc_int;					// HDD割り込み要求
		int hdc_en;					// HDD割り込み許可
		uint32_t vbase;					// ベクタベース
		int vector;						// 要求中の割り込みベクタ
	} iosc_t;

public:
	// 基本ファンクション
	IOSC(VM *p);
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
	void FASTCALL GetIOSC(iosc_t *buffer) const;
										// 内部データ取得
	uint32_t FASTCALL GetVector() const	{ return iosc.vbase; }
										// ベクタベース取得
	void FASTCALL IntAck();
										// 割り込み応答
	void FASTCALL IntFDC(int flag);
										// FDC割り込み
	void FASTCALL IntFDD(int flag);
										// FDD割り込み
	void FASTCALL IntHDC(int flag);
										// HDC割り込み
	void FASTCALL IntPRT(int flag);
										// プリンタ割り込み

private:
	void FASTCALL IntChk();
										// 割り込みチェック
	iosc_t iosc;
										// 内部データ
	Printer *printer;
										// プリンタ
};

#endif	// iosc_h
