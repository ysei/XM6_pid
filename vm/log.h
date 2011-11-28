//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ログ ]
//
//---------------------------------------------------------------------------

#if !defined(log_h)
#define log_h
#if defined(XM6_USE_LOG)
//===========================================================================
//
//	ログ
//
//===========================================================================
class Log
{
public:
	enum loglevel {
		Detail,							// 詳細レベル
		Normal,							// 通常レベル
		Warning							// 警告レベル
	};
	typedef struct {
		uint32_t number;					// 通し番号(リセットでクリア)
		uint32_t total;					// 通し番号(累積)
		uint32_t time;						// 仮想時間
		uint32_t id;						// デバイスID
		uint32_t pc;						// プログラムカウンタ
		loglevel level;					// レベル
		char *string;					// 文字列実体
	} logdata_t;

public:
	// 基本ファンクション
	Log();
										// コンストラクタ
	int FASTCALL Init(VM *vm);
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// 出力
	void Format(loglevel level, const Device *dev, char *format, ...);
										// ログ出力(...)
	void vFormat(loglevel level, const Device *dev, char *format, va_list args);
										// ログ出力(va)
	void FASTCALL AddString(uint32_t id, loglevel level, char *string);
										// ログデータ追加

	// 取得
	int FASTCALL GetNum() const;
										// ログ項目数を取得
	int FASTCALL GetMax() const;
										// ログ最大記録数を取得
	int FASTCALL GetData(int index, logdata_t *ptr);
										// ログデータ取得

private:
	enum {
		LogMax = 0x4000					// ログ最大記録数(2の倍数であること)
	};

private:
	void FASTCALL Clear();
										// データをクリア
	int logtop;
										// 先頭ポインタ(最も古い)
	int lognum;
										// ログ記録数
	int logcount;
										// ログカウント
	logdata_t *logdata[LogMax];
										// ログポインタ
	Sync *sync;
										// 同期オブジェクト
	CPU *cpu;
										// CPU
	Scheduler *scheduler;
										// スケジューラ
};

//---------------------------------------------------------------------------
//
//	ログ出力マクロ
//
//---------------------------------------------------------------------------
#if !defined(NO_LOG)
class LogProxy {
public:
	// コンストラクタ
	LogProxy(const Device* device, Log* log);

	// ログ出力
	void operator()(enum Log::loglevel level, char* format, ...) const;
private:
	const Device* m_device;
	Log* m_log;
};
#define LOG						LogProxy(this, GetLog())
#define LOG0(l, s)			  	GetLog()->Format(l, this, s)
#define LOG1(l, s, a)		  	GetLog()->Format(l, this, s, a)
#define LOG2(l, s, a, b)	  	GetLog()->Format(l, this, s, a, b)
#define LOG3(l, s, a, b, c)   	GetLog()->Format(l, this, s, a, b, c)
#define LOG4(l, s, a, b, c, d)	GetLog()->Format(l, this, s, a, b, c, d)
#else
#define LOG						LOG_NONE
#define LOG0(l, s)				((void)0)
#define LOG1(l, s, a)			((void)0)
#define LOG2(l, s, a, b)		((void)0)
#define LOG3(l, s, a, b, c)	  	((void)0)
#define LOG4(l, s, a, b, c, d)	((void)0)
static inline LOG_NONE(enum Log::loglevel level, char *format, ...) {}
#endif	// !NO_LOG
#else	//XM6_USE_LOG
#define LOG(...)				((void)0)
#define LOG0(l, s)				((void)0)
#define LOG1(l, s, a)			((void)0)
#define LOG2(l, s, a, b)		((void)0)
#define LOG3(l, s, a, b, c)	  	((void)0)
#define LOG4(l, s, a, b, c, d)	((void)0)
#endif	// XM6_USE_LOG
#endif	// log_h
