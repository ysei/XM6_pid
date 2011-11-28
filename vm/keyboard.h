//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �L�[�{�[�h ]
//
//---------------------------------------------------------------------------

#if !defined(keyboard_h)
#define keyboard_h

#include "device.h"
#include "event.h"
#include "sync.h"

//===========================================================================
//
//	�L�[�{�[�h
//
//===========================================================================
class Keyboard : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		int connect;					// �ڑ��t���O
		int status[0x80];				// �����X�e�[�^�X
		uint32_t rep_code;					// ���s�[�g�R�[�h
		uint32_t rep_count;				// ���s�[�g�J�E���^
		uint32_t rep_start;				// ���s�[�g����(hus�P��)
		uint32_t rep_next;					// ���s�[�g����(hus�P��)
		int send_en;					// �L�[�f�[�^���M��
		int send_wait;					// �L�[�f�[�^���M�����~��
		uint32_t msctrl;					// �}�E�X����M��
		int tv_mode;					// X68000�e���r���[�h
		int tv_ctrl;					// �R�}���h�ɂ��e���r�R���g���[��
		int opt2_ctrl;					// OPT2�ɂ��e���r�R���g���[��
		uint32_t bright;					// �L�[���邳
		uint32_t led;						// �L�[LED(1�œ_��)
		uint32_t cmdbuf[0x10];				// �R�}���h�o�b�t�@
		uint32_t cmdread;					// �R�}���h���[�h�|�C���^
		uint32_t cmdwrite;					// �R�}���h���C�g�|�C���^
		uint32_t cmdnum;					// �R�}���h��
	} keyboard_t;

public:
	// ��{�t�@���N�V����
	Keyboard(VM *p);
										// �R���X�g���N�^
	int FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	int FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	int FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p

	// �O��API
	void FASTCALL Connect(int connect);
										// �ڑ�
	int FASTCALL IsConnect() const		{ return keyboard.connect; }
										// �ڑ��`�F�b�N
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL MakeKey(uint32_t code);
										// ���C�N
	void FASTCALL BreakKey(uint32_t code);
										// �u���[�N
	void FASTCALL Command(uint32_t data);
										// �R�}���h
	int FASTCALL GetCommand(uint32_t& data);
										// �R�}���h�擾
	void FASTCALL ClrCommand();
										// �R�}���h�N���A
	void FASTCALL SendWait(int flag);
										// �L�[�f�[�^���M�E�F�C�g
	int FASTCALL IsSendWait() const	{ return keyboard.send_wait; }
										// �L�[�f�[�^���M�E�F�C�g�擾
	void FASTCALL GetKeyboard(keyboard_t *buffer) const;
										// �����f�[�^�擾

private:
	MFP *mfp;
										// MFP
	Mouse *mouse;
										// �}�E�X
	keyboard_t keyboard;
										// �����f�[�^
	Event event;
										// �C�x���g
	Sync *sync;
										// �R�}���hSync
};

#endif	// keyboard_h
