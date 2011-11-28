//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ FDD(FD55GFR) ]
//
//---------------------------------------------------------------------------

#if !defined(fdd_h)
#define fdd_h

#include "device.h"
#include "event.h"
//#include "filepath.h"
class Filepath;

//---------------------------------------------------------------------------
//
//	エラー定義
//	※上位ST1, 下位ST2, 空きビット一部使用
//
//---------------------------------------------------------------------------
#define FDD_NOERROR			0x0000		// エラー無し
#define FDD_EOT				0x8000		// EOTオーバー
#define FDD_DDAM			0x4000		// DDAMセクタ
#define FDD_DATAERR			0x2000		// IDCRCまたはデータCRC(ReadIDを除く)
#define FDD_OVERRUN			0x1000		// オーバーラン(ロストデータ)
#define FDD_IDCRC			0x0800		// IDフィールドCRC
#define FDD_NODATA			0x0400		// 有効なセクタなし
#define FDD_NOTWRITE		0x0200		// 書き込み禁止信号を検出
#define FDD_MAM				0x0100		// アドレスマークなし
#define FDD_NOTREADY		0x0080		// ノットレディ
#define FDD_CM				0x0040		// DDAMまたはDAMを検出
#define FDD_DATACRC			0x0020		// データCRC
#define FDD_NOCYL			0x0010		// シリンダが異なる
#define FDD_SCANEQ			0x0008		// SCANで一致した
#define FDD_SCANNOT			0x0004		// SCANで一致するものがない
#define FDD_BADCYL			0x0002		// シリンダが不正
#define FDD_MDAM			0x0001		// DAMが見つからない

//---------------------------------------------------------------------------
//
//	ドライブステータス定義
//
//---------------------------------------------------------------------------
#define FDST_INSERT			0x80		// 挿入(誤挿入含む)
#define FDST_INVALID		0x40		// 誤挿入
#define FDST_EJECT			0x20		// イジェクトできる
#define FDST_BLINK			0x10		// 点滅状態
#define FDST_CURRENT		0x08		// 点滅のどちらかの状態を示す
#define FDST_MOTOR			0x04		// モータ安定回転中
#define FDST_SELECT			0x02		// セレクト中
#define FDST_ACCESS			0x01		// アクセス中

//===========================================================================
//
//	FDD
//
//===========================================================================
class FDD : public Device
{
public:
	// ドライブデータ定義
	typedef struct {
		FDI *fdi;						// フロッピーディスクイメージ
		FDI *next;						// 次に挿入するイメージ
		int seeking;					// シーク中
		int cylinder;					// シリンダ
		int insert;					// 挿入
		int invalid;					// 誤挿入
		int eject;						// イジェクトできる
		int blink;						// 挿入されていなければ点滅
		int access;					// アクセス中
	} drv_t;

	// 内部データ定義
	typedef struct {
		int motor;						// モータフラグ
		int settle;					// セトリング中
		int force;						// 強制レディフラグ
		int selected;					// セレクトドライブ
		int first;						// モータON後の初回シーク
		int hd;						// HDフラグ

		int fast;						// 高速モード
	} fdd_t;

public:
	// 基本ファンクション
	FDD(VM *p);
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

	// 外部API
	void FASTCALL GetDrive(int drive, drv_t *buffer) const;
										// ドライブワーク取得
	void FASTCALL GetFDD(fdd_t *buffer) const;
										// 内部ワーク取得
	FDI* FASTCALL GetFDI(int drive);
										// FDI取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL ForceReady(int flag);
										// 強制レディ
	uint32_t FASTCALL GetRotationPos() const;
										// 回転位置取得
	uint32_t FASTCALL GetRotationTime() const;
										// 回転時間取得
	uint32_t FASTCALL GetSearch();
										// 検索時間取得
	void FASTCALL SetHD(int hd);
										// HDフラグ設定
	int FASTCALL IsHD() const;
										// HDフラグ取得
	void FASTCALL Access(int flag);
										// アクセスLED設定

	// ドライブ別
	int FASTCALL Open(int drive, const Filepath& path, int media = 0);
										// イメージオープン
	void FASTCALL Insert(int drive);
										// インサート
	void FASTCALL Eject(int drive, int force);
										// イジェクト
	void FASTCALL Invalid(int drive);
										// 誤挿入
	void FASTCALL Control(int drive, uint32_t func);
										// ドライブコントロール
	int FASTCALL IsReady(int drive, int fdc = TRUE) const;
										// レディチェック
	int FASTCALL IsWriteP(int drive) const;
										// 書き込み禁止チェック
	int FASTCALL IsReadOnly(int drive) const;
										// Read Onlyチェック
	void FASTCALL WriteP(int drive, int flag);
										// 書き込み禁止設定
	int FASTCALL GetStatus(int drive) const;
										// ドライブステータス取得
	void FASTCALL SetMotor(int drive, int flag);
										// モータ設定＋ドライブセレクト
	int FASTCALL GetCylinder(int drive) const;
										// シリンダ取得
	void FASTCALL GetName(int drive, char *buf, int media = -1) const;
										// ディスク名取得
	void FASTCALL GetPath(int drive, Filepath& path) const;
										// パス取得
	int FASTCALL GetDisks(int drive) const;
										// イメージ内ディスク枚数取得
	int FASTCALL GetMedia(int drive) const;
										// イメージ内カレントメディア取得

	// シーク
	void FASTCALL Recalibrate(uint32_t srt);
										// リキャリブレート
	void FASTCALL StepIn(int step, uint32_t srt);
										// ステップイン
	void FASTCALL StepOut(int step, uint32_t srt);
										// ステップアウト

	// 読み込み・書き込み
	int FASTCALL ReadID(uint32_t *buf, int mfm, int hd);
										// リードID
	int FASTCALL ReadSector(uint8_t *buf, int *len, int mfm, uint32_t *chrn, int hd);
										// リードセクタ
	int FASTCALL WriteSector(const uint8_t *buf, int *len, int mfm, uint32_t *chrn, int hd, int deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(uint8_t *buf, int *len, int mfm, uint32_t *chrn, int hd);
										// リードダイアグ
	int FASTCALL WriteID(const uint8_t *buf, uint32_t d, int sc, int mfm, int hd, int gpl);
										// ライトID

private:
	void FASTCALL SeekInOut(int cylinder, uint32_t srt);
										// シーク共通
	void FASTCALL Rotation();
										// モータ回転
	FDC *fdc;
										// FDC
	IOSC *iosc;
										// IOSC
	RTC *rtc;
										// RTC
	drv_t drv[4];
										// ドライブデータ
	fdd_t fdd;
										// 内部データ
	Event eject;
										// イジェクトイベント
	Event seek;
										// シークイベント
	Event rotation;
										// 回転数イベント
};

#endif	// fdd_h
