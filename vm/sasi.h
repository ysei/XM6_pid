//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SASI ]
//
//---------------------------------------------------------------------------

#if !defined(sasi_h)
#define sasi_h

#include "device.h"
#include "event.h"
#include "disk.h"

//===========================================================================
//
//	SASI
//
//===========================================================================
class SASI : public MemDevice
{
public:
	// 最大数
	enum {
		SASIMax = 16,					// 最大SASIディスク数(LUN含む)
		SCSIMax = 6						// 最大SCSIハードディスク数
	};

	// フェーズ定義
	enum phase_t {
		busfree,						// バスフリーフェーズ
		selection,						// セレクションフェーズ
		command,						// コマンドフェーズ
		execute,						// 実行フェーズ
		read,							// 読み込みフェーズ
		write,							// 書き込みフェーズ
		status,							// ステータスフェーズ
		message							// メッセージフェーズ
	};

	// 内部データ定義
	typedef struct {
		// SASIコントローラ
		phase_t phase;					// フェーズ
		int sel;						// Select信号
		int msg;						// Message信号
		int cd;						// Command/Data信号
		int io;						// Input/Output信号
		int bsy;						// Busy信号
		int req;						// Request信号
		uint32_t ctrl;						// セレクトされたコントローラ
		uint32_t cmd[10];					// コマンドデータ
		uint32_t status;					// ステータスデータ
		uint32_t message;					// メッセージデータ
		uint8_t buffer[0x800];				// 転送バッファ
		uint32_t blocks;					// 転送ブロック数
		uint32_t next;						// 次のレコード
		uint32_t offset;					// 転送オフセット
		uint32_t length;					// 転送残り長さ

		// ディスク
		Disk *disk[SASIMax];			// ディスク
		Disk *current;					// ディスク(カレント)
		SCSIMO *mo;						// ディスク(MO)

		// コンフィグ
		int sasi_drives;				// ドライブ数(SASI)
		int memsw;						// メモリスイッチ更新
		int parity;					// パリティ付加
		int sxsi_drives;				// ドライブ数(SxSI)
		int mo_first;					// MO優先フラグ(SxSI)
		int scsi_type;					// SCSIタイプ

		// MOパラメータ
		int writep;					// MO書き込み禁止フラグ
	} sasi_t;

public:
	// 基本ファンクション
	SASI(VM *p);
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

	// MOアクセス
	int FASTCALL Open(const Filepath& path);
										// MO オープン
	void FASTCALL Eject(int force);
										// MO イジェクト
	void FASTCALL WriteP(int writep);
										// MO 書き込み禁止
	int FASTCALL IsWriteP() const;
										// MO 書き込み禁止チェック
	int FASTCALL IsReadOnly() const;
										// MO ReadOnlyチェック
	int FASTCALL IsLocked() const;
										// MO Lockチェック
	int FASTCALL IsReady() const;
										// MO Readyチェック
	int FASTCALL IsValid() const;
										// MO 有効チェック
	void FASTCALL GetPath(Filepath &path) const;
										// MO パス取得

	// 外部API
	void FASTCALL GetSASI(sasi_t *buffer) const;
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL Construct();
										// ディスク再構築
	int FASTCALL IsBusy() const;
										// HD BUSY取得
	uint32_t FASTCALL GetBusyDevice() const;
										// BUSYデバイス取得

private:
	uint32_t FASTCALL ReadData();
										// データ読み出し
	void FASTCALL WriteData(uint32_t data);
										// データ書き込み

	// フェーズ処理
	void FASTCALL BusFree();
										// バスフリーフェーズ
	void FASTCALL Selection(uint32_t data);
										// セレクションフェーズ
	void FASTCALL Command();
										// コマンドフェーズ
	void FASTCALL Execute();
										// 実行フェーズ
	void FASTCALL Status();
										// ステータスフェーズ
	void FASTCALL Message();
										// メッセージフェーズ
	void FASTCALL Error();
										// 共通エラー処理

	// コマンド
	void FASTCALL TestUnitReady();
										// TEST UNIT READYコマンド
	void FASTCALL Rezero();
										// REZERO UNITコマンド
	void FASTCALL RequestSense();
										// REQUEST SENSEコマンド
	void FASTCALL Format();
										// FORMATコマンド
	void FASTCALL Reassign();
										// REASSIGN BLOCKSコマンド
	void FASTCALL Read6();
										// READ(6)コマンド
	void FASTCALL Write6();
										// WRITE(6)コマンド
	void FASTCALL Seek6();
										// SEEK(6)コマンド
	void FASTCALL Assign();
										// ASSIGNコマンド
	void FASTCALL Inquiry();
										// INQUIRYコマンド
	void FASTCALL ModeSense();
										// MODE SENSEコマンド
	void FASTCALL StartStop();
										// START STOP UNITコマンド
	void FASTCALL Removal();
										// PREVENT/ALLOW MEDIUM REMOVALコマンド
	void FASTCALL ReadCapacity();
										// READ CAPACITYコマンド
	void FASTCALL Read10();
										// READ(10)コマンド
	void FASTCALL Write10();
										// WRITE(10)コマンド
	void FASTCALL Seek10();
										// SEEK(10)コマンド
	void FASTCALL Verify();
										// VERIFYコマンド
	void FASTCALL Specify();
										// SPECIFYコマンド

	// ワークエリア
	sasi_t sasi;
										// 内部データ
	Event event;
										// イベント
//	Filepath sasihd[SASIMax];
	Filepath* sasihd;
										// SASI-HDファイルパス
//	Filepath scsihd[SCSIMax];
	Filepath* scsihd;
										// SCSI-HDファイルパス
//	Filepath scsimo;
	Filepath* scsimo;
										// SCSI-MOファイルパス
	DMAC *dmac;
										// DMAC
	IOSC *iosc;
										// IOSC
	SRAM *sram;
										// SRAM
	SCSI *scsi;
										// SCSI
	int sxsicpu;
										// SxSI CPU転送フラグ
};

#endif	// sasi_h
