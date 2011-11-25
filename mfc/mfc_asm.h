//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �A�Z���u���T�u ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_asm_h)
#define mfc_asm_h

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	�v���g�^�C�v�錾
//
//---------------------------------------------------------------------------
void SoundMMX(DWORD *pSrc, WORD *pDst, int nBytes);	// �T�E���h�T���v���T�C�W���O(MMX)
void SoundEMMS();									// �T�E���h�T���v���T�C�W���O(EMMS)

#if defined(__cplusplus)
}
#endif	//__cplusplus

#endif	// mfc_asm_h
#endif	// _WIN32
