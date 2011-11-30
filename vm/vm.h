//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 仮想マシン ]
//
//---------------------------------------------------------------------------

#if !defined(vm_h)
#define vm_h

#include "log.h"
#include "schedule.h"
#include "cpu.h"
#include "filepath.h"

//===========================================================================
//
//	仮想マシン
//
//===========================================================================
class VM
{
public:
	// 基本ファンクション
	VM();
										// コンストラクタ
	~VM();
	int FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用

	// ステート保存
	uint32_t FASTCALL Save(const Filepath& path);
										// セーブ
	uint32_t FASTCALL Load(const Filepath& path);
										// ロード
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	void FASTCALL Clear();
										// パスをクリア

	// デバイス管理
	void FASTCALL AddDevice(Device *device);
										// デバイス追加(子から呼ばれる)
	void FASTCALL DelDevice(const Device *device);
										// デバイス削除(子から呼ばれる)
	Device* FASTCALL GetFirstDevice() const	{ return first_device; }
										// 最初のデバイスを取得
	Device* FASTCALL SearchDevice(uint32_t id) const;
										// 任意IDのデバイスを取得

	// 実行
	int FASTCALL Exec(uint32_t hus);
										// 実行
	void FASTCALL Trace();
										// トレース
	void FASTCALL Break()				{ scheduler->Break(); }
										// 実行中止

	// バージョン
	void FASTCALL SetVersion(uint32_t major, uint32_t minor);
										// バージョン設定
	void FASTCALL GetVersion(uint32_t& major, uint32_t& minor);
										// バージョン取得

	// システム制御
	void FASTCALL PowerSW(int sw);
										// 電源スイッチ制御
	int FASTCALL IsPowerSW() const		{ return power_sw; }
										// 電源スイッチ状態取得
	void FASTCALL SetPower(int flag);
										// 電源制御
	int FASTCALL IsPower() const		{ return power; }
										// 電源状態取得
	void FASTCALL Interrupt() const		{ cpu->Interrupt(7, -1); }
										// NMI割り込み
#if defined(XM6_USE_LOG)
	Log log;
										// ログ
#endif
	void FASTCALL SetHostRtcCallback(XM6_pid::XM6_RTC_CALLBACK cb);
	int FASTCALL GetHostRtc(XM6_pid::XM6_RTC* xm6_rtc);	// take a current time from host

	void FASTCALL SetHostFileSystem(XM6_pid::XM6_FILEIO_SYSTEM* fios);
	XM6_pid::XM6_FILEIO_SYSTEM* FASTCALL GetHostFileSystem();

private:
	int status;
										// 初期化ステータス
	Device *first_device;
										// 最初のデバイス
	Scheduler *scheduler;
										// スケジューラ
	CPU *cpu;
										// CPU
	MFP *mfp;
										// MFP
	RTC *rtc;
										// RTC
	SRAM *sram;
										// SRAM
	int power_sw;
										// 電源スイッチ
	int power;
										// 電源
	uint32_t major_ver;
										// メジャーバージョン
	uint32_t minor_ver;
										// マイナーバージョン
//	Filepath current;
										// カレントデータ
	Filepath* pCurrent;
										// カレントデータ

	XM6_pid::XM6_RTC_CALLBACK	xm6_rtc_cb;
	XM6_pid::XM6_FILEIO_SYSTEM*	xm6_fios;
};

#endif	// vm_h
