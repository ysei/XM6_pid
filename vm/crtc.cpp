//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ CRTC(VICON) ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "tvram.h"
#include "mfp.h"
#include "sprite.h"
#include "render.h"
#include "schedule.h"
#include "cpu.h"
#include "gvram.h"
#include "printer.h"
#include "fileio.h"
#include "crtc.h"

//===========================================================================
//
//	CRTC
//
//===========================================================================
//#define CRTC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CRTC::CRTC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('C', 'R', 'T', 'C');
	dev.desc = "CRTC (VICON)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe80000;
	memdev.last = 0xe81fff;

	// その他ワーク
	tvram = NULL;
	gvram = NULL;
	sprite = NULL;
	mfp = NULL;
	render = NULL;
	printer = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// テキストVRAMを取得
	tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(tvram);

	// グラフィックVRAMを取得
	gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(gvram);

	// スプライトコントローラを取得
	sprite = (Sprite*)vm->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(sprite);

	// MFPを取得
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// レンダラを取得
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// プリンタを取得
	printer = (Printer*)vm->SearchDevice(MAKEID('P', 'R', 'N', ' '));
	ASSERT(printer);

	// イベント初期化
	event.SetDevice(this);
	event.SetDesc("H-Sync");
	event.SetTime(0);
	scheduler->AddEvent(&event);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::Cleanup()
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
void FASTCALL CRTC::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// レジスタをクリア
	memset(crtc.reg, 0, sizeof(crtc.reg));
	for (i=0; i<18; i++) {
		crtc.reg[i] = ResetTable[i];
	}
	for (i=0; i<8; i++) {
		crtc.reg[i + 0x28] = ResetTable[i + 18];
	}

	// 解像度
	crtc.hrl = FALSE;
	crtc.lowres = FALSE;
	crtc.textres = TRUE;
	crtc.changed = FALSE;

	// 特殊機能
	crtc.raster_count = 0;
	crtc.raster_int = 0;
	crtc.raster_copy = FALSE;
	crtc.raster_exec = FALSE;
	crtc.fast_clr = 0;

	// 水平
	crtc.h_sync = 31745;
	crtc.h_pulse = 3450;
	crtc.h_back = 4140;
	crtc.h_front = 2070;
	crtc.h_dots = 768;
	crtc.h_mul = 1;
	crtc.hd = 2;

	// 垂直
	crtc.v_sync = 568;
	crtc.v_pulse = 6;
	crtc.v_back = 35;
	crtc.v_front = 15;
	crtc.v_dots = 512;
	crtc.v_mul = 1;
	crtc.vd = 1;

	// イベント
	crtc.ns = 0;
	crtc.hus = 0;
	crtc.v_synccnt = 1;
	crtc.v_blankcnt = 1;
	crtc.h_disp = TRUE;
	crtc.v_disp = TRUE;
	crtc.v_blank = TRUE;
	crtc.v_count = 0;
	crtc.v_scan = 0;

	// 以下いらない
	crtc.h_disptime = 0;
	crtc.h_synctime = 0;
	crtc.v_cycletime = 0;
	crtc.v_blanktime = 0;
	crtc.v_backtime = 0;
	crtc.v_synctime = 0;

	// メモリモード
	crtc.tmem = FALSE;
	crtc.gmem = TRUE;
	crtc.siz = 0;
	crtc.col = 3;

	// スクロール
	crtc.text_scrlx = 0;
	crtc.text_scrly = 0;
	for (i=0; i<4; i++) {
		crtc.grp_scrlx[i] = 0;
		crtc.grp_scrly[i] = 0;
	}

	// H-Syncイベントを設定(31.5us)
	event.SetTime(63);
}

//---------------------------------------------------------------------------
//
//	CRTCリセットデータ
//
//---------------------------------------------------------------------------
const BYTE CRTC::ResetTable[] = {
	0x00, 0x89, 0x00, 0x0e, 0x00, 0x1c, 0x00, 0x7c,
	0x02, 0x37, 0x00, 0x05, 0x00, 0x28, 0x02, 0x28,
	0x00, 0x1b,
	0x0b, 0x16, 0x00, 0x33, 0x7a, 0x7b, 0x00, 0x00
};

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(crtc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&crtc, (int)sz)) {
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
BOOL FASTCALL CRTC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "ロード");

	// サイズをロード
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(crtc_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&crtc, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	// レンダラへ通知
	render->TextScrl(crtc.text_scrlx, crtc.text_scrly);
	render->GrpScrl(0, crtc.grp_scrlx[0], crtc.grp_scrly[0]);
	render->GrpScrl(1, crtc.grp_scrlx[1], crtc.grp_scrly[1]);
	render->GrpScrl(2, crtc.grp_scrlx[2], crtc.grp_scrly[2]);
	render->GrpScrl(3, crtc.grp_scrlx[3], crtc.grp_scrly[3]);
	render->SetCRTC();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL CRTC::ReadByte(DWORD addr)
{
	BYTE data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $800単位でループ
	addr &= 0x7ff;

	// ウェイト
	scheduler->Wait(1);

	// $E80000-$E803FF : レジスタエリア
	if (addr < 0x400) {
		addr &= 0x3f;
		if (addr >= 0x30) {
			return 0xff;
		}

		// R20, R21のみ読み書き可能。それ以外は$00
		if ((addr < 40) || (addr > 43)) {
			return 0;
		}

		// 読み込み(エンディアンを反転させる)
		addr ^= 1;
		return crtc.reg[addr];
	}

	// $E80480-$E804FF : 動作ポート
	if ((addr >= 0x480) && (addr <= 0x4ff)) {
		// 上位バイトは 0
		if ((addr & 1) == 0) {
			return 0;
		}

		// 下位バイトはラスタコピー、グラフィック高速クリアのみ
		data = 0;
		if (crtc.raster_copy) {
			data |= 0x08;
		}
		if (crtc.fast_clr == 2) {
			data |= 0x02;
		}
		return data;
	}

	LOG1(Log::Warning, "未実装アドレス読み込み $%06X", memdev.first + addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::WriteByte(DWORD addr, DWORD data)
{
	int reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $800単位でループ
	addr &= 0x7ff;

	// ウェイト
	scheduler->Wait(1);

	// $E80000-$E803FF : レジスタエリア
	if (addr < 0x400) {
		addr &= 0x3f;
		if (addr >= 0x30) {
			return;
		}

		// 書き込み(エンディアンを反転させる)
		addr ^= 1;
		if (crtc.reg[addr] == data) {
			return;
		}
		crtc.reg[addr] = (BYTE)data;

		// GVRAMアドレス構成
		if (addr == 0x29) {
			if (data & 0x10) {
				crtc.tmem = TRUE;
			}
			else {
				crtc.tmem = FALSE;
			}
			if (data & 0x08) {
				crtc.gmem = TRUE;
			}
			else {
				crtc.gmem = FALSE;
			}
			crtc.siz = (data & 4) >> 2;
			crtc.col = (data & 3);

			// グラフィックVRAMへ通知
			gvram->SetType(data & 0x0f);
			return;
		}

		// 解像度変更
		if ((addr <= 15) || (addr == 40)) {
			// スプライトメモリの接続・切断は瞬時に行う(OS-9/68000)
			if (addr == 0x28) {
				if ((crtc.reg[0x28] & 3) >= 2) {
					sprite->Connect(FALSE);
				}
				else {
					sprite->Connect(TRUE);
				}
			}

			// 次の周期で再計算
			crtc.changed = TRUE;
			return;
		}

		// ラスタ割り込み
		if ((addr == 18) || (addr == 19)) {
			crtc.raster_int = (crtc.reg[19] << 8) + crtc.reg[18];
			crtc.raster_int &= 0x3ff;
			CheckRaster();
			return;
		}

		// テキストスクロール
		if ((addr >= 20) && (addr <= 23)) {
			crtc.text_scrlx = (crtc.reg[21] << 8) + crtc.reg[20];
			crtc.text_scrlx &= 0x3ff;
			crtc.text_scrly = (crtc.reg[23] << 8) + crtc.reg[22];
			crtc.text_scrly &= 0x3ff;
			render->TextScrl(crtc.text_scrlx, crtc.text_scrly);

#if defined(CRTC_LOG)
			LOG2(Log::Normal, "テキストスクロール x=%d y=%d", crtc.text_scrlx, crtc.text_scrly);
#endif	// CRTC_LOG
			return;
		}

		// グラフィックスクロール
		if ((addr >= 24) && (addr <= 39)) {
			reg = addr & ~3;
			addr -= 24;
			addr >>= 2;
			ASSERT(addr <= 3);
			crtc.grp_scrlx[addr] = (crtc.reg[reg+1] << 8) + crtc.reg[reg+0];
			crtc.grp_scrly[addr] = (crtc.reg[reg+3] << 8) + crtc.reg[reg+2];
			if (addr == 0) {
				crtc.grp_scrlx[addr] &= 0x3ff;
				crtc.grp_scrly[addr] &= 0x3ff;
			}
			else {
				crtc.grp_scrlx[addr] &= 0x1ff;
				crtc.grp_scrly[addr] &= 0x1ff;
			}
			render->GrpScrl(addr, crtc.grp_scrlx[addr], crtc.grp_scrly[addr]);
			return;
		}

		// テキストVRAM
		if ((addr >= 42) && (addr <= 47)) {
			TextVRAM();
		}
		return;
	}

	// $E80480-$E804FF : 動作ポート
	if ((addr >= 0x480) && (addr <= 0x4ff)) {
		// 上位バイトは何もない
		if ((addr & 1) == 0) {
			return;
		}

		// 下位バイトはラスタコピー・高速クリア制御
		if (data & 0x08) {
			crtc.raster_copy = TRUE;
		}
		else {
			crtc.raster_copy = FALSE;
		}
		if (data & 0x02) {
			// ラスタコピーと共用、ラスタコピー優先(大戦略III'90)
			if ((crtc.fast_clr == 0) && !crtc.raster_copy) {
#if defined(CRTC_LOG)
				LOG0(Log::Normal, "グラフィック高速クリア指示");
#endif	// CRTC_LOG
				crtc.fast_clr = 1;
			}
#if defined(CRTC_LOG)
			else {
				LOG1(Log::Normal, "グラフィック高速クリア指示無効 State=%d", crtc.fast_clr);
			}
#endif	//CRTC_LOG
		}
		return;
	}

	LOG2(Log::Warning, "未実装アドレス書き込み $%06X <- $%02X",
							memdev.first + addr, data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL CRTC::ReadOnly(DWORD addr) const
{
	BYTE data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $800単位でループ
	addr &= 0x7ff;

	// $E80000-$E803FF : レジスタエリア
	if (addr < 0x400) {
		addr &= 0x3f;
		if (addr >= 0x30) {
			return 0xff;
		}

		// 読み込み(エンディアンを反転させる)
		addr ^= 1;
		return crtc.reg[addr];
	}

	// $E80480-$E804FF : 動作ポート
	if ((addr >= 0x480) && (addr <= 0x4ff)) {
		// 上位バイトは0
		if ((addr & 1) == 0) {
			return 0;
		}

		// 下位バイトはグラフィック高速クリアのみ
		data = 0;
		if (crtc.raster_copy) {
			data |= 0x08;
		}
		if (crtc.fast_clr == 2) {
			data |= 0x02;
		}
		return data;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::GetCRTC(crtc_t *buffer) const
{
	ASSERT(buffer);

	// 内部データをコピー
	*buffer = crtc;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::Callback(Event* /*ev*/)
{
	ASSERT(this);

	// HSync,HDispの2つを呼び分ける
	if (crtc.h_disp) {
		HSync();
	}
	else {
		HDisp();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	H-SYNC開始
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::HSync()
{
	int hus;

	ASSERT(this);

	// プリンタに通知(定期的にBUSYを落とすため)
	ASSERT(printer);
	printer->HSync();

	// V-SYNCカウント
	crtc.v_synccnt--;
	if (crtc.v_synccnt == 0) {
		VSync();
	}

	// V-BLANKカウント
	crtc.v_blankcnt--;
	if (crtc.v_blankcnt == 0) {
		VBlank();
	}

	// 次のタイミング(H-DISP開始)までの時間を設定
	crtc.ns += crtc.h_pulse;
	hus = Ns2Hus(crtc.ns);
	hus -= crtc.hus;
	event.SetTime(hus);
	crtc.hus += hus;

	// 同期処理(40msごと)
	if (crtc.hus >= 80000) {
		crtc.hus -= 80000;
		ASSERT(crtc.ns >= 40000000);
		crtc.ns -= 40000000;
	}

	// フラグ設定
	crtc.h_disp = FALSE;

	// GPIP設定
	mfp->SetGPIP(7, 1);

	// 描画
	crtc.v_scan++;
	if (!crtc.v_blank) {
		// レンダリング
		render->HSync(crtc.v_scan);
	}

	// ラスタ割り込み
#if 0
	CheckRaster();
	crtc.raster_count++;
#endif

	// テキスト画面ラスタコピー
	if (crtc.raster_copy && crtc.raster_exec) {
		tvram->RasterCopy();
		crtc.raster_exec = FALSE;
	}

	// グラフィック画面高速クリア
	if (crtc.fast_clr == 2) {
		gvram->FastClr(&crtc);
	}
}

//---------------------------------------------------------------------------
//
//	H-DISP開始
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::HDisp()
{
	int ns;
	int hus;

	ASSERT(this);

#if 1
	// ラスタ割り込み
	CheckRaster();
	crtc.raster_count++;
#endif

	// 次のタイミング(H-SYNC開始)までの時間を設定
	ns = crtc.h_sync - crtc.h_pulse;
	ASSERT(ns > 0);
	crtc.ns += ns;
	hus = Ns2Hus(crtc.ns);
	hus -= crtc.hus;
	event.SetTime(hus);
	crtc.hus += hus;

	// フラグ設定
	crtc.h_disp = TRUE;

	// GPIP設定
	mfp->SetGPIP(7,0);

	// ラスタコピー許可
	crtc.raster_exec = TRUE;
}

//---------------------------------------------------------------------------
//
//	V-SYNC開始(V-DISP開始を含む)
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::VSync()
{
	ASSERT(this);

	// V-SYNC終了なら
	if (!crtc.v_disp) {
		// フラグ設定
		crtc.v_disp = TRUE;

		// 時間設定
		crtc.v_synccnt = (crtc.v_sync - crtc.v_pulse);
		return;
	}

	// 解像度変更があれば、ここで変更
	if (crtc.changed) {
		ReCalc();
	}

	// V-SYNC終了までの時間を設定
	crtc.v_synccnt = crtc.v_pulse;

	// V-BLANKの状態と、時間を設定
	if (crtc.v_front < 0) {
		// まだ表示中(特殊)
		crtc.v_blank = FALSE;
		crtc.v_blankcnt = (-crtc.v_front) + 1;
	}
	else {
		// すでにブランク中(通常)
		crtc.v_blank = TRUE;
		crtc.v_blankcnt = (crtc.v_pulse + crtc.v_back + 1);
	}

	// フラグ設定
	crtc.v_disp = FALSE;

	// ラスタカウント初期化
	crtc.raster_count = 0;
}

//---------------------------------------------------------------------------
//
//	再計算
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::ReCalc()
{
	int dc;
	int over;
	WORD *p;

	ASSERT(this);
	ASSERT(crtc.changed);

	// CRTCレジスタ0がクリアされていれば、無効(Macエミュレータ)
	if (crtc.reg[0x0] != 0) {
#if defined(CRTC_LOG)
		LOG0(Log::Normal, "再計算");
#endif	// CRTC_LOG

		// ドットクロックを取得
		dc = Get8DotClock();

		// 水平(すべてns単位)
		crtc.h_sync = (crtc.reg[0x0] + 1) * dc / 100;
		crtc.h_pulse = (crtc.reg[0x02] + 1) * dc / 100;
		crtc.h_back = (crtc.reg[0x04] + 5 - crtc.reg[0x02] - 1) * dc / 100;
		crtc.h_front = (crtc.reg[0x0] + 1 - crtc.reg[0x06] - 5) * dc / 100;

		// 垂直(すべてH-Sync単位)
		p = (WORD *)crtc.reg;
		crtc.v_sync = ((p[4] & 0x3ff) + 1);
		crtc.v_pulse = ((p[5] & 0x3ff) + 1);
		crtc.v_back = ((p[6] & 0x3ff) + 1) - crtc.v_pulse;
		crtc.v_front = crtc.v_sync - ((p[7] & 0x3ff) + 1);

		// V-FRONTがマイナスすぎる場合は、1水平期間分のみ(ヘルハウンド、コットン)
		if (crtc.v_front < 0) {
			over = -crtc.v_front;
			over -= crtc.v_back;
			if (over >= crtc.v_pulse) {
				crtc.v_front = -1;
			}
		}

		// ドット数を算出
		crtc.h_dots = (crtc.reg[0x0] + 1);
		crtc.h_dots -= (crtc.reg[0x02] + 1);
		crtc.h_dots -= (crtc.reg[0x04] + 5 - crtc.reg[0x02] - 1);
		crtc.h_dots -= (crtc.reg[0x0] + 1 - crtc.reg[0x06] - 5);
		crtc.h_dots *= 8;
		crtc.v_dots = crtc.v_sync - crtc.v_pulse - crtc.v_back - crtc.v_front;
	}

	// 倍率設定(水平)
	crtc.hd = (crtc.reg[0x28] & 3);
	if (crtc.hd == 3) {
		LOG0(Log::Warning, "横ドット数50MHzモード(CompactXVI)");
	}
	if (crtc.hd == 0) {
		crtc.h_mul = 2;
	}
	else {
		crtc.h_mul = 1;
	}

	// crtc.hdが2以上の場合、スプライトは切り離される
	if (crtc.hd >= 2) {
		// 768x512 or VGAモード(スプライトなし)
		sprite->Connect(FALSE);
		crtc.textres = TRUE;
	}
	else {
		// 256x256 or 512x512モード(スプライトあり)
		sprite->Connect(TRUE);
		crtc.textres = FALSE;
	}

	// 倍率設定(垂直)
	crtc.vd = (crtc.reg[0x28] >> 2) & 3;
	if (crtc.reg[0x28] & 0x10) {
		// 31kHz
		crtc.lowres = FALSE;
		if (crtc.vd == 3) {
			// インタレース1024dotモード
			crtc.v_mul = 0;
		}
		else {
			// インタレース、通常512モード(x1)、倍256dotモード(x2)
			crtc.v_mul = 2 - crtc.vd;
		}
	}
	else {
		// 15kHz
		crtc.lowres = TRUE;
		if (crtc.vd == 0) {
			// 通常の256dotモード(x2)
			crtc.v_mul = 2;
		}
		else {
			// インタレース512dotモード(x1)
			crtc.v_mul = 0;
		}
	}

	// レンダラへ通知
	render->SetCRTC();

	// フラグおろす
	crtc.changed = FALSE;
}


//---------------------------------------------------------------------------
//
//	V-BLANK開始(V-SCREEN開始を含む)
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::VBlank()
{
	ASSERT(this);

	// 表示中であれば、ブランク開始
	if (!crtc.v_blank) {
		// ブランク区間を設定
		crtc.v_blankcnt = crtc.v_pulse + crtc.v_back + crtc.v_front;
		ASSERT((crtc.v_front < 0) || ((int)crtc.v_synccnt == crtc.v_front));

		// フラグ
		crtc.v_blank = TRUE;

		// GPIP
		mfp->EventCount(0, 0);
		mfp->SetGPIP(4, 0);

		// グラフィック高速クリア
		if (crtc.fast_clr == 2) {
#if defined(CRTC_LOG)
			LOG0(Log::Normal, "グラフィック高速クリア終了");
#endif	// CRTC_LOG
			crtc.fast_clr = 0;
		}

		// レンダラ合成終了
		render->EndFrame();
		crtc.v_scan = crtc.v_dots + 1;
		return;
	}

	// 表示区間を設定
	crtc.v_blankcnt = crtc.v_sync;
	crtc.v_blankcnt -= (crtc.v_pulse + crtc.v_back + crtc.v_front);

	// フラグ
	crtc.v_blank = FALSE;

	// GPIP
	mfp->EventCount(0, 1);
	mfp->SetGPIP(4, 1);

	// グラフィック高速クリア
	if (crtc.fast_clr == 1) {
#if defined(CRTC_LOG)
		LOG1(Log::Normal, "グラフィック高速クリア開始 data=%02X", crtc.reg[42]);
#endif	// CRTC_LOG
		crtc.fast_clr = 2;
		gvram->FastSet((DWORD)crtc.reg[42]);
		gvram->FastClr(&crtc);
	}

	// レンダラ合成開始、カウンタアップ
	crtc.v_scan = 0;
	render->StartFrame();
	crtc.v_count++;
}

//---------------------------------------------------------------------------
//
//	表示周波数取得
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::GetHVHz(DWORD *h, DWORD *v) const
{
	DWORD d;
	DWORD t;

	// assert
	ASSERT(h);
	ASSERT(v);

	// チェック
	if ((crtc.h_sync == 0) || (crtc.v_sync < 100)) {
		// NO SIGNAL
		*h = 0;
		*v = 0;
		return;
	}

	// ex. 31.5kHz = 3150
	d = 100 * 1000 * 1000;
	d /= crtc.h_sync;
	*h = d;

	// ex. 55.46Hz = 5546
	t = crtc.v_sync;
    t *= crtc.h_sync;
	t /= 100;
	d = 1000 * 1000 * 1000;
	d /= t;
	*v = d;
}

//---------------------------------------------------------------------------
//
//	8ドットクロックを取得(×100)
//
//---------------------------------------------------------------------------
int FASTCALL CRTC::Get8DotClock() const
{
	int hf;
	int hd;
	int index;

	ASSERT(this);

	// HF, HDをCRTC R20より取得
	hf = (crtc.reg[0x28] >> 4) & 1;
	hd = (crtc.reg[0x28] & 3);

	// インデックス作成
	index = hf * 4 + hd;
	if (crtc.hrl) {
		index += 8;
	}

	return DotClockTable[index];
}

//---------------------------------------------------------------------------
//
//	8ドットクロックテーブル
//	(HRL,HF,HDから得られる値。0.01ns単位)
//
//---------------------------------------------------------------------------
const int CRTC::DotClockTable[16] = {
	// HRL=0
	164678, 82339, 164678, 164678,
	69013, 34507, 23004, 31778,
	// HRL=1
	164678, 82339, 164678, 164678,
	92017, 46009, 23004, 31778
};

//---------------------------------------------------------------------------
//
//	HRL設定
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::SetHRL(BOOL flag)
{
	if (crtc.hrl != flag) {
		// 次の周期で再計算
		crtc.hrl = flag;
		crtc.changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	HRL取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::GetHRL() const
{
	return crtc.hrl;
}

//---------------------------------------------------------------------------
//
//	ラスタ割り込みチェック
//	※インタレースモードには未対応
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::CheckRaster()
{
#if 1
	if (crtc.raster_count == crtc.raster_int) {
#else
	if (crtc.raster_count == crtc.raster_int) {
#endif
		// 要求
		mfp->SetGPIP(6, 0);
#if defined(CRTC_LOG)
		LOG2(Log::Normal, "ラスタ割り込み要求 raster=%d scan=%d", crtc.raster_count, crtc.v_scan);
#endif	// CRTC_LOG
	}
	else {
		// 取り下げ
		mfp->SetGPIP(6, 1);
	}
}

//---------------------------------------------------------------------------
//
//	テキストVRAM効果
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::TextVRAM()
{
	DWORD b;
	DWORD w;

	// 同時アクセス
	if (crtc.reg[43] & 1) {
		b = (DWORD)crtc.reg[42];
		b >>= 4;

		// b4はマルチフラグ
		b |= 0x10;
		tvram->SetMulti(b);
	}
	else {
		tvram->SetMulti(0);
	}

	// アクセスマスク
	if (crtc.reg[43] & 2) {
		w = (DWORD)crtc.reg[47];
		w <<= 8;
		w |= (DWORD)crtc.reg[46];
		tvram->SetMask(w);
	}
	else {
		tvram->SetMask(0);
	}

	// ラスタコピー
	tvram->SetCopyRaster((DWORD)crtc.reg[45], (DWORD)crtc.reg[44],
						(DWORD)(crtc.reg[42] & 0x0f));
}
