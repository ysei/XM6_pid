//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ OPM(YM2151) ]
//
//---------------------------------------------------------------------------

#if !defined(opmif_h)
#define opmif_h

#include "device.h"
#include "event.h"
#include "opm.h"

//===========================================================================
//
//	OPM
//
//===========================================================================
class OPMIF : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t reg[0x100];				// ���W�X�^
		uint32_t key[8];					// �L�[���
		uint32_t addr;						// �Z���N�g�A�h���X
		int busy;						// BUSY�t���O
		int enable[2];					// �^�C�}�C�l�[�u��
		int action[2];					// �^�C�}����
		int interrupt[2];				// �^�C�}���荞��
		uint32_t time[2];					// �^�C�}����
		int started;					// �J�n�t���O
	} opm_t;

	// �o�b�t�@�Ǘ���`
	typedef struct {
		uint32_t max;						// �ő吔
		uint32_t num;						// �L���f�[�^��
		uint32_t read;						// �ǂݎ��|�C���g
		uint32_t write;					// �������݃|�C���g
		uint32_t samples;					// �����T���v����
		uint32_t rate;						// �������[�g
		uint32_t under;					// �A���_�[����
		uint32_t over;						// �I�[�o�[����
		int sound;						// FM�L��
	} opmbuf_t;

public:
	// ��{�t�@���N�V����
	OPMIF(VM *p);
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
	void FASTCALL GetOPM(opm_t *buffer);
										// �����f�[�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL Output(uint32_t addr, uint32_t data);
										// ���W�X�^�o��
	void FASTCALL SetEngine(FM::OPM *p);
										// �G���W���w��
	void FASTCALL InitBuf(uint32_t rate);
										// �o�b�t�@������
	uint32_t FASTCALL ProcessBuf();
										// �o�b�t�@����
	void FASTCALL GetBuf(uint32_t *buf, int samples);
										// �o�b�t�@���擾
	void FASTCALL GetBufInfo(opmbuf_t *buffer);
										// �o�b�t�@���𓾂�
	void FASTCALL EnableFM(int flag)	{ bufinfo.sound = flag; }
										// FM�����L��
	void FASTCALL ClrStarted()			{ opm.started = FALSE; }
										// �X�^�[�g�t���O���~�낷
	int FASTCALL IsStarted() const		{ return opm.started; }
										// �X�^�[�g�t���O�擾

private:
	void FASTCALL CalcTimerA();
										// �^�C�}A�Z�o
	void FASTCALL CalcTimerB();
										// �^�C�}B�Z�o
	void FASTCALL CtrlTimer(uint32_t data);
										// �^�C�}����
	void FASTCALL CtrlCT(uint32_t data);
										// CT����
	MFP *mfp;
										// MFP
	ADPCM *adpcm;
										// ADPCM
	FDD *fdd;
										// FDD
	opm_t opm;
										// OPM�����f�[�^
	opmbuf_t bufinfo;
										// �o�b�t�@���
	Event event[2];
										// �^�C�}�[�C�x���g
	FM::OPM *engine;
										// �����G���W��
	enum {
		BufMax = 0x10000				// �o�b�t�@�T�C�Y
	};
	uint32_t *opmbuf;
										// �����o�b�t�@
};

#endif	// opmif_h
