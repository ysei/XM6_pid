//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ デバイス共通 ]
//
//---------------------------------------------------------------------------

#if !defined(device_h)
#define device_h

class VM;

//===========================================================================
//
//	デバイス
//
//===========================================================================
class Device
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t id;						// ID
		const char *desc;				// 名称
		Device* next;					// 次のデバイス
	} device_t;

public:
	// 基本ファンクション
	Device(VM *p);
										// コンストラクタ
	virtual ~Device();
										// デストラクタ
	virtual int FASTCALL Init();
										// 初期化
	virtual void FASTCALL Cleanup();
										// クリーンアップ
	virtual void FASTCALL Reset();
										// リセット
	virtual int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	virtual int FASTCALL Load(Fileio *fio, int ver);
										// ロード
	virtual void FASTCALL ApplyCfg(const Config *config);
										// 設定適用
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// 外部API
	Device* FASTCALL GetNextDevice() const { return dev.next; }
										// 次のデバイスを取得
	void FASTCALL SetNextDevice(Device *p) { dev.next = p; }
										// 次のデバイスを設定
	uint32_t FASTCALL GetID() const		{ return dev.id; }
										// デバイスID取得
	const char* FASTCALL GetDesc() const { return dev.desc; }
										// デバイス名称取得
	VM* FASTCALL GetVM() const			{ return vm; }
										// VM取得
	virtual int FASTCALL Callback(Event *ev);
										// イベントコールバック

protected:
#if defined(XM6_USE_LOG)
	Log* FASTCALL GetLog() const		{ return log; }
										// ログ取得
	Log *log;
										// ログ
#endif
	device_t dev;
										// 内部データ
	VM *vm;
										// 仮想マシン本体
};

//===========================================================================
//
//	メモリマップドデバイス
//
//===========================================================================
class MemDevice : public Device
{
public:
	// 内部データ定義
	typedef struct {
		uint32_t first;					// 開始アドレス
		uint32_t last;						// 最終アドレス
	} memdev_t;

public:
	// 基本ファンクション
	MemDevice(VM *p);
										// コンストラクタ
	virtual int FASTCALL Init();
										// 初期化

	// メモリデバイス
	virtual uint32_t FASTCALL ReadByte(uint32_t addr);
										// バイト読み込み
	virtual uint32_t FASTCALL ReadWord(uint32_t addr);
										// ワード読み込み
	virtual void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// バイト書き込み
	virtual void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ワード書き込み
	virtual uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// 読み込みのみ
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// 外部API
	uint32_t FASTCALL GetFirstAddr() const	{ return memdev.first; }
										// 最初のアドレスを取得
	uint32_t FASTCALL GetLastAddr() const	{ return memdev.last; }
										// 最後のアドレスを取得

protected:
	memdev_t memdev;
										// 内部データ
	CPU *cpu;
										// CPU
	Scheduler *scheduler;
										// スケジューラ
};

#endif	// device_h
