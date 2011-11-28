//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ CRTC(VICON) ]
//
//---------------------------------------------------------------------------

#if !defined(crtc_h)
#define crtc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	CRTC
//
//===========================================================================
class CRTC : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		uint8_t reg[24 * 2];				// CRTC���W�X�^
		int hrl;						// HRL(�V�X�e���|�[�g)
		int lowres;					// 15kHz���[�h
		int textres;					// 768�~512���[�h
		int changed;					// �𑜓x�ύX�t���O

		int h_sync;						// ������������
		int h_pulse;					// ���������p���X��
		int h_back;						// �����o�b�N�|�[�`
		int h_front;					// �����t�����g�|�[�`
		int h_dots;						// �����h�b�g��
		int h_mul;						// �����{��
		int hd;							// 256,512,768,����`

		int v_sync;						// ������������(H�P��)
		int v_pulse;					// ���������p���X��(H�P��)
		int v_back;						// �����o�b�N�|�[�`(H�P��)
		int v_front;					// �����t�����g�|�[�`(H�P��)
		int v_dots;						// �����h�b�g��
		int v_mul;						// �����{��(0:interlace)
		int vd;							// 256,512,����`,����`

		uint32_t ns;						// ns�J�E���^
		uint32_t hus;						// hus�J�E���^
		uint32_t v_synccnt;				// V-SYNC�J�E���^
		uint32_t v_blankcnt;				// V-BLANK�J�E���^
		int h_disp;					// �����\���t���O
		int v_disp;					// V-DISP�t���O
		int v_blank;					// V-BLANK�t���O
		uint32_t v_count;					// V-DISP�J�E���^
		int v_scan;						// �X�L�������C��

		// �ȉ�����Ȃ�
		int h_synctime;					// ��������(hus)
		int h_disptime;					// �����\��(hus)
		int v_cycletime;				// ��������(hus)
		int v_blanktime;				// �����u�����N(hus)
		int v_synctime;					// ��������(hus)
		int v_backtime;					// �����o�b�N�|�[�`(hus)

		int tmem;						// �e�L�X�gVRAM��\��
		int gmem;						// �O���t�B�b�NVRAM��\��
		uint32_t siz;						// �O���t�B�b�NVRAM1024�~1024���[�h
		uint32_t col;						// �O���t�B�b�NVRAM�F���[�h

		uint32_t text_scrlx;				// �e�L�X�g�X�N���[��X
		uint32_t text_scrly;				// �e�L�X�g�X�N���[��Y
		uint32_t grp_scrlx[4];				// �O���t�B�b�N�X�N���[��X
		uint32_t grp_scrly[4];				// �O���t�B�b�N�X�N���[��Y

		int raster_count;				// ���X�^�J�E���^
		int raster_int;					// ���X�^���荞�݈ʒu
		int raster_copy;				// ���X�^�R�s�[�t���O
		int raster_exec;				// ���X�^�R�s�[���s�t���O
		uint32_t fast_clr;					// �O���t�B�b�N�����N���A
	} crtc_t;

public:
	// ��{�t�@���N�V����
	CRTC(VM *p);
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
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);
										// �o�C�g��������
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetCRTC(crtc_t *buffer) const;
										// �����f�[�^�擾
	int FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL SetHRL(int h);
										// HRL�ݒ�
	int FASTCALL GetHRL() const;
										// HRL�擾
	void FASTCALL GetHVHz(uint32_t *h, uint32_t *v) const;
										// �\�����g���擾
	uint32_t FASTCALL GetDispCount() const	{ return crtc.v_count; }
										// �\���J�E���^�擾
	const crtc_t* FASTCALL GetWorkAddr() const { return &crtc; }
										// ���[�N�A�h���X�擾

private:
	void FASTCALL ReCalc();
										// �Čv�Z
	void FASTCALL HSync();
										// H-SYNC�J�n
	void FASTCALL HDisp();
										// H-DISP�J�n
	void FASTCALL VSync();
										// V-SYNC�J�n
	void FASTCALL VBlank();
										// V-BLANK�J�n
	int FASTCALL Ns2Hus(int ns)			{ return ns / 500; }
										// ns��0.5us���Z
	int FASTCALL Hus2Ns(int hus)		{ return hus * 500; }
										// 0.5us��ns���Z
	void FASTCALL CheckRaster();
										// ���X�^���荞�݃`�F�b�N
	void FASTCALL TextVRAM();
										// �e�L�X�gVRAM����
	int FASTCALL Get8DotClock() const;
										// 8�h�b�g�N���b�N�𓾂�
	static const int DotClockTable[16];
										// 8�h�b�g�N���b�N�e�[�u��
	static const uint8_t ResetTable[26];
										// RESET���W�X�^�e�[�u��
	crtc_t crtc;
										// CRTC�����f�[�^
	Event event;
										// �C�x���g
	TVRAM *tvram;
										// �e�L�X�gVRAM
	GVRAM *gvram;
										// �O���t�B�b�NVRAM
	Sprite *sprite;
										// �X�v���C�g�R���g���[��
	MFP *mfp;
										// MFP
	Render *render;
										// �����_��
	Printer *printer;
										// �v�����^
};

#endif	// crtc_h
