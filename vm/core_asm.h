//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 仮想マシンコア アセンブラサブ ]
//
//---------------------------------------------------------------------------

#if !defined (core_asm_h)
#define core_asm_h

//#if _MSC_VER >= 1200

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	プロトタイプ宣言
//
//---------------------------------------------------------------------------
void MemInitDecode(Memory *mem, MemDevice* list[]);
										// メモリデコーダ初期化
void ReadByteC(uint32_t addr);
										// バイト読み込み
void ReadWordC(uint32_t addr);
										// ワード読み込み
void WriteByteC(uint32_t addr, uint32_t data);
										// バイト書き込み
void WriteWordC(uint32_t addr, uint32_t data);
										// ワード書き込み
void ReadErrC(uint32_t addr);
										// バスエラー読み込み
void WriteErrC(uint32_t addr, uint32_t data);
										// バスエラー書き込み
void NotifyEvent(Event *first);
										// イベント群 指定
uint32_t GetMinEvent(uint32_t hus);
										// イベント群 最小のものを探す
int SubExecEvent(uint32_t hus);
										// イベント群 減算＆実行
extern unsigned int MemDecodeTable[];
										// メモリデコードテーブル

#if defined(__cplusplus)
}
#endif	//__cplusplus

//#endif	// _MSC_VER
#endif	// mem_asm_h
