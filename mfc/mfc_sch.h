//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �X�P�W���[�� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sch_h)
#define mfc_sch_h

void schedulerInit();
BOOL schedulerIsEnable();
void schedulerSetEnable(BOOL b);

#endif	// mfc_sch_h
#endif	// _WIN32
