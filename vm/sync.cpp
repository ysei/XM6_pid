//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �����I�u�W�F�N�g ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "sync.h"

#if defined(_WIN32)

//===========================================================================
//
//	�����I�u�W�F�N�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Sync::Sync()
{
	// �N���e�B�J���Z�N�V�����쐬
//	csect = new CCriticalSection;
	pCriticalSection = new CRITICAL_SECTION;
	InitializeCriticalSection(pCriticalSection);
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Sync::~Sync()
{
//	// ���b�N����
//	Lock();
//
//	// �N���e�B�J���Z�N�V�����폜
//	ASSERT(csect);
//	delete csect;
//	csect = NULL;
	DeleteCriticalSection(pCriticalSection);
	delete pCriticalSection;
	pCriticalSection = 0;
}

//---------------------------------------------------------------------------
//
//	���b�N
//
//---------------------------------------------------------------------------
void FASTCALL Sync::Lock()
{
//	ASSERT(csect);
//	csect->Lock();
	EnterCriticalSection(pCriticalSection);
}

//---------------------------------------------------------------------------
//
//	�A�����b�N
//
//---------------------------------------------------------------------------
void FASTCALL Sync::Unlock()
{
//	ASSERT(csect);
//	csect->Unlock();
	LeaveCriticalSection(pCriticalSection);
}

#endif	// _WIN32
