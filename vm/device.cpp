//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �f�o�C�X ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "device.h"

//===========================================================================
//
//	�f�o�C�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Device::Device(VM *p)
{
	// ���[�N������
	dev.next = NULL;
	dev.id = 0;
	dev.desc = NULL;

	// ���[�N�L���A�f�o�C�X�ǉ�
	vm = p;
	vm->AddDevice(this);
#if defined(XM6_USE_LOG)
	log = &(vm->log);
#endif
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Device::~Device()
{
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL Device::Init()
{
	ASSERT(this);

	// �����ŁA�������m�ہE�t�@�C�����[�h�����s��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Device::Cleanup()
{
	ASSERT(this);

	// VM�ɑ΂��A�f�o�C�X�폜���˗�
	ASSERT(vm);
	vm->DelDevice(this);

	// �������g���폜
	delete this;
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Device::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL Device::Save(Fileio*, int)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Device::Load(Fileio*, int)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Device::ApplyCfg(const Config*)
{
	ASSERT(this);
	ASSERT_DIAG();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL Device::AssertDiag() const
{
	ASSERT(this);
	ASSERT(dev.id != 0);
	ASSERT(dev.desc);
	ASSERT(vm);
#if defined(XM6_USE_LOG)
	ASSERT(log);
	ASSERT(log == &(vm->log));
#endif
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
int FASTCALL Device::Callback(Event*)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//===========================================================================
//
//	�������}�b�v�h�f�o�C�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
MemDevice::MemDevice(VM *p) : Device(p)
{
	// �������A�h���X���蓖�ĂȂ�
	memdev.first = 0;
	memdev.last = 0;

	// �|�C���^������
	cpu = NULL;
	scheduler = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL MemDevice::Init()
{
	ASSERT(this);
	ASSERT((memdev.first != 0) || (memdev.last != 0));
	ASSERT(memdev.first <= 0xffffff);
	ASSERT(memdev.last <= 0xffffff);
	ASSERT(memdev.first <= memdev.last);

	// �f�o�C�X�̏��������ɍs��
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU�A�X�P�W���[���擾
	cpu = (CPU*)vm->SearchDevice(XM6_MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);
	scheduler = (Scheduler*)vm->SearchDevice(XM6_MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL MemDevice::ReadByte(uint32_t)
{
	ASSERT(this);
	ASSERT_DIAG();

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL MemDevice::ReadWord(uint32_t addr)
{
	uint32_t data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// ��ʁ����ʂ̏���Read
	data = ReadByte(addr);
	data <<= 8;
	data |= ReadByte(addr + 1);

	return data;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::WriteByte(uint32_t, uint32_t)
{
	ASSERT(this);
	ASSERT_DIAG();
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::WriteWord(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// ��ʁ����ʂ̏���Write
	WriteByte(addr, (uint8_t)(data >> 8));
	WriteByte(addr + 1, (uint8_t)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
uint32_t FASTCALL MemDevice::ReadOnly(uint32_t) const
{
	ASSERT(this);
	ASSERT_DIAG();

	return 0xff;
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::AssertDiag() const
{
	// ��{�N���X
	Device::AssertDiag();

	ASSERT(this);
	ASSERT((memdev.first != 0) || (memdev.last != 0));
	ASSERT(memdev.first <= 0xffffff);
	ASSERT(memdev.last <= 0xffffff);
	ASSERT(memdev.first <= memdev.last);
	ASSERT(cpu);
	ASSERT(cpu->GetID() == XM6_MAKEID('C', 'P', 'U', ' '));
	ASSERT(scheduler);
	ASSERT(scheduler->GetID() == XM6_MAKEID('S', 'C', 'H', 'E'));
}
#endif	// NDEBUG
