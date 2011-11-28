//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ADPCM(MSM6258V) ]
//
//---------------------------------------------------------------------------

#if !defined(adpcm_h)
#define adpcm_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	ADPCM
//
//===========================================================================
class ADPCM : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		uint32_t panpot;					// �p���|�b�g
		int play;						// �Đ����[�h
		int rec;						// �^�����[�h
		int active;					// �A�N�e�B�u�t���O
		int started;					// �Đ���L�ׂȃf�[�^�����o
		uint32_t clock;					// �����N���b�N(4 or 8)
		uint32_t ratio;					// �N���b�N�䗦 (0 or 1 or 2)
		uint32_t speed;					// �i�s���x(128,192,256,384,512)
		uint32_t data;						// �T���v���f�[�^(4bit * 2sample)

		int offset;						// �����I�t�Z�b�g (0-48)
		int sample;						// �T���v���f�[�^
		int out;						// �o�̓f�[�^
		int vol;						// ����

		int enable;					// �C�l�[�u���t���O
		int sound;						// ADPCM�o�͗L���t���O
		uint32_t readpoint;				// �o�b�t�@�ǂݍ��݃|�C���g
		uint32_t writepoint;				// �o�b�t�@�������݃|�C���g
		uint32_t number;					// �o�b�t�@�L���f�[�^��
		int wait;						// �����E�F�C�g
		uint32_t sync_cnt;					// �����J�E���^
		uint32_t sync_rate;				// �������[�g(882,960,etc...)
		uint32_t sync_step;				// �����X�e�b�v(���`��ԑΉ�)
		int interp;					// ��ԃt���O
	} adpcm_t;

public:
	// ��{�t�@���N�V����
	ADPCM(VM *p);
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
	void FASTCALL GetADPCM(adpcm_t *buffer);
										// �����f�[�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL SetClock(uint32_t clk);
										// ��N���b�N�w��
	void FASTCALL SetRatio(uint32_t ratio);
										// �N���b�N�䗦�w��
	void FASTCALL SetPanpot(uint32_t pan);
										// �p���|�b�g�w��
	void FASTCALL Enable(int enable);
										// �����C�l�[�u��
	void FASTCALL InitBuf(uint32_t rate);
										// �o�b�t�@������
	void FASTCALL GetBuf(uint32_t *buffer, int samples);
										// �o�b�t�@�擾
	void FASTCALL Wait(int num);
										// �E�F�C�g�w��
	void FASTCALL EnableADPCM(int flag) { adpcm.sound = flag; }
										// �Đ��L��
	void FASTCALL SetVolume(int volume);
										// ���ʐݒ�
	void FASTCALL ClrStarted()			{ adpcm.started = FALSE; }
										// �X�^�[�g�t���O�N���A
	int FASTCALL IsStarted() const		{ return adpcm.started; }
										// �X�^�[�g�t���O�擾

private:
	enum {
		BufMax = 0x10000				// �o�b�t�@�T�C�Y
	};
	void FASTCALL MakeTable();
										// �e�[�u���쐬
	void FASTCALL CalcSpeed();
										// ���x�Čv�Z
	void FASTCALL Start(int type);
										// �^���E�Đ��X�^�[�g
	void FASTCALL Stop();
										// �^���E�Đ��X�g�b�v
	void FASTCALL Decode(int data, int num, int valid);
										// 4bit�f�R�[�h
	Event event;
										// �^�C�}�[�C�x���g
	adpcm_t adpcm;
										// �����f�[�^
	DMAC *dmac;
										// DMAC
	uint32_t *adpcmbuf;
										// �����o�b�t�@
	int DiffTable[49 * 16];
										// �����e�[�u��
	static const int NextTable[16];
										// �ψʃe�[�u��
	static const int OffsetTable[58];
										// �I�t�Z�b�g�e�[�u��
};

#endif	// adpcm_h
