//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#if !defined(mfp_h)
#define mfp_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
class MFP : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		// ���荞��
		int ier[0x10];					// ���荞�݃C�l�[�u�����W�X�^
		int ipr[0x10];					// ���荞�݃y���f�B���O���W�X�^
		int isr[0x10];					// ���荞�݃C���T�[�r�X���W�X�^
		int imr[0x10];					// ���荞�݃}�X�N���W�X�^
		int ireq[0x10];				// ���荞�݃��N�G�X�g���W�X�^
		uint32_t vr;						// �x�N�^���W�X�^
		int iidx;						// ���荞�݃C���f�b�N�X

		// �^�C�}
		uint32_t tcr[4];					// �^�C�}�R���g���[�����W�X�^
		uint32_t tdr[4];					// �^�C�}�f�[�^���W�X�^
		uint32_t tir[4];					// �^�C�}�C���^�[�i�����W�X�^
		uint32_t tbr[2];					// �^�C�}�o�b�N�A�b�v���W�X�^
		uint32_t sram;						// si, info.ram�΍�t���O
		uint32_t tecnt;					// �C�x���g�J�E���g���[�h�J�E���^

		// GPIP
		uint32_t gpdr;						// GPIP�f�[�^���W�X�^
		uint32_t aer;						// �A�N�e�B�u�G�b�W���W�X�^
		uint32_t ddr;						// �f�[�^�������W�X�^
		uint32_t ber;						// �o�b�N�A�b�v�G�b�W���W�X�^

		// USART
		uint32_t scr;						// SYNC�L�����N�^���W�X�^
		uint32_t ucr;						// USART�R���g���[�����W�X�^
		uint32_t rsr;						// ���V�[�o�X�e�[�^�X���W�X�^
		uint32_t tsr;						// �g�����X�~�b�^�X�e�[�^�X���W�X�^
		uint32_t rur;						// ���V�[�o���[�U���W�X�^
		uint32_t tur;						// �g�����X�~�b�^���[�U���W�X�^
		uint32_t buffer[0x10];				// USART FIFO�o�b�t�@
		int datacount;					// USART �L���f�[�^��
		int readpoint;					// USART MFP�ǂݎ��|�C���g
		int writepoint;					// USART �L�[�{�[�h�������݃|�C���g
	} mfp_t;

public:
	// ��{�t�@���N�V����
	MFP(VM *p);
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

	// �������f�o�C�X
	uint32_t FASTCALL ReadByte(uint32_t addr);
										// �o�C�g�ǂݍ���
	uint32_t FASTCALL ReadWord(uint32_t addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	void FASTCALL WriteWord(uint32_t addr, uint32_t data);
										// ���[�h��������
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetMFP(mfp_t *buffer) const;
										// �����f�[�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck();
										// ���荞�݉���
	void FASTCALL EventCount(int channel, int value);
										// �C�x���g�J�E���g
	void FASTCALL SetGPIP(int num, int value);
										// GPIP�ݒ�
	void FASTCALL KeyData(uint32_t data);
										// �L�[�f�[�^�ݒ�
	uint32_t FASTCALL GetVR() const;
										// �x�N�^���W�X�^�擾

private:
	// ���荞�݃R���g���[��
	void FASTCALL Interrupt(int level, int enable);
										// ���荞��
	void FASTCALL IntCheck();
										// ���荞�ݗD�揇�ʃ`�F�b�N
	void FASTCALL SetIER(int offset, uint32_t data);
										// IER�ݒ�
	uint32_t FASTCALL GetIER(int offset) const;
										// IER�擾
	void FASTCALL SetIPR(int offset, uint32_t data);
										// IPR�ݒ�
	uint32_t FASTCALL GetIPR(int offset) const;
										// IPR�擾
	void FASTCALL SetISR(int offset, uint32_t data);
										// ISR�ݒ�
	uint32_t FASTCALL GetISR(int offset) const;
										// ISR�擾
	void FASTCALL SetIMR(int offset, uint32_t data);
										// IMR�ݒ�
	uint32_t FASTCALL GetIMR(int offset) const;
										// IMR�ݒ�
	void FASTCALL SetVR(uint32_t data);
										// VR�ݒ�
	static const char* IntDesc[0x10];
										// ���荞�ݖ��̃e�[�u��

	// �^�C�}
	void FASTCALL SetTCR(int channel, uint32_t data);
										// TCR�ݒ�
	uint32_t FASTCALL GetTCR(int channel) const;
										// TCR�擾
	void FASTCALL SetTDR(int channel, uint32_t data);
										// TDR�ݒ�
	uint32_t FASTCALL GetTIR(int channel) const;
										// TIR�擾
	void FASTCALL Proceed(int channel);
										// �^�C�}��i�߂�
	Event timer[4];
										// �^�C�}�C�x���g
	static const int TimerInt[4];
										// �^�C�}���荞�݃e�[�u��
	static const uint32_t TimerHus[8];
										// �^�C�}���ԃe�[�u��

	// GPIP
	void FASTCALL SetGPDR(uint32_t data);
										// GPDR�ݒ�
	void FASTCALL IntGPIP();
										// GPIP���荞��
	static const int GPIPInt[8];
										// GPIP���荞�݃e�[�u��

	// USART
	void FASTCALL SetRSR(uint32_t data);
										// RSR�ݒ�
	void FASTCALL Receive();
										// USART�f�[�^��M
	void FASTCALL SetTSR(uint32_t data);
										// TSR�ݒ�
	void FASTCALL Transmit(uint32_t data);
										// USART�f�[�^���M
	void FASTCALL USART();
										// USART����
	Event usart;
										// USART�C�x���g
	Sync *sync;
										// USART Sync
	Keyboard *keyboard;
										// �L�[�{�[�h
	mfp_t mfp;
										// �����f�[�^
};

#endif	// mfp_h
