//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ���O ]
//
//---------------------------------------------------------------------------

#if !defined(log_h)
#define log_h
#if defined(XM6_USE_LOG)
//===========================================================================
//
//	���O
//
//===========================================================================
class Log
{
public:
	enum loglevel {
		Detail,							// �ڍ׃��x��
		Normal,							// �ʏ탌�x��
		Warning							// �x�����x��
	};
	typedef struct {
		uint32_t number;					// �ʂ��ԍ�(���Z�b�g�ŃN���A)
		uint32_t total;					// �ʂ��ԍ�(�ݐ�)
		uint32_t time;						// ���z����
		uint32_t id;						// �f�o�C�XID
		uint32_t pc;						// �v���O�����J�E���^
		loglevel level;					// ���x��
		char *string;					// ���������
	} logdata_t;

public:
	// ��{�t�@���N�V����
	Log();
										// �R���X�g���N�^
	int FASTCALL Init(VM *vm);
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �o��
	void Format(loglevel level, const Device *dev, char *format, ...);
										// ���O�o��(...)
	void vFormat(loglevel level, const Device *dev, char *format, va_list args);
										// ���O�o��(va)
	void FASTCALL AddString(uint32_t id, loglevel level, char *string);
										// ���O�f�[�^�ǉ�

	// �擾
	int FASTCALL GetNum() const;
										// ���O���ڐ����擾
	int FASTCALL GetMax() const;
										// ���O�ő�L�^�����擾
	int FASTCALL GetData(int index, logdata_t *ptr);
										// ���O�f�[�^�擾

private:
	enum {
		LogMax = 0x4000					// ���O�ő�L�^��(2�̔{���ł��邱��)
	};

private:
	void FASTCALL Clear();
										// �f�[�^���N���A
	int logtop;
										// �擪�|�C���^(�ł��Â�)
	int lognum;
										// ���O�L�^��
	int logcount;
										// ���O�J�E���g
	logdata_t *logdata[LogMax];
										// ���O�|�C���^
	Sync *sync;
										// �����I�u�W�F�N�g
	CPU *cpu;
										// CPU
	Scheduler *scheduler;
										// �X�P�W���[��
};

//---------------------------------------------------------------------------
//
//	���O�o�̓}�N��
//
//---------------------------------------------------------------------------
#if !defined(NO_LOG)
class LogProxy {
public:
	// �R���X�g���N�^
	LogProxy(const Device* device, Log* log);

	// ���O�o��
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
