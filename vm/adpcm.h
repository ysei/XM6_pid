//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ADPCM(MSM6258V) ]
//
//---------------------------------------------------------------------------

#if !defined(adpcm_h)
#define adpcm_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	ADPCM
//
//===========================================================================
class ADPCM : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t panpot;					// パンポット
		int play;						// 再生モード
		int rec;						// 録音モード
		int active;					// アクティブフラグ
		int started;					// 再生後有為なデータを検出
		uint32_t clock;					// 供給クロック(4 or 8)
		uint32_t ratio;					// クロック比率 (0 or 1 or 2)
		uint32_t speed;					// 進行速度(128,192,256,384,512)
		uint32_t data;						// サンプルデータ(4bit * 2sample)

		int offset;						// 合成オフセット (0-48)
		int sample;						// サンプルデータ
		int out;						// 出力データ
		int vol;						// 音量

		int enable;					// イネーブルフラグ
		int sound;						// ADPCM出力有効フラグ
		uint32_t readpoint;				// バッファ読み込みポイント
		uint32_t writepoint;				// バッファ書き込みポイント
		uint32_t number;					// バッファ有効データ数
		int wait;						// 合成ウェイト
		uint32_t sync_cnt;					// 同期カウンタ
		uint32_t sync_rate;				// 同期レート(882,960,etc...)
		uint32_t sync_step;				// 同期ステップ(線形補間対応)
		int interp;					// 補間フラグ
	} adpcm_t;

public:
	// 基本ファンクション
	ADPCM(VM *p);
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
	void FASTCALL GetADPCM(adpcm_t *buffer);
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL SetClock(uint32_t clk);
										// 基準クロック指定
	void FASTCALL SetRatio(uint32_t ratio);
										// クロック比率指定
	void FASTCALL SetPanpot(uint32_t pan);
										// パンポット指定
	void FASTCALL Enable(int enable);
										// 合成イネーブル
	void FASTCALL InitBuf(uint32_t rate);
										// バッファ初期化
	void FASTCALL GetBuf(uint32_t *buffer, int samples);
										// バッファ取得
	void FASTCALL Wait(int num);
										// ウェイト指定
	void FASTCALL EnableADPCM(int flag) { adpcm.sound = flag; }
										// 再生有効
	void FASTCALL SetVolume(int volume);
										// 音量設定
	void FASTCALL ClrStarted()			{ adpcm.started = FALSE; }
										// スタートフラグクリア
	int FASTCALL IsStarted() const		{ return adpcm.started; }
										// スタートフラグ取得

private:
	enum {
		BufMax = 0x10000				// バッファサイズ
	};
	void FASTCALL MakeTable();
										// テーブル作成
	void FASTCALL CalcSpeed();
										// 速度再計算
	void FASTCALL Start(int type);
										// 録音・再生スタート
	void FASTCALL Stop();
										// 録音・再生ストップ
	void FASTCALL Decode(int data, int num, int valid);
										// 4bitデコード
	Event event;
										// タイマーイベント
	adpcm_t adpcm;
										// 内部データ
	DMAC *dmac;
										// DMAC
	uint32_t *adpcmbuf;
										// 合成バッファ
	int DiffTable[49 * 16];
										// 差分テーブル
	static const int NextTable[16];
										// 変位テーブル
	static const int OffsetTable[58];
										// オフセットテーブル
};

#endif	// adpcm_h
