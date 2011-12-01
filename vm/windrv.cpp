//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood��gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "memory_xm6.h"
#include "cpu.h"
#include "config.h"

#include "windrv.h"



//===========================================================================
//
//	Windrv
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Windrv::Windrv(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = XM6_MAKEID('W', 'D', 'R', 'V');
	dev.desc = "Windrv";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe9e000;
	memdev.last = 0xe9ffff;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL Windrv::Init()
{
	return MemDevice::Init();
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Cleanup()
{
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Reset()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Windrv::ReadByte(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	uint32_t result;

	if (addr == 0xE9F001) {
		// Port#1
		result = 0x01;
	} else {
		result = ReadOnly(addr);
		if (result == 0xFF) {
			cpu->BusErr(addr, TRUE);
		}
	}

	return result;
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Windrv::ReadOnly(uint32_t addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	uint32_t result;

	// ���ʃ|�[�g�ȊO��-1
	if(addr == 0xE9F000) {
		result = 'X';
	} else {
		result = 0xFF;
	}

	return result;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::WriteByte(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	switch (addr) {
	case 0xE9F000:
	case 0xE9F001:
		break;
	default:
		cpu->BusErr(addr, FALSE);
		break;
	}
}
