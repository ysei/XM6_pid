//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ RTC(RP5C15) ]
//
//---------------------------------------------------------------------------

#if !defined(rtc_h)
#define rtc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	RTC
//
//===========================================================================
class RTC : public MemDevice
{
public:
	typedef struct {
		uint32_t sec;						// 秒
		uint32_t min;						// 分
		uint32_t hour;						// 時間
		uint32_t week;						// 週の曜日
		uint32_t day;						// 日
		uint32_t month;					// 月
		uint32_t year;						// 年
		int carry;						// 秒キャリー

		int timer_en;					// タイマー動作許可
		int alarm_en;					// アラーム動作許可
		uint32_t bank;						// バンク番号
		uint32_t test;						// TESTレジスタ
		int alarm_1hz;					// 1Hzパルス出力制御
		int alarm_16hz;				// 16Hzパルス出力制御
		int under_reset;				// 秒アンダーリセット
		int alarm_reset;				// アラームリセット

		uint32_t clkout;					// CLKOUTレジスタ
		int adjust;					// アジャスト

		uint32_t alarm_min;				// 分
		uint32_t alarm_hour;				// 時間
		uint32_t alarm_week;				// 週の曜日
		uint32_t alarm_day;				// 日

		int fullhour;					// 24時間フラグ
		uint32_t leap;						// 閏年カウンタ

		int signal_1hz;				// 1Hzシグナル(500msおきに変化)
		int signal_16hz;				// 16Hzシグナル(31.25msおきに変化)
		uint32_t signal_count;				// 16Hzカウンタ(0～15)
		uint32_t signal_blink;				// 点滅シグナル(781.25msおきに変化)
		int alarm;						// アラーム信号
		int alarmout;					// ALARM OUT
	} rtc_t;

public:
	// 基本ファンクション
	RTC(VM *p);
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
	void FASTCALL GetRTC(rtc_t *buffer);
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	int FASTCALL GetTimerLED() const;
										// タイマーLED取得
	int FASTCALL GetAlarmOut() const;
										// ALARM信号取得
	int FASTCALL GetBlink(int drive) const;
										// FDD用点滅信号取得
	void FASTCALL Adjust(int alarm);
										// 現在時刻を設定

private:
	void FASTCALL AlarmOut();
										// ALARM信号出力
	void FASTCALL SecUp();
										// 秒アップ
	void FASTCALL MinUp();
										// 分アップ
	void FASTCALL AlarmCheck();
										// アラームチェック
	MFP *mfp;
										// MFP
	rtc_t rtc;
										// 内部データ
	Event event;
										// イベント
	static const uint32_t DayTable[];
										// 日付テーブル
};

#endif	// rtc_h
