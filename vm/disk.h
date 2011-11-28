//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ディスク ]
//
//---------------------------------------------------------------------------

#if !defined(disk_h)
#define disk_h

//---------------------------------------------------------------------------
//
//	クラス先行定義
//
//---------------------------------------------------------------------------
class DiskTrack;
class DiskCache;
class Disk;
class SASIHD;
class SCSIHD;
class SCSIMO;
class SCSICDTrack;
class SCSICD;
class Filepath;

//---------------------------------------------------------------------------
//
//	エラー定義(REQUEST SENSEで返されるセンスコード)
//
//	MSB		予約(0x00)
//			センスキー
//			拡張センスコード(ASC)
//	LSB		拡張センスコードクォリファイア(ASCQ)
//
//---------------------------------------------------------------------------
#define DISK_NOERROR		0x00000000	// NO ADDITIONAL SENSE INFO.
#define DISK_DEVRESET		0x00062900	// POWER ON OR RESET OCCURED
#define DISK_NOTREADY		0x00023a00	// MEDIUM NOT PRESENT
#define DISK_ATTENTION		0x00062800	// MEDIUIM MAY HAVE CHANGED
#define DISK_PREVENT		0x00045302	// MEDIUM REMOVAL PREVENTED
#define DISK_READFAULT		0x00031100	// UNRECOVERED READ ERROR
#define DISK_WRITEFAULT		0x00030300	// PERIPHERAL DEVICE WRITE FAULT
#define DISK_WRITEPROTECT	0x00042700	// WRITE PROTECTED
#define DISK_MISCOMPARE		0x000e1d00	// MISCOMPARE DURING VERIFY
#define DISK_INVALIDCMD		0x00052000	// INVALID COMMAND OPERATION CODE
#define DISK_INVALIDLBA		0x00052100	// LOGICAL BLOCK ADDR. OUT OF RANGE
#define DISK_INVALIDCDB		0x00052400	// INVALID FIELD IN CDB
#define DISK_INVALIDLUN		0x00052500	// LOGICAL UNIT NOT SUPPORTED
#define DISK_INVALIDPRM		0x00052600	// INVALID FIELD IN PARAMETER LIST
#define DISK_INVALIDMSG		0x00054900	// INVALID MESSAGE ERROR
#define DISK_PARAMLEN		0x00051a00	// PARAMETERS LIST LENGTH ERROR
#define DISK_PARAMNOT		0x00052601	// PARAMETERS NOT SUPPORTED
#define DISK_PARAMVALUE		0x00052602	// PARAMETERS VALUE INVALID
#define DISK_PARAMSAVE		0x00053900	// SAVING PARAMETERS NOT SUPPORTED

#if 0
#define DISK_AUDIOPROGRESS	0x00??0011	// AUDIO PLAY IN PROGRESS
#define DISK_AUDIOPAUSED	0x00??0012	// AUDIO PLAY PAUSED
#define DISK_AUDIOSTOPPED	0x00??0014	// AUDIO PLAY STOPPED DUE TO ERROR
#define DISK_AUDIOCOMPLETE	0x00??0013	// AUDIO PLAY SUCCESSFULLY COMPLETED
#endif

//===========================================================================
//
//	ディスクトラック
//
//===========================================================================
class DiskTrack
{
public:
	// 内部データ定義
	typedef struct {
		int track;						// トラックナンバー
		int size;						// セクタサイズ(8 or 9)
		int sectors;					// セクタ数(<=0x100)
		uint8_t *buffer;					// データバッファ
		int init;						// ロード済みか
		int changed;					// 変更済みフラグ
		int *changemap;				// 変更済みマップ
		int raw;						// RAWモード
	} disktrk_t;

public:
	// 基本ファンクション
	DiskTrack(int track, int size, int sectors, int raw = FALSE);
										// コンストラクタ
	virtual ~DiskTrack();
										// デストラクタ
	int FASTCALL Load(const Filepath& path);
										// ロード
	int FASTCALL Save(const Filepath& path);
										// セーブ

	// リード・ライト
	int FASTCALL Read(uint8_t *buf, int sec) const;
										// セクタリード
	int FASTCALL Write(const uint8_t *buf, int sec);
										// セクタライト

	// その他
	int FASTCALL GetTrack() const		{ return dt.track; }
										// トラック取得
	int FASTCALL IsChanged() const		{ return dt.changed; }
										// 変更フラグチェック

private:
	// 内部データ
	disktrk_t dt;
										// 内部データ
};

//===========================================================================
//
//	ディスクキャッシュ
//
//===========================================================================
class DiskCache
{
public:
	// 内部データ定義
	typedef struct {
		DiskTrack *disktrk;				// 割り当てトラック
		uint32_t serial;					// 最終シリアル
	} cache_t;

	// キャッシュ数
	enum {
		CacheMax = 16					// キャッシュするトラック数
	};

public:
	// 基本ファンクション
	DiskCache(const Filepath& path, int size, int blocks);
										// コンストラクタ
	virtual ~DiskCache();
										// デストラクタ
	void FASTCALL SetRawMode(int raw);
										// CD-ROM rawモード設定

	// アクセス
	int FASTCALL Save();
										// 全セーブ＆解放
	int FASTCALL Read(uint8_t *buf, int block);
										// セクタリード
	int FASTCALL Write(const uint8_t *buf, int block);
										// セクタライト
	int FASTCALL GetCache(int index, int& track, uint32_t& serial) const;
										// キャッシュ情報取得

private:
	// 内部管理
	void FASTCALL Clear();
										// トラックをすべてクリア
	DiskTrack* FASTCALL Assign(int track);
										// トラックのロード
	int FASTCALL Load(int index, int track);
										// トラックのロード
	void FASTCALL Update();
										// シリアル番号更新

	// 内部データ
	cache_t cache[CacheMax];
										// キャッシュ管理
	uint32_t serial;
										// 最終アクセスシリアルナンバ
//	Filepath sec_path;
	Filepath* pSec_path;
										// パス
	int sec_size;
										// セクタサイズ(8 or 9 or 11)
	int sec_blocks;
										// セクタブロック数
	int cd_raw;
										// CD-ROM RAWモード
};

//===========================================================================
//
//	ディスク
//
//===========================================================================
class Disk
{
public:
	// 内部ワーク
	typedef struct {
		uint32_t id;						// メディアID
		int ready;						// 有効なディスク
		int writep;					// 書き込み禁止
		int readonly;					// 読み込み専用
		int removable;					// 取り外し
		int lock;						// ロック
		int attn;						// アテンション
		int reset;						// リセット
		int size;						// セクタサイズ
		int blocks;						// 総セクタ数
		uint32_t lun;						// LUN
		uint32_t code;						// ステータスコード
		DiskCache *dcache;				// ディスクキャッシュ
	} disk_t;

public:
	// 基本ファンクション
	Disk(Device *dev);
										// コンストラクタ
	virtual ~Disk();
										// デストラクタ
	virtual void FASTCALL Reset();
										// デバイスリセット
	virtual int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	virtual int FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// ID
	uint32_t FASTCALL GetID() const		{ return disk.id; }
										// メディアID取得
	int FASTCALL IsNULL() const;
										// NULLチェック
	int FASTCALL IsSASI() const;
										// SASIチェック

	// メディア操作
	virtual int FASTCALL Open(const Filepath& path);
										// オープン
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	void FASTCALL Eject(int force);
										// イジェクト
	int FASTCALL IsReady() const		{ return disk.ready; }
										// Readyチェック
	void FASTCALL WriteP(int flag);
										// 書き込み禁止
	int FASTCALL IsWriteP() const		{ return disk.writep; }
										// 書き込み禁止チェック
	int FASTCALL IsReadOnly() const	{ return disk.readonly; }
										// Read Onlyチェック
	int FASTCALL IsRemovable() const	{ return disk.removable; }
										// リムーバブルチェック
	int FASTCALL IsLocked() const		{ return disk.lock; }
										// ロックチェック
	int FASTCALL IsAttn() const		{ return disk.attn; }
										// 交換チェック
	int FASTCALL Flush();
										// キャッシュフラッシュ
	void FASTCALL GetDisk(disk_t *buffer) const;
										// 内部ワーク取得

	// プロパティ
	void FASTCALL SetLUN(uint32_t lun)		{ disk.lun = lun; }
										// LUNセット
	uint32_t FASTCALL GetLUN()				{ return disk.lun; }
										// LUN取得

	// コマンド
	virtual int FASTCALL Inquiry(const uint32_t *cdb, uint8_t *buf);
										// INQUIRYコマンド
	virtual int FASTCALL RequestSense(const uint32_t *cdb, uint8_t *buf);
										// REQUEST SENSEコマンド
	int FASTCALL SelectCheck(const uint32_t *cdb);
										// SELECTチェック
	int FASTCALL ModeSelect(const uint8_t *buf, int size);
										// MODE SELECTコマンド
	int FASTCALL ModeSense(const uint32_t *cdb, uint8_t *buf);
										// MODE SENSEコマンド
	int FASTCALL TestUnitReady(const uint32_t *cdb);
										// TEST UNIT READYコマンド
	int FASTCALL Rezero(const uint32_t *cdb);
										// REZEROコマンド
	int FASTCALL Format(const uint32_t *cdb);
										// FORMAT UNITコマンド
	int FASTCALL Reassign(const uint32_t *cdb);
										// REASSIGN UNITコマンド
	virtual int FASTCALL Read(uint8_t *buf, int block);
										// READコマンド
	int FASTCALL WriteCheck(int block);
										// WRITEチェック
	int FASTCALL Write(const uint8_t *buf, int block);
										// WRITEコマンド
	int FASTCALL Seek(const uint32_t *cdb);
										// SEEKコマンド
	int FASTCALL StartStop(const uint32_t *cdb);
										// START STOP UNITコマンド
	int FASTCALL SendDiag(const uint32_t *cdb);
										// SEND DIAGNOSTICコマンド
	int FASTCALL Removal(const uint32_t *cdb);
										// PREVENT/ALLOW MEDIUM REMOVALコマンド
	int FASTCALL ReadCapacity(const uint32_t *cdb, uint8_t *buf);
										// READ CAPACITYコマンド
	int FASTCALL Verify(const uint32_t *cdb);
										// VERIFYコマンド
	virtual int FASTCALL ReadToc(const uint32_t *cdb, uint8_t *buf);
										// READ TOCコマンド
	virtual int FASTCALL PlayAudio(const uint32_t *cdb);
										// PLAY AUDIOコマンド
	virtual int FASTCALL PlayAudioMSF(const uint32_t *cdb);
										// PLAY AUDIO MSFコマンド
	virtual int FASTCALL PlayAudioTrack(const uint32_t *cdb);
										// PLAY AUDIO TRACKコマンド
	void FASTCALL InvalidCmd()			{ disk.code = DISK_INVALIDCMD; }
										// サポートしていないコマンド

protected:
	// サブ処理
	int FASTCALL AddError(int change, uint8_t *buf);
										// エラーページ追加
	int FASTCALL AddFormat(int change, uint8_t *buf);
										// フォーマットページ追加
	int FASTCALL AddOpt(int change, uint8_t *buf);
										// オプティカルページ追加
	int FASTCALL AddCache(int change, uint8_t *buf);
										// キャッシュページ追加
	int FASTCALL AddCDROM(int change, uint8_t *buf);
										// CD-ROMページ追加
	int FASTCALL AddCDDA(int change, uint8_t *buf);
										// CD-DAページ追加
	int FASTCALL CheckReady();
										// レディチェック

	// 内部データ
	disk_t disk;
										// ディスク内部データ
	Device *ctrl;
										// コントローラデバイス
//	Filepath diskpath;
	Filepath* pDiskpath;
										// パス(GetPath用)
};

//===========================================================================
//
//	SASI ハードディスク
//
//===========================================================================
class SASIHD : public Disk
{
public:
	// 基本ファンクション
	SASIHD(Device *dev);
										// コンストラクタ
	int FASTCALL Open(const Filepath& path);
										// オープン

	// メディア操作
	void FASTCALL Reset();
										// デバイスリセット

	// コマンド
	int FASTCALL RequestSense(const uint32_t *cdb, uint8_t *buf);
										// REQUEST SENSEコマンド
};

//===========================================================================
//
//	SCSI ハードディスク
//
//===========================================================================
class SCSIHD : public Disk
{
public:
	// 基本ファンクション
	SCSIHD(Device *dev);
										// コンストラクタ
	int FASTCALL Open(const Filepath& path);
										// オープン

	// コマンド
	int FASTCALL Inquiry(const uint32_t *cdb, uint8_t *buf);
										// INQUIRYコマンド
};

//===========================================================================
//
//	SCSI 光磁気ディスク
//
//===========================================================================
class SCSIMO : public Disk
{
public:
	// 基本ファンクション
	SCSIMO(Device *dev);
										// コンストラクタ
	int FASTCALL Open(const Filepath& path, int attn = TRUE);
										// オープン
	int FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// コマンド
	int FASTCALL Inquiry(const uint32_t *cdb, uint8_t *buf);
										// INQUIRYコマンド
};

//===========================================================================
//
//	CD-ROM トラック
//
//===========================================================================
class CDTrack
{
public:
	// 基本ファンクション
	CDTrack(SCSICD *scsicd);
										// コンストラクタ
	virtual ~CDTrack();
										// デストラクタ
	int FASTCALL Init(int track, uint32_t first, uint32_t last);
										// 初期化

	// プロパティ
	void FASTCALL SetPath(int cdda, const Filepath& path);
										// パス設定
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	void FASTCALL AddIndex(int index, uint32_t lba);
										// インデックス追加
	uint32_t FASTCALL GetFirst() const;
										// 開始LBA取得
	uint32_t FASTCALL GetLast() const;
										// 終端LBA取得
	uint32_t FASTCALL GetBlocks() const;
										// ブロック数取得
	int FASTCALL GetTrackNo() const;
										// トラック番号取得
	int FASTCALL IsValid(uint32_t lba) const;
										// 有効なLBAか
	int FASTCALL IsAudio() const;
										// オーディオトラックか

private:
	SCSICD *cdrom;
										// 親デバイス
	int valid;
										// 有効なトラック
	int track_no;
										// トラック番号
	uint32_t first_lba;
										// 開始LBA
	uint32_t last_lba;
										// 終了LBA
	int audio;
										// オーディオトラックフラグ
	int raw;
										// RAWデータフラグ
//	Filepath imgpath;
	Filepath* pImgpath;
										// イメージファイルパス
};

//===========================================================================
//
//	CD-DA バッファ
//
//===========================================================================
class CDDABuf
{
public:
	// 基本ファンクション
	CDDABuf();
										// コンストラクタ
	virtual ~CDDABuf();
										// デストラクタ
#if 0
	int Init();
										// 初期化
	int FASTCALL Load(const Filepath& path);
										// ロード
	int FASTCALL Save(const Filepath& path);
										// セーブ

	// API
	void FASTCALL Clear();
										// バッファクリア
	int FASTCALL Open(Filepath& path);
										// ファイル指定
	int FASTCALL GetBuf(uint32_t *buffer, int frames);
										// バッファ取得
	int FASTCALL IsValid();
										// 有効チェック
	int FASTCALL ReadReq();
										// 読み込み要求
	int FASTCALL IsEnd() const;
										// 終了チェック

private:
	Filepath wavepath;
										// Waveパス
	int valid;
										// オープン結果
	uint32_t *buf;
										// データバッファ
	uint32_t read;
										// Readポインタ
	uint32_t write;
										// Writeポインタ
	uint32_t num;
										// データ有効数
	uint32_t rest;
										// ファイル残りサイズ
#endif
};

//===========================================================================
//
//	SCSI CD-ROM
//
//===========================================================================
class SCSICD : public Disk
{
public:
	// トラック数
	enum {
		TrackMax = 96					// トラック最大数
	};

public:
	// 基本ファンクション
	SCSICD(Device *dev);
										// コンストラクタ
	virtual ~SCSICD();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, int attn = TRUE);
										// オープン
	int FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// コマンド
	int FASTCALL Inquiry(const uint32_t *cdb, uint8_t *buf);
										// INQUIRYコマンド
	int FASTCALL Read(uint8_t *buf, int block);
										// READコマンド
	int FASTCALL ReadToc(const uint32_t *cdb, uint8_t *buf);
										// READ TOCコマンド
	int FASTCALL PlayAudio(const uint32_t *cdb);
										// PLAY AUDIOコマンド
	int FASTCALL PlayAudioMSF(const uint32_t *cdb);
										// PLAY AUDIO MSFコマンド
	int FASTCALL PlayAudioTrack(const uint32_t *cdb);
										// PLAY AUDIO TRACKコマンド

	// CD-DA
	int FASTCALL NextFrame();
										// フレーム通知
	void FASTCALL GetBuf(uint32_t *buffer, int samples, uint32_t rate);
										// CD-DAバッファ取得

	// LBA-MSF変換
	void FASTCALL LBAtoMSF(uint32_t lba, uint8_t *msf) const;
										// LBA→MSF変換
	uint32_t FASTCALL MSFtoLBA(const uint8_t *msf) const;
										// MSF→LBA変換

private:
	// オープン
	int FASTCALL OpenCue(const Filepath& path);
										// オープン(CUE)
	int FASTCALL OpenIso(const Filepath& path);
										// オープン(ISO)
	int rawfile;
										// RAWフラグ

	// トラック管理
	void FASTCALL ClearTrack();
										// トラッククリア
	int FASTCALL SearchTrack(uint32_t lba) const;
										// トラック検索
	CDTrack* track[TrackMax];
										// トラックオブジェクト
	int tracks;
										// トラックオブジェクト有効数
	int dataindex;
										// 現在のデータトラック
	int audioindex;
										// 現在のオーディオトラック

	int frame;
										// フレーム番号

#if 0
	CDDABuf da_buf;
										// CD-DAバッファ
	int da_num;
										// CD-DAトラック数
	int da_cur;
										// CD-DAカレントトラック
	int da_next;
										// CD-DAネクストトラック
	int da_req;
										// CD-DAデータ要求
#endif
};

#endif	// disk_h
