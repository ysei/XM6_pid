//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ プリンタ ]
//
//---------------------------------------------------------------------------

#if !defined(printer_h)
#define printer_h

#include "device.h"

//===========================================================================
//
//	プリンタ
//
//===========================================================================
class Printer : public MemDevice
{
public:
	// 定数値
	enum {
		BufMax = 0x1000				// バッファサイズ(2の倍数)
	};

	// 内部データ定義
	typedef struct {
		int connect;					// 接続
		int strobe;					// ストローブ
		int ready;						// レディ
		uint8_t data;						// 書き込みデータ
		uint8_t buf[BufMax];				// バッファデータ
		uint32_t read;						// バッファ読み込み位置
		uint32_t write;					// バッファ書き込み位置
		uint32_t num;						// バッファ有効数
	} printer_t;

public:
	// 基本ファンクション
	Printer(VM *p);
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
	int FASTCALL IsReady() const		{ return printer.ready; }
										// レディ取得
	void FASTCALL HSync();
										// H-Sync通知
	void FASTCALL GetPrinter(printer_t *buffer) const;
										// 内部データ取得
	void FASTCALL Connect(int flag);
										// プリンタ接続
	int FASTCALL GetData(uint8_t *ptr);
										// 先頭データ取得

private:
	IOSC *iosc;
										// IOSC
	Sync *sync;
										// 同期オブジェクト
	printer_t printer;
										// 内部データ
};

#endif	// printer_h
