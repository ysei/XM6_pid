//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MIDI(YM3802) ]
//
//---------------------------------------------------------------------------

#if !defined(midi_h)
#define midi_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
class MIDI : public MemDevice
{
public:
	// �萔��`
	enum {
		TransMax = 0x2000,				// ���M�o�b�t�@��
		RecvMax = 0x2000				// ��M�o�b�t�@��
	};

	// MIDI�o�C�g�f�[�^��`
	typedef struct {
		uint32_t data;						// �f�[�^����(8bit)
		uint32_t vtime;					// ���z����
	} mididata_t;

	// �����f�[�^��`
	typedef struct {
		// ���Z�b�g
		int reset;						// ���Z�b�g�t���O
		int access;					// �A�N�Z�X�t���O

		// �{�[�h�f�[�^�A���荞��
		uint32_t bid;						// �{�[�hID(0:�{�[�h����)
		uint32_t ilevel;					// ���荞�݃��x��
		int vector;						// ���荞�ݗv���x�N�^

		// MCS���W�X�^(���)
		uint32_t wdr;						// �������݃f�[�^���W�X�^
		uint32_t rgr;						// ���W�X�^�O���[�v���W�X�^

		// MCS���W�X�^(���荞��)
		uint32_t ivr;						// ���荞�݃x�N�^���W�X�^
		uint32_t isr;						// ���荞�݃T�[�r�X���W�X�^
		uint32_t imr;						// ���荞�݃��[�h���W�X�^
		uint32_t ier;						// ���荞�݋����W�X�^

		// MCS���W�X�^(���A���^�C�����b�Z�[�W)
		uint32_t dmr;						// ���A���^�C�����b�Z�[�W���[�h���W�X�^
		uint32_t dcr;						// ���A���^�C�����b�Z�[�W�R���g���[�����W�X�^

		// MCS���W�X�^(��M)
		uint32_t rrr;						// ��M���[�g���W�X�^
		uint32_t rmr;						// ��M���[�h���W�X�^
		uint32_t amr;						// �A�h���X�n���^���[�h���W�X�^
		uint32_t adr;						// �A�h���X�n���^�f�o�C�X���W�X�^
		uint32_t asr;						// �A�h���X�n���^�X�e�[�^�X���W�X�^
		uint32_t rsr;						// ��M�o�b�t�@�X�e�[�^�X���W�X�^
		uint32_t rcr;						// ��M�o�b�t�@�R���g���[�����W�X�^
		uint32_t rcn;						// ����M�J�E���^

		// MCS���W�X�^(���M)
		uint32_t trr;						// ���M���[�g���W�X�^
		uint32_t tmr;						// ���M���[�h���W�X�^
		int tbs;						// ���MBUSY���W�X�^
		uint32_t tcr;						// ���M�R���g���[�����W�X�^
		uint32_t tcn;						// �����M�J�E���^

		// MCS���W�X�^(FSK)
		uint32_t fsr;						// FSK�X�e�[�^�X���W�X�^
		uint32_t fcr;						// FSK�R���g���[�����W�X�^

		// MCS���W�X�^(�J�E���^)
		uint32_t ccr;						// �N���b�N�R���g���[�����W�X�^
		uint32_t cdr;						// �N���b�N�f�[�^���W�X�^
		uint32_t ctr;						// �N���b�N�^�C�}���W�X�^
		uint32_t srr;						// ���R�[�f�B���O�J�E���^���W�X�^
		uint32_t scr;						// �N���b�N��ԃ��W�X�^
		uint32_t sct;						// �N���b�N��ԃJ�E���^
		uint32_t spr;						// �v���C�o�b�N�J�E���^���W�X�^
		uint32_t str;						// �v���C�o�b�N�^�C�}���W�X�^
		uint32_t gtr;						// �ėp�^�C�}���W�X�^
		uint32_t mtr;						// MIDI�N���b�N�^�C�}���W�X�^

		// MCS���W�X�^(GPIO)
		uint32_t edr;						// �O���|�[�g�f�B���N�V�������W�X�^
		uint32_t eor;						// �O���|�[�gOutput���W�X�^
		uint32_t eir;						// �O���|�[�gInput���W�X�^

		// �ʏ�o�b�t�@
		uint32_t normbuf[16];				// �ʏ�o�b�t�@
		uint32_t normread;					// �ʏ�o�b�t�@Read
		uint32_t normwrite;				// �ʏ�o�b�t�@Write
		uint32_t normnum;					// �ʏ�o�b�t�@��
		uint32_t normtotal;				// �ʏ�o�b�t�@�g�[�^��

		// ���A���^�C�����M�o�b�t�@
		uint32_t rtbuf[4];					// ���A���^�C�����M�o�b�t�@
		uint32_t rtread;					// ���A���^�C�����M�o�b�t�@Read
		uint32_t rtwrite;					// ���A���^�C�����M�o�b�t�@Write
		uint32_t rtnum;					// ���A���^�C�����M�o�b�t�@��
		uint32_t rttotal;					// ���A���^�C�����M�o�b�t�@�g�[�^��

		// ��ʃo�b�t�@
		uint32_t stdbuf[0x80];				// ��ʃo�b�t�@
		uint32_t stdread;					// ��ʃo�b�t�@Read
		uint32_t stdwrite;					// ��ʃo�b�t�@Write
		uint32_t stdnum;					// ��ʃo�b�t�@��
		uint32_t stdtotal;					// ��ʃo�b�t�@�g�[�^��

		// ���A���^�C����M�o�b�t�@
		uint32_t rrbuf[4];					// ���A���^�C����M�o�b�t�@
		uint32_t rrread;					// ���A���^�C����M�o�b�t�@Read
		uint32_t rrwrite;					// ���A���^�C����M�o�b�t�@Write
		uint32_t rrnum;					// ���A���^�C����M�o�b�t�@��
		uint32_t rrtotal;					// ���A���^�C����M�o�b�t�@�g�[�^��

		// ���M�o�b�t�@(�f�o�C�X�Ƃ̎󂯓n���p)
		mididata_t *transbuf;			// ���M�o�b�t�@
		uint32_t transread;				// ���M�o�b�t�@Read
		uint32_t transwrite;				// ���M�o�b�t�@Write
		uint32_t transnum;					// ���M�o�b�t�@��

		// ��M�o�b�t�@(�f�o�C�X�Ƃ̎󂯓n���p)
		mididata_t *recvbuf;			// ��M�o�b�t�@
		uint32_t recvread;					// ��M�o�b�t�@Read
		uint32_t recvwrite;				// ��M�o�b�t�@Write
		uint32_t recvnum;					// ��M�o�b�t�@��
	} midi_t;

public:
	// ��{�t�@���N�V����
	MIDI(VM *p);
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
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

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
	int FASTCALL IsActive() const;
										// MIDI�A�N�e�B�u�`�F�b�N
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck(int level);
										// ���荞��ACK
	void FASTCALL GetMIDI(midi_t *buffer) const;
										// �����f�[�^�擾
	uint32_t FASTCALL GetExCount(int index) const;
										// �G�N�X�N���[�V�u�J�E���g�擾

	// ���M(MIDI OUT)
	uint32_t FASTCALL GetTransNum() const;
										// ���M�o�b�t�@���擾
	const mididata_t* FASTCALL GetTransData(uint32_t proceed);
										// ���M�o�b�t�@�f�[�^�擾
	void FASTCALL DelTransData(uint32_t number);
										// ���M�o�b�t�@�폜
	void FASTCALL ClrTransData();
										// ���M�o�b�t�@�N���A

	// ��M(MIDI IN)
	void FASTCALL SetRecvData(const uint8_t *ptr, uint32_t length);
										// ��M�f�[�^�ݒ�
	void FASTCALL SetRecvDelay(int delay);
										// ��M�f�B���C�ݒ�

	// ���Z�b�g
	int FASTCALL IsReset() const		{ return midi.reset; }
										// ���Z�b�g�t���O�擾
	void FASTCALL ClrReset()			{ midi.reset = FALSE; }
										// ���Z�b�g�t���O�N���A

private:
	void FASTCALL Receive();
										// ��M�R�[���o�b�N
	void FASTCALL Transmit();
										// ���M�R�[���o�b�N
	void FASTCALL Clock();
										// MIDI�N���b�N���o
	void FASTCALL General();
										// �ėp�^�C�}�R�[���o�b�N

	void FASTCALL InsertTrans(uint32_t data);
										// ���M�o�b�t�@�֑}��
	void FASTCALL InsertRecv(uint32_t data);
										// ��M�o�b�t�@�֑}��
	void FASTCALL InsertNorm(uint32_t data);
										// �ʏ�o�b�t�@�֑}��
	void FASTCALL InsertRT(uint32_t data);
										// ���A���^�C�����M�o�b�t�@�֑}��
	void FASTCALL InsertStd(uint32_t data);
										// ��ʃo�b�t�@�֑}��
	void FASTCALL InsertRR(uint32_t data);
										// ���A���^�C����M�o�b�t�@�֑}��

	void FASTCALL ResetReg();
										// ���W�X�^���Z�b�g
	uint32_t FASTCALL ReadReg(uint32_t reg);
										// ���W�X�^�ǂݏo��
	void FASTCALL WriteReg(uint32_t reg, uint32_t data);
										// ���W�X�^��������
	uint32_t FASTCALL ReadRegRO(uint32_t reg) const;
										// ���W�X�^�ǂݏo��(ReadOnly)

	void FASTCALL SetICR(uint32_t data);
										// ICR�ݒ�
	void FASTCALL SetIOR(uint32_t data);
										// IOR�ݒ�
	void FASTCALL SetIMR(uint32_t data);
										// IMR�ݒ�
	void FASTCALL SetIER(uint32_t data);
										// IER�ݒ�
	void FASTCALL SetDMR(uint32_t data);
										// DMR�ݒ�
	void FASTCALL SetDCR(uint32_t data);
										// DCR�ݒ�
	uint32_t FASTCALL GetDSR() const;
										// DSR�擾
	void FASTCALL SetDNR(uint32_t data);
										// DNR�ݒ�
	void FASTCALL SetRRR(uint32_t data);
										// RRR�ݒ�
	void FASTCALL SetRMR(uint32_t data);
										// RMR�ݒ�
	void FASTCALL SetAMR(uint32_t data);
										// AMR�ݒ�
	void FASTCALL SetADR(uint32_t data);
										// ADR�ݒ�
	uint32_t FASTCALL GetRSR() const;
										// RSR�擾
	void FASTCALL SetRCR(uint32_t data);
										// RCR�ݒ�
	uint32_t FASTCALL GetRDR();
										// RDR�擾(�X�V����)
	uint32_t FASTCALL GetRDRRO() const;
										// RDR�擾(Read Only)
	void FASTCALL SetTRR(uint32_t data);
										// TRR�ݒ�
	void FASTCALL SetTMR(uint32_t data);
										// TMR�ݒ�
	uint32_t FASTCALL GetTSR() const;
										// TSR�擾
	void FASTCALL SetTCR(uint32_t data);
										// TCR�ݒ�
	void FASTCALL SetTDR(uint32_t data);
										// TDR�ݒ�
	uint32_t FASTCALL GetFSR() const;
										// FSR�擾
	void FASTCALL SetFCR(uint32_t data);
										// FCR�ݒ�
	void FASTCALL SetCCR(uint32_t data);
										// CCR�ݒ�
	void FASTCALL SetCDR(uint32_t data);
										// CDR�ݒ�
	uint32_t FASTCALL GetSRR() const;
										// SRR�擾
	void FASTCALL SetSCR(uint32_t data);
										// SCR�ݒ�
	void FASTCALL SetSPR(uint32_t data, int high);
										// SPR�ݒ�
	void FASTCALL SetGTR(uint32_t data, int high);
										// GTR�ݒ�
	void FASTCALL SetMTR(uint32_t data, int high);
										// MTR�ݒ�
	void FASTCALL SetEDR(uint32_t data);
										// EDR�ݒ�
	void FASTCALL SetEOR(uint32_t data);
										// EOR�ݒ�
	uint32_t FASTCALL GetEIR() const;
										// EIR�擾

	void FASTCALL CheckRR();
										// ���A���^�C�����b�Z�[�W��M�o�b�t�@�`�F�b�N
	void FASTCALL Interrupt(int type, int flag);
										// ���荞�ݔ���
	void FASTCALL IntCheck();
										// ���荞�݃`�F�b�N
	Event event[3];
										// �C�x���g
	midi_t midi;
										// �����f�[�^
	Sync *sync;
										// �f�[�^Sync
	uint32_t recvdelay;
										// ��M�x�ꎞ��(hus)
	uint32_t ex_cnt[4];
										// �G�N�X�N���[�V�u�J�E���g
};

#endif	// midi_h
