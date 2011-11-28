//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ レンダラ アセンブラサブ ]
//
//---------------------------------------------------------------------------

#if !defined (rend_asm_h)
#define rend_asm_h

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	プロトタイプ宣言
//
//---------------------------------------------------------------------------
void RendTextMem(const uint8_t *tvrm, int *flag, uint8_t *buf);
										// テキストレンダリング(水平垂直変換)
void RendTextPal(const uint8_t *buf, uint32_t *out, int *flag, const uint32_t *pal);
										// テキストレンダリング(パレット)
void RendTextAll(const uint8_t *buf, uint32_t *out, const uint32_t *pal);
										// テキストレンダリング(パレット全て)
void RendTextCopy(const uint8_t *src, const uint8_t *dst, uint32_t plane, int *textmem, int *textflag);
										// テキストラスタコピー

int Rend1024A(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック1024レンダリング(ページ0,1-All)
int Rend1024B(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック1024レンダリング(ページ2,3-All)
void Rend1024C(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック1024レンダリング(ページ0)
void Rend1024D(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック1024レンダリング(ページ1)
void Rend1024E(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック1024レンダリング(ページ2)
void Rend1024F(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック1024レンダリング(ページ3)
int Rend16A(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック16レンダリング(ページ0-All)
int Rend16B(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック16レンダリング(ページ0)
int Rend16C(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック16レンダリング(ページ1-All)
int Rend16D(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック16レンダリング(ページ1)
int Rend16E(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック16レンダリング(ページ2-All)
int Rend16F(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック16レンダリング(ページ2)
int Rend16G(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック16レンダリング(ページ3-All)
int Rend16H(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック16レンダリング(ページ3)
void Rend256A(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック256レンダリング(ページ0)
void Rend256B(const uint8_t *gvrm, uint32_t *buf, int *flag, const uint32_t *pal);
										// グラフィック256レンダリング(ページ1)
int Rend256C(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック256レンダリング(ページ0-All)
int Rend256D(const uint8_t *gvrm, uint32_t *buf, const uint32_t *pal);
										// グラフィック256レンダリング(ページ1-All)
void Rend64KA(const uint8_t *gvrm, uint32_t *buf, int *flag, uint8_t *plt, uint32_t *pal);
										// グラフィック64Kレンダリング
int Rend64KB(const uint8_t *gvrm, uint32_t *buf, uint8_t *plt, uint32_t *pal);
										// グラフィック64Kレンダリング(All)

void RendClrSprite(uint32_t *buf, uint32_t color, int len);
										// スプライトバッファクリア
void RendSprite(const uint32_t *line, uint32_t *buf, uint32_t x, uint32_t flag);
										// スプライトレンダリング(単体)
void RendSpriteC(const uint32_t *line, uint32_t *buf, uint32_t x, uint32_t flag);
										// スプライトレンダリング(単体、CMOV)
void RendPCGNew(uint32_t index, const uint8_t *mem, uint32_t *buf, uint32_t *pal);
										// PCGレンダリング(NewVer)
void RendBG8(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(8x8、割り切れる)
void RendBG8C(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(8x8、割り切れる、CMOV)
void RendBG8P(uint32_t **ptr, uint32_t *buf, int offset, int length, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(8x8、部分のみ)
void RendBG16(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(16x16、割り切れる)
void RendBG16C(uint32_t **ptr, uint32_t *buf, int x, int len, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(16x16、割り切れる、CMOV)
void RendBG16P(uint32_t **ptr, uint32_t *buf, int offset, int length, int *ready, const uint8_t *mem,
			uint32_t *pcgbuf, uint32_t *pal);	// BG(16x16、部分のみ)

void RendMix00(uint32_t *buf, int *flag, int len);
										// 合成(0面)
void RendMix01(uint32_t *buf, const uint32_t *src, int *flag, int len);
										// 合成(1面)
void RendMix02(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// 合成(2面、カラー0重ね合わせ)
void RendMix02C(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// 合成(2面、カラー0重ね合わせ、CMOV)
void RendMix03(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// 合成(2面、通常重ね合わせ)
void RendMix03C(uint32_t *buf, const uint32_t *f, const uint32_t *s, int *flag, int len);
										// 合成(2面、通常重ね合わせ、CMOV)
void RendMix04(uint32_t *buf, const uint32_t *f, const uint32_t *s, uint32_t *t, int *flag, int len);
										// 合成(3面、通常重ね合わせ、CMOV)
void RendMix04C(uint32_t *buf, const uint32_t *f, const uint32_t *s, uint32_t *t, int *flag, int len);
										// 合成(3面、通常重ね合わせ、CMOV)
void RendGrp02(uint32_t *buf, const uint32_t *f, const uint32_t *s, int len);
										// グラフィック合成(2面)
void RendGrp02C(uint32_t *buf, const uint32_t *f, const uint32_t *s, int len);
										// グラフィック合成(2面、CMOV)
void RendGrp03(uint32_t *buf, const uint32_t *f, const uint32_t *s, const uint32_t *t, int len);
										// グラフィック合成(3面)
void RendGrp03C(uint32_t *buf, const uint32_t *f, const uint32_t *s, const uint32_t *t, int len);
										// グラフィック合成(3面、CMOV)
void RendGrp04(uint32_t *buf, uint32_t *f, uint32_t *s, uint32_t *t, uint32_t *e, int len);
										// グラフィック合成(4面)

#if defined(__cplusplus)
}
#endif	//__cplusplus

#endif	// rend_asm_h
