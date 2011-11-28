//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood＠gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------

#if !defined(windrv_h)
#define windrv_h

//===========================================================================
//
//	Windrv
//
//===========================================================================
class Windrv : public MemDevice
{
public:
	Windrv(VM *p);										// コンストラクタ

private:
	int FASTCALL Init();								// 初期化
	void FASTCALL Cleanup();							// クリーンアップ
	void FASTCALL Reset();								// リセット

	// メモリデバイス
	uint32_t FASTCALL ReadByte(uint32_t addr);				// バイト読み込み
	void FASTCALL WriteByte(uint32_t addr, uint32_t data);	// バイト書き込み
	uint32_t FASTCALL ReadOnly(uint32_t addr) const;			// 読み込みのみ
};
#endif // windrv_h
