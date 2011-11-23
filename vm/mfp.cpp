//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "cpu.h"
#include "vm.h"
#include "log.h"
#include "event.h"
#include "schedule.h"
#include "keyboard.h"
#include "fileio.h"
#include "sync.h"
#include "mfp.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
//#define MFP_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
MFP::MFP(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('M', 'F', 'P', ' ');
	dev.desc = "MFP (MC68901)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe88000;
	memdev.last = 0xe89fff;

	// Syncオブジェクト
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Init()
{
	int i;
	char buf[0x20];

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// Sync作成
	sync = new Sync;

	// タイマイベント初期化
	for (i=0; i<4; i++) {
		timer[i].SetDevice(this);
		sprintf(buf, "Timer-%c", 'A' + i);
		timer[i].SetDesc(buf);
		timer[i].SetUser(i);
		timer[i].SetTime(0);

		// Timer-Bはイベントカウントで使う機会がないため、外す
		if (i != 1) {
			scheduler->AddEvent(&timer[i]);
		}
	}

	// キーボードを取得
	keyboard = (Keyboard*)vm->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));

	// USARTイベント初期化
	// 1(us)x13(回)x(デューティ50%)x16(分周)x10(bit)で約2400bps
	usart.SetDevice(this);
	usart.SetUser(4);
	usart.SetDesc("USART 2400bps");
	usart.SetTime(8320);
	scheduler->AddEvent(&usart);

	// リセットでは初期化されないレジスタを設定(データシート3.3)
	for (i=0; i<4; i++) {
		if (i == 0) {
			// タイマAは1にして、VDISPSTがすぐ起こるようにする(DiskX)
			SetTDR(i, 1);
			mfp.tir[i] = 1;
		}
		else {
			// タイマB,タイマC,タイマDは0
			SetTDR(i, 0);
			mfp.tir[i] = 0;
		}
	}
	mfp.tsr = 0;
	mfp.rur = 0;

	// リセット時にキーボードから$FFが送信されるため、Initで初期化
	sync->Lock();
	mfp.datacount = 0;
	mfp.readpoint = 0;
	mfp.writepoint = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Cleanup()
{
	ASSERT(this);

	// Sync削除
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// 割り込みコントロール
	mfp.vr = 0;
	mfp.iidx = -1;
	for (i=0; i<0x10; i++) {
		mfp.ier[i] = FALSE;
		mfp.ipr[i] = FALSE;
		mfp.imr[i] = FALSE;
		mfp.isr[i] = FALSE;
		mfp.ireq[i] = FALSE;
	}

	// タイマ
	for (i=0; i<4; i++) {
		timer[i].SetTime(0);
		SetTCR(i, 0);
	}
	mfp.tbr[0] = 0;
	mfp.tbr[1] = 0;
	mfp.sram = 0;
	mfp.tecnt = 0;

	// GPIP (GPIP5は常にHレベル)
	mfp.gpdr = 0;
	mfp.aer = 0;
	mfp.ddr = 0;
	mfp.ber = (DWORD)~mfp.aer;
	mfp.ber ^= mfp.gpdr;
	SetGPIP(5, 1);

	// USART
	mfp.scr = 0;
	mfp.ucr = 0;
	mfp.rsr = 0;
	mfp.tsr = (DWORD)(mfp.tsr & ~0x01);
	mfp.tur = 0;

	// GPIP初期化(電源関連)
	SetGPIP(1, 1);
	if (vm->IsPowerSW()) {
		SetGPIP(2, 0);
	}
	else {
		SetGPIP(2, 1);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// 本体
	sz = sizeof(mfp_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&mfp, (int)sz)) {
		return FALSE;
	}

	// イベント(タイマ)
	for (i=0; i<4; i++) {
		if (!timer[i].Save(fio, ver)) {
			return FALSE;
		}
	}

	// イベント(USART)
	if (!usart.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Load(Fileio *fio, int ver)
{
	int i;
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// 本体
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(mfp_t)) {
		return FALSE;
	}
	if (!fio->Read(&mfp, (int)sz)) {
		return FALSE;
	}

	// イベント(タイマ)
	for (i=0; i<4; i++) {
		if (!timer[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// イベント(USART)
	if (!usart.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL MFP::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// ウェイト
		scheduler->Wait(3);

		// 64バイト単位でループ
		addr &= 0x3f;
		addr >>= 1;

		switch (addr) {
			// GPIP
			case 0x00:
				return mfp.gpdr;

			// AER
			case 0x01:
				return mfp.aer;

			// DDR
			case 0x02:
				return mfp.ddr;

			// IER(A)
			case 0x03:
				return GetIER(0);

			// IER(B)
			case 0x04:
				return GetIER(1);

			// IPR(A)
			case 0x05:
				return GetIPR(0);

			// IPR(B)
			case 0x06:
				return GetIPR(1);

			// ISR(A)
			case 0x07:
				return GetISR(0);

			// ISR(B)
			case 0x08:
				return GetISR(1);

			// IMR(A)
			case 0x09:
				return GetIMR(0);

			// IMR(B)
			case 0x0a:
				return GetIMR(1);

			// VR
			case 0x0b:
				return GetVR();

			// タイマAコントロール
			case 0x0c:
				return GetTCR(0);

			// タイマBコントロール
			case 0x0d:
				return GetTCR(1);

			// タイマC&Dコントロール
			case 0x0e:
				data = GetTCR(2);
				data <<= 4;
				data |= GetTCR(3);
				return data;

			// タイマAデータ
			case 0x0f:
				return GetTIR(0);

			// タイマBデータ
			case 0x10:
				return GetTIR(1);

			// タイマCデータ
			case 0x11:
				return GetTIR(2);

			// タイマDデータ
			case 0x12:
				return GetTIR(3);

			// SYNCキャラクタ
			case 0x13:
				return mfp.scr;

			// USARTコントロール
			case 0x14:
				return mfp.ucr;

			// レシーバステータス
			case 0x15:
				return mfp.rsr;

			// トランスミッタステータス
			case 0x16:
				// TEビットはクリアされる
				mfp.tsr = (DWORD)(mfp.tsr & ~0x40);
				return mfp.tsr;

			// USARTデータ
			case 0x17:
				Receive();
				return mfp.rur;

			// それ以外
			default:
				LOG1(Log::Warning, "未実装レジスタ読み込み R%02d", addr);
				return 0xff;
		}
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadWord(DWORD addr)
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
void FASTCALL MFP::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		// バスエラーは発生しない
		return;
	}

	// 64バイト単位でループ
	addr &= 0x3f;
	addr >>= 1;

	// ウェイト
	scheduler->Wait(3);

	switch (addr) {
		// GPIP
		case 0x00:
			// VDISPをAND.Bでチェックする場合がある(MOON FIGHTER)
			SetGPDR(data);
			return;

		// AER
		case 0x01:
			mfp.aer = data;
			mfp.ber = (DWORD)(~data);
			mfp.ber ^= mfp.gpdr;
			IntGPIP();
			return;

		// DDR
		case 0x02:
			mfp.ddr = data;
			if (mfp.ddr != 0) {
				LOG0(Log::Warning, "GPIP出力ディレクション");
			}
			return;

		// IER(A)
		case 0x03:
			SetIER(0, data);
			return;

		// IER(B)
		case 0x04:
			SetIER(1, data);
			return;

		// IPR(A)
		case 0x05:
			SetIPR(0, data);
			return;

		// IPR(B)
		case 0x06:
			SetIPR(1, data);
			return;

		// ISR(A)
		case 0x07:
			SetISR(0, data);
			return;

		// ISR(B)
		case 0x08:
			SetISR(1, data);
			return;

		// IMR(A)
		case 0x09:
			SetIMR(0, data);
			return;

		// IMR(B)
		case 0x0a:
			SetIMR(1, data);
			return;

		// VR
		case 0x0b:
			SetVR(data);
			return;

		// タイマAコントロール
		case 0x0c:
			SetTCR(0, data);
			return;

		// タイマBコントロール
		case 0x0d:
			SetTCR(1, data);
			return;

		// タイマC&Dコントロール
		case 0x0e:
			SetTCR(2, (DWORD)(data >> 4));
			SetTCR(3, (DWORD)(data & 0x0f));
			return;

		// タイマAデータ
		case 0x0f:
			SetTDR(0, data);
			return;

		// タイマBデータ
		case 0x10:
			SetTDR(1, data);
			return;

		// タイマCデータ
		case 0x11:
			SetTDR(2, data);
			return;

		// タイマDデータ
		case 0x12:
			SetTDR(3, data);
			return;

		// SYNCキャラクタ
		case 0x13:
			mfp.scr = data;
			return;

		// USARTコントロール
		case 0x14:
			if (data != 0x88) {
				LOG1(Log::Warning, "USART パラメータエラー %02X", data);
			}
			mfp.ucr = data;
			return;

		// レシーバステータス
		case 0x15:
			SetRSR(data);
			return;

		// トランスミッタステータス
		case 0x16:
			SetTSR(data);
			return;

		// USARTデータ
		case 0x17:
			Transmit(data);
			return;
	}

	LOG2(Log::Warning, "未実装レジスタ書き込み R%02d <- $%02X",
							addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL MFP::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 64バイト単位でループ
	addr &= 0x3f;
	addr >>= 1;

	switch (addr) {
		// GPIP
		case 0x00:
			return mfp.gpdr;

		// AER
		case 0x01:
			return mfp.aer;

		// DDR
		case 0x02:
			return mfp.ddr;

		// IER(A)
		case 0x03:
			return GetIER(0);

		// IER(B)
		case 0x04:
			return GetIER(1);

		// IPR(A)
		case 0x05:
			return GetIPR(0);

		// IPR(B)
		case 0x06:
			return GetIPR(1);

		// ISR(A)
		case 0x07:
			return GetISR(0);

		// ISR(B)
		case 0x08:
			return GetISR(1);

		// IMR(A)
		case 0x09:
			return GetIMR(0);

		// IMR(B)
		case 0x0a:
			return GetIMR(1);

		// VR
		case 0x0b:
			return GetVR();

		// タイマAコントロール
		case 0x0c:
			return mfp.tcr[0];

		// タイマBコントロール
		case 0x0d:
			return mfp.tcr[1];

		// タイマC&Dコントロール
		case 0x0e:
			data = mfp.tcr[2];
			data <<= 4;
			data |= mfp.tcr[3];
			return data;

		// タイマAデータ
		case 0x0f:
			return mfp.tir[0];

		// タイマBデータ(ランダムな値を返す)
		case 0x10:
			return ((scheduler->GetTotalTime() % 13) + 1);

		// タイマCデータ
		case 0x11:
			return mfp.tir[2];

		// タイマDデータ
		case 0x12:
			return mfp.tir[3];

		// SYNCキャラクタ
		case 0x13:
			return mfp.scr;

		// USARTコントロール
		case 0x14:
			return mfp.ucr;

		// レシーバステータス
		case 0x15:
			return mfp.rsr;

		// トランスミッタステータス
		case 0x16:
			return mfp.tsr;

		// USARTデータ
		case 0x17:
			return mfp.rur;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL MFP::GetMFP(mfp_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// データをコピー
	*buffer = mfp;
}

//---------------------------------------------------------------------------
//
//	割り込み
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Interrupt(int level, BOOL enable)
{
	int index;

	ASSERT(this);
	ASSERT((level >= 0) && (level < 0x10));

	index = 15 - level;
	if (enable) {
		// 既に要求されているか
		if (mfp.ireq[index]) {
			return;
		}

		// イネーブルレジスタはどうか
		if (!mfp.ier[index]) {
			return;
		}

		// フラグUp、割り込みチェック
		mfp.ireq[index] = TRUE;
		IntCheck();
	}
	else {
		// 既に取り消し処理したか、要求受理された後か
		if (!mfp.ireq[index] && !mfp.ipr[index]) {
			return;
		}
		mfp.ireq[index] = FALSE;

		// ペンディングを下げ、割り込み解除する
		mfp.ipr[index] = FALSE;
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	割り込み優先順位判定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntCheck()
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);

	// 割り込みキャンセルチェック
	if (mfp.iidx >= 0) {
		// ペンディング解除またはマスクで、割り込み取り下げ(データシート2.7.1)
		if (!mfp.ipr[mfp.iidx] || !mfp.imr[mfp.iidx]) {
			cpu->IntCancel(6);
#if defined(MFP_LOG)
			sprintf(buffer, "割り込み取り消し %s", IntDesc[mfp.iidx]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = -1;
		}
	}

	// 割り込み発生受理
	for (i=0; i<0x10; i++) {
		// 割り込みイネーブルか
		if (mfp.ier[i]) {
			// 割り込みリクエストがあるか
			if (mfp.ireq[i]) {
				// ペンディングレジスタを1にする(割り込み受理)
				mfp.ipr[i] = TRUE;
				mfp.ireq[i] = FALSE;
			}
		}
		else {
			// イネーブルレジスタ0で、割り込みペンディング解除
			mfp.ipr[i] = FALSE;
			mfp.ireq[i] = FALSE;
		}
	}

	// 割り込みベクタ送出
	for (i=0; i<0x10; i++) {
		// 割り込みペンディングか
		if (!mfp.ipr[i]) {
			continue;
		}

		// 割り込みマスクか
		if (!mfp.imr[i]) {
			continue;
		}

		// サービス中でないか
		if (mfp.isr[i]) {
			continue;
		}

		// 既に要求している割り込みより上位なら、割り込みを乗っ取る
		if (mfp.iidx > i) {
			cpu->IntCancel(6);
#if defined(MFP_LOG)
			sprintf(buffer, "割り込み優先取り消し %s", IntDesc[mfp.iidx]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = -1;
		}

		// ベクタ送出
		if (cpu->Interrupt(6, (mfp.vr & 0xf0) + (15 - i))) {
			// CPUに受け付けられた。インデックス記憶
#if defined(MFP_LOG)
			sprintf(buffer, "割り込み要求 %s", IntDesc[i]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = i;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	割り込み応答
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntAck()
{
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);

	// リセット直後に、CPUから割り込みが間違って入る場合がある
	if (mfp.iidx < 0) {
		LOG0(Log::Warning, "要求していない割り込み");
		return;
	}

#if defined(MFP_LOG)
	sprintf(buffer, "割り込み応答 %s", IntDesc[mfp.iidx]);
	LOG0(Log::Normal, buffer);
#endif	// MFP_LOG

	// 割り込みが受け付けられた。ペンディング解除
	mfp.ipr[mfp.iidx] = FALSE;

	// インサービス(オートEOIで0、マニュアルEOIで1)
	if (mfp.vr & 0x08) {
		mfp.isr[mfp.iidx] = TRUE;
	}
	else {
		mfp.isr[mfp.iidx] = FALSE;
	}

	// インデックスを割り込み無しに変更
	mfp.iidx = -1;

	// 再度、割り込みチェックを行う
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IER設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIER(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// 初期設定
	offset <<= 3;

	// 8回、ビット分離
	for (i=offset; i<offset+8; i++) {
		if (data & 0x80) {
			mfp.ier[i] = TRUE;
		}
		else {
			// IPRを下げる(データシート4.3.1)
			mfp.ier[i] = FALSE;
			mfp.ipr[i] = FALSE;
		}

		data <<= 1;
	}

	// 割り込み優先順位
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IER取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIER(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// 初期設定
	offset <<= 3;
	bit = 0;

	// 8回、ビット合成
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.ier[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	IPR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIPR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// 初期設定
	offset <<= 3;

	// 8回、ビット分離
	for (i=offset; i<offset+8; i++) {
		if (!(data & 0x80)) {
			// IPRはCPUから1にすることは出来ない
			mfp.ipr[i] = FALSE;
		}

		data <<= 1;
	}

	// 割り込み優先順位
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IPR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIPR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// 初期設定
	offset <<= 3;
	bit = 0;

	// 8回、ビット合成
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.ipr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	ISR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetISR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// 初期設定
	offset <<= 3;

	// 8回、ビット分離
	for (i=offset; i<offset+8; i++) {
		if (!(data & 0x80)) {
			mfp.isr[i] = FALSE;
		}

		data <<= 1;
	}

	// 割り込み優先順位
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	ISR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetISR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// 初期設定
	offset <<= 3;
	bit = 0;

	// 8回、ビット合成
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.isr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	IMR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIMR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// 初期設定
	offset <<= 3;

	// 8回、ビット分離
	for (i=offset; i<offset+8; i++) {
		if (data & 0x80) {
			mfp.imr[i] = TRUE;
		}
		else {
			mfp.imr[i] = FALSE;
		}

		data <<= 1;
	}

	// 割り込み優先順位
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IMR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIMR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// 初期設定
	offset <<= 3;
	bit = 0;

	// 8回、ビット合成
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.imr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	VR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetVR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	if (mfp.vr != data) {
		mfp.vr = data;
		LOG1(Log::Detail, "割り込みベクタベース $%02X", data & 0xf0);

		if (mfp.vr & 0x08) {
			LOG0(Log::Warning, "マニュアルEOIモード");
		}
	}
}

//---------------------------------------------------------------------------
//
//	VR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetVR() const
{
	ASSERT(this);
	return mfp.vr;
}

//---------------------------------------------------------------------------
//
//	割り込み名称テーブル
//
//---------------------------------------------------------------------------
const char* MFP::IntDesc[0x10] = {
	"H-SYNC",
	"CIRQ",
	"Timer-A",
	"RxFull",
	"RxError",
	"TxEmpty",
	"TxError",
	"Timer-B",
	"(NoUse)",
	"V-DISP",
	"Timer-C",
	"Timer-D",
	"FMIRQ",
	"POW SW",
	"EXPON",
	"ALARM"
};

//---------------------------------------------------------------------------
//
//	イベントコールバック(ディレイモードで使用)
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Callback(Event *ev)
{
	int channel;
	DWORD low;

	ASSERT(this);
	ASSERT(ev);

	// ユーザデータから種別を得る
	channel = (int)ev->GetUser();

	// タイマ
	if ((channel >= 0) && (channel <= 3)) {
		low = (mfp.tcr[channel] & 0x0f);

		// タイマオンか
		if (low == 0) {
			// タイマオフ
			return FALSE;
		}

		// ディレイモードか
		if (low & 0x08) {
			// カウント0から少し遅れて、割り込み発生
			Interrupt(TimerInt[channel], TRUE);

			// ワンショット
			return FALSE;
		}

		// タイマを進める
		Proceed(channel);
		return TRUE;
	}

	// USART
	ASSERT(channel == 4);
	USART();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	タイマイベント入力(イベントカウントモードで使用)
//
//---------------------------------------------------------------------------
void FASTCALL MFP::EventCount(int channel, int value)
{
	DWORD edge;
	BOOL flag;

	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 1));
	ASSERT((value == 0) || (value == 1));
	ASSERT((mfp.tbr[channel] == 0) || (mfp.tbr[channel] == 1));

	// イベントカウントモードか(タイマA,Bのみ)
	if ((mfp.tcr[channel] & 0x0f) == 0x08) {
		// 方向はGPIP4, GPIP3で決まる
		if (channel == 0) {
			edge = mfp.aer & 0x10;
		}
		else {
			edge = mfp.aer & 0x08;
		}

		// フラグオフ
		flag = FALSE;

		// エッジ判定
		if (edge == 1) {
			// エッジが1のとき、0→1でタイマを進める
			if ((mfp.tbr[channel] == 0) && (value == 1)) {
				flag = TRUE;
			}
		}
		else {
			// エッジが0のとき、1→0でタイマを進める
			if ((mfp.tbr[channel] == 1) && (value == 0)) {
				flag = TRUE;
			}
		}

		// タイマを進める
		if (flag) {
			Proceed(channel);
		}
	}

	// TBRを更新
	mfp.tbr[channel] = (DWORD)value;
}

//---------------------------------------------------------------------------
//
//	TCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTCR(int channel, DWORD data)
{
	DWORD prev;
	DWORD now;
	DWORD speed;

	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));
	ASSERT(data < 0x100);

	// 型変換、一致チェック
	now = data;
	now &= 0x0f;
	prev = mfp.tcr[channel];
	if (now == prev) {
		return;
	}
	mfp.tcr[channel] = now;

	// タイマBは0x01のみ可(エミュレーションしない)
	if (channel == 1) {
		if ((now != 0x01) && (now != 0x00)) {
			LOG1(Log::Warning, "タイマBコントロール $%02X", now);
		}
		now = 0;
	}

	// タイマストップか
	if (now == 0) {
#if defined(MFP_LOG)
		LOG1(Log::Normal, "タイマ%c 停止", channel + 'A');
#endif	// MFP_LOG
		timer[channel].SetTime(0);

		// 割り込みの取り下げを行う必要あり(悪魔城ドラキュラ)
		Interrupt(TimerInt[channel], FALSE);

		// Timer-DはCPUクロックを戻す(CH30.SYS)
		if (channel == 3) {
			if (mfp.sram != 0) {
				// CPUクロックを戻す
				scheduler->SetCPUSpeed(mfp.sram);
				mfp.sram = 0;
			}
		}
		return;
	}

	// パルス幅測定モードはサポートしていない
	if (now > 0x08) {
		LOG2(Log::Warning, "タイマ%c パルス幅測定モード$%02X", channel + 'A', now);
		return;
	}

	// イベントカウントモードか
	if (now == 0x08) {
#if defined(MFP_LOG)
		LOG1(Log::Normal, "タイマ%c イベントカウントモード", channel + 'A');
#endif	// MFP_LOG
		// 一度イベントを止める
		timer[channel].SetTime(0);

		// タイマOFF→ONなら、カウントをロード
		if (prev == 0) {
			mfp.tir[channel] = mfp.tdr[channel];
		}
		return;
	}

	// ディレイモードでは、プリスケーラを設定
#if defined(MFP_LOG)
	LOG3(Log::Normal, "タイマ%c ディレイモード %d.%dus",
				channel + 'A', (TimerHus[now] / 2), (TimerHus[now] & 1) * 5);
#endif	// MFP_LOG

	// タイマOFF→ONなら、カウンタをロード(_VDISPSTパッチ)
	if (prev == 0) {
		mfp.tir[channel] = mfp.tdr[channel];
	}

	// イベントをセット
	timer[channel].SetTime(TimerHus[now]);

	// Timer-Cの場合、INFO.RAM専用対策を行う(CPU速度計測を無理やりあわせる)
	if (channel == 2) {
		if ((now == 3) && (mfp.sram == 0)) {
			speed = cpu->GetPC();
			if ((speed >= 0xed0100) && (speed <= 0xedffff)) {
				// CPUクロックを落とす
				speed = scheduler->GetCPUSpeed();
				mfp.sram = speed;
				speed *= 83;
				speed /= 96;
				scheduler->SetCPUSpeed(speed);
			}
		}
		if ((now != 3) && (mfp.sram != 0)) {
			// CPUクロックを戻す
			scheduler->SetCPUSpeed(mfp.sram);
			mfp.sram = 0;
		}
	}

	// Timer-Dの場合、CH30.SYS専用対策を行う(CPU速度計測を無理やりあわせる)
	if (channel == 3) {
		if ((now == 7) && (mfp.sram == 0)) {
			speed = cpu->GetPC();
			if ((speed >= 0xed0100) && (speed <= 0xedffff)) {
				// CPUクロックを落とす
				speed = scheduler->GetCPUSpeed();
				mfp.sram = speed;
				speed *= 85;
				speed /= 96;
				scheduler->SetCPUSpeed(speed);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	TCR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetTCR(int channel) const
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	return mfp.tcr[channel];
}

//---------------------------------------------------------------------------
//
//	TDR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTDR(int channel, DWORD data)
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));
	ASSERT(data < 0x100);

	mfp.tdr[channel] = data;

	// タイマBは固定値のはず
	if (channel == 1) {
		if (data != 0x0d) {
			LOG1(Log::Warning, "タイマBリロード値 %02X", data);
		}
	}
}

//---------------------------------------------------------------------------
//
//	TIR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetTIR(int channel) const
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	// タイマBはXM6では読み出しを禁止する(実機は1us x 14タイマ)
	if (channel == 1) {
		// (源平討魔伝)
		LOG0(Log::Warning, "タイマB データレジスタ読み出し");
		return (DWORD)((scheduler->GetTotalTime() % 13) + 1);
	}

	return mfp.tir[channel];
}

//---------------------------------------------------------------------------
//
//	タイマを進める
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Proceed(int channel)
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	// カウンタを減算
	if (mfp.tir[channel] > 0) {
		mfp.tir[channel]--;
	}
	else {
		mfp.tir[channel] = 0xff;
	}

	// 0になったらリロード、割り込み
	if (mfp.tir[channel] == 0) {
#if defined(MFP_LOG)
	LOG1(Log::Normal, "タイマ%c オーバーフロー", channel + 'A');
#endif	// MFP_LOG

		// リロード
		mfp.tir[channel] = mfp.tdr[channel];

		// イベントカウントモードは割り込みイベント発生(スピンディジーII)
		if (mfp.tcr[channel] == 0x08) {
			// GPIP変更を先に行い、少し遅れて割り込みを出すようにする
			// メタルオレンジEX(12ではダメ、要調査)
			timer[channel].SetTime(36);
		}
		else {
			// 通常割り込み
			Interrupt(TimerInt[channel], TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	タイマ割り込みテーブル
//
//---------------------------------------------------------------------------
const int MFP::TimerInt[4] = {
	13,									// Timer-A
	8,									// Timer-B
	5,									// Timer-C
	4									// Timer-D
};

//---------------------------------------------------------------------------
//
//	タイマ時間テーブル
//
//---------------------------------------------------------------------------
const DWORD MFP::TimerHus[8] = {
	0,									// タイマストップ
	2,									// 1.0us
	5,									// 2.5us
	8,									// 4us
	25,									// 12.5us
	32,									// 16us
	50,									// 25us
	100									// 50us
};

//---------------------------------------------------------------------------
//
//	GPDR設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetGPDR(DWORD data)
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT(data < 0x100);

	// DDRが1のビットのみ有効
	for (i=0; i<8; i++) {
		bit = (DWORD)(1 << i);
		if (mfp.ddr & bit) {
			if (data & bit) {
				SetGPIP(i, 1);
			}
			else {
				SetGPIP(i, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	GPIP設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetGPIP(int num, int value)
{
	DWORD data;

	ASSERT(this);
	ASSERT((num >= 0) && (num < 8));
	ASSERT((value == 0) || (value == 1));

	// バックアップを取る
	data = mfp.gpdr;

	// ビット作成
	mfp.gpdr &= (DWORD)(~(1 << num));
	if (value == 1) {
		mfp.gpdr |= (DWORD)(1 << num);
	}

	// 違っていれば割り込みチェック
	if (mfp.gpdr != data) {
		IntGPIP();
	}
}

//---------------------------------------------------------------------------
//
//	GPIP割り込みチェック
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntGPIP()
{
	DWORD data;
	int i;

	ASSERT(this);

	// Inside68kの記述は逆！MFPデータシートに従うと次の様。
	// AER0 1->0で割り込み
	// AER1 0->1で割り込み

	// ~AERとGPDRをXOR
	data = (DWORD)(~mfp.aer);
	data ^= (DWORD)mfp.gpdr;

	// BERを見て、1→0に変化するとき割り込み発生
	// (ただしパルス幅測定タイマがあればGPIP4,GPIP3は割り込み処理しない)
	for (i=0; i<8; i++) {
		if (data & 0x80) {
			if (!(mfp.ber & 0x80)) {
				if (i == 3) {
					// GPIP4。タイマAをチェック
					if ((mfp.tcr[0] & 0x0f) > 0x08) {
						data <<= 1;
						mfp.ber <<= 1;
						continue;
					}
				}
				if (i == 4) {
					// GPIP3。タイマBをチェック
					if ((mfp.tcr[1] & 0x0f) > 0x08) {
						data <<= 1;
						mfp.ber <<= 1;
						continue;
					}
				}

				// 割り込み要求(SORCERIAN X1->88)
				Interrupt(GPIPInt[i], TRUE);
			}
		}

		// 次へ
		data <<= 1;
		mfp.ber <<= 1;
	}

	// BERを作成
	mfp.ber = (DWORD)(~mfp.aer);
	mfp.ber ^= mfp.gpdr;
}

//---------------------------------------------------------------------------
//
//	GPIP割り込みテーブル
//
//---------------------------------------------------------------------------
const int MFP::GPIPInt[8] = {
	15,									// H-SYNC
	14,									// CIRQ
	7,									// (NoUse)
	6,									// V-DISP
	3,									// OPM
	2,									// POWER
	1,									// EXPON
	0									// ALARM
};

//---------------------------------------------------------------------------
//
//	レシーバステータス設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetRSR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// REのみ設定可能
	data &= 0x01;

	mfp.rsr &= ~0x01;
	mfp.rsr |= (DWORD)(mfp.rsr | data);
}

//---------------------------------------------------------------------------
//
//	CPU←MFP 受信
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Receive()
{
	ASSERT(this);

	// タイマB条件チェック
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}

	// USARTコントロール条件チェック
	if (mfp.ucr != 0x88) {
		return;
	}

	// レシーバディセーブルの場合、処理しない
	if (!(mfp.rsr & 0x01)) {
		return;
	}

	// BF=1の場合、特に引き取るデータは無い
	if (!(mfp.rsr & 0x80)) {
		return;
	}

	// データ引き取り。BF、OEを0に設定
	mfp.rsr &= (DWORD)~0xc0;
}

//---------------------------------------------------------------------------
//
//	トランスミッタステータス設定
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTSR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// BE,UE,ENDは直接クリアできない
	mfp.tsr = (DWORD)(mfp.tsr & 0xd0);
	data &= (DWORD)~0xd0;
	mfp.tsr = (DWORD)(mfp.tsr | data);

	// TE=1で、UE,ENDをクリア
	if (mfp.tsr & 0x01) {
		mfp.tsr = (DWORD)(mfp.tsr & ~0x50);
	}
}

//---------------------------------------------------------------------------
//
//	CPU→MFP 送信
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Transmit(DWORD data)
{
	ASSERT(this);

	// タイマB条件チェック
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}

	// USARTコントロール条件チェック
	if (mfp.ucr != 0x88) {
		return;
	}

	// トランスミッタディセーブルの場合、処理しない
	if (!(mfp.tsr & 0x01)) {
		return;
	}

	// ステータス及びデータをセット
	mfp.tsr = (DWORD)(mfp.tsr & ~0x80);
	mfp.tur = data;
#if defined(MFP_LOG)
	LOG1(Log::Normal, "USART送信データ受付 %02X", (BYTE)data);
#endif	// MFP_LOG
	return;
}

//---------------------------------------------------------------------------
//
//	USART処理
//
//---------------------------------------------------------------------------
void FASTCALL MFP::USART()
{
	ASSERT(this);
	ASSERT((mfp.readpoint >= 0) && (mfp.readpoint < 0x10));
	ASSERT((mfp.writepoint >= 0) && (mfp.writepoint < 0x10));
	ASSERT((mfp.datacount >= 0) && (mfp.datacount <= 0x10));

	// タイマ及びUSART設定をチェック
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}
	if (mfp.ucr != 0x88) {
		return;
	}

	//
	//	送信
	//

	if (!(mfp.tsr & 0x80)) {
		// ここでトランスミッタディセーブルなら、ENDが発生
		if (!(mfp.tsr & 0x01)) {
			mfp.tsr = (DWORD)(mfp.tsr & ~0x80);
			mfp.tsr = (DWORD)(mfp.tsr | 0x10);
			LOG0(Log::Warning, "USART 送信終了エラー");
			Interrupt(9, TRUE);
			return;
		}

		// バッファエンプティ、オートターンアラウンド
		mfp.tsr = (DWORD)(mfp.tsr | 0x80);
		if (mfp.tsr & 0x20) {
			mfp.tsr = (DWORD)(mfp.tsr & ~0x20);
			SetRSR((DWORD)(mfp.rsr | 0x01));
		}

		// キーボードへデータ送出、送信バッファエンプティ割り込み
#if defined(MFP_LOG)
		LOG1(Log::Normal, "USART送信 %02X", mfp.tur);
#endif	// MFP_LOG
		keyboard->Command(mfp.tur);
		Interrupt(10, TRUE);
	}
	else {
		if (!(mfp.tsr & 0x40)) {
			mfp.tsr = (DWORD)(mfp.tsr | 0x40);
			Interrupt(9, TRUE);
#if defined(MFP_LOG)
			LOG0(Log::Normal, "USART アンダーランエラー");
#endif	// MPF_LOG
		}
	}

	//
	//	受信
	//

	// 有効な受信データがなければ、何もしない
	if (mfp.datacount == 0) {
		return;
	}

	// レシーバディセーブルなら、何もしない
	if (!(mfp.rsr & 0x01)) {
		return;
	}

	// ここでロック
	sync->Lock();

	// 既に受信データがあれば、オーバーランとしてデータを捨てる
	if (mfp.rsr & 0x80) {
		mfp.rsr |= 0x40;
		mfp.readpoint = (mfp.readpoint + 1) & 0x0f;
		mfp.datacount--;
		sync->Unlock();

		LOG0(Log::Warning, "USART オーバーランエラー");
		Interrupt(11, TRUE);
		return;
	}

	// データ受信。BFをセットし、データを記憶
	mfp.rsr |= 0x80;
	mfp.rur = mfp.buffer[mfp.readpoint];
	mfp.readpoint = (mfp.readpoint + 1) & 0x0f;
	mfp.datacount--;
	sync->Unlock();

	Interrupt(12, TRUE);
}

//---------------------------------------------------------------------------
//
//	キーデータ受信
//
//---------------------------------------------------------------------------
void FASTCALL MFP::KeyData(DWORD data)
{
	ASSERT(this);
	ASSERT((mfp.readpoint >= 0) && (mfp.readpoint < 0x10));
	ASSERT((mfp.writepoint >= 0) && (mfp.writepoint < 0x10));
	ASSERT((mfp.datacount >= 0) && (mfp.datacount <= 0x10));
	ASSERT(data < 0x100);

	// ロック
	sync->Lock();

	// 書き込みポイントへ格納
	mfp.buffer[mfp.writepoint] = data;

	// 書き込みポイント移動、カウンタ＋１
	mfp.writepoint = (mfp.writepoint + 1) & 0x0f;
	mfp.datacount++;
	if (mfp.datacount > 0x10) {
		mfp.datacount = 0x10;
	}

	// Syncアンロック
	sync->Unlock();
}
