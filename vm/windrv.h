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
	BOOL FASTCALL Init();								// 初期化
	void FASTCALL Cleanup();							// クリーンアップ
	void FASTCALL Reset();								// リセット

	// メモリデバイス
	DWORD FASTCALL ReadByte(DWORD addr);				// バイト読み込み
	void FASTCALL WriteByte(DWORD addr, DWORD data);	// バイト書き込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;			// 読み込みのみ
};
#endif // windrv_h
