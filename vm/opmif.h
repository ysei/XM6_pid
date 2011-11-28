//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ OPM(YM2151) ]
//
//---------------------------------------------------------------------------

#if !defined(opmif_h)
#define opmif_h

#include "device.h"
#include "event.h"
#include "opm.h"

//===========================================================================
//
//	OPM
//
//===========================================================================
class OPMIF : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t reg[0x100];				// レジスタ
		uint32_t key[8];					// キー情報
		uint32_t addr;						// セレクトアドレス
		int busy;						// BUSYフラグ
		int enable[2];					// タイマイネーブル
		int action[2];					// タイマ動作
		int interrupt[2];				// タイマ割り込み
		uint32_t time[2];					// タイマ時間
		int started;					// 開始フラグ
	} opm_t;

	// バッファ管理定義
	typedef struct {
		uint32_t max;						// 最大数
		uint32_t num;						// 有効データ数
		uint32_t read;						// 読み取りポイント
		uint32_t write;					// 書き込みポイント
		uint32_t samples;					// 合成サンプル数
		uint32_t rate;						// 合成レート
		uint32_t under;					// アンダーラン
		uint32_t over;						// オーバーラン
		int sound;						// FM有効
	} opmbuf_t;

public:
	// 基本ファンクション
	OPMIF(VM *p);
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
	void FASTCALL GetOPM(opm_t *buffer);
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL Output(uint32_t addr, uint32_t data);
										// レジスタ出力
	void FASTCALL SetEngine(FM::OPM *p);
										// エンジン指定
	void FASTCALL InitBuf(uint32_t rate);
										// バッファ初期化
	uint32_t FASTCALL ProcessBuf();
										// バッファ処理
	void FASTCALL GetBuf(uint32_t *buf, int samples);
										// バッファより取得
	void FASTCALL GetBufInfo(opmbuf_t *buffer);
										// バッファ情報を得る
	void FASTCALL EnableFM(int flag)	{ bufinfo.sound = flag; }
										// FM音源有効
	void FASTCALL ClrStarted()			{ opm.started = FALSE; }
										// スタートフラグを降ろす
	int FASTCALL IsStarted() const		{ return opm.started; }
										// スタートフラグ取得

private:
	void FASTCALL CalcTimerA();
										// タイマA算出
	void FASTCALL CalcTimerB();
										// タイマB算出
	void FASTCALL CtrlTimer(uint32_t data);
										// タイマ制御
	void FASTCALL CtrlCT(uint32_t data);
										// CT制御
	MFP *mfp;
										// MFP
	ADPCM *adpcm;
										// ADPCM
	FDD *fdd;
										// FDD
	opm_t opm;
										// OPM内部データ
	opmbuf_t bufinfo;
										// バッファ情報
	Event event[2];
										// タイマーイベント
	FM::OPM *engine;
										// 合成エンジン
	enum {
		BufMax = 0x10000				// バッファサイズ
	};
	uint32_t *opmbuf;
										// 合成バッファ
};

#endif	// opmif_h
