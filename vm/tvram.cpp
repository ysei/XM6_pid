//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �e�L�X�gVRAM ]
//
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "schedule.h"
#include "render.h"
#include "renderin.h"
#include "tvram.h"

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
TVRAMHandler::TVRAMHandler(Render *rend, uint8_t *mem)
{
	ASSERT(rend);
	ASSERT(mem);

	// �L��
	render = rend;
	tvram = mem;

	// ���[�N��������
	multi = 0;
	mask = 0;
	rev = 0;
	maskh = 0;
	revh = 0;
}

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�m�[�}��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
TVRAMNormal::TVRAMNormal(Render *rend, uint8_t *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMNormal::WriteByte(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	if (tvram[addr] != data) {
		tvram[addr] = (uint8_t)data;
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMNormal::WriteWord(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	if ((uint32_t)*(uint16_t*)(&tvram[addr]) != data) {
		*(uint16_t*)(&tvram[addr]) = (uint16_t)data;
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}�X�N)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
TVRAMMask::TVRAMMask(Render *rend, uint8_t *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMask::WriteByte(uint32_t addr, uint32_t data)
{
	uint32_t mem;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	// mask��1�͕ύX���Ȃ��A0�͕ύX����
	mem = (uint32_t)tvram[addr];
	if (addr & 1) {
		// 68000�ł͋����A�h���X��b15-b8���g��
		mem &= maskh;
		data &= revh;
	}
	else {
		// 68000�ł͊�A�h���X��b7-b0���g��
		mem &= mask;
		data &= rev;
	}

	// ����
	data |= mem;

	// ��������
	if (tvram[addr] != data) {
		tvram[addr] = (uint8_t)data;
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMask::WriteWord(uint32_t addr, uint32_t data)
{
	uint32_t mem;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	// mask��1�͕ύX���Ȃ��A0�͕ύX����
	mem = (uint32_t)*(uint16_t*)(&tvram[addr]);
	mem &= mask;
	data &= rev;

	// ����
	data |= mem;

	if ((uint32_t)*(uint16_t*)(&tvram[addr]) != data) {
		*(uint16_t*)(&tvram[addr]) = (uint16_t)data;
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}���`)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
TVRAMMulti::TVRAMMulti(Render *rend, uint8_t *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMulti::WriteByte(uint32_t addr, uint32_t data)
{
	int flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	// ������
	addr &= 0x1ffff;
	flag = FALSE;

	// �v���[��B
	if (multi & 1) {
		if (tvram[addr] != data) {
			tvram[addr] = (uint8_t)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��G
	if (multi & 2) {
		if (tvram[addr] != data) {
			tvram[addr] = (uint8_t)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��R
	if (multi & 4) {
		if (tvram[addr] != data) {
			tvram[addr] = (uint8_t)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��I
	if (multi & 8) {
		if (tvram[addr] != data) {
			tvram[addr] = (uint8_t)data;
			flag = TRUE;
		}
	}

	// �����_���֒ʒm
	if (flag) {
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMulti::WriteWord(uint32_t addr, uint32_t data)
{
	int flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	// ������
	addr &= 0x1fffe;
	flag = FALSE;

	// �v���[��B
	if (multi & 1) {
		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != data) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��G
	if (multi & 2) {
		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != data) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��R
	if (multi & 4) {
		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != data) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��I
	if (multi & 8) {
		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != data) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)data;
			flag = TRUE;
		}
	}

	// �����_���֒ʒm
	if (flag) {
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(����)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
TVRAMBoth::TVRAMBoth(Render *rend, uint8_t *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMBoth::WriteByte(uint32_t addr, uint32_t data)
{
	uint32_t mem;
	uint32_t maskhl;
	int flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	// �����E��̔���͐�ɍς܂���
	if (addr & 1) {
		maskhl = maskh;
		data &= revh;
	}
	else {
		maskhl = mask;
		data &= rev;
	}

	// ������
	addr &= 0x1ffff;
	flag = FALSE;

	// �v���[��B
	if (multi & 1) {
		mem = (uint32_t)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (uint8_t)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��G
	if (multi & 2) {
		mem = (uint32_t)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (uint8_t)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��R
	if (multi & 4) {
		mem = (uint32_t)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (uint8_t)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��I
	if (multi & 8) {
		mem = (uint32_t)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (uint8_t)mem;
			flag = TRUE;
		}
	}

	// �����_���֒ʒm
	if (flag) {
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMBoth::WriteWord(uint32_t addr, uint32_t data)
{
	uint32_t mem;
	int flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	// �f�[�^�͐�Ƀ}�X�N���Ă���
	data &= rev;

	// ������
	addr &= 0x1fffe;
	flag = FALSE;

	// �v���[��B
	if (multi & 1) {
		mem = (uint32_t)*(uint16_t*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != mem) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��G
	if (multi & 2) {
		mem = (uint32_t)*(uint16_t*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != mem) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��R
	if (multi & 4) {
		mem = (uint32_t)*(uint16_t*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != mem) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// �v���[��I
	if (multi & 8) {
		mem = (uint32_t)*(uint16_t*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((uint32_t)*(uint16_t*)(&tvram[addr]) != mem) {
			*(uint16_t*)(&tvram[addr]) = (uint16_t)mem;
			flag = TRUE;
		}
	}

	// �����_���֒ʒm
	if (flag) {
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	�e�L�X�gVRAM
//
//===========================================================================
//#define TVRAM_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
TVRAM::TVRAM(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('T', 'V', 'R', 'M');
	dev.desc = "Text VRAM";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe00000;
	memdev.last = 0xe7ffff;

	// �n���h��
	normal = NULL;
	mask = NULL;
	multi = NULL;
	both = NULL;

	// ���̑�
	render = NULL;
	tvram = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL TVRAM::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �����_���擾
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// �������m�ہA�N���A
	try {
		tvram = new uint8_t[ 0x80000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!tvram) {
		return FALSE;
	}
	memset(tvram, 0, 0x80000);

	// �n���h���쐬
	normal = new TVRAMNormal(render, tvram);
	mask = new TVRAMMask(render, tvram);
	multi = new TVRAMMulti(render, tvram);
	both = new TVRAMBoth(render, tvram);
	handler = normal;

	// ���[�N�G���A������
	tvdata.multi = 0;
	tvdata.mask = 0;
	tvdata.rev = 0xffffffff;
	tvdata.maskh = 0;
	tvdata.revh = 0xffffffff;
	tvdata.src = 0;
	tvdata.dst = 0;
	tvdata.plane = 0;
	tvcount = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::Cleanup()
{
	ASSERT(this);

	// �n���h���폜
	if (both) {
		delete both;
		both = NULL;
	}
	if (multi) {
		delete multi;
		multi = NULL;
	}
	if (mask) {
		delete mask;
		mask = NULL;
	}
	if (normal) {
		delete normal;
		normal = NULL;
	}
	handler = NULL;

	// ���������
	delete[] tvram;
	tvram = NULL;

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// ���[�N�G���A������
	tvdata.multi = 0;
	tvdata.mask = 0;
	tvdata.rev = 0xffffffff;
	tvdata.maskh = 0;
	tvdata.revh = 0xffffffff;
	tvdata.src = 0;
	tvdata.dst = 0;
	tvdata.plane = 0;

	// �A�N�Z�X�J�E���g0
	tvcount = 0;

	// �n���h���̓m�[�}��
	handler = normal;
	ASSERT(handler);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL TVRAM::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// ���������Z�[�u
	if (!fio->Write(tvram, 0x80000)) {
		return FALSE;
	}

	// �T�C�Y���Z�[�u
	sz = sizeof(tvram_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&tvdata, (int)sz)) {
		return FALSE;
	}

	// tvcount(version2.04�Œǉ�)
	if (!fio->Write(&tvcount, sizeof(tvcount))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL TVRAM::Load(Fileio *fio, int ver)
{
	size_t sz;
	uint32_t addr;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �����������[�h
	if (!fio->Read(tvram, 0x80000)) {
		return FALSE;
	}

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(tvram_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&tvdata, (int)sz)) {
		return FALSE;
	}

	// tvcount(version2.04�Œǉ�)
	tvcount = 0;
	if (ver >= 0x0204) {
		if (!fio->Read(&tvcount, sizeof(tvcount))) {
			return FALSE;
		}
	}

	// �����_���֒ʒm
	for(addr=0; addr<0x20000; addr++) {
		render->TextMem(addr);
	}

	// �n���h���ݒ�
	SelectHandler();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('T', 'V', 'R', 'M'));
	ASSERT(memdev.first == 0xe00000);
	ASSERT(memdev.last == 0xe7ffff);
	ASSERT(tvram);
	ASSERT(normal);
	ASSERT(mask);
	ASSERT(multi);
	ASSERT(both);
	ASSERT(handler);
	ASSERT(tvdata.multi <= 0x1f);
	ASSERT(tvdata.mask < 0x10000);
	ASSERT(tvdata.rev >= 0xffff0000);
	ASSERT(tvdata.maskh < 0x100);
	ASSERT(tvdata.revh >= 0xffffff00);
	ASSERT(tvdata.src < 0x100);
	ASSERT(tvdata.dst < 0x100);
	ASSERT(tvdata.plane <= 0x0f);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL TVRAM::ReadByte(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �E�F�C�g(0.75�E�F�C�g)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// �G���f�B�A���𔽓]�����ēǂݍ���
	return (uint32_t)tvram[(addr & 0x7ffff) ^ 1];
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL TVRAM::ReadWord(uint32_t addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// �E�F�C�g(0.75�E�F�C�g)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// �ǂݍ���
	return (uint32_t)*(uint16_t *)(&tvram[addr & 0x7ffff]);
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::WriteByte(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �E�F�C�g(0.75�E�F�C�g)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// ��������
	handler->WriteByte((addr & 0x7ffff) ^ 1, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::WriteWord(uint32_t addr, uint32_t data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// �E�F�C�g(0.75�E�F�C�g)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// ��������
	handler->WriteWord(addr & 0x7ffff, data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
uint32_t FASTCALL TVRAM::ReadOnly(uint32_t addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �G���f�B�A���𔽓]�����ēǂݍ���
	return tvram[(addr & 0x7ffff) ^ 1];
}

//---------------------------------------------------------------------------
//
//	TVRAM�擾
//
//---------------------------------------------------------------------------
const uint8_t* FASTCALL TVRAM::GetTVRAM() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return tvram;
}

//---------------------------------------------------------------------------
//
//	�����������ݐݒ�
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SetMulti(uint32_t data)
{
	ASSERT(this);
	ASSERT(data <= 0x1f);
	ASSERT_DIAG();

	// ��v�`�F�b�N
	if (tvdata.multi == data) {
		return;
	}

	// �f�[�^���R�s�[
	tvdata.multi = data;

	// �n���h���I��
	SelectHandler();
}

//---------------------------------------------------------------------------
//
//	�A�N�Z�X�}�X�N�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SetMask(uint32_t data)
{
	ASSERT(this);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// ��v�`�F�b�N
	if (tvdata.mask == data) {
		return;
	}

	// �f�[�^���R�s�[
	tvdata.mask = data;
	tvdata.rev = ~tvdata.mask;
	tvdata.maskh = tvdata.mask >> 8;
	tvdata.revh = ~tvdata.maskh;

	// �n���h���I��
	SelectHandler();
}

//---------------------------------------------------------------------------
//
//	�n���h���I��
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SelectHandler()
{
	ASSERT(this);
	ASSERT_DIAG();

	// �ʏ�
	handler = normal;

	// �}���`�`�F�b�N
	if (tvdata.multi != 0) {
		// �}�X�N�ƕ��p��
		if (tvdata.mask != 0) {
			// ����
			handler = both;

			// �}���`�f�[�^��ݒ�
			handler->multi = tvdata.multi;

			// �}�X�N�f�[�^��ݒ�
			handler->mask = tvdata.mask;
			handler->rev = tvdata.rev;
			handler->maskh = tvdata.maskh;
			handler->revh = tvdata.revh;
		}
		else {
			// �}���`
			handler = multi;

			// �}���`�f�[�^��ݒ�
			handler->multi = tvdata.multi;
		}
		return;
	}

	// �}�X�N�`�F�b�N
	if (tvdata.mask != 0) {
		// �}�X�N
		handler = mask;

		// �}�X�N�f�[�^��ݒ�
		handler->mask = tvdata.mask;
		handler->rev = tvdata.rev;
		handler->maskh = tvdata.maskh;
		handler->revh = tvdata.revh;
	}
}

//---------------------------------------------------------------------------
//
//	���X�^�R�s�[�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SetCopyRaster(uint32_t src, uint32_t dst, uint32_t plane)
{
	ASSERT(this);
	ASSERT(src < 0x100);
	ASSERT(dst < 0x100);
	ASSERT(plane <= 0x0f);
	ASSERT_DIAG();

	tvdata.src = src;
	tvdata.dst = dst;
	tvdata.plane = plane;
}

//---------------------------------------------------------------------------
//
//	���X�^�R�s�[���s
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::RasterCopy()
{
	ASSERT(this);
	ASSERT_DIAG();
#if 0
	uint32_t *p;
	uint32_t *q;
	int i;
	int j;
	uint32_t plane;

	// �|�C���^�A�v���[����������
	p = (uint32_t*)&tvram[tvdata.src << 9];
	q = (uint32_t*)&tvram[tvdata.dst << 9];
	plane = tvdata.plane;

	// �v���[���ʂɍs��
	for (i=0; i<4; i++) {
		if (plane & 1) {
			for (j=7; j>=0; j--) {
				q[0] = p[0];
				q[1] = p[1];
				q[2] = p[2];
				q[3] = p[3];
				q[4] = p[4];
				q[5] = p[5];
				q[6] = p[6];
				q[7] = p[7];
				q[8] = p[8];
				q[9] = p[9];
				q[10] = p[10];
				q[11] = p[11];
				q[12] = p[12];
				q[13] = p[13];
				q[14] = p[14];
				q[15] = p[15];
				p += 16;
				q += 16;
			}
			p -= 128;
			q -= 128;
		}
		p += 0x8000;
		q += 0x8000;
		plane >>= 1;
	}

	// �����_���ɁA�R�s�[��̃G���A���u������������Ƃ�ʒm
	plane = tvdata.dst;
	plane <<= 9;
	for (i=0; i<0x200; i++) {
		render->TextMem(plane + i);
	}
#endif
	// �����_�����Ă�
	if (tvdata.plane != 0) {
		render->TextCopy(tvdata.src, tvdata.dst, tvdata.plane);
	}
}
