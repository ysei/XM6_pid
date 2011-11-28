//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �����_�� ]
//
//---------------------------------------------------------------------------

#if !defined(render_h)
#define render_h

#include "device.h"
#include "vc.h"

//===========================================================================
//
//	�����_��
//
//===========================================================================
class Render : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		// �S�̐���
		int act;						// �������Ă��邩
		int enable;					// ��������
		int count;						// �X�P�W���[���A�g�J�E���^
		int ready;						// �`�揀���ł��Ă��邩
		int first;						// ���������X�^
		int last;						// �\���I�����X�^

		// CRTC
		int crtc;						// CRTC�ύX�t���O
		int width;						// X�����h�b�g��(256�`)
		int h_mul;						// X�����{��(1,2)
		int height;						// Y�����h�b�g��(256�`)
		int v_mul;						// Y�����{��(0,1,2)
		int lowres;					// 15kHz�t���O

		// VC
		int vc;						// VC�ύX�t���O

		// ����
		int mix[1024];					// �����t���O(���C��)
		uint32_t *mixbuf;					// �����o�b�t�@
		uint32_t *mixptr[8];				// �����|�C���^
		uint32_t mixshift[8];				// �����|�C���^��Y�V�t�g
		uint32_t *mixx[8];					// �����|�C���^��X�X�N���[���|�C���^
		uint32_t *mixy[8];					// �����|�C���^��Y�X�N���[���|�C���^
		uint32_t mixand[8];				// �����|�C���^�̃X�N���[��AND�l
		int mixmap[3];					// �����}�b�v
		int mixtype;					// �����^�C�v
		int mixpage;					// �����O���t�B�b�N�y�[�W��
		int mixwidth;					// �����o�b�t�@��
		int mixheight;					// �����o�b�t�@����
		int mixlen;						// ��������������(x����)

		// �`��
		int draw[1024];				// �`��t���O(���C��)
		int *drawflag;					// �`��t���O(16dot)

		// �R���g���X�g
		int contrast;					// �R���g���X�g�ύX�t���O
		int contlevel;					// �R���g���X�g

		// �p���b�g
		int palette;					// �p���b�g�ύX�t���O
		int palmod[0x200];				// �p���b�g�ύX�t���O
		uint32_t *palbuf;					// �p���b�g�o�b�t�@
		uint32_t *palptr;					// �p���b�g�|�C���^
		const uint16_t *palvc;				// �p���b�gVC�|�C���^
		uint32_t paldata[0x200];			// �p���b�g�f�[�^
		uint8_t pal64k[0x200];				// �p���b�g�f�[�^�ό`

		// �e�L�X�gVRAM
		int texten;					// �e�L�X�g�\���t���O
		int textpal[1024];				// �e�L�X�g�p���b�g�t���O
		int textmod[1024];				// �e�L�X�g�X�V�t���O(���C��)
		int *textflag;					// �e�L�X�g�X�V�t���O(32dot)
		uint8_t *textbuf;					// �e�L�X�g�o�b�t�@(�p���b�g�O)
		uint32_t *textout;					// �e�L�X�g�o�b�t�@(�p���b�g��)
		const uint8_t *texttv;				// �e�L�X�gTVRAM�|�C���^
		uint32_t textx;					// �e�L�X�g�X�N���[��X
		uint32_t texty;					// �e�L�X�g�X�N���[��Y

		// �O���t�B�b�NVRAM
		int grptype;					// �O���t�B�b�N�^�C�v(0�`4)
		int grpen[4];					// �O���t�B�b�N�u���b�N�\���t���O
		int grppal[2048];				// �O���t�B�b�N�p���b�g�t���O
		int grpmod[2048];				// �O���t�B�b�N�X�V�t���O(���C��)
		int *grpflag;					// �O���t�B�b�N�X�V�t���O(16dot)
		uint32_t *grpbuf[4];				// �O���t�B�b�N�u���b�N�o�b�t�@
		const uint8_t* grpgv;				// �O���t�B�b�NGVRAM�|�C���^
		uint32_t grpx[4];					// �O���t�B�b�N�u���b�N�X�N���[��X
		uint32_t grpy[4];					// �O���t�B�b�N�u���b�N�X�N���[��Y

		// PCG
		int pcgready[256 * 16];		// PCG����OK�t���O
		uint32_t pcguse[256 * 16];			// PCG�g�p���J�E���g
		uint32_t pcgpal[16];				// PCG�p���b�g�g�p�J�E���g
		uint32_t *pcgbuf;					// PCG�o�b�t�@
		const uint8_t* sprmem;				// �X�v���C�g������

		// �X�v���C�g
		uint32_t **spptr;					// �X�v���C�g�|�C���^�o�b�t�@
		uint32_t spreg[0x200];				// �X�v���C�g���W�X�^�ۑ�
		int spuse[128];				// �X�v���C�g�g�p���t���O

		// BG
		uint32_t bgreg[2][64 * 64];		// BG���W�X�^�{�ύX�t���O($10000)
		int bgall[2][64];				// BG�ύX�t���O(�u���b�N�P��)
		int bgdisp[2];					// BG�\���t���O
		int bgarea[2];					// BG�\���G���A
		int bgsize;					// BG�\���T�C�Y(16dot=TRUE)
		uint32_t **bgptr[2];				// BG�|�C���^+�f�[�^
		int bgmod[2][1024];			// BG�X�V�t���O
		uint32_t bgx[2];					// BG�X�N���[��(X)
		uint32_t bgy[2];					// BG�X�N���[��(Y)

		// BG/�X�v���C�g����
		int bgspflag;					// BG/�X�v���C�g�\���t���O
		int bgspdisp;					// BG/�X�v���C�gCPU/Video�t���O
		int bgspmod[512];				// BG/�X�v���C�g�X�V�t���O
		uint32_t *bgspbuf;					// BG/�X�v���C�g�o�b�t�@
		uint32_t zero;						// �X�N���[���_�~�[(0)
	} render_t;

public:
	// ��{�t�@���N�V����
	Render(VM *p);
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

	// �O��API(�R���g���[��)
	void FASTCALL EnableAct(int enable){ render.enable = enable; }
										// ��������
	int FASTCALL IsActive() const		{ return render.act; }
										// �A�N�e�B�u��
	int FASTCALL IsReady() const		{ return (int)(render.count > 0); }
										// �`�惌�f�B�󋵎擾
	void FASTCALL Complete()			{ render.count = 0; }
										// �`�抮��
	void FASTCALL StartFrame();
										// �t���[���J�n(V-DISP)
	void FASTCALL EndFrame();
										// �t���[���I��(V-BLANK)
	void FASTCALL HSync(int raster)		{ render.last = raster; if (render.act) Process(); }
										// ��������(raster�܂ŏI���)
	void FASTCALL SetMixBuf(uint32_t *buf, int width, int height);
										// �����o�b�t�@�w��
	render_t* FASTCALL GetWorkAddr() 	{ return &render; }
										// ���[�N�A�h���X�擾

	// �O��API(���)
	void FASTCALL SetCRTC();
										// CRTC�Z�b�g
	void FASTCALL SetVC();
										// VC�Z�b�g
	void FASTCALL SetContrast(int cont);
										// �R���g���X�g�ݒ�
	int FASTCALL GetContrast() const;
										// �R���g���X�g�擾
	void FASTCALL SetPalette(int index);
										// �p���b�g�ݒ�
	const uint32_t* FASTCALL GetPalette() const;
										// �p���b�g�o�b�t�@�擾
	void FASTCALL TextMem(uint32_t addr);
										// �e�L�X�gVRAM�ύX
	void FASTCALL TextScrl(uint32_t x, uint32_t y);
										// �e�L�X�g�X�N���[���ύX
	void FASTCALL TextCopy(uint32_t src, uint32_t dst, uint32_t plane);
										// ���X�^�R�s�[
	void FASTCALL GrpMem(uint32_t addr, uint32_t block);
										// �O���t�B�b�NVRAM�ύX
	void FASTCALL GrpAll(uint32_t line, uint32_t block);
										// �O���t�B�b�NVRAM�ύX
	void FASTCALL GrpScrl(int block, uint32_t x, uint32_t y);
										// �O���t�B�b�N�X�N���[���ύX
	void FASTCALL SpriteReg(uint32_t addr, uint32_t data);
										// �X�v���C�g���W�X�^�ύX
	void FASTCALL BGScrl(int page, uint32_t x, uint32_t y);
										// BG�X�N���[���ύX
	void FASTCALL BGCtrl(int index, int flag);
										// BG�R���g���[���ύX
	void FASTCALL BGMem(uint32_t addr,uint16_t data);
										// BG�ύX
	void FASTCALL PCGMem(uint32_t addr);
										// PCG�ύX

	const uint32_t* FASTCALL GetTextBuf() const;
										// �e�L�X�g�o�b�t�@�擾
	const uint32_t* FASTCALL GetGrpBuf(int index) const;
										// �O���t�B�b�N�o�b�t�@�擾
	const uint32_t* FASTCALL GetPCGBuf() const;
										// PCG�o�b�t�@�擾
	const uint32_t* FASTCALL GetBGSpBuf() const;
										// BG/�X�v���C�g�o�b�t�@�擾
	const uint32_t* FASTCALL GetMixBuf() const;
										// �����o�b�t�@�擾

private:
	void FASTCALL Process();
										// �����_�����O
	void FASTCALL Video();
										// VC����
	void FASTCALL SetupGrp(int first);
										// �O���t�B�b�N�Z�b�g�A�b�v
	void FASTCALL Contrast();
										// �R���g���X�g����
	void FASTCALL Palette();
										// �p���b�g����
	void FASTCALL MakePalette();
										// �p���b�g�쐬
	uint32_t FASTCALL ConvPalette(int color, int ratio);
										// �F�ϊ�
	void FASTCALL Text(int raster);
										// �e�L�X�g
	void FASTCALL Grp(int block, int raster);
										// �O���t�B�b�N
	void FASTCALL SpriteReset();
										// �X�v���C�g���Z�b�g
	void FASTCALL BGSprite(int raster);
										// BG/�X�v���C�g
	void FASTCALL BG(int page, int raster, uint32_t *buf);
										// BG
	void FASTCALL BGBlock(int page, int y);
										// BG(���u���b�N)
	void FASTCALL Mix(int offset);
										// ����
	void FASTCALL MixGrp(int y, uint32_t *buf);
										// ����(�O���t�B�b�N)
	CRTC *crtc;
										// CRTC
	VC *vc;
										// VC
	Sprite *sprite;
										// �X�v���C�g
	render_t render;
										// �����f�[�^
	int cmov;
										// CMOV�L���b�V��
};

#endif	// render_h
