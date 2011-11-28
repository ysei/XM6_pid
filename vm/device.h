//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �f�o�C�X���� ]
//
//---------------------------------------------------------------------------

#if !defined(device_h)
#define device_h

class VM;

//===========================================================================
//
//	�f�o�C�X
//
//===========================================================================
class Device
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t id;						// ID
		const char *desc;				// ����
		Device* next;					// ���̃f�o�C�X
	} device_t;

public:
	// ��{�t�@���N�V����
	Device(VM *p);
										// �R���X�g���N�^
	virtual ~Device();
										// �f�X�g���N�^
	virtual int FASTCALL Init();
										// ������
	virtual void FASTCALL Cleanup();
										// �N���[���A�b�v
	virtual void FASTCALL Reset();
										// ���Z�b�g
	virtual int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	virtual int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	virtual void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �O��API
	Device* FASTCALL GetNextDevice() const { return dev.next; }
										// ���̃f�o�C�X���擾
	void FASTCALL SetNextDevice(Device *p) { dev.next = p; }
										// ���̃f�o�C�X��ݒ�
	uint32_t FASTCALL GetID() const		{ return dev.id; }
										// �f�o�C�XID�擾
	const char* FASTCALL GetDesc() const { return dev.desc; }
										// �f�o�C�X���̎擾
	VM* FASTCALL GetVM() const			{ return vm; }
										// VM�擾
	virtual int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N

protected:
#if defined(XM6_USE_LOG)
	Log* FASTCALL GetLog() const		{ return log; }
										// ���O�擾
	Log *log;
										// ���O
#endif
	device_t dev;
										// �����f�[�^
	VM *vm;
										// ���z�}�V���{��
};

//===========================================================================
//
//	�������}�b�v�h�f�o�C�X
//
//===========================================================================
class MemDevice : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t first;					// �J�n�A�h���X
		uint32_t last;						// �ŏI�A�h���X
	} memdev_t;

public:
	// ��{�t�@���N�V����
	MemDevice(VM *p);
										// �R���X�g���N�^
	virtual int FASTCALL Init();
										// ������

	// �������f�o�C�X
	virtual uint32_t FASTCALL ReadByte(uint32_t addr);
										// �o�C�g�ǂݍ���
	virtual uint32_t FASTCALL ReadWord(uint32_t addr);
										// ���[�h�ǂݍ���
	virtual void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	virtual void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
	virtual uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �O��API
	uint32_t FASTCALL GetFirstAddr() const	{ return memdev.first; }
										// �ŏ��̃A�h���X���擾
	uint32_t FASTCALL GetLastAddr() const	{ return memdev.last; }
										// �Ō�̃A�h���X���擾

protected:
	memdev_t memdev;
										// �����f�[�^
	CPU *cpu;
										// CPU
	Scheduler *scheduler;
										// �X�P�W���[��
};

#endif	// device_h
