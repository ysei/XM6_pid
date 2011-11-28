//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SCC(Z8530) ]
//
//---------------------------------------------------------------------------

#if !defined(scc_h)
#define scc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	SCC
//
//===========================================================================
class SCC : public MemDevice
{
public:
	// ���荞�݃^�C�v
	enum itype_t {
		rxi,							// ��M���荞��
		rsi,							// �X�y�V����Rx�R���f�B�V�������荞��
		txi,							// ���M���荞��
		exti							// �O���X�e�[�^�X�ω����荞��
	};

	// �`���l����`
	typedef struct {
		// �O���[�o��
		uint32_t index;					// �`���l���ԍ�(0 or 1)

		// RR0
		int ba;						// Break/Abort
		int tu;						// Tx�A���_�[����
		int cts;						// CTS
		int sync;						// SYNC
		int dcd;						// DCD
		int zc;						// �[���J�E���g

		// WR0
		uint32_t reg;						// �A�N�Z�X���W�X�^�I��
		int ph;						// �|�C���g�n�C(��ʃ��W�X�^�I��)
		int txpend;					// ���M���荞�݃y���f�B���O
		int rxno;						// ��M�f�[�^�Ȃ�

		// RR1
		int framing;					// �t���[�~���O�G���[
		int overrun;					// �I�[�o�[�����G���[
		int parerr;					// �p���e�B�G���[
		int txsent;					// ���M����

		// WR1
		int extie;						// �O���X�e�[�^�X���荞�݋���
		int txie;						// ���M���荞�݋���
		int parsp;						// �p���e�B�G���[��S-Rx���荞�݂ɂ���
		uint32_t rxim;						// ��M���荞�݃��[�h

		// RR3
		int rxip;						// ��M���荞�݃y���f�B���O
		int rsip;						// �X�y�V����Rx���荞�݃y���f�B���O
		int txip;						// ���M���荞�݃y���f�B���O
		int extip;						// �O���X�e�[�^�X�ω����荞�݃y���f�B���O

		// WR3
		uint32_t rxbit;					// ��M�L�����N�^�r�b�g��(5-8)
		int aen;						// �I�[�g���[�h�C�l�[�u��
		int rxen;						// ��M�C�l�[�u��

		// WR4
		uint32_t clkm;						// �N���b�N���[�h
		uint32_t stopbit;					// �X�g�b�v�r�b�g
		uint32_t parity;					// �p���e�B���[�h

		// WR5
		int dtr;						// DTR�M����
		uint32_t txbit;					// ���M�L�����N�^�r�b�g��(5-8)
		int brk;						// �u���[�N���o
		int txen;						// ���M�C�l�[�u��
		int rts;						// RTS�M����

		// WR8
		uint32_t tdr;						// ���M�f�[�^���W�X�^
		int tdf;						// ���M�f�[�^�L��

		// WR12, WR13
		uint32_t tc;						// �{�[���[�g�ݒ�l

		// WR14
		int loopback;					// ���[�v�o�b�N���[�h
		int aecho;						// �I�[�g�G�R�[���[�h
		int dtrreq;					// DTR�M�����L��
		int brgsrc;					// �{�[���[�g�W�F�l���[�^�N���b�N��
		int brgen;						// �{�[���[�g�W�F�l���[�^�C�l�[�u��

		// WR15
		int baie;						// Break/Abort���荞�݃C�l�[�u��
		int tuie;						// Tx�A���_�[�������荞�݃C�l�[�u��
		int ctsie;						// CTS���荞�݃C�l�[�u��
		int syncie;					// SYNC���荞�݃C�l�[�u��
		int dcdie;						// DCD���荞�݃C�l�[�u��
		int zcie;						// �[���J�E���g���荞�݃C�l�[�u��

		// �ʐM���x
		uint32_t baudrate;					// �{�[���[�g
		uint32_t cps;						// �L�����N�^/sec
		uint32_t speed;					// ���x(hus�P��)

		// ��MFIFO
		uint32_t rxfifo;					// ��MFIFO�L����
		uint32_t rxdata[3];				// ��MFIFO�f�[�^

		// ��M�o�b�t�@
		uint8_t rxbuf[0x1000];				// ��M�f�[�^
		uint32_t rxnum;					// ��M�f�[�^��
		uint32_t rxread;					// ��M�ǂݍ��݃|�C���^
		uint32_t rxwrite;					// ��M�������݃|�C���^
		uint32_t rxtotal;					// ��M�g�[�^��

		// ���M�o�b�t�@
		uint8_t txbuf[0x1000];				// ���M�f�[�^
		uint32_t txnum;					// ���M�f�[�^��
		uint32_t txread;					// ���M�ǂݍ��݃|�C���^
		uint32_t txwrite;					// ���M�������݃|�C���^
		uint32_t txtotal;					// Tx�g�[�^��
		int txwait;					// Tx�E�F�C�g�t���O
	} ch_t;

	// �����f�[�^��`
	typedef struct {
		// �`���l��
		ch_t ch[2];						// �`���l���f�[�^

		// RR2
		uint32_t request;					// ���荞�݃x�N�^(�v����)

		// WR2
		uint32_t vbase;					// ���荞�݃x�N�^(�x�[�X)

		// WR9
		int shsl;						// �x�N�^�ω����[�hb4-b6/b3-b1
		int mie;						// ���荞�݃C�l�[�u��
		int dlc;						// ���ʃ`�F�[���֎~
		int nv;						// ���荞�݃x�N�^�o�̓C�l�[�u��
		int vis;						// ���荞�݃x�N�^�ω����[�h

		int ireq;						// �v�����̊��荞�݃^�C�v
		int vector;						// �v�����̃x�N�^
	} scc_t;

public:
	// ��{�t�@���N�V����
	SCC(VM *p);
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
	void FASTCALL GetSCC(scc_t *buffer) const;
										// �����f�[�^�擾
	const SCC::scc_t* FASTCALL GetWork() const;
										// ���[�N�擾 
	uint32_t FASTCALL GetVector(int type) const;
										// �x�N�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck();
										// ���荞�݉���

	// ���MAPI(SCC�֑��M)
	void FASTCALL Send(int channel, uint32_t data);
										// �f�[�^���M
	void FASTCALL ParityErr(int channel);
										// �p���e�B�G���[�̐���
	void FASTCALL FramingErr(int channel);
										// �t���[�~���O�G���[�̐���
	void FASTCALL SetBreak(int channel, int flag);
										// �u���[�N��Ԃ̒ʒm
	int FASTCALL IsRxEnable(int channel) const;
										// ��M�`�F�b�N
	int FASTCALL IsBaudRate(int channel, uint32_t baudrate) const;
										// �{�[���[�g�`�F�b�N
	uint32_t FASTCALL GetRxBit(int channel) const;
										// ��M�f�[�^�r�b�g���擾
	uint32_t FASTCALL GetStopBit(int channel) const;
										// �X�g�b�v�r�b�g�擾
	uint32_t FASTCALL GetParity(int channel) const;
										// �p���e�B�擾
	int FASTCALL IsRxBufEmpty(int channel) const;
										// ��M�o�b�t�@�̋󂫃`�F�b�N

	// ��MAPI(SCC�����M)
	uint32_t FASTCALL Receive(int channel);
										// �f�[�^��M
	int FASTCALL IsTxEmpty(int channel);
										// ���M�o�b�t�@�G���v�e�B�`�F�b�N
	int FASTCALL IsTxFull(int channel);
										// ���M�o�b�t�@�t���`�F�b�N
	void FASTCALL WaitTx(int channel, int wait);
										// ���M�u���b�N

	// �n�[�h�t���[
	void FASTCALL SetCTS(int channel, int flag);
										// CTS�Z�b�g
	void FASTCALL SetDCD(int channel, int flag);
										// DCD�Z�b�g
	int FASTCALL GetRTS(int channel);
										// RTS�擾
	int FASTCALL GetDTR(int channel);
										// DTR�擾
	int FASTCALL GetBreak(int channel);
										// �u���[�N�擾

private:
	void FASTCALL ResetCh(ch_t *p);
										// �`���l�����Z�b�g
	uint32_t FASTCALL ReadSCC(ch_t *p, uint32_t reg);
										// �`���l���ǂݏo��
	uint32_t FASTCALL ReadRR0(const ch_t *p) const;
										// RR0�ǂݏo��
	uint32_t FASTCALL ReadRR1(const ch_t *p) const;
										// RR1�ǂݏo��
	uint32_t FASTCALL ReadRR2(ch_t *p);
										// RR2�ǂݏo��
	uint32_t FASTCALL ReadRR3(const ch_t *p) const;
										// RR3�ǂݏo��
	uint32_t FASTCALL ReadRR8(ch_t *p);
										// RR8�ǂݏo��
	uint32_t FASTCALL ReadRR15(const ch_t *p) const;
										// RR15�ǂݏo��
	uint32_t FASTCALL ROSCC(const ch_t *p, uint32_t reg) const;
										// �ǂݏo���̂�
	void FASTCALL WriteSCC(ch_t *p, uint32_t reg, uint32_t data);
										// �`���l����������
	void FASTCALL WriteWR0(ch_t *p, uint32_t data);
										// WR0��������
	void FASTCALL WriteWR1(ch_t *p, uint32_t data);
										// WR1��������
	void FASTCALL WriteWR3(ch_t *p, uint32_t data);
										// WR3��������
	void FASTCALL WriteWR4(ch_t *p, uint32_t data);
										// WR4��������
	void FASTCALL WriteWR5(ch_t *p, uint32_t data);
										// WR5��������
	void FASTCALL WriteWR8(ch_t *p, uint32_t data);
										// WR8��������
	void FASTCALL WriteWR9(uint32_t data);
										// WR9��������
	void FASTCALL WriteWR10(ch_t *p, uint32_t data);
										// WR10��������
	void FASTCALL WriteWR11(ch_t *p, uint32_t data);
										// WR11��������
	void FASTCALL WriteWR12(ch_t *p, uint32_t data);
										// WR12��������
	void FASTCALL WriteWR13(ch_t *p, uint32_t data);
										// WR13��������
	void FASTCALL WriteWR14(ch_t *p, uint32_t data);
										// WR14��������
	void FASTCALL WriteWR15(ch_t *p, uint32_t data);
										// WR15��������
	void FASTCALL ResetSCC(ch_t *p);
										// ���Z�b�g
	void FASTCALL ClockSCC(ch_t *p);
										// �{�[���[�g�Čv�Z
	void FASTCALL IntSCC(ch_t *p, itype_t type, int flag);
										// ���荞�݃��N�G�X�g
	void FASTCALL IntCheck();
										// ���荞�݃`�F�b�N
	void FASTCALL EventRx(ch_t *p);
										// �C�x���g(��M)
	void FASTCALL EventTx(ch_t *p);
										// �C�x���g(���M)
	Mouse *mouse;
										// �}�E�X
	scc_t scc;
										// �����f�[�^
	Event event[2];
										// �C�x���g
	int clkup;
										// 7.5MHz���[�h
};

#endif	// scc_h
