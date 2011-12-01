//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ イベント ]
//
//---------------------------------------------------------------------------

#if !defined(event_h)
#define event_h

//===========================================================================
//
//	イベント
//
//===========================================================================
class Event
{
public:
	// 内部データ定義
#if defined(_WIN32)
#pragma pack(push, 8)
#endif	// _WIN32
	typedef struct {
		uint32_t remain;					// +4  残り時間
		uint32_t time;						// +8  トータル時間
		uint32_t user;						// +12 ユーザ定義データ
		Device *device;					// +16 親デバイス
		Scheduler *scheduler;			// +20 スケジューラ
		Event *next;					// +24 次のイベント
		char desc[0x20];				// 名称
	} event_t;
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

public:
	// 基本ファンクション
	Event();
										// コンストラクタ
	virtual ~Event();
										// デストラクタ
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// ロード・セーブ
	int FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	int FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// プロパティ
	void FASTCALL SetDevice(Device *p);
										// 親デバイス設定
	Device* FASTCALL GetDevice() const	{ return ev.device; }
										// 親デバイス取得
#if defined(XM6_USE_EVENT_DESC)
	void FASTCALL SetDesc(const char *fmt, ...);
										// 名称設定
#else
	void FASTCALL SetDesc(const char *fmt, ...) {}	// 名称設定
#endif
	const char* FASTCALL GetDesc() const;
										// 名称取得
	void FASTCALL SetUser(uint32_t data)	{ ev.user = data; }
										// ユーザ定義データ設定
	uint32_t FASTCALL GetUser() const		{ return ev.user; }
										// ユーザ定義データ取得

	// 時間管理
	void FASTCALL SetTime(uint32_t hus);
										// 時間周期設定
	uint32_t FASTCALL GetTime() const		{ return ev.time; }
										// 時間周期取得
	uint32_t FASTCALL GetRemain() const	{ return ev.remain; }
										// 残り時間取得
	void FASTCALL Exec(uint32_t hus);
										// 時間を進める

	// リンク設定・削除
	void FASTCALL SetNextEvent(Event *p) { ev.next = p; }
										// 次のイベントを設定
	Event* FASTCALL GetNextEvent() const { return ev.next; }
										// 次のイベントを取得

private:
	// 内部データ定義(Ver2.01まで。enableがある)
	typedef struct {
		Device *device;					// 親デバイス
		Scheduler *scheduler;			// スケジューラ
		Event *next;					// 次のイベント
		char desc[0x20];				// 名称
		uint32_t user;						// ユーザ定義データ
		int enable;					// イネーブル時間
		uint32_t time;						// トータル時間
		uint32_t remain;					// 残り時間
	} event201_t;

	int FASTCALL Load201(Fileio *fio);
										// ロード(version 2.01以前)
	event_t ev;
										// 内部ワーク
};

#endif	// event_h
