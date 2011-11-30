//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�^�e�B�b�NRAM ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "filepath.h"
#include "fileio.h"
#include "cpu.h"
#include "schedule.h"
#include "memory_xm6.h"
#include "config.h"
#include "sram.h"

//===========================================================================
//
//	�X�^�e�B�b�NRAM
//
//===========================================================================
//#define SRAM_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
SRAM::SRAM(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('S', 'R', 'A', 'M');
	dev.desc = "Static RAM";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xed0000;
	memdev.last = 0xedffff;

	// ���̑�������
	sram_size = 16;
	write_en = FALSE;
	mem_sync = TRUE;
	changed = FALSE;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL SRAM::Init()
{
	Fileio fio;
	int i;
	uint8_t data;

	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ������
	memset(sram, 0xff, sizeof(sram));

	// �p�X�쐬�A�ǂݍ���
	sram_path.SysFile(XM6_pid::SYS_FILE_TYPE_SRAM);
	fio.Load(sram_path, sram, sizeof(sram));

	// �G���f�B�A�����]
	for (i=0; i<sizeof(sram); i+=2) {
		data = sram[i];
		sram[i] = sram[i + 1];
		sram[i + 1] = data;
	}

	// �ύX�Ȃ�
	ASSERT(!changed);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::Cleanup()
{
	Fileio fio;
	int i;
	uint8_t data;

	ASSERT(this);

	// �ύX�����
	if (changed) {
		// �G���f�B�A�����]
		for (i=0; i<sizeof(sram); i+=2) {
			data = sram[i];
			sram[i] = sram[i + 1];
			sram[i + 1] = data;
		}

		// ��������
		fio.Save(sram_path, sram, sram_size << 10);
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// �������݋֎~�ɏ�����
	write_en = FALSE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL SRAM::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// SRAM�T�C�Y
	if (!fio->Write(&sram_size, sizeof(sram_size))) {
		return FALSE;
	}

	// SRAM�{��(64KB�܂Ƃ߂�)
	if (!fio->Write(&sram, sizeof(sram))) {
		return FALSE;
	}

	// �������݋��t���O
	if (!fio->Write(&write_en, sizeof(write_en))) {
		return FALSE;
	}

	// �����t���O
	if (!fio->Write(&mem_sync, sizeof(mem_sync))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL SRAM::Load(Fileio *fio, int /*ver*/)
{
	uint8_t *buf;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �o�b�t�@�m��
	try {
		buf = new uint8_t[sizeof(sram)];
	}
	catch (...) {
		buf = NULL;
	}
	if (!buf) {
		return FALSE;
	}

	// SRAM�T�C�Y
	if (!fio->Read(&sram_size, sizeof(sram_size))) {
		delete[] buf;
		return FALSE;
	}

	// SRAM�{��(64KB�܂Ƃ߂�)
	if (!fio->Read(buf, sizeof(sram))) {
		delete[] buf;
		return FALSE;
	}

	// ��r�Ɠ]��
	if (memcmp(sram, buf, sizeof(sram)) != 0) {
		memcpy(sram, buf, sizeof(sram));
		changed = TRUE;
	}

	// ��Ƀo�b�t�@���
	delete[] buf;
	buf = NULL;

	// �������݋��t���O
	if (!fio->Read(&write_en, sizeof(write_en))) {
		return FALSE;
	}

	// �����t���O
	if (!fio->Read(&mem_sync, sizeof(mem_sync))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");

	// SRAM�T�C�Y
	if (config->sram_64k) {
		sram_size = 64;
#if defined(SRAM_LOG)
		LOG0(Log::Detail, "�������T�C�Y 64KB");
#endif	// SRAM_LOG
	}
	else {
		sram_size = 16;
	}

	// �������X�C�b�`��������
	mem_sync = config->ram_sramsync;
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'R', 'A', 'M'));
	ASSERT(memdev.first == 0xed0000);
	ASSERT(memdev.last == 0xedffff);
	ASSERT((sram_size == 16) || (sram_size == 32) || (sram_size == 48) || (sram_size == 64));
	ASSERT((write_en == TRUE) || (write_en == FALSE));
	ASSERT((mem_sync == TRUE) || (mem_sync == FALSE));
	ASSERT((changed == TRUE) || (changed == FALSE));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL SRAM::ReadByte(uint32_t addr)
{
	uint32_t size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �I�t�Z�b�g�Z�o
	addr -= memdev.first;
	size = (uint32_t)(sram_size << 10);

	// �������`�F�b�N
	if (size <= addr) {
		// �o�X�G���[
		cpu->BusErr(memdev.first + addr, TRUE);
		return 0xff;
	}

	// �E�F�C�g
	scheduler->Wait(1);

	// �ǂݍ���(�G���f�B�A���𔽓]������)
	return (uint32_t)sram[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL SRAM::ReadWord(uint32_t addr)
{
	uint32_t size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// �I�t�Z�b�g�Z�o
	addr -= memdev.first;
	size = (uint32_t)(sram_size << 10);

	// �������`�F�b�N
	if (size <= addr) {
		// �o�X�G���[
		cpu->BusErr(memdev.first + addr, TRUE);
		return 0xff;
	}

	// �E�F�C�g
	scheduler->Wait(1);

	// �ǂݍ���(�G���f�B�A���ɒ���)
	return (uint32_t)(*(uint16_t *)&sram[addr]);
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::WriteByte(uint32_t addr, uint32_t data)
{
	uint32_t size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �I�t�Z�b�g�Z�o
	addr -= memdev.first;
	size = (uint32_t)(sram_size << 10);

	// �������`�F�b�N
	if (size <= addr) {
		// �o�X�G���[
		cpu->BusErr(memdev.first + addr, FALSE);
		return;
	}

	// �������݉\�`�F�b�N
	if (!write_en) {
		LOG1(Log::Warning, "�������݋֎~ $%06X", memdev.first + addr);
		return;
	}

	// �E�F�C�g
	scheduler->Wait(1);

	// �A�h���X$09��$00 or $10���������܂��ꍇ�A�������X�C�b�`�����X�V�ł���΃X�L�b�v������
	// (���Z�b�g����Memory::Reset���珑�����܂�Ă��邽�߁A�㏑���ɂ��j���h��)
	if ((addr == 0x09) && (data == 0x10)) {
		if (cpu->GetPC() == 0xff03a8) {
			if (mem_sync) {
				LOG2(Log::Warning, "�X�C�b�`�ύX�}�� $%06X <- $%02X", memdev.first + addr, data);
				return;
			}
		}
	}

	// ��������(�G���f�B�A���𔽓]������)
	if (addr < 0x100) {
		LOG2(Log::Detail, "�������X�C�b�`�ύX $%06X <- $%02X", memdev.first + addr, data);
	}
	if (sram[addr ^ 1] != (uint8_t)data) {
		sram[addr ^ 1] = (uint8_t)data;
		changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::WriteWord(uint32_t addr, uint32_t data)
{
	uint32_t size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// �I�t�Z�b�g�Z�o
	addr -= memdev.first;
	size = (uint32_t)(sram_size << 10);

	// �������`�F�b�N
	if (size <= addr) {
		// �o�X�G���[
		cpu->BusErr(memdev.first + addr, FALSE);
		return;
	}

	// �������݉\�`�F�b�N
	if (!write_en) {
		LOG1(Log::Warning, "�������݋֎~ $%06X", memdev.first + addr);
		return;
	}

	// �E�F�C�g
	scheduler->Wait(1);

	// ��������(�G���f�B�A���ɒ���)
	if (addr < 0x100) {
		LOG2(Log::Detail, "�������X�C�b�`�ύX $%06X <- $%04X", memdev.first + addr, data);
	}
	if (*(uint16_t *)&sram[addr] != (uint16_t)data) {
		*(uint16_t *)&sram[addr] = (uint16_t)data;
		changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
uint32_t FASTCALL SRAM::ReadOnly(uint32_t addr) const
{
	uint32_t size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �I�t�Z�b�g�Z�o
	addr -= memdev.first;
	size = (uint32_t)(sram_size << 10);

	// �������`�F�b�N
	if (size <= addr) {
		return 0xff;
	}

	// �ǂݍ���(�G���f�B�A���𔽓]������)
	return (uint32_t)sram[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	SRAM�A�h���X�擾
//
//---------------------------------------------------------------------------
const uint8_t* FASTCALL SRAM::GetSRAM() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return sram;
}

//---------------------------------------------------------------------------
//
//	SRAM�T�C�Y�擾
//
//---------------------------------------------------------------------------
int FASTCALL SRAM::GetSize() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return sram_size;
}

//---------------------------------------------------------------------------
//
//	�������݋���
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::WriteEnable(int enable)
{
	ASSERT(this);
	ASSERT_DIAG();

	write_en = enable;

	if (write_en) {
		LOG0(Log::Detail, "SRAM�������݋���");
	}
	else {
		LOG0(Log::Detail, "SRAM�������݋֎~");
	}
}

//---------------------------------------------------------------------------
//
//	�������X�C�b�`�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::SetMemSw(uint32_t offset, uint32_t data)
{
	ASSERT(this);
	ASSERT(offset < 0x100);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	LOG2(Log::Detail, "�������X�C�b�`�ݒ� $%06X <- $%02X", memdev.first + offset, data);
	if (sram[offset ^ 1] != (uint8_t)data) {
		sram[offset ^ 1] = (uint8_t)data;
		changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	�������X�C�b�`�擾
//
//---------------------------------------------------------------------------
uint32_t FASTCALL SRAM::GetMemSw(uint32_t offset) const
{
	ASSERT(this);
	ASSERT(offset < 0x100);
	ASSERT_DIAG();

	return (uint32_t)sram[offset ^ 1];
}

//---------------------------------------------------------------------------
//
//	�N���J�E���^�X�V
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::UpdateBoot()
{
	uint16_t *ptr;

	ASSERT(this);
	ASSERT_DIAG();

	// ��ɕύX����
	changed = TRUE;

	// �|�C���^�ݒ�($ED0044)
	ptr = (uint16_t *)&sram[0x0044];

	// �C���N�������g(Low)
	if (ptr[1] != 0xffff) {
		ptr[1] = ptr[1] + 1;
		return;
	}

	// �C���N�������g(High)
	ptr[1] = 0x0000;
	ptr[0] = ptr[0] + 1;
}
