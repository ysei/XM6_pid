//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SCSI(MB89352) ]
//
//---------------------------------------------------------------------------

#if !defined(scsi_h)
#define scsi_h

#include "device.h"
#include "event.h"
#include "disk.h"
class Filepath;

//===========================================================================
//
//	SCSI
//
//===========================================================================
class SCSI : public MemDevice
{
public:
	// 最大数
	enum {
		DeviceMax = 8,					// 最大SCSIデバイス数
		HDMax = 5						// 最大SCSI HD数
	};

	// フェーズ定義
	enum phase_t {
		busfree,						// バスフリーフェーズ
		arbitration,					// アービトレーションフェーズ
		selection,						// セレクションフェーズ
		reselection,					// リセレクションフェーズ
		command,						// コマンドフェーズ
		execute,						// 実行フェーズ
		msgin,							// メッセージインフェーズ
		msgout,							// メッセージアウトフェーズ
		datain,							// データインフェーズ
		dataout,						// データアウトフェーズ
		status							// ステータスフェーズ
	};

	// 内部データ定義
	typedef struct {
		// 全般
		int type;						// SCSIタイプ(0:なし 1:外付 2:内蔵)
		phase_t phase;					// フェーズ
		int id;							// カレントID(0-7)

		// 割り込み
		int vector;						// 要求ベクタ(-1で要求なし)
		int ilevel;						// 割り込みレベル

		// 信号
		int bsy;						// Busy信号
		int sel;						// Select信号
		int atn;						// Attention信号
		int msg;						// Message信号
		int cd;						// Command/Data信号
		int io;						// Input/Output信号
		int req;						// Request信号
		int ack;						// Ack信号
		int rst;						// Reset信号

		// レジスタ
		uint32_t bdid;						// BDIDレジスタ(ビット表示)
		uint32_t sctl;						// SCTLレジスタ
		uint32_t scmd;						// SCMDレジスタ
		uint32_t ints;						// INTSレジスタ
		uint32_t sdgc;						// SDGCレジスタ
		uint32_t pctl;						// PCTLレジスタ
		uint32_t mbc;						// MBCレジスタ
		uint32_t temp;						// TEMPレジスタ
		uint32_t tc;						// TCH,TCM,TCLレジスタ

		// コマンド
		uint32_t cmd[10];					// コマンドデータ
		uint32_t status;					// ステータスデータ
		uint32_t message;					// メッセージデータ

		// 転送
		int trans;						// 転送フラグ
		uint8_t buffer[0x800];				// 転送バッファ
		uint32_t blocks;					// 転送ブロック数
		uint32_t next;						// 次のレコード
		uint32_t offset;					// 転送オフセット
		uint32_t length;					// 転送残り長さ

		// コンフィグ
		int scsi_drives;				// SCSIドライブ数
		int memsw;						// メモリスイッチ更新
		int mo_first;					// MO優先フラグ(SxSI)

		// ディスク
		Disk *disk[DeviceMax];			// デバイス
		Disk *hd[HDMax];				// HD
		SCSIMO *mo;						// MO
		SCSICD *cdrom;					// CD-ROM
	} scsi_t;

public:
	// 基本ファンクション
	SCSI(VM *p);
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
	void FASTCALL GetSCSI(scsi_t *buffer) const;
										// 内部データ取得
	int FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck(int level);
										// 割り込みACK
	int FASTCALL GetSCSIID() const;
										// SCSI-ID取得
	int FASTCALL IsBusy() const;
										// BUSYか
	uint32_t FASTCALL GetBusyDevice() const;
										// BUSYデバイス取得

	// MO/CDアクセス
	int FASTCALL Open(const Filepath& path, int mo = TRUE);
										// MO/CD オープン
	void FASTCALL Eject(int force, int mo = TRUE);
										// MO/CD イジェクト
	void FASTCALL WriteP(int writep);
										// MO 書き込み禁止
	int FASTCALL IsWriteP() const;
										// MO 書き込み禁止チェック
	int FASTCALL IsReadOnly() const;
										// MO ReadOnlyチェック
	int FASTCALL IsLocked(int mo = TRUE) const;
										// MO/CD Lockチェック
	int FASTCALL IsReady(int mo = TRUE) const;
										// MO/CD Readyチェック
	int FASTCALL IsValid(int mo = TRUE) const;
										// MO/CD 有効チェック
	void FASTCALL GetPath(Filepath &path, int mo = TRUE) const;
										// MO/CD パス取得

	// CD-DA
	void FASTCALL GetBuf(uint32_t *buffer, int samples, uint32_t rate);
										// CD-DAバッファ取得

private:
	// レジスタ
	void FASTCALL ResetReg();
										// レジスタリセット
	void FASTCALL ResetCtrl();
										// 転送リセット
	void FASTCALL ResetBus(int reset);
										// バスリセット
	void FASTCALL SetBDID(uint32_t data);
										// BDID設定
	void FASTCALL SetSCTL(uint32_t data);
										// SCTL設定
	void FASTCALL SetSCMD(uint32_t data);
										// SCMD設定
	void FASTCALL SetINTS(uint32_t data);
										// INTS設定
	uint32_t FASTCALL GetPSNS() const;
										// PSNS取得
	void FASTCALL SetSDGC(uint32_t data);
										// SDGC設定
	uint32_t FASTCALL GetSSTS() const;
										// SSTS取得
	uint32_t FASTCALL GetSERR() const;
										// SERR取得
	void FASTCALL SetPCTL(uint32_t data);
										// PCTL設定
	uint32_t FASTCALL GetDREG();
										// DREG取得
	void FASTCALL SetDREG(uint32_t data);
										// DREG設定
	void FASTCALL SetTEMP(uint32_t data);
										// TEMP設定
	void FASTCALL SetTCH(uint32_t data);
										// TCH設定
	void FASTCALL SetTCM(uint32_t data);
										// TCM設定
	void FASTCALL SetTCL(uint32_t data);
										// TCL設定

	// SPCコマンド
	void FASTCALL BusRelease();
										// バスリリース
	void FASTCALL Select();
										// セレクション/リセレクション
	void FASTCALL ResetATN();
										// ATNライン=0
	void FASTCALL SetATN();
										// ATNライン=1
	void FASTCALL Transfer();
										// 転送
	void FASTCALL TransPause();
										// 転送中断
	void FASTCALL TransComplete();
										// 転送完了
	void FASTCALL ResetACKREQ();
										// ACK/REQライン=0
	void FASTCALL SetACKREQ();
										// ACK/REQライン=1

	// データ転送
	void FASTCALL Xfer(uint32_t *reg);
										// データ転送
	void FASTCALL XferNext();
										// データ転送継続
	int FASTCALL XferIn();
										// データ転送IN
	int FASTCALL XferOut(int cont);
										// データ転送OUT
	int FASTCALL XferMsg(uint32_t msg);
										// データ転送MSG

	// フェーズ
	void FASTCALL BusFree();
										// バスフリーフェーズ
	void FASTCALL Arbitration();
										// アービトレーションフェーズ
	void FASTCALL Selection();
										// セレクションフェーズ
	void FASTCALL SelectTime();
										// セレクションフェーズ(時間設定)
	void FASTCALL Command();
										// コマンドフェーズ
	void FASTCALL Execute();
										// 実行フェーズ
	void FASTCALL MsgIn();
										// メッセージインフェーズ
	void FASTCALL MsgOut();
										// メッセージアウトフェーズ
	void FASTCALL DataIn();
										// データインフェーズ
	void FASTCALL DataOut();
										// データアウトフェーズ
	void FASTCALL Status();
										// ステータスフェーズ

	// 割り込み
	void FASTCALL Interrupt(int type, int flag);
										// 割り込み要求
	void FASTCALL IntCheck();
										// 割り込みチェック

	// SCSIコマンド共通
	void FASTCALL Error();
										// 共通エラー

	// SCSIコマンド別
	void FASTCALL TestUnitReady();
										// TEST UNIT READYコマンド
	void FASTCALL Rezero();
										// REZERO UNITコマンド
	void FASTCALL RequestSense();
										// REQUEST SENSEコマンド
	void FASTCALL Format();
										// FORMAT UNITコマンド
	void FASTCALL Reassign();
										// REASSIGN BLOCKSコマンド
	void FASTCALL Read6();
										// READ(6)コマンド
	void FASTCALL Write6();
										// WRITE(6)コマンド
	void FASTCALL Seek6();
										// SEEK(6)コマンド
	void FASTCALL Inquiry();
										// INQUIRYコマンド
	void FASTCALL ModeSelect();
										// MODE SELECTコマンド
	void FASTCALL ModeSense();
										// MODE SENSEコマンド
	void FASTCALL StartStop();
										// START STOP UNITコマンド
	void FASTCALL SendDiag();
										// SEND DIAGNOSTICコマンド
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
	void FASTCALL ReadToc();
										// READ TOCコマンド
	void FASTCALL PlayAudio10();
										// PLAY AUDIO(10)コマンド
	void FASTCALL PlayAudioMSF();
										// PLAY AUDIO MSFコマンド
	void FASTCALL PlayAudioTrack();
										// PLAY AUDIO TRACK INDEXコマンド

	// CD-ROM・CD-DA
	Event cdda;
										// フレームイベント

	// ドライブ・ファイルパス
	void FASTCALL Construct();
										// ドライブ構築
	Filepath* scsihd[DeviceMax];
										// SCSI-HDファイルパス
	// その他
	scsi_t scsi;
										// 内部データ
	uint8_t verifybuf[0x800];
										// ベリファイバッファ
	Event event;
										// イベント
	Memory *memory;
										// メモリ
	SRAM *sram;
										// SRAM
};

#endif	// scsi_h
