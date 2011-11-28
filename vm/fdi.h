//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ フロッピーディスクイメージ ]
//
//---------------------------------------------------------------------------

#if !defined(fdi_h)
#define fdi_h

#include "filepath.h"

//---------------------------------------------------------------------------
//
//	クラス先行定義
//
//---------------------------------------------------------------------------
class FDI;
class FDIDisk;
class FDITrack;
class FDISector;

class FDIDisk2HD;
class FDITrack2HD;
class FDIDiskDIM;
class FDITrackDIM;
class FDIDiskD68;
class FDITrackD68;
class FDIDiskBAD;
class FDITrackBAD;
class FDIDisk2DD;
class FDITrack2DD;
class FDIDisk2HQ;
class FDITrack2HQ;

//---------------------------------------------------------------------------
//
//	物理フォーマット定義
//
//---------------------------------------------------------------------------
#define FDI_2HD			0x00			// 2HD
#define FDI_2HDA		0x01			// 2HDA
#define FDI_2HS			0x02			// 2HS
#define FDI_2HC			0x03			// 2HC
#define FDI_2HDE		0x04			// 2HDE
#define FDI_2HQ			0x05			// 2HQ
#define FDI_N88B		0x06			// N88-BASIC
#define FDI_OS9			0x07			// OS-9/68000
#define FDI_2DD			0x08			// 2DD

//===========================================================================
//
//	FDIセクタ
//
//===========================================================================
class FDISector
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t chrn[4];					// CHRN
		int mfm;						// MFMフラグ
		int error;						// エラーコード
		int length;						// データ長
		int gap3;						// GAP3
		uint8_t *buffer;					// データバッファ
		uint32_t pos;						// ポジション
		int changed;					// 変更済みフラグ
		FDISector *next;				// 次のセクタ
	} sector_t;

public:
	// 基本ファンクション
	FDISector(int mfm, const uint32_t *chrn);
										// コンストラクタ
	virtual ~FDISector();
										// デストラクタ
	void FASTCALL Load(const uint8_t *buf, int len, int gap, int err);
										// 初期ロード

	// リード・ライト
	int FASTCALL IsMatch(int mfm, const uint32_t *chrn) const;
										// セクタマッチするか
	void FASTCALL GetCHRN(uint32_t *chrn)	const;
										// CHRNを取得
	int FASTCALL IsMFM() const			{ return sec.mfm; }
										// MFMか
	int FASTCALL Read(uint8_t *buf) const;
										// リード
	int FASTCALL Write(const uint8_t *buf, int deleted);
										// ライト
	int FASTCALL Fill(uint32_t d);
										// フィル

	// プロパティ
	const uint8_t* FASTCALL GetSector() const	{ return sec.buffer; }
										// セクタデータ取得
	int FASTCALL GetLength() const		{ return sec.length; }
										// データ長取得
	int FASTCALL GetError() const		{ return sec.error; }
										// エラーコード取得
	int FASTCALL GetGAP3() const		{ return sec.gap3; }
										// GAP3バイト数取得

	// ポジション
	void FASTCALL SetPos(uint32_t pos)		{  sec.pos = pos; }
										// ポジション設定
	uint32_t FASTCALL GetPos() const		{ return sec.pos; }
										// ポジション取得

	// 変更フラグ
	int FASTCALL IsChanged() const		{ return sec.changed; }
										// 変更フラグチェック
	void FASTCALL ClrChanged()			{ sec.changed = FALSE; }
										// 変更フラグを落とす
	void FASTCALL ForceChanged()		{ sec.changed = TRUE; }
										// 変更フラグを上げる

	// インデックス・リンク
	void FASTCALL SetNext(FDISector *next)	{ sec.next = next; }
										// 次のセクタを設定
	FDISector* FASTCALL GetNext() const	{ return sec.next; }
										// 次のセクタを取得

private:
	// 内部データ
	sector_t sec;
										// セクタ内部データ
};

//===========================================================================
//
//	FDIトラック
//
//===========================================================================
class FDITrack
{
public:
	// 内部ワーク
	typedef struct {
		FDIDisk *disk;					// 親ディスク
		int track;						// トラック
		int init;						// ロード済みか
		int sectors[3];					// 所有セクタ数(ALL/FM/MFM)
		int hd;						// 密度フラグ
		int mfm;						// 先頭セクタMFMフラグ
		FDISector *first;				// 最初のセクタ
		FDITrack *next;					// 次のトラック
	} track_t;

public:
	// 基本ファンクション
	FDITrack(FDIDisk *disk, int track, int hd = TRUE);
										// コンストラクタ
	virtual ~FDITrack();
										// デストラクタ
	virtual int FASTCALL Save(Fileio *fio, uint32_t offset);
										// セーブ
	virtual int FASTCALL Save(const Filepath& path, uint32_t offset);
										// セーブ
	void FASTCALL Create(uint32_t phyfmt);
										// 物理フォーマット
	int FASTCALL IsHD() const			{ return trk.hd; }
										// HDフラグ取得

	// リード・ライト
	int FASTCALL ReadID(uint32_t *buf, int mfm);
										// リードID
	int FASTCALL ReadSector(uint8_t *buf, int *len, int mfm, const uint32_t *chrn);
										// リードセクタ
	int FASTCALL WriteSector(const uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(uint8_t *buf, int *len, int mfm, const uint32_t *chrn);
										// リードダイアグ
	virtual int FASTCALL WriteID(const uint8_t *buf, uint32_t d, int sc, int mfm, int gpl);
										// ライトID

	// インデックス・リンク
	int FASTCALL GetTrack() const		{ return trk.track; }
										// このトラックを取得
	void FASTCALL SetNext(FDITrack *p)	{ trk.next = p; }
										// 次のトラックを設定
	FDITrack* FASTCALL GetNext() const	{ return trk.next; }
										// 次のトラックを取得

	// セクタ
	int FASTCALL IsChanged() const;
										// 変更フラグチェック
	uint32_t FASTCALL GetTotalLength() const;
										// セクタレングス累計算出
	void FASTCALL ForceChanged();
										// 強制変更
	FDISector* FASTCALL Search(int mfm, const uint32_t *chrn);
										// 条件に合うセクタをサーチ
	FDISector* FASTCALL GetFirst() const{ return trk.first; }
										// 最初のセクタを取得

protected:
	// 物理フォーマット
	void FASTCALL Create2HD(int lim);
										// 物理フォーマット(2HD)
	void FASTCALL Create2HS();
										// 物理フォーマット(2HS)
	void FASTCALL Create2HC();
										// 物理フォーマット(2HC)
	void FASTCALL Create2HDE();
										// 物理フォーマット(2HDE)
	void FASTCALL Create2HQ();
										// 物理フォーマット(2HQ)
	void FASTCALL CreateN88B();
										// 物理フォーマット(N88-BASIC)
	void FASTCALL CreateOS9();
										// 物理フォーマット(OS-9)
	void FASTCALL Create2DD();
										// 物理フォーマット(2DD)

	// ディスク、回転管理
	FDIDisk* FASTCALL GetDisk() const	{ return trk.disk; }
										// ディスク取得
	int FASTCALL IsMFM() const			{ return trk.mfm; }
										// 先頭セクタがMFMか
	int FASTCALL GetGAP1() const;
										// GAP1長さ取得
	int FASTCALL GetTotal() const;
										// トータル長さ取得
	void FASTCALL CalcPos();
										// セクタ先頭の位置を算出
	uint32_t FASTCALL GetSize(FDISector *sector) const;
										// セクタのサイズ(ID,GAP3含む)を取得
	FDISector* FASTCALL GetCurSector() const;
										// カレント位置以降の最初のセクタを取得

	// トラック
	int IsInit() const					{ return trk.init; }

	// セクタ
	void FASTCALL AddSector(FDISector *sector);
										// セクタ追加
	void FASTCALL ClrSector();
										// セクタ全削除
	int FASTCALL GetAllSectors() const	{ return trk. sectors[0]; }
										// セクタ数取得(All)
	int FASTCALL GetMFMSectors() const	{ return trk.sectors[1]; }
										// セクタ数取得(MFM)
	int FASTCALL GetFMSectors() const	{ return trk.sectors[2]; }
										// セクタ数取得(FM)

	// ダイアグ
	int FASTCALL MakeGAP1(uint8_t *buf, int offset) const;
										// GAP1作成
	int FASTCALL MakeSector(uint8_t *buf, int offset, FDISector *sector) const;
										// セクタデータ作成
	int FASTCALL MakeGAP4(uint8_t *buf, int offset) const;
										// GAP4作成
	int FASTCALL MakeData(uint8_t *buf, int offset, uint8_t data, int length) const;
										// Diagデータ作成
uint16_t FASTCALL CalcCRC(uint8_t *buf, int length) const;
										// CRC算出
	static const uint16_t CRCTable[0x100];
										// CRC算出テーブル

	// 内部データ
	track_t trk;
										// トラック内部データ
};

//===========================================================================
//
//	FDIディスク
//
//===========================================================================
class FDIDisk
{
public:
	// 新規オプション定義
	typedef struct {
		uint32_t phyfmt;					// 物理フォーマット種別
		int logfmt;					// 論理フォーマット有無
		char name[60];					// コメント(DIM)/ディスク名(D68)
	} option_t;

	// 内部データ定義
	typedef struct {
		int index;						// インデックス
		FDI *fdi;						// 親FDI
		uint32_t id;						// ID
		int writep;					// 書き込み禁止
		int readonly;					// 読み込み専用
		char name[60];					// ディスク名
		Filepath path;					// パス
		uint32_t offset;					// ファイルのオフセット
		FDITrack *first;				// 最初のトラック
		FDITrack *head[2];				// ヘッドに対応したトラック
		int search;						// 検索時間(１周=0x10000)
		FDIDisk *next;					// 次のディスク
	} disk_t;

public:
	FDIDisk(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk();
										// デストラクタ
	uint32_t FASTCALL GetID() const		{ return disk.id; }
										// ID取得

	// メディア操作
	virtual int FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	virtual int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL GetName(char *buf) const;
										// ディスク名取得
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	int FASTCALL IsWriteP() const		{ return disk.writep; }
										// ライトプロテクトか
	int FASTCALL IsReadOnly() const	{ return disk.readonly; }
										// Read Onlyディスクイメージか
	void FASTCALL WriteP(int flag);
										// 書き込み禁止をセット
	virtual int FASTCALL Flush();
										// バッファをフラッシュ

	// アクセス
	virtual void FASTCALL Seek(int c);
										// シーク
	int FASTCALL ReadID(uint32_t *buf, int mfm, int hd);
										// リードID
	int FASTCALL ReadSector(uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int hd);
										// リードセクタ
	int FASTCALL WriteSector(const uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int hd, int deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int hd);
										// リードダイアグ
	int FASTCALL WriteID(const uint8_t *buf, uint32_t d, int sc, int mfm, int hd, int gpl);
										// ライトID

	// 回転管理
	uint32_t FASTCALL GetRotationPos() const;
										// 回転位置取得
	uint32_t FASTCALL GetRotationTime() const;
										// 回転時間取得
	uint32_t FASTCALL GetSearch() const	{ return disk.search; }
										// 検索長さ取得
	void FASTCALL SetSearch(uint32_t len)	{ disk.search = len; }
										// 検索長さ設定
	void FASTCALL CalcSearch(uint32_t pos);
										// 検索長さ算出
	int FASTCALL IsHD() const;
										// ドライブHD状態取得
	FDITrack* FASTCALL Search(int track) const;
										// トラックサーチ

	// インデックス・リンク
	int FASTCALL GetIndex() const		{ return disk.index; }
										// インデックス取得
	void FASTCALL SetNext(FDIDisk *p)	{ disk.next = p; }
										// 次のトラックを設定
	FDIDisk* FASTCALL GetNext() const	{ return disk.next; }
										// 次のトラックを取得

protected:
	// 論理フォーマット
	void FASTCALL Create2HD(int flag2hd);
										// 論理フォーマット(2HD, 2HDA)
	static const uint8_t IPL2HD[0x200];
										// IPL(2HD, 2HDA)
	void FASTCALL Create2HS();
										// 論理フォーマット(2HS)
	static const uint8_t IPL2HS[0x800];
										// IPL(2HS)
	void FASTCALL Create2HC();
										// 論理フォーマット(2HC)
	static const uint8_t IPL2HC[0x200];
										// IPL(2HC)
	void FASTCALL Create2HDE();
										// 論理フォーマット(2HDE)
	static const uint8_t IPL2HDE[0x800];
										// IPL(2HDE)
	void FASTCALL Create2HQ();
										// 論理フォーマット(2HQ)
	static const uint8_t IPL2HQ[0x200];
										// IPL(2HQ)
	void FASTCALL Create2DD();
										// 論理フォーマット(2DD)

	// イメージ
	FDI* FASTCALL GetFDI() const		{ return disk.fdi; }
										// 親イメージ取得

	// トラック
	void FASTCALL AddTrack(FDITrack *track);
										// トラック追加
	void FASTCALL ClrTrack();
										// トラック全削除
	FDITrack* FASTCALL GetFirst() const	{ return disk.first; }
										// 最初のトラックを取得
	FDITrack* FASTCALL GetHead(int idx) { ASSERT((idx==0)||(idx==1)); return disk.head[idx]; }
										// ヘッドに対応するトラックを取得

	// 内部データ
	disk_t disk;
										// ディスク内部データ
};

//===========================================================================
//
//	FDI
//
//===========================================================================
class FDI
{
public:
	// 内部データ定義
	typedef struct {
		FDD *fdd;						// FDD
		int disks;						// ディスク数
		FDIDisk *first;					// 最初のディスク
		FDIDisk *disk;					// 現在のディスク
	} fdi_t;

public:
	FDI(FDD *fdd);
										// コンストラクタ
	virtual ~FDI();
										// デストラクタ

	// メディア操作
	int FASTCALL Open(const Filepath& path, int media);
										// オープン
	uint32_t FASTCALL GetID() const;
										// ID取得
	int FASTCALL IsMulti() const;
										// マルチディスクイメージか
	FDIDisk* GetDisk() const			{ return fdi.disk; }
										// 現在のディスクを取得
	int FASTCALL GetDisks() const		{ return fdi.disks; }
										// ディスク数を取得
	int FASTCALL GetMedia() const;
										// マルチディスクインデックス取得
	void FASTCALL GetName(char *buf, int index = -1) const;
										// ディスク名取得
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	int FASTCALL Load(Fileio *fio, int ver, int *ready, int *media, Filepath& path);
										// ロード
	void FASTCALL Adjust();
										// 調整(特殊)

	// メディア状態
	int FASTCALL IsReady() const;
										// メディアがセットされているか
	int FASTCALL IsWriteP() const;
										// ライトプロテクトか
	int FASTCALL IsReadOnly() const;
										// Read Onlyディスクイメージか
	void FASTCALL WriteP(int flag);
										// 書き込み禁止をセット

	// アクセス
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL ReadID(uint32_t *buf, int mfm, int hd);
										// リードID
	int FASTCALL ReadSector(uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int hd);
										// リードセクタ
	int FASTCALL WriteSector(const uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int hd, int deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(uint8_t *buf, int *len, int mfm, const uint32_t *chrn, int hd);
										// リードダイアグ
	int FASTCALL WriteID(const uint8_t *buf, uint32_t d, int sc, int mfm, int hd, int gpl);
										// ライトID

	// 回転管理
	uint32_t FASTCALL GetRotationPos() const;
										// 回転位置取得
	uint32_t FASTCALL GetRotationTime() const;
										// 回転時間取得
	uint32_t FASTCALL GetSearch() const;
										// 検索時間取得
	int FASTCALL IsHD() const;
										// ドライブHD状態取得

private:
	// ドライブ
	FDD* FASTCALL GetFDD() const		{ return fdi.fdd; }

	// ディスク
	void FASTCALL AddDisk(FDIDisk *disk);
										// ディスク追加
	void FASTCALL ClrDisk();
										// ディスク全削除
	FDIDisk* GetFirst() const			{ return fdi.first; }
										// 最初のディスクを取得
	FDIDisk* FASTCALL Search(int index) const;
										// ディスクサーチ

	// 内部データ
	fdi_t fdi;
										// FDI内部データ
};

//===========================================================================
//
//	FDIトラック(2HD)
//
//===========================================================================
class FDITrack2HD : public FDITrack
{
public:
	// 基本ファンクション
	FDITrack2HD(FDIDisk *disk, int track);
										// コンストラクタ
	int FASTCALL Load(const Filepath& path, uint32_t offset);
										// ロード
};

//===========================================================================
//
//	FDIディスク(2HD)
//
//===========================================================================
class FDIDisk2HD : public FDIDisk
{
public:
	FDIDisk2HD(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk2HD();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	int FASTCALL Flush();
										// バッファをフラッシュ
};

//===========================================================================
//
//	FDIトラック(DIM)
//
//===========================================================================
class FDITrackDIM : public FDITrack
{
public:
	// 基本ファンクション
	FDITrackDIM(FDIDisk *disk, int track, int type);
										// コンストラクタ
	int FASTCALL Load(const Filepath& path, uint32_t offset, int load);
										// ロード
	int FASTCALL IsDIMMFM() const		{ return dim_mfm; }
										// DIM MFMフラグ取得
	int FASTCALL GetDIMSectors() const	{ return dim_secs; }
										// DIM セクタ数取得
	int FASTCALL GetDIMN() const		{ return dim_n; }
										// DIM レングス取得

private:
	int dim_mfm;
										// DIM MFMフラグ
	int dim_secs;
										// DIM セクタ数
	int dim_n;
										// DIM レングス
	int dim_type;
										// DIM タイプ
};

//===========================================================================
//
//	FDIディスク(DIM)
//
//===========================================================================
class FDIDiskDIM : public FDIDisk
{
public:
	FDIDiskDIM(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDiskDIM();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	int FASTCALL Flush();
										// バッファをフラッシュ

private:
	int FASTCALL GetDIMMap(int track) const;
										// トラックマップ取得
	uint32_t FASTCALL GetDIMOffset(int track) const;
										// トラックオフセット取得
	int FASTCALL Save();
										// 削除前の保存
	uint8_t dim_hdr[0x100];
										// DIMヘッダ
	int dim_load;
										// ヘッダ確認フラグ
};

//===========================================================================
//
//	FDIトラック(D68)
//
//===========================================================================
class FDITrackD68 : public FDITrack
{
public:
	// 基本ファンクション
	FDITrackD68(FDIDisk *disk, int track, int hd);
										// コンストラクタ
	int FASTCALL Load(const Filepath& path, uint32_t offset);
										// ロード
	int FASTCALL Save(const Filepath& path, uint32_t offset);
										// セーブ
	int FASTCALL Save(Fileio *fio, uint32_t offset);
										// セーブ
	int FASTCALL WriteID(const uint8_t *buf, uint32_t d, int sc, int mfm, int gpl);
										// ライトID
	void FASTCALL ForceFormat()			{ d68_format = TRUE; }
										// 強制フォーマット
	int FASTCALL IsFormated() const	{ return d68_format; }
										// フォーマット変更されているか
	uint32_t FASTCALL GetD68Length() const;
										// D68形式での長さを取得

private:
	int d68_format;
										// フォーマット変更フラグ
	static const int Gap3Table[];
										// GAP3テーブル
};

//===========================================================================
//
//	FDIディスク(D68)
//
//===========================================================================
class FDIDiskD68 : public FDIDisk
{
public:
	FDIDiskD68(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDiskD68();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	void FASTCALL AdjustOffset();
										// オフセット更新
	static int FASTCALL CheckDisks(const Filepath& path, uint32_t *offbuf);
										// D68ヘッダチェック
	int FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	int FASTCALL Flush();
										// バッファをフラッシュ

private:
	uint32_t FASTCALL GetD68Offset(int track) const;
										// トラックオフセット取得
	int FASTCALL Save();
										// 削除前の保存
	uint8_t d68_hdr[0x2b0];
										// D68ヘッダ
	int d68_load;
										// ヘッダ確認フラグ
};

//===========================================================================
//
//	FDIトラック(BAD)
//
//===========================================================================
class FDITrackBAD : public FDITrack
{
public:
	// 基本ファンクション
	FDITrackBAD(FDIDisk *disk, int track);
										// コンストラクタ
	int FASTCALL Load(const Filepath& path, uint32_t offset);
										// ロード
	int FASTCALL Save(const Filepath& path, uint32_t offset);
										// セーブ

private:
	int bad_secs;
										// セクタ数
};

//===========================================================================
//
//	FDIディスク(BAD)
//
//===========================================================================
class FDIDiskBAD : public FDIDisk
{
public:
	FDIDiskBAD(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDiskBAD();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL Flush();
										// バッファをフラッシュ
};

//===========================================================================
//
//	FDIトラック(2DD)
//
//===========================================================================
class FDITrack2DD : public FDITrack
{
public:
	// 基本ファンクション
	FDITrack2DD(FDIDisk *disk, int track);
										// コンストラクタ
	int FASTCALL Load(const Filepath& path, uint32_t offset);
										// ロード
};

//===========================================================================
//
//	FDIディスク(2DD)
//
//===========================================================================
class FDIDisk2DD : public FDIDisk
{
public:
	FDIDisk2DD(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk2DD();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	int FASTCALL Flush();
										// バッファをフラッシュ
};

//===========================================================================
//
//	FDIトラック(2HQ)
//
//===========================================================================
class FDITrack2HQ : public FDITrack
{
public:
	// 基本ファンクション
	FDITrack2HQ(FDIDisk *disk, int track);
										// コンストラクタ
	int FASTCALL Load(const Filepath& path, uint32_t offset);
										// ロード
};

//===========================================================================
//
//	FDIディスク(2HQ)
//
//===========================================================================
class FDIDisk2HQ : public FDIDisk
{
public:
	FDIDisk2HQ(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk2HQ();
										// デストラクタ
	int FASTCALL Open(const Filepath& path, uint32_t offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	int FASTCALL Flush();
										// バッファをフラッシュ
};

#endif	// fdi_h
