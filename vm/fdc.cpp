//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ FDC(uPD72065) ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "fdd.h"
#include "iosc.h"
#include "dmac.h"
#include "schedule.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "config.h"
#include "fdc.h"

//===========================================================================
//
//	FDC
//
//===========================================================================
//#define FDC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
FDC::FDC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('F', 'D', 'C', ' ');
	dev.desc = "FDC (uPD72065)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe94000;
	memdev.last = 0xe95fff;

	// オブジェクト
	iosc = NULL;
	dmac = NULL;
	fdd = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
int FASTCALL FDC::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// IOSC取得
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// DMAC取得
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// FDD取得
	fdd = (FDD*)vm->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(fdd);

	// イベント初期化
	event.SetDevice(this);
#if defined(XM6_USE_EVENT_DESC)
	event.SetDesc("Data Transfer");
#endif
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// 高速モードフラグ、デュアルドライブフラグ(ApplyCfg)
	fdc.fast = FALSE;
	fdc.dual = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// データレジスタ・ステータスレジスタ
	fdc.dr = 0;
	fdc.sr = 0;
	fdc.sr |= sr_rqm;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_ndm;
	fdc.sr &= ~sr_cb;

	// ドライブセレクトレジスタ・ST0-ST3
	fdc.dcr = 0;
	fdc.dsr = 0;
	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;
	fdc.st[3] = 0;

	// コマンド共通パラメータ
	fdc.srt = 1 * 2000;
	fdc.hut = 16 * 2000;
	fdc.hlt = 2 * 2000;
	fdc.hd = 0;
	fdc.us = 0;
	fdc.cyl[0] = 0;
	fdc.cyl[1] = 0;
	fdc.cyl[2] = 0;
	fdc.cyl[3] = 0;
	fdc.chrn[0] = 0;
	fdc.chrn[1] = 0;
	fdc.chrn[2] = 0;
	fdc.chrn[3] = 0;

	// その他
	fdc.eot = 0;
	fdc.gsl = 0;
	fdc.dtl = 0;
	fdc.sc = 0;
	fdc.gpl = 0;
	fdc.d = 0;
	fdc.err = 0;
	fdc.seek = FALSE;
	fdc.ndm = FALSE;
	fdc.mfm = FALSE;
	fdc.mt = FALSE;
	fdc.sk = FALSE;
	fdc.tc = FALSE;
	fdc.load = FALSE;

	// 転送系
	fdc.offset = 0;
	fdc.len = 0;
	memset(fdc.buffer, 0, sizeof(fdc.buffer));

	// フェーズ、コマンド
	fdc.phase = idle;
	fdc.cmd = no_cmd;

	// パケット管理
	fdc.in_len = 0;
	fdc.in_cnt = 0;
	memset(fdc.in_pkt, 0, sizeof(fdc.in_pkt));
	fdc.out_len = 0;
	fdc.out_cnt = 0;
	memset(fdc.out_pkt, 0, sizeof(fdc.out_pkt));

	// イベント停止
	event.SetTime(0);

	// アクセス停止(FDDもリセットでfdd.selected=0となる)
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	ソフトウェアリセット
//
//---------------------------------------------------------------------------
void FASTCALL FDC::SoftReset()
{
	// 内部レジスタ(FDC)
	fdc.dr = 0;
	fdc.sr = 0;
	fdc.sr |= sr_rqm;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_ndm;
	fdc.sr &= ~sr_cb;

	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;
	fdc.st[3] = 0;

	fdc.srt = 1 * 2000;
	fdc.hut = 16 * 2000;
	fdc.hlt = 2 * 2000;
	fdc.hd = 0;
	fdc.us = 0;
	fdc.cyl[0] = 0;
	fdc.cyl[1] = 0;
	fdc.cyl[2] = 0;
	fdc.cyl[3] = 0;
	fdc.chrn[0] = 0;
	fdc.chrn[1] = 0;
	fdc.chrn[2] = 0;
	fdc.chrn[3] = 0;

	fdc.eot = 0;
	fdc.gsl = 0;
	fdc.dtl = 0;
	fdc.sc = 0;
	fdc.gpl = 0;
	fdc.d = 0;
	fdc.err = 0;
	fdc.seek = FALSE;
	fdc.ndm = FALSE;
	fdc.mfm = FALSE;
	fdc.mt = FALSE;
	fdc.sk = FALSE;
	fdc.tc = FALSE;
	fdc.load = FALSE;

	fdc.offset = 0;
	fdc.len = 0;

	// フェーズ、コマンド
	fdc.phase = idle;
	fdc.cmd = fdc_reset;

	// パケット管理
	fdc.in_len = 0;
	fdc.in_cnt = 0;
	memset(fdc.in_pkt, 0, sizeof(fdc.in_pkt));
	fdc.out_len = 0;
	fdc.out_cnt = 0;
	memset(fdc.out_pkt, 0, sizeof(fdc.out_pkt));

	// イベント停止
	event.SetTime(0);

	// アクセス停止
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
int FASTCALL FDC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(fdc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 本体をセーブ
	if (!fio->Write(&fdc, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
int FASTCALL FDC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、比較
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(fdc_t)) {
		return FALSE;
	}

	// 本体をロード
	if (!fio->Read(&fdc, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);

	LOG0(Log::Normal, "設定適用");

	// 高速モード
	fdc.fast = config->floppy_speed;
#if defined(FDC_LOG)
	if (fdc.fast) {
		LOG0(Log::Normal, "高速モード ON");
	}
	else {
		LOG0(Log::Normal, "高速モード OFF");
	}
#endif	// FDC_LOG

	// 2DD/2HD兼用ドライブ
	fdc.dual = config->dual_fdd;
#if defined(FDC_LOG)
	if (fdc.dual) {
		LOG0(Log::Normal, "2DD/2HD兼用ドライブ");
	}
	else {
		LOG0(Log::Normal, "2HD専用ドライブ");
	}
#endif	// FDC_LOG
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL FDC::ReadByte(uint32_t addr)
{
	int i;
	int status;
	uint32_t bit;
	uint32_t data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8バイト単位でループ
	addr &= 0x07;
	addr >>= 1;

	// ウェイト
	scheduler->Wait(1);

	switch (addr) {
		// ステータスレジスタ
		case 0:
			return fdc.sr;

		// データレジスタ
		case 1:
			// SEEK完了割り込みでなければ、割り込みネゲート
			if (!fdc.seek) {
				Interrupt(FALSE);
			}

			switch (fdc.phase) {
				// 実行フェーズ(ER)
				case read:
					fdc.sr &= ~sr_rqm;
					return Read();

				// リザルトフェーズ
				case result:
					ASSERT(fdc.out_cnt >= 0);
					ASSERT(fdc.out_cnt < 0x10);
					ASSERT(fdc.out_len > 0);

					// パケットからデータを取り出す
					data = fdc.out_pkt[fdc.out_cnt];
					fdc.out_cnt++;
					fdc.out_len--;

					// 残りレングスが0になったら、アイドルフェーズへ
					if (fdc.out_len == 0) {
						Idle();
					}
					return data;
			}
			LOG0(Log::Warning, "FDCデータレジスタ読み込み無効");
			return 0xff;

		// ドライブステータスレジスタ
		case 2:
			data = 0;
			bit = 0x08;
			for (i=3; i>=0; i--) {
				// DCRのビットが立っているか
				if ((fdc.dcr & bit) != 0) {
					// 該当ドライブのステータスをOR(b7,b6のみ)
					status = fdd->GetStatus(i);
					data |= (uint32_t)(status & 0xc0);
				}
				bit >>= 1;
			}

			// FDD割り込みを落とす(FDC割り込みではない、注意)
			iosc->IntFDD(FALSE);
			return data;

		// ドライブセレクトレジスタ
		case 3:
			LOG0(Log::Warning, "ドライブセレクトレジスタ読み込み");
			return 0xff;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
uint32_t FASTCALL FDC::ReadWord(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteByte(uint32_t addr, uint32_t data)
{
	int i;
	uint32_t bit;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return;
	}

	// 8バイト単位でループ
	addr &= 0x07;
	addr >>= 1;

	// ウェイト
	scheduler->Wait(1);

	switch (addr) {
		// 特殊コマンドレジスタ
		case 0:
			switch (data) {
				// RESET STANDBY
				case 0x34:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "RESET STANDBYコマンド");
#endif	// FDC_LOG
					fdc.cmd = reset_stdby;
					Result();
					return;
				// SET STANDBY
				case 0x35:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "SET STANDBYコマンド");
#endif	// FDC_LOG
					fdc.cmd = set_stdby;
					Idle();
					return;
				// SOFTWARE RESET
				case 0x36:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "SOFTWARE RESETコマンド");
#endif	// FDC_LOG
					SoftReset();
					return;
			}
			LOG1(Log::Warning, "無効な特殊コマンド書き込み %02X", data);
			return;

		// データレジスタ
		case 1:
			// SEEK完了割り込みでなければ、割り込みネゲート
			if (!fdc.seek) {
				Interrupt(FALSE);
			}

			switch (fdc.phase) {
				// アイドルフェーズ
				case idle:
					Command(data);
					return;

				// コマンドフェーズ
				case command:
					ASSERT(fdc.in_cnt >= 0);
					ASSERT(fdc.in_cnt < 0x10);
					ASSERT(fdc.in_len > 0);

					// パケットにデータをセット
					fdc.in_pkt[fdc.in_cnt] = data;
					fdc.in_cnt++;
					fdc.in_len--;

					// 残りレングスが0になったら、実行フェーズへ
					if (fdc.in_len == 0) {
						Execute();
					}
					return;

				// 実行フェーズ(EW)
				case write:
					fdc.sr &= ~sr_rqm;
					Write(data);
					return;
			}
			LOG1(Log::Warning, "FDCデータレジスタ書き込み無効 $%02X", data);
			return;

		// ドライブコントロールレジスタ
		case 2:
			// 下位4bitが1→0になったところを調べる
			bit = 0x01;
			for (i=0; i<4; i++) {
				if ((fdc.dcr & bit) != 0) {
					if ((data & bit) == 0) {
						// 1→0のエッジで、DCRの上位4ビットを適用
						fdd->Control(i, fdc.dcr);
					}
				}
				bit <<= 1;
			}

			// 値を保存
			fdc.dcr = data;
			return;

		// ドライブセレクトレジスタ
		case 3:
			// 下位2bitでアクセスドライブ選択
			fdc.dsr = (uint32_t)(data & 0x03);

			// 最上位でモータ制御
			if (data & 0x80) {
				fdd->SetMotor(fdc.dsr, TRUE);
			}
			else {
				fdd->SetMotor(fdc.dsr, FALSE);
			}

			// 2HD/2DD切り替え(ドライブが2DD対応でなければ無効)
			if (fdc.dual) {
				if (data & 0x10) {
					fdd->SetHD(FALSE);
				}
				else {
					fdd->SetHD(TRUE);
				}
			}
			else {
				fdd->SetHD(TRUE);
			}
			return;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteWord(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	WriteByte(addr + 1, (uint8_t)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
uint32_t FASTCALL FDC::ReadOnly(uint32_t addr) const
{
	int i;
	int status;
	uint32_t bit;
	uint32_t data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8バイト単位でループ
	addr &= 0x07;
	addr >>= 1;

	switch (addr) {
		// ステータスレジスタ
		case 0:
			return fdc.sr;

		// データレジスタ
		case 1:
			if (fdc.phase == result) {
				// パケットからデータを取り出す(更新しない);
				return fdc.out_pkt[fdc.out_cnt];
			}
			return 0xff;

		// ドライブステータスレジスタ
		case 2:
			data = 0;
			bit = 0x08;
			for (i=3; i>=0; i--) {
				// DCRのビットが立っているか
				if ((fdc.dcr & bit) != 0) {
					// 該当ドライブのステータスをOR(b7,b6のみ)
					status = fdd->GetStatus(i);
					data |= (uint32_t)(status & 0xc0);
				}
				bit >>= 1;
			}
			return data;

		// ドライブセレクトレジスタ
		case 3:
			return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
int FASTCALL FDC::Callback(Event *ev)
{
	int i;
	int thres;

	ASSERT(this);
	ASSERT(ev);

	// アイドルフェーズはヘッドアンロード
	if (fdc.phase == idle) {
		fdc.load = FALSE;

		// 単発
		return FALSE;
	}

	// ヘッドロード
	fdc.load = TRUE;

	// 実行フェーズ
	if (fdc.phase == execute) {
		// IDもしくはNO DATAを見つけるまでの時間
		Result();

		// 単発
		return FALSE;
	}

	// Write IDは専用処理
	if (fdc.cmd == write_id) {
		ASSERT(fdc.len > 0);
		ASSERT((fdc.len & 3) == 0);

		// 時間再設定
		if (fdc.fast) {
			ev->SetTime(32 * 4);
		}
		else {
			ev->SetTime(fdd->GetRotationTime() / fdc.sc);
		}

		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		return TRUE;
	}

	// Read(Del)Data/Write(Del)Data/Scan/ReadDiag。時間再設定
	EventRW();

	// データ転送リクエスト
	fdc.sr |= sr_rqm;

	// データ転送
	if (!fdc.ndm) {
		// DMAモード(DMAリクエスト)。64バイトまとめて行う
		if (fdc.fast) {
			// 1回のイベントで、余剰CPUパワーの2/3だけ転送する
			thres = (int)scheduler->GetCPUSpeed();
			thres <<= 1;
			thres /= 3;

			// リザルトフェーズに入るまで繰り返す
			while (fdc.phase != result) {
				// CPUパワーを見ながら途中で打ち切る
				if (scheduler->GetCPUCycle() > thres) {
					break;
				}

				// 転送
				dmac->ReqDMA(0);
			}
		}
		else {
			// 通常。64バイトまとめて
			for (i=0; i<64; i++) {
				if (fdc.phase == result) {
					break;
				}
				dmac->ReqDMA(0);
			}
		}
		return TRUE;
	}

	// Non-DMAモード(割り込みリクエスト)
	Interrupt(TRUE);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	内部ワークアドレス取得
//
//---------------------------------------------------------------------------
const FDC::fdc_t* FASTCALL FDC::GetWork() const
{
	ASSERT(this);

	// アドレスを与える(fdc.bufferが大きいため)
	return &fdc;
}

//---------------------------------------------------------------------------
//
//	シーク完了
//
//---------------------------------------------------------------------------
void FASTCALL FDC::CompleteSeek(int drive, int status)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDC_LOG)
	if (status) {
		LOG2(Log::Normal, "シーク成功 ドライブ%d シリンダ%02X",
					drive, fdd->GetCylinder(drive));
	}
	else {
		LOG2(Log::Normal, "シーク失敗 ドライブ%d シリンダ%02X",
					drive, fdd->GetCylinder(drive));
	}
#endif	// FDC_LOG

	// recalibrateまたはseekのみ有効
	if ((fdc.cmd == recalibrate) || (fdc.cmd == seek)) {
		// ST0作成(ただしUSのみ)
		fdc.st[0] = fdc.us;

		// ステータス判別
		if (status) {
			// ドライブ2,3はECを立てる
			if (drive <= 1) {
				// Seek End
				fdc.st[0] |= 0x20;
			}
			else {
				// Equipment Check, Attention Interrupt
				fdc.st[0] |= 0x10;
				fdc.st[0] |= 0xc0;
			}
		}
		else {
			if (drive <= 1) {
				// Seek End
				fdc.st[0] |= 0x20;
			}
			// Not Ready, Abnormal Terminate
			fdc.st[0] |= 0x08;
			fdc.st[0] |= 0x40;
		}

		// SEEK完了割り込み
		Interrupt(TRUE);
		fdc.seek = TRUE;
		Idle();
		return;
	}

	LOG1(Log::Warning, "無効なシーク完了通知 ドライブ%d", drive);
}

//---------------------------------------------------------------------------
//
//	TCアサート
//
//---------------------------------------------------------------------------
void FASTCALL FDC::SetTC()
{
	ASSERT(this);

	// アイドルフェーズでクリアするため、アイドルフェーズ以外なら可
	if (fdc.phase != idle) {
		fdc.tc = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	アイドルフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Idle()
{
	ASSERT(this);

	// フェーズ設定
	fdc.phase = idle;
	fdc.err = 0;
	fdc.tc = FALSE;

	// イベント終了
	event.SetTime(0);

	// ヘッドロード状態なら、アンロードのためのイベントを設定
	if (fdc.load) {
		// アンロードの必要あり
		if (fdc.hut > 0) {
			event.SetTime(fdc.hut);
		}
	}

	// ステータスレジスタはコマンド待ち
	fdc.sr = sr_rqm;

	// アクセス終了
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	コマンドフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Command(uint32_t data)
{
	uint32_t mask;

	ASSERT(this);
	ASSERT(data < 0x100);

	// コマンドフェーズ(FDC BUSY)
	fdc.phase = command;
	fdc.sr |= sr_cb;

	// 入力パケット初期化
	fdc.in_pkt[0] = data;
	fdc.in_cnt = 1;
	fdc.in_len = 0;

	// マスク(1)
	mask = data;

	// FDCリセットはいつでも実行できる
	switch (mask) {
		// RESET STANDBY
		case 0x34:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "RESET STANDBYコマンド");
#endif	// FDC_LOG
			fdc.cmd = reset_stdby;
			Result();
			return;

		// SET STANDBY
		case 0x35:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SET STANDBYコマンド");
#endif	// FDC_LOG
			fdc.cmd = set_stdby;
			Idle();
			return;

		// SOFTWARE RESET
		case 0x36:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SOFTWARE RESETコマンド");
#endif	// FDC_LOG
			SoftReset();
			return;
	}

	// シーク系コマンド実行直後は、SENSE INTERRUPT STATUS以外許されない
	if (fdc.seek) {
		// SENSE INTERRUPT STATUS
		if (mask == 0x08) {
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SENSE INTERRUPT STATUSコマンド");
#endif	// FDC_LOG
			fdc.cmd = sense_int_stat;

			// 割り込みネゲート
			fdc.sr &= 0xf0;
			fdc.seek = FALSE;
			Interrupt(FALSE);

			// パラメータなし、実行フェーズなし
			Result();
			return;
		}

		// それ以外は全て無効コマンド
#if defined(FDC_LOG)
		LOG0(Log::Normal, "INVALIDコマンド");
#endif	// FDC_LOG
		fdc.cmd = invalid;
		Result();
		return;
	}

	// SENSE INTERRUPT STATUS(無効)
	if (mask == 0x08) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "INVALIDコマンド");
#endif	// FDC_LOG
		fdc.cmd = invalid;
		Result();
		return;
	}

	// ステータスをクリア
	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;

	// 通常
	switch (mask) {
		// READ DIAGNOSTIC(FMモード)
		case 0x02:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ DIAGNOSTICコマンド(FMモード)");
#endif	// FDC_LOG
			CommandRW(read_diag, data);
			return;

		// SPECIFY
		case 0x03:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SPECIFYコマンド");
#endif	// FDC_LOG
			fdc.cmd = specify;
			fdc.in_len = 2;
			return;

		// SENSE DEVICE STATUS
		case 0x04:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SENSE DEVICE STATUSコマンド");
#endif	// FDC_LOG
			fdc.cmd = sense_dev_stat;
			fdc.in_len = 1;
			return;

		// RECALIBRATE
		case 0x07:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "RECALIBRATEコマンド");
#endif	// FDC_LOG
			fdc.cmd = recalibrate;
			fdc.in_len = 1;
			return;

		// READ ID(FMモード)
		case 0x0a:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ IDコマンド(FMモード)");
#endif	// FDC_LOG
			fdc.cmd = read_id;
			fdc.mfm = FALSE;
			fdc.in_len = 1;
			return;

		// WRITE ID(FMモード)
		case 0x0d:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "WRITE IDコマンド(FMモード)");
#endif	// FDC_LOG
			fdc.cmd = write_id;
			fdc.mfm = FALSE;
			fdc.in_len = 5;
			return;

		// SEEK
		case 0x0f:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SEEKコマンド");
#endif	// FDC_LOG
			fdc.cmd = seek;
			fdc.in_len = 2;
			return;

		// READ DIAGNOSTIC(MFMモード)
		case 0x42:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ DIAGNOSTICコマンド(MFMモード)");
#endif	// FDC_LOG
			CommandRW(read_diag, data);
			return;

		// READ ID(MFMモード)
		case 0x4a:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ IDコマンド(MFMモード)");
#endif	// FDC_LOG
			fdc.cmd = read_id;
			fdc.mfm = TRUE;
			fdc.in_len = 1;
			return;

		// WRITE ID(MFMモード)
		case 0x4d:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "WRITE IDコマンド(MFMモード)");
#endif	// FDC_LOG
			fdc.cmd = write_id;
			fdc.mfm = TRUE;
			fdc.in_len = 5;
			return;
	}

	// マスク(2)
	mask &= 0x3f;

	// WRITE DATA
	if (mask == 0x05) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "WRITE DATAコマンド");
#endif	// FDC_LOG
		CommandRW(write_data, data);
		return;
	}

	// WRITE DELETED DATA
	if (mask == 0x09) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "WRITE DELETED DATAコマンド");
#endif	// FDC_LOG
		CommandRW(write_del_data, data);
		return;
	}

	// マスク(3);
	mask &= 0x1f;

	// READ DATA
	if (mask == 0x06) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "READ DATAコマンド");
#endif	// FDC_LOG
		CommandRW(read_data, data);
		return;
	}

	// READ DELETED DATA
	if (mask == 0x0c) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "READ DELETED DATAコマンド");
#endif	// FDC_LOG
		CommandRW(read_data, data);
		return;
	}

	// SCAN EQUAL
	if (mask == 0x11) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN EQUALコマンド");
#endif	// FDC_LOG
		CommandRW(scan_eq, data);
		return;
	}

	// SCAN LOW OR EQUAL
	if (mask == 0x19) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN LOW OR EQUALコマンド");
#endif	// FDC_LOG
		CommandRW(scan_lo_eq, data);
		return;
	}

	// SCAN HIGH OR EQUAL
	if (mask == 0x1d) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN HIGH OR EQUALコマンド");
#endif	// FDC_LOG
		CommandRW(scan_hi_eq, data);
		return;
	}

	// 未実装
	LOG1(Log::Warning, "コマンドフェーズ未対応コマンド $%02X", data);
	Idle();
}

//---------------------------------------------------------------------------
//
//	コマンドフェーズ(Read/Write系)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::CommandRW(fdccmd cmd, uint32_t data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// コマンド
	fdc.cmd = cmd;

	// MT
	if (data & 0x80) {
		fdc.mt = TRUE;
	}
	else {
		fdc.mt = FALSE;
	}

	// MFM
	if (data & 0x40) {
		fdc.mfm = TRUE;
	}
	else {
		fdc.mfm = FALSE;
	}

	// SK(READ/SCANのみ)
	if (data & 0x20) {
		fdc.sk = TRUE;
	}
	else {
		fdc.sk = FALSE;
	}

	// コマンドフェーズの残りバイト数
	fdc.in_len = 8;
}

//---------------------------------------------------------------------------
//
//	実行フェーズ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Execute()
{
	ASSERT(this);

	// 実行フェーズへ
	fdc.phase = execute;

	// アクセス開始、イベント停止
	fdd->Access(TRUE);
	event.SetTime(0);

	// コマンド別
	switch (fdc.cmd) {
		// SPECIFY
		case specify:
			// SRT
			fdc.srt = (fdc.in_pkt[1] >> 4) & 0x0f;
			fdc.srt = 16 - fdc.srt;
			fdc.srt <<= 11;

			// HUT (0は16と同じ扱い。i82078データシートによる)
			fdc.hut = fdc.in_pkt[1] & 0x0f;
			if (fdc.hut == 0) {
				fdc.hut = 16;
			}
			fdc.hut <<= 15;

			// HLT (0はHUTと同様)
			fdc.hlt = (fdc.in_pkt[2] >> 1) & 0x7f;
			if (fdc.hlt == 0) {
				fdc.hlt = 0x80;
			}
			fdc.hlt <<= 12;

			// NDM
			if (fdc.in_pkt[2] & 1) {
				fdc.ndm = TRUE;
				LOG0(Log::Warning, "Non-DMAモードに設定");
			}
			else {
				fdc.ndm = FALSE;
			}

			// リザルトフェーズ不要
			Idle();
			return;

		// SENSE DEVICE STATUS
		case sense_dev_stat:
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.hd = fdc.in_pkt[1] & 0x04;

			// リザルトフェーズ
			Result();
			return;

		// RECALIBRATE
		case recalibrate:
			// トラック0へシーク
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.cyl[fdc.us] = 0;

			// SR作成(SEEK系コマンド実行中はNon-Busy)
			fdc.sr &= 0xf0;
			fdc.sr &= ~sr_cb;
			fdc.sr &= ~sr_rqm;
			fdc.sr |= (1 << fdc.dsr);

			// 最後に実行を呼ぶ(内部でCompleteSeekが呼ばれるため)
			fdd->Recalibrate(fdc.srt);
			return;

		// SEEK
		case seek:
			fdc.us = fdc.in_pkt[1] & 0x03;

			// SR作成(SEEK系コマンド実行中はNon-Busy)
			fdc.sr &= 0xf0;
			fdc.sr &= ~sr_cb;
			fdc.sr &= ~sr_rqm;
			fdc.sr |= (1 << fdc.dsr);

			// 最後に実行を呼ぶ(内部でCompleteSeekが呼ばれるため)
			if (fdc.cyl[fdc.us] < fdc.in_pkt[2]) {
				// ステップイン
				fdd->StepIn(fdc.in_pkt[2] - fdc.cyl[fdc.us], fdc.srt);
			}
			else {
				// ステップアウト
				fdd->StepOut(fdc.cyl[fdc.us] - fdc.in_pkt[2], fdc.srt);
			}
			fdc.cyl[fdc.us] = fdc.in_pkt[2];
			return;

		// READ ID
		case read_id:
			ReadID();
			return;

		// WRITE ID
		case write_id:
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.hd = fdc.in_pkt[1] & 0x04;
			fdc.st[0] = fdc.us;
			fdc.st[0] |= fdc.hd;
			fdc.chrn[3] = fdc.in_pkt[2];
			fdc.sc = fdc.in_pkt[3];
			fdc.gpl = fdc.in_pkt[4];
			fdc.d = fdc.in_pkt[5];
			if (!WriteID()) {
				Result();
			}
			return;

		// READ DIAGNOSTIC
		case read_diag:
			ExecuteRW();
			if (!ReadDiag()) {
				Result();
			}
			return;

		// READ DATA
		case read_data:
			ExecuteRW();
			if (!ReadData()) {
				Result();
			}
			return;

		// READ DELETED DATA
		case read_del_data:
			ExecuteRW();
			if (!ReadData()) {
				Result();
			}
			return;

		// WRITE DATA
		case write_data:
			ExecuteRW();
			if (!WriteData()) {
				Result();
			}
			return;

		// WRITE DELETED_DATA
		case write_del_data:
			ExecuteRW();
			if (!WriteData()) {
				Result();
			}
			return;

		// SCAN系
		case scan_eq:
		case scan_lo_eq:
		case scan_hi_eq:
			ExecuteRW();
			if (!Scan()) {
				Result();
			}
			return;
	}

	LOG1(Log::Warning, "実行フェーズ未対応コマンド $%02X", fdc.in_pkt[0]);
}

//---------------------------------------------------------------------------
//
//	実行フェーズ(ReadID)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ReadID()
{
	uint32_t hus;

	ASSERT(this);

	// HD, USを記憶
	fdc.us = fdc.in_pkt[1] & 0x03;
	fdc.hd = fdc.in_pkt[1] & 0x04;

	// FDDに実行させる。NOTREADY, NODATA, MAMが考えられる
	fdc.err = fdd->ReadID(&(fdc.out_pkt[3]), fdc.mfm, fdc.hd);

	// NOT READYならすぐリザルトフェーズ
	if (fdc.err & FDD_NOTREADY) {
		Result();
		return;
	}

	// 検索にかかる時間を設定
	hus = fdd->GetSearch();
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
}

//---------------------------------------------------------------------------
//
//	実行フェーズ(Read/Write系)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ExecuteRW()
{
	ASSERT(this);

	// 8バイトのパケットを分割(最終バイトは常にDTLにセット)
	fdc.us = fdc.in_pkt[1] & 0x03;
	fdc.hd = fdc.in_pkt[1] & 0x04;
	fdc.st[0] = fdc.us;
	fdc.st[0] |= fdc.hd;

	fdc.chrn[0] = fdc.in_pkt[2];
	fdc.chrn[1] = fdc.in_pkt[3];
	fdc.chrn[2] = fdc.in_pkt[4];
	fdc.chrn[3] = fdc.in_pkt[5];

	fdc.eot = fdc.in_pkt[6];
	fdc.gsl = fdc.in_pkt[7];
	fdc.dtl = fdc.in_pkt[8];
}

//---------------------------------------------------------------------------
//
//	実行フェーズ(Read)
//
//---------------------------------------------------------------------------
uint8_t FASTCALL FDC::Read()
{
	uint8_t data;

	ASSERT(fdc.len > 0);
	ASSERT(fdc.offset < 0x4000);

	// バッファからデータを供給
	data = fdc.buffer[fdc.offset];
	fdc.offset++;
	fdc.len--;

	// 最後でなければそのまま続ける
	if (fdc.len > 0) {
		return data;
	}

	// READ DIAGNOSTICの場合はここで終了
	if (fdc.cmd == read_diag) {
		// 正常終了なら、セクタを進める
		if (fdc.err == FDD_NOERROR) {
			NextSector();
		}
		// イベントを打ち切り、リザルトフェーズへ
		event.SetTime(0);
		Result();
		return data;
	}

	// 異常終了なら、このセクタで打ち切り
	if (fdc.err != FDD_NOERROR) {
		// イベントを打ち切り、リザルトフェーズへ
		event.SetTime(0);
		Result();
		return data;
	}

	// マルチセクタ処理
	if (!NextSector()) {
		// イベントを打ち切り、リザルトフェーズへ
		event.SetTime(0);
		Result();
		return data;
	}

	// 次のセクタがあるので、準備
	if (!ReadData()) {
		// セクタ読み取り不能
		event.SetTime(0);
		Result();
		return data;
	}

	// OK、次のセクタへ
	return data;
}

//---------------------------------------------------------------------------
//
//	実行フェーズ(Write)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Write(uint32_t data)
{
	ASSERT(this);
	ASSERT(fdc.len > 0);
	ASSERT(fdc.offset < 0x4000);
	ASSERT(data < 0x100);

	// WRITE IDの場合はバッファに溜めるのみ
	if (fdc.cmd == write_id) {
		fdc.buffer[fdc.offset] = (uint8_t)data;
		fdc.offset++;
		fdc.len--;

		// 終了チェック
		if (fdc.len == 0) {
			WriteBack();
			event.SetTime(0);
			Result();
		}
		return;
	}

	// スキャン系の場合は比較
	if ((fdc.cmd == scan_eq) || (fdc.cmd == scan_lo_eq) || (fdc.cmd == scan_hi_eq)) {
		Compare(data);
		return;
	}

	// バッファへデータを書き込む
	fdc.buffer[fdc.offset] = (uint8_t)data;
	fdc.offset++;
	fdc.len--;

	// 最後でなければそのまま続ける
	if (fdc.len > 0) {
		return;
	}

	// 書き込み終了
	WriteBack();
	if (fdc.err != FDD_NOERROR) {
		event.SetTime(0);
		Result();
		return;
	}

	// マルチセクタ処理
	if (!NextSector()) {
		// イベントを打ち切り、リザルトフェーズへ
		event.SetTime(0);
		Result();
		return;
	}

	// 次のセクタがあるので、準備
	if (!WriteData()) {
		event.SetTime(0);
		Result();
	}
}

//---------------------------------------------------------------------------
//
//	実行フェーズ(Compare)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Compare(uint32_t data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	if (data != 0xff) {
		// 有効バイトで、まだ判定出てないなら
		if (!(fdc.err & FDD_SCANNOT)) {
			// 比較が必要
			switch (fdc.cmd) {
				case scan_eq:
					if (fdc.buffer[fdc.offset] != (uint8_t)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				case scan_lo_eq:
					if (fdc.buffer[fdc.offset] > (uint8_t)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				case scan_hi_eq:
					if (fdc.buffer[fdc.offset] < (uint8_t)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				default:
					ASSERT(FALSE);
					break;
			}

		}
	}

	// 次のデータへ
	fdc.offset++;
	fdc.len--;

	// 最後でなければそのまま続ける
	if (fdc.len > 0) {
		return;
	}

	// 最後なので、結果まとめ
	if (!(fdc.err & FDD_SCANNOT)) {
		// ok!
		fdc.err |= FDD_SCANEQ;
		event.SetTime(0);
		Result();
	}

	// STPが2のときは、+1
	if (fdc.dtl == 0x02) {
		fdc.chrn[2]++;
	}

	// マルチセクタ処理
	if (!NextSector()) {
		// SCAN NOTは上がったままなので都合がよい
		event.SetTime(0);
		Result();
		return;
	}

	// 次のセクタがあるので、準備
	if (!Scan()) {
		// SCAN NOTは上がったままなので都合がよい
		event.SetTime(0);
		Result();
	}

	// SCAN NOTを下げてもう１セクタ
	fdc.err &= ~FDD_SCANNOT;
}

//---------------------------------------------------------------------------
//
//	リザルトフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Result()
{
	ASSERT(this);

	// リザルトフェーズ
	fdc.phase = result;
	fdc.sr |= sr_rqm;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_ndm;

	// コマンド別
	switch (fdc.cmd) {
		// SENSE DEVICE STATUS
		case sense_dev_stat:
			// ST3を作成、データ転送
			MakeST3();
			fdc.out_pkt[0] = fdc.st[3];
			fdc.out_len = 1;
			fdc.out_cnt = 0;
			return;

		// SENSE INTERRUPT STATUS
		case sense_int_stat:
			// ST0・シリンダを返す。データ転送
			fdc.out_pkt[0] = fdc.st[0];
			fdc.out_pkt[1] = fdc.cyl[fdc.us];
			fdc.out_len = 2;
			fdc.out_cnt = 0;
			return;

		// READ ID
		case read_id:
			// ST0,ST1,ST2作成。NOTREADY, NODATA, MAMが考えられる
			fdc.st[0] = fdc.us;
			fdc.st[0] |= fdc.hd;
			if (fdc.err & FDD_NOTREADY) {
				// Not Ready
				fdc.st[0] |= 0x08;
				fdc.st[1] = 0;
				fdc.st[2] = 0;
			}
			else {
				if (fdc.err != FDD_NOERROR) {
					// Abnormal Teriminate
					fdc.st[0] |= 0x40;
				}
				fdc.st[1] = fdc.err >> 8;
				fdc.st[2] = fdc.err & 0xff;
			}

			// データ転送、リザルトフェーズ割り込み
			fdc.out_pkt[0] = fdc.st[0];
			fdc.out_pkt[1] = fdc.st[1];
			fdc.out_pkt[2] = fdc.st[2];
			fdc.out_len = 7;
			fdc.out_cnt = 0;
			Interrupt(TRUE);
			return;

		// INVALID, RESET STANDBY
		case invalid:
		case reset_stdby:
			fdc.out_pkt[0] = 0x80;
			fdc.out_len = 1;
			fdc.out_cnt = 0;
			return;

		// READ,WRITE,SCAN系
		case read_data:
		case read_del_data:
		case write_data:
		case write_del_data:
		case scan_eq:
		case scan_lo_eq:
		case scan_hi_eq:
		case read_diag:
		case write_id:
			ResultRW();
			return;
	}

	LOG1(Log::Warning, "リザルトフェーズ未対応コマンド $%02X", fdc.in_pkt[0]);
}

//---------------------------------------------------------------------------
//
//	リザルトフェーズ(Read/Write系)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ResultRW()
{
	ASSERT(this);

	// ST0,ST1,ST2作成
	if (fdc.err & FDD_NOTREADY) {
		// Not Ready
		fdc.st[0] |= 0x08;
		fdc.st[1] = 0;
		fdc.st[2] = 0;
	}
	else {
		if ((fdc.err != FDD_NOERROR) && (fdc.err != FDD_SCANEQ)) {
			// Abnormal Teriminate
			fdc.st[0] |= 0x40;
		}
		fdc.st[1] = fdc.err >> 8;
		fdc.st[2] = fdc.err & 0xff;
	}

	// READ DIAGNOSTICは0x40を出さない
	if (fdc.cmd == read_diag) {
		if (fdc.st[0] & 0x40) {
			fdc.st[0] &= ~0x40;
		}
	}

	// リザルトパケットを設定
	fdc.out_pkt[0] = fdc.st[0];
	fdc.out_pkt[1] = fdc.st[1];
	fdc.out_pkt[2] = fdc.st[2];
	fdc.out_pkt[3] = fdc.chrn[0];
	fdc.out_pkt[4] = fdc.chrn[1];
	fdc.out_pkt[5] = fdc.chrn[2];
	fdc.out_pkt[6] = fdc.chrn[3];
	fdc.out_len = 7;
	fdc.out_cnt = 0;

	// 通常はリザルトフェーズ割り込み
	Interrupt(TRUE);
}

//---------------------------------------------------------------------------
//
//	割り込み
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Interrupt(int flag)
{
	ASSERT(this);

	// IOSCに通知
	iosc->IntFDC(flag);
}

//---------------------------------------------------------------------------
//
//	ST3作成
//
//---------------------------------------------------------------------------
void FASTCALL FDC::MakeST3()
{
	ASSERT(this);

	// HD,USをセット
	fdc.st[3] = fdc.hd;
	fdc.st[3] |= fdc.us;

	// レディか
	if (fdd->IsReady(fdc.dsr)) {
		// レディ
		fdc.st[3] |= 0x20;

		// ライトプロテクトか
		if (fdd->IsWriteP(fdc.dsr)) {
			fdc.st[3] |= 0x40;
		}
	}
	else {
		// レディでない
		fdc.st[3] = 0x40;
	}

	// TRACK0か
	if (fdd->GetCylinder(fdc.dsr) == 0) {
		fdc.st[3] |= 0x10;
	}
}

//---------------------------------------------------------------------------
//
//	READ (DELETED) DATAコマンド
//
//---------------------------------------------------------------------------
int FASTCALL FDC::ReadData()
{
	int len;
	uint32_t hus;

	ASSERT(this);
	ASSERT((fdc.cmd == read_data) || (fdc.cmd == read_del_data));

	// SR設定
	fdc.sr |= sr_cb;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// ドライブに任せる。NOTREADY,NODATA,MAM,CYL系,CRC系,DDAM
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->ReadSector(fdc.buffer, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd);

	// DDAM(Deleted Sector)の有無で、CM(Control Mark)を決める
	if (fdc.cmd == read_data) {
		// Read Data (DDAMはエラー)
		if (fdc.err & FDD_DDAM) {
			fdc.err &= ~FDD_DDAM;
			fdc.err |= FDD_CM;
		}
	}
	else {
		// Read Deleted Data (DDAMでなければエラー)
		if (!(fdc.err & FDD_DDAM)) {
			fdc.err |= FDD_CM;
		}
		fdc.err &= ~FDD_DDAM;
	}

	// IDCRCまたはDATACRCなら、DATAERRを乗せる
	if ((fdc.err & FDD_IDCRC) || (fdc.err & FDD_DATACRC)) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0ではNとしてDTLを使う
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		// (Macエミュレータ)
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Readyはリザルトフェーズへ
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// CMはSK=1ならリザルトフェーズへ(i82078データシートによる)
	if (fdc.err & FDD_CM) {
		if (fdc.sk) {
			return FALSE;
		}
	}

	// 検索時間を計算(ヘッドロードは考えない)
	hus = fdd->GetSearch();

	// No Data時はこの時間後、リザルトフェーズへ
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// オフセット初期化、イベントスタート、ERフェーズ開始
	fdc.offset = 0;
	EventRW();
	fdc.phase = read;

	// 検索時間がヘッドロード時間より短ければ、１周待ち
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// 時間を加算
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WRITE (DELETED) DATAコマンド
//
//---------------------------------------------------------------------------
int FASTCALL FDC::WriteData()
{
	int len;
	uint32_t hus;
	int deleted;

	ASSERT(this);
	ASSERT((fdc.cmd == write_data) || (fdc.cmd == write_del_data));

	// SR設定
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// ドライブに任せる。NOTREADY,NOTWRITE,NODATA,MAM,CYL系,IDCRC,DDAM
	deleted = FALSE;
	if (fdc.cmd == write_del_data) {
		deleted = TRUE;
	}
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->WriteSector(NULL, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd, deleted);
	fdc.err &= ~FDD_DDAM;

	// IDCRCなら、DATAERRを乗せる
	if (fdc.err & FDD_IDCRC) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0ではNとしてDTLを使う
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready, Not Writableはリザルトフェーズへ
	if ((fdc.err & FDD_NOTREADY) || (fdc.err & FDD_NOTWRITE)) {
		return FALSE;
	}

	// 検索時間を計算(ヘッドロードは考えない)
	hus = fdd->GetSearch();

	// No Dataは実行後リザルトへ
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// オフセット初期化、イベント設定、EWフェーズ開始
	fdc.offset = 0;
	EventRW();
	fdc.phase = write;

	// 検索時間がヘッドロード時間より短ければ、１周待ち
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// 時間を加算
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	SCAN系コマンド
//
//---------------------------------------------------------------------------
int FASTCALL FDC::Scan()
{
	int len;
	uint32_t hus;

	ASSERT(this);

	// SR設定
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// ドライブに任せる。NOTREADY,NODATA,MAM,CYL系,CRC系,DDAM
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->ReadSector(fdc.buffer, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd);

	// DDAM(Deleted Sector)の有無で、CM(Control Mark)を決める
	if (fdc.err & FDD_DDAM) {
		fdc.err &= ~FDD_DDAM;
		fdc.err |= FDD_CM;
	}

	// IDCRCまたはDATACRCなら、DATAERRを乗せる
	if ((fdc.err & FDD_IDCRC) || (fdc.err & FDD_DATACRC)) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0ではNとしてDTLを使う
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Readyはリザルトフェーズへ
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// CMはSK=1ならリザルトフェーズへ(i82078データシートによる)
	if (fdc.err & FDD_CM) {
		if (fdc.sk) {
			return FALSE;
		}
	}

	// 検索時間を計算(ヘッドロードは考えない)
	hus = fdd->GetSearch();

	// No Data時はこの時間後、リザルトフェーズへ
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// オフセット初期化、イベントスタート、ERフェーズ開始
	fdc.offset = 0;
	EventRW();
	fdc.phase = write;

	// 検索時間がヘッドロード時間より短ければ、１周待ち
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// 時間を加算
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	READ DIAGNOSTICコマンド
//
//---------------------------------------------------------------------------
int FASTCALL FDC::ReadDiag()
{
	uint32_t hus;

	ASSERT(this);
	ASSERT(fdc.cmd == read_diag);

	// SR設定
	fdc.sr |= sr_cb;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// EOT=0はリザルトフェーズへ(NO DATA)
	if (fdc.eot == 0) {
		if (fdd->IsReady(fdc.dsr)) {
			fdc.err = FDD_NODATA;
		}
		else {
			fdc.err = FDD_NOTREADY;
		}
		return FALSE;
	}

	// ドライブに任せる。NOTREADY,NODATA,MAM,CRC系,DDAM
	fdc.err = fdd->ReadDiag(fdc.buffer, &fdc.len, fdc.mfm,
								fdc.chrn, fdc.hd);
	// Not Readyはリザルトフェーズへ
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// 検索時間を計算(ヘッドロードは考えない)
	hus = fdd->GetSearch();

	// MAMなら時間待ち後、リザルトフェーズへ。NODATAでも続けるため
	if (fdc.err & FDD_MAM) {
		EventErr(hus);
		return TRUE;
	}

	ASSERT(fdc.len > 0);

	// オフセット初期化、イベントスタート、ERフェーズ開始
	fdc.offset = 0;
	EventRW();
	fdc.phase = read;

	// 時間を加算
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WRITE IDコマンド
//
//---------------------------------------------------------------------------
int FASTCALL FDC::WriteID()
{
	uint32_t hus;

	ASSERT(this);
	ASSERT(fdc.cmd == write_id);

	// SR設定
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// SC=0チェック
	if (fdc.sc == 0) {
		fdc.err = 0;
		return FALSE;
	}

	// ドライブに任せる。NOTREADY,NOTWRITE
	fdc.err = fdd->WriteID(NULL, fdc.d, fdc.sc, fdc.mfm, fdc.hd, fdc.gpl);
	// Not Ready, Not Writableはリザルトフェーズへ
	if ((fdc.err & FDD_NOTREADY) || (fdc.err & FDD_NOTWRITE)) {
		return FALSE;
	}

	// オフセット初期化
	fdc.offset = 0;
	fdc.len = fdc.sc * 4;

	// イベント設定
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
		LOG0(Log::Warning, "Non-DMAモードでWrite ID");
	}
	else {
		fdc.sr &= ~sr_ndm;
	}
	// Nは7までに制限(N=7は16KB/sector, アンフォーマット)
	if (fdc.chrn[3] > 7) {
		fdc.chrn[3] = 7;
	}

	// 時間を設定。１周弱をセクタで割った数
	hus = fdd->GetSearch();
	hus += (fdd->GetRotationTime() / fdc.sc);
	if (fdc.fast) {
		hus = 32 * 4;
	}

	// イベントスタート、RQMを落とす
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
	fdc.phase = write;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	イベント(Read/Write系)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::EventRW()
{
	uint32_t hus;

	// SR設定(Non-DMA)
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
	}
	else {
		fdc.sr &= ~sr_ndm;
	}

	// イベント作成
	if (fdc.ndm) {
		// Non-DMA。16us/32us
		if (fdc.mfm) {
			hus = 32;
		}
		else {
			hus = 64;
		}
	}
	else {
		// DMAは64バイトまとめて行う。1024us/2048us
		if (fdc.mfm) {
			hus = 32 * 64;
		}
		else {
			hus = 64 * 64;
		}
	}

	// DDはその倍
	if (!fdd->IsHD()) {
		hus <<= 1;
	}

	// fast時は64us固定(DMAに限る)
	if (fdc.fast) {
		if (!fdc.ndm) {
			hus = 128;
		}
	}

	// イベントスタート、RQMを落とす
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
}

//---------------------------------------------------------------------------
//
//	イベント(エラー)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::EventErr(uint32_t hus)
{
	// SR設定(Non-DMA)
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
	}
	else {
		fdc.sr &= ~sr_ndm;
	}

	// イベントスタート、RQMを落とす
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
	fdc.phase = execute;
}

//---------------------------------------------------------------------------
//
//	書き込み完了
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteBack()
{
	switch (fdc.cmd) {
		// Write Data
		case write_data:
			fdc.err = fdd->WriteSector(fdc.buffer, &fdc.len,
							fdc.mfm, fdc.chrn, fdc.hd, FALSE);
			return;

		// Write Deleted Data
		case write_del_data:
			fdc.err = fdd->WriteSector(fdc.buffer, &fdc.len,
							fdc.mfm, fdc.chrn, fdc.hd, TRUE);
			return;

		// Write ID
		case write_id:
			fdc.err = fdd->WriteID(fdc.buffer, fdc.d, fdc.sc,
							fdc.mfm, fdc.hd, fdc.gpl);
			return;
	}

	// ありえない
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	次セクタ
//
//---------------------------------------------------------------------------
int FASTCALL FDC::NextSector()
{
	// TCチェック
	if (fdc.tc) {
		// C,H,R,Nを移動
		if (fdc.chrn[2] < fdc.eot) {
			fdc.chrn[2]++;
			return FALSE;
		}
		fdc.chrn[2] = 0x01;
		// MTによって分ける
		if (fdc.mt && (!(fdc.chrn[1] & 0x01))) {
			// サイド1へ
			fdc.chrn[1] |= 0x01;
			fdc.hd |= 0x04;
			return FALSE;
		}
		// C+1, R=1で終了
		fdc.chrn[0]++;
		return FALSE;
	}

	// EOTチェック
	if (fdc.chrn[2] < fdc.eot) {
		fdc.chrn[2]++;
		return TRUE;
	}

	// EOT。R=1
	fdc.err |= FDD_EOT;
	fdc.chrn[2] = 0x01;

	// MTによって分ける
	if (fdc.mt && (!(fdc.chrn[1] & 0x01))) {
		// サイド1へ
		fdc.chrn[1] |= 0x01;
		fdc.hd |= 0x04;
		return TRUE;
	}

	// C+1, R=1で終了
	fdc.chrn[0]++;
	return FALSE;
}
