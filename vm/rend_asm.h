//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �����_�� �A�Z���u���T�u ]
//
//---------------------------------------------------------------------------

#if !defined (rend_asm_h)
#define rend_asm_h

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	�v���g�^�C�v�錾
//
//---------------------------------------------------------------------------
void RendTextMem(const uint8_t *tvrm, int *flag, uint8_t *buf);
										// �e�L�X�g�����_�����O(���������ϊ�)
void RendTextPal(const uint8_t *buf, uint32_t *out, int *flag, const uint32_t *pal);
										// �e�L�X�g�����_�����O(�p���b�g)
void RendTextAll(const uint8_t *buf, uint32_t *out, const uint32_t *pal);
										// �e�L�X�g�����_�����O(�p���b�g�S��)
void RendTextCopy(const uint8_t *src, const uint8_t *dst, uint32_t plane, int *textmem, int *textflag);
										// �e�L�X�g���X�^�R�s�[

int Rend1024A(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N1024�����_�����O(�y�[�W0,1-All)
int Rend1024B(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N1024�����_�����O(�y�[�W2,3-All)
void Rend1024C(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N1024�����_�����O(�y�[�W0)
void Rend1024D(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N1024�����_�����O(�y�[�W1)
void Rend1024E(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N1024�����_�����O(�y�[�W2)
void Rend1024F(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N1024�����_�����O(�y�[�W3)
int Rend16A(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W0-All)
int Rend16B(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W0)
int Rend16C(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W1-All)
int Rend16D(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W1)
int Rend16E(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W2-All)
int Rend16F(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W2)
int Rend16G(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W3-All)
int Rend16H(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N16�����_�����O(�y�[�W3)
void Rend256A(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N256�����_�����O(�y�[�W0)
void Rend256B(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// �O���t�B�b�N256�����_�����O(�y�[�W1)
int Rend256C(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N256�����_�����O(�y�[�W0-All)
int Rend256D(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// �O���t�B�b�N256�����_�����O(�y�[�W1-All)
void Rend64KA(const uint8_t *gvrm, uint32_t *buf, int *flag, uint8_t *plt, uint32_t *pal);
										// �O���t�B�b�N64K�����_�����O
int Rend64KB(const uint8_t *gvrm, uint32_t *buf, uint8_t *plt, uint32_t *pal);
										// �O���t�B�b�N64K�����_�����O(All)

void RendClrSprite(uint32_t *buf, uint32_t color, int len);
										// �X�v���C�g�o�b�t�@�N���A
void RendSprite(const uint32_t *line, uint32_t *buf, uint32_t x, uint32_t flag);
										// �X�v���C�g�����_�����O(�P��)
void RendSpriteC(const uint32_t *line, uint32_t *buf, uint32_t x, uint32_t flag);
										// �X�v���C�g�����_�����O(�P�́ACMOV)
void RendPCGNew(uint32_t index, const uint8_t *mem, uint32_t *buf, uint32_t *pal);
										// PCG�����_�����O(NewVer)
void RendBG8(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(8x8�A����؂��)
void RendBG8C(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(8x8�A����؂��ACMOV)
void RendBG8P(uint32_t **ptr, uint32_t *buf, int offset, int length, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(8x8�A�����̂�)
void RendBG16(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(16x16�A����؂��)
void RendBG16C(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(16x16�A����؂��ACMOV)
void RendBG16P(uint32_t **ptr, uint32_t *buf, int offset, int length, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(16x16�A�����̂�)

void RendMix00(uint32_t *buf, int *flag, int len);
										// ����(0��)
void RendMix01(uint32_t *buf, const uint32_t *src, int *flag, int len);
										// ����(1��)
void RendMix02(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// ����(2�ʁA�J���[0�d�ˍ��킹)
void RendMix02C(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// ����(2�ʁA�J���[0�d�ˍ��킹�ACMOV)
void RendMix03(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// ����(2�ʁA�ʏ�d�ˍ��킹)
void RendMix03C(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// ����(2�ʁA�ʏ�d�ˍ��킹�ACMOV)
void RendMix04(uint32_t *buf, const uint32_t *f, const uint32_t *s, uint32_t *t, int *flag, int len);
										// ����(3�ʁA�ʏ�d�ˍ��킹�ACMOV)
void RendMix04C(uint32_t *buf, const uint32_t *f, const uint32_t *s, uint32_t *t, int *flag, int len);
										// ����(3�ʁA�ʏ�d�ˍ��킹�ACMOV)
void RendGrp02(uint32_t *buf, const uint32_t *f, const uint32_t *s, int len);
										// �O���t�B�b�N����(2��)
void RendGrp02C(uint32_t *buf, const uint32_t *f, const uint32_t *s, int len);
										// �O���t�B�b�N����(2�ʁACMOV)
void RendGrp03(uint32_t *buf, const uint32_t *f, const uint32_t *s, const uint32_t *t, int len);
										// �O���t�B�b�N����(3��)
void RendGrp03C(uint32_t *buf, const uint32_t *f, const uint32_t *s, const uint32_t *t, int len);
										// �O���t�B�b�N����(3�ʁACMOV)
void RendGrp04(uint32_t *buf, uint32_t *f, uint32_t *s, uint32_t *t, uint32_t *e, int len);
										// �O���t�B�b�N����(4��)

#if defined(__cplusplus)
}
#endif	//__cplusplus

#endif	// rend_asm_h
