//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スケジューラ ]
//
//---------------------------------------------------------------------------

#if !defined(scheduler_h)
#define scheduler_h

#include "device.h"
#include "cpu.h"

//===========================================================================
//
//	スケジューラ
//
//===========================================================================
class Scheduler : public Device
{
public:
	// ブレークポイント定義
	typedef struct{
		int use;						// 使用フラグ
		uint32_t addr;						// アドレス
		int enable;					// 有効フラグ
		uint32_t time;						// 停止時の時間
		uint32_t count;					// 停止回数
	} breakpoint_t;

	// スケジューラ定義
	typedef struct {
		// 時間
		uint32_t total;					// トータル実行時間
		uint32_t one;						// 1回の実行時間
		uint32_t sound;					// サウンド更新時間

		// CPU
		int clock;						// CPUクロック(0～5)
		uint32_t speed;					// CPU速度(clockにより決まる)
		int cycle;						// CPUサイクル数
		uint32_t time;						// CPUサイクル調整用時間

		// ブレークポイント
		int brk;						// ブレークした
		int check;						// 有効なブレークポイントあり

		// イベント
		Event *first;					// 最初のイベント
		int exec;						// イベント実行中
	} scheduler_t;

	// 個数定義
	enum {
		BreakMax = 8					// ブレークポイント総数
	};

public:
	// 基本ファンクション
	Scheduler(VM *p);
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
#if defined(_DEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// _DEBUG

	// 外部API
	void FASTCALL GetScheduler(scheduler_t *buffer) const;
										// 内部データ取得
	uint32_t FASTCALL Exec(uint32_t hus);
										// 実行
	uint32_t FASTCALL Trace(uint32_t hus);
										// トレース
	void FASTCALL Break()				{ sch.brk = TRUE; }
										// 実行中止
	void FASTCALL Wait(uint32_t cycle)		{ cpu->Wait(cycle); sch.cycle += cycle; }
										// CPUウェイト

	// 特殊操作(DMAC向け)
	int FASTCALL GetCPUCycle() const	{ return sch.cycle; }
										// ウェイト数取得
	void FASTCALL SetCPUCycle(int cycle) { sch.cycle = cycle; }
										// ウェイト数設定

	// 時間情報
	uint32_t FASTCALL GetTotalTime() const	{ return (GetPassedTime() + sch.total); }
										// トータル実行時間を取得
	uint32_t FASTCALL GetOneTime() const	{ return sch.one; }
										// 微少実行時間を取得
	uint32_t FASTCALL GetPassedTime() const;
										// 経過時間を取得
	uint32_t FASTCALL GetCPUSpeed() const	{ return sch.speed; }
										// CPU速度取得
	void FASTCALL SetCPUSpeed(uint32_t speed);
										// CPU速度設定
	uint32_t FASTCALL GetSoundTime() const	{ return sch.sound; }
										// サウンド時間を取得
	void FASTCALL SetSoundTime(uint32_t hus) { sch.sound = hus; }
										// サウンド時間を設定

	// ブレークポイント
	void FASTCALL SetBreak(uint32_t addr, int enable = TRUE);
										// ブレークポイント設定
	void FASTCALL DelBreak(uint32_t addr);
										// ブレークポイント削除
	void FASTCALL GetBreak(int index, breakpoint_t *buf) const;
										// ブレークポイント取得
	void FASTCALL EnableBreak(int index, int enable = TRUE);
										// ブレークポイント有効・無効
	void FASTCALL ClearBreak(int index);
										// ブレーク回数クリア
	void FASTCALL AddrBreak(int index, uint32_t addr);
										// ブレークアドレス変更
	int FASTCALL IsBreak(uint32_t addr, int any = FALSE) const;
										// ブレークアドレスチェック

	// イベント
	void FASTCALL AddEvent(Event *event);
										// イベント追加
	void FASTCALL DelEvent(Event *event);
										// イベント削除
	int FASTCALL HasEvent(Event *event) const;
										// イベント所有チェック
	Event* FASTCALL GetFirstEvent()	const { return sch.first; }
										// 最初のイベントを取得
	int FASTCALL GetEventNum() const;
										// イベントの個数を取得

	// 外部操作フラグ
	int dma_active;
										// DMACオートリクエスト有効

private:
	uint32_t FASTCALL GetMinRemain(uint32_t hus);
										// 最短のイベントを得る
	void FASTCALL ExecEvent(uint32_t hus);
										// イベント実行
	void FASTCALL OnBreak(uint32_t addr);
										// ブレークポイント適用

	// 内部データ
	breakpoint_t breakp[BreakMax];
										// ブレークポイント
	scheduler_t sch;
										// スケジューラ

	// デバイス
	CPU *cpu;
										// CPU
	DMAC *dmac;
										// DMAC

	// テーブル
	static const uint32_t ClockTable[];
										// クロックテーブル
	static int CycleTable[0x1000];
										// 時間(hus)→サイクル数
};

#endif	// scheduler_h
