//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �C�x���g ]
//
//---------------------------------------------------------------------------

#if !defined(event_h)
#define event_h

//===========================================================================
//
//	�C�x���g
//
//===========================================================================
class Event
{
public:
	// �����f�[�^��`
#if defined(_WIN32)
#pragma pack(push, 8)
#endif	// _WIN32
	typedef struct {
		uint32_t remain;					// +4  �c�莞��
		uint32_t time;						// +8  �g�[�^������
		uint32_t user;						// +12 ���[�U��`�f�[�^
		Device *device;					// +16 �e�f�o�C�X
		Scheduler *scheduler;			// +20 �X�P�W���[��
		Event *next;					// +24 ���̃C�x���g
		char desc[0x20];				// ����
	} event_t;
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

public:
	// ��{�t�@���N�V����
	Event();
										// �R���X�g���N�^
	virtual ~Event();
										// �f�X�g���N�^
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// ���[�h�E�Z�[�u
	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// �v���p�e�B
	void FASTCALL SetDevice(Device *p);
										// �e�f�o�C�X�ݒ�
	Device* FASTCALL GetDevice() const	{ return ev.device; }
										// �e�f�o�C�X�擾
#if defined(XM6_USE_EVENT_DESC)
	void FASTCALL SetDesc(const char *fmt, ...);
										// ���̐ݒ�
#else
	void FASTCALL SetDesc(const char *fmt, ...) {}	// ���̐ݒ�
#endif
	const char* FASTCALL GetDesc() const;
										// ���̎擾
	void FASTCALL SetUser(uint32_t data)	{ ev.user = data; }
										// ���[�U��`�f�[�^�ݒ�
	uint32_t FASTCALL GetUser() const		{ return ev.user; }
										// ���[�U��`�f�[�^�擾

	// ���ԊǗ�
	void FASTCALL SetTime(uint32_t hus);
										// ���Ԏ����ݒ�
	uint32_t FASTCALL GetTime() const		{ return ev.time; }
										// ���Ԏ����擾
	uint32_t FASTCALL GetRemain() const	{ return ev.remain; }
										// �c�莞�Ԏ擾
	void FASTCALL Exec(uint32_t hus);
										// ���Ԃ�i�߂�

	// �����N�ݒ�E�폜
	void FASTCALL SetNextEvent(Event *p) { ev.next = p; }
										// ���̃C�x���g��ݒ�
	Event* FASTCALL GetNextEvent() const { return ev.next; }
										// ���̃C�x���g���擾

private:
	// �����f�[�^��`(Ver2.01�܂ŁBenable������)
	typedef struct {
		Device *device;					// �e�f�o�C�X
		Scheduler *scheduler;			// �X�P�W���[��
		Event *next;					// ���̃C�x���g
		char desc[0x20];				// ����
		uint32_t user;						// ���[�U��`�f�[�^
		int enable;					// �C�l�[�u������
		uint32_t time;						// �g�[�^������
		uint32_t remain;					// �c�莞��
	} event201_t;

	int FASTCALL Load201(Fileio *fio);
										// ���[�h(version 2.01�ȑO)
	event_t ev;
										// �������[�N
};

#endif	// event_h
