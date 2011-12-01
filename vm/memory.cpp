//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ������ ]
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
#include "areaset.h"
#include "gvram.h"
#include "tvram.h"
#include "sram.h"
#include "config.h"
#include "core_asm.h"
#include "memory_xm6.h"

//---------------------------------------------------------------------------
//
//	�X�^�e�B�b�N ���[�N
//
//---------------------------------------------------------------------------
static CPU *pCPU;

//---------------------------------------------------------------------------
//
//	�o�X�G���[�Č���(���C���������������G���A�̂�)
//
//---------------------------------------------------------------------------
extern "C" {

//---------------------------------------------------------------------------
//
//	�ǂݍ��݃o�X�G���[
//
//---------------------------------------------------------------------------
void ReadBusErr(uint32_t addr)
{
	pCPU->BusErr(addr, TRUE);
}

//---------------------------------------------------------------------------
//
//	�������݃o�X�G���[
//
//---------------------------------------------------------------------------
void WriteBusErr(uint32_t addr)
{
	pCPU->BusErr(addr, FALSE);
}
}

//===========================================================================
//
//	������
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Memory::Memory(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = XM6_MAKEID('M', 'E', 'M', ' ');
	dev.desc = "Memory Ctrl (OHM2)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0;
	memdev.last = 0xffffff;

	// RAM/ROM�o�b�t�@
	mem.ram = NULL;
	mem.ipl = NULL;
	mem.cg = NULL;
	mem.scsi = NULL;

	// RAM��2MB
	mem.size = 2;
	mem.config = 0;
	mem.length = 0;

	// �������^�C�v�͖����[�h
	mem.type = None;
	mem.now = None;

	// �I�u�W�F�N�g
	areaset = NULL;
	sram = NULL;

	// ���̑�
	memset(mem.table, 0, sizeof(mem.table));
	mem.memsw = TRUE;

	// static���[�N
	::pCPU = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
int FASTCALL Memory::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ���C��������
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new uint8_t[ mem.length ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ram) {
		return FALSE;
	}

	// ���C�����������[���N���A����
	memset(mem.ram, 0x00, mem.length);

	// IPL ROM
	try {
		mem.ipl = new uint8_t[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ipl) {
		return FALSE;
	}

	// CG ROM
	try {
		mem.cg = new uint8_t[ 0xc0000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.cg) {
		return FALSE;
	}

	// SCSI ROM
	try {
		mem.scsi = new uint8_t[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.scsi) {
		return FALSE;
	}

	// SASI��ROM�͕K�{�Ȃ̂ŁA��Ƀ��[�h����
	if (!LoadROM(SASI)) {
		// IPLROM.DAT, CGROM.DAT�����݂��Ȃ��p�^�[��
		return FALSE;
	}

	// ����ROM������΁AXVI��Compact��030�̏��ŁA��Ɍ����������̂�D�悷��
	if (LoadROM(XVI)) {
		mem.now = XVI;
	}
	if (mem.type == None) {
		if (LoadROM(Compact)) {
			mem.now = Compact;
		}
	}
	if (mem.type == None) {
		if (LoadROM(X68030)) {
			mem.now = X68030;
		}
	}

	// XVI,Compact,030����������݂��Ȃ���΁A�ēxSASI��ǂ�
	if (mem.type == None) {
		LoadROM(SASI);
		mem.now = SASI;
	}

	// �G���A�Z�b�g�擾
	areaset = (AreaSet*)vm->SearchDevice(XM6_MAKEID('A', 'R', 'E', 'A'));
	ASSERT(areaset);

	// SRAM�擾
	sram = (SRAM*)vm->SearchDevice(XM6_MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// static���[�N
	::pCPU = cpu;

	// �������e�[�u���ݒ�
	InitTable();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ROM���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Memory::LoadROM(memtype target)
{
	Filepath path;
	Fileio fio;
	int i;
	uint8_t data;
	uint8_t *ptr;
	int scsi_req;
	int scsi_size;

	ASSERT(this);

	// ��U���ׂĂ�ROM�G���A���������ANone��
	memset(mem.ipl, 0xff, 0x20000);
	memset(mem.cg, 0xff, 0xc0000);
	memset(mem.scsi, 0xff, 0x20000);
	mem.type = None;

	// IPL
	switch (target) {
		case SASI:
		case SCSIInt:
		case SCSIExt:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_IPL);
			break;
		case XVI:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_IPLXVI);
			break;
		case Compact:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_IPLCompact);
			break;
		case X68030:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_IPL030);
			break;
		default:
			ASSERT(FALSE);
			return FALSE;
	}
	if (!fio.Load(path, mem.ipl, 0x20000)) {
		return FALSE;
	}

	// IPL�o�C�g�X���b�v
	ptr = mem.ipl;
	for (i=0; i<0x10000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// CG
	path.SysFile(XM6_pid::SYS_FILE_TYPE_CG);
	if (!fio.Load(path, mem.cg, 0xc0000)) {
		// �t�@�C�����Ȃ���΁ACGTMP�Ń��g���C
		path.SysFile(XM6_pid::SYS_FILE_TYPE_CGTMP);
		if (!fio.Load(path, mem.cg, 0xc0000)) {
			return FALSE;
		}
	}

	// CG�o�C�g�X���b�v
	ptr = mem.cg;
	for (i=0; i<0x60000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// SCSI
	scsi_req = FALSE;
	switch (target) {
		// ����
		case SCSIInt:
		case XVI:
		case Compact:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_SCSIInt);
			scsi_req = TRUE;
			break;
		case X68030:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_ROM030);
			scsi_req = TRUE;
			break;
		// �O�t
		case SCSIExt:
			path.SysFile(XM6_pid::SYS_FILE_TYPE_SCSIExt);
			scsi_req = TRUE;
			break;
		// SASI(ROM�K�v�Ȃ�)
		case SASI:
			break;
		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}
	if (scsi_req) {
		// X68030�̂�ROM30.DAT(0x20000�o�C�g)�A���̑���0x2000�o�C�g�Ńg���C
		if (target == X68030) {
			scsi_size = 0x20000;
		}
		else {
			scsi_size = 0x2000;
		}

		// ��Ƀ|�C���^��ݒ�
		ptr = mem.scsi;

		// ���[�h
		if (!fio.Load(path, mem.scsi, scsi_size)) {
			// SCSIExt��0x1fe0�o�C�g������(WinX68k�����łƌ݊����Ƃ�)
			if (target != SCSIExt) {
				return FALSE;
			}

			// 0x1fe0�o�C�g�ōăg���C
			scsi_size = 0x1fe0;
			ptr = &mem.scsi[0x20];
			if (!fio.Load(path, &mem.scsi[0x0020], scsi_size)) {
				return FALSE;
			}
		}

		// SCSI�o�C�g�X���b�v
		for (i=0; i<scsi_size; i+=2) {
			data = ptr[0];
			ptr[0] = ptr[1];
			ptr[1] = data;
			ptr += 2;
		}
	}

	// �^�[�Q�b�g���J�����g�ɃZ�b�g���āA����
	mem.type = target;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�������e�[�u���쐬
//	���������f�R�[�_�Ɉˑ�
//
//---------------------------------------------------------------------------
void FASTCALL Memory::InitTable()
{
#if defined(_WIN32)
#pragma pack(push, 1)
#endif	// _WIN32
	MemDevice* devarray[0x40];
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

	MemDevice *mdev;
	uint8_t *table;
	uint32_t ptr;
	int i;

	ASSERT(this);

	// �|�C���^������
	mdev = this;
	i = 0;

	// Memory�ȍ~�̃f�o�C�X������āA�|�C���^��z��ɗ��Ƃ�
	while (mdev) {
		devarray[i] = mdev;

		// ����
		i++;
		mdev = (MemDevice*)mdev->GetNextDevice();
	}

	// �A�Z���u�����[�`�����Ăяo���A�e�[�u���������n��
	MemInitDecode(this, devarray);

	// �A�Z���u�����[�`���ŏo�����e�[�u�����t�ɖ߂�(�A���C�������g�ɒ���)
	table = (uint8_t*) MemDecodeTable;
	for (i=0; i<0x180; i++) {
		// 4�o�C�g���Ƃ�uint32_t�l����荞�݁A�|�C���^�ɃL���X�g
		ptr = *(uint32_t*)table;
		mem.table[i] = (MemDevice*)ptr;

		// ����
		table += 4;
	}
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Cleanup()
{
	ASSERT(this);

	// ���������
	if (mem.ram) {
		delete[] mem.ram;
		mem.ram = NULL;
	}
	if (mem.ipl) {
		delete[] mem.ipl;
		mem.ipl = NULL;
	}
	if (mem.cg) {
		delete[] mem.cg;
		mem.cg = NULL;
	}
	if (mem.scsi) {
		delete[] mem.scsi;
		mem.scsi = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Reset()
{
	int size;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �������^�C�v����v���Ă��邩
	if (mem.type != mem.now) {
		if (LoadROM(mem.type)) {
			// ROM�����݂��Ă���B���[�h�ł���
			mem.now = mem.type;
		}
		else {
			// ROM�����݂��Ȃ��BSASI�^�C�v�Ƃ��āA�ݒ��SASI�ɖ߂�
			LoadROM(SASI);
			mem.now = SASI;
			mem.type = SASI;
		}

		// �R���e�L�X�g����蒼��(CPU::Reset�͊������Ă��邽�߁A�K��FALSE)
		MakeContext(FALSE);
	}

	// �������T�C�Y����v���Ă��邩
	if (mem.size == ((mem.config + 1) * 2)) {
		// ��v���Ă���̂ŁA�������X�C�b�`�����X�V�`�F�b�N
		if (mem.memsw) {
			// $ED0008 : ���C��RAM�T�C�Y
			size = mem.size << 4;
			sram->SetMemSw(0x08, 0x00);
			sram->SetMemSw(0x09, size);
			sram->SetMemSw(0x0a, 0x00);
			sram->SetMemSw(0x0b, 0x00);
		}
		return;
	}

	// �ύX
	mem.size = (mem.config + 1) * 2;

	// �Ċm��
	ASSERT(mem.ram);
	delete[] mem.ram;
	mem.ram = NULL;
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new uint8_t[ mem.length ];
	}
	catch (...) {
		// �������s���̏ꍇ��2MB�ɌŒ�
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new uint8_t[ mem.length ];
	}
	if (!mem.ram) {
		// �������s���̏ꍇ��2MB�ɌŒ�
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new uint8_t[ mem.length ];
	}

	// ���������m�ۂł��Ă���ꍇ�̂�
	if (mem.ram) {
		memset(mem.ram, 0x00, mem.length);

		// �R���e�L�X�g����蒼��(CPU::Reset�͊������Ă��邽�߁A�K��FALSE)
		MakeContext(FALSE);
	}

	// �������X�C�b�`�����X�V
	if (mem.memsw) {
		// $ED0008 : ���C��RAM�T�C�Y
		size = mem.size << 4;
		sram->SetMemSw(0x08, 0x00);
		sram->SetMemSw(0x09, size);
		sram->SetMemSw(0x0a, 0x00);
		sram->SetMemSw(0x0b, 0x00);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
int FASTCALL Memory::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	// �^�C�v������
	if (!fio->Write(&mem.now, sizeof(mem.now))) {
		return FALSE;
	}

	// SCSI ROM�̓��e������ (X68030�ȊO)
	if (mem.now != X68030) {
		if (!fio->Write(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.size������
	if (!fio->Write(&mem.size, sizeof(mem.size))) {
		return FALSE;
	}

	// ������������
	if (!fio->Write(mem.ram, mem.length)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL Memory::Load(Fileio *fio, int /*ver*/)
{
	int size;
	int context;

	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	// �R���e�L�X�g����蒼���Ȃ�
	context = FALSE;

	// �^�C�v��ǂ�
	if (!fio->Read(&mem.type, sizeof(mem.type))) {
		return FALSE;
	}

	// �^�C�v�����݂̂��̂ƈ���Ă����
	if (mem.type != mem.now) {
		// ROM��ǂݒ���
		if (!LoadROM(mem.type)) {
			// �Z�[�u���ɑ��݂��Ă���ROM���A�Ȃ��Ȃ��Ă���
			LoadROM(mem.now);
			return FALSE;
		}

		// ROM�̓ǂݒ����ɐ�������
		mem.now = mem.type;
		context = TRUE;
	}

	// SCSI ROM�̓��e��ǂ� (X68030�ȊO)
	if (mem.type != X68030) {
		if (!fio->Read(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.size��ǂ�
	if (!fio->Read(&size, sizeof(size))) {
		return FALSE;
	}

	// mem.size�ƈ�v���Ă��Ȃ����
	if (mem.size != size) {
		// �ύX����
		mem.size = size;

		// �Ċm��
		delete[] mem.ram;
		mem.ram = NULL;
		mem.length = mem.size * 0x100000;
		try {
			mem.ram = new uint8_t[ mem.length ];
		}
		catch (...) {
			mem.ram = NULL;
		}
		if (!mem.ram) {
			// �������s���̏ꍇ��2MB�ɌŒ�
			mem.config = 0;
			mem.size = 2;
			mem.length = mem.size * 0x100000;
			mem.ram = new uint8_t[ mem.length ];

			// ���[�h���s
			return FALSE;
		}

		// �R���e�L�X�g�č쐬���K�v
		context = TRUE;
	}

	// ��������ǂ�
	if (!fio->Read(mem.ram, mem.length)) {
		return FALSE;
	}

	// �K�v�ł���΁A�R���e�L�X�g����蒼��
	if (context) {
		MakeContext(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Memory::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// ���������(ROM���[�h�͎��񃊃Z�b�g��)
	mem.type = (memtype)config->mem_type;

	// RAM�T�C�Y(�������m�ۂ͎��񃊃Z�b�g��)
	mem.config = config->ram_size;
	ASSERT((mem.config >= 0) && (mem.config <= 5));

	// �������X�C�b�`�����X�V
	mem.memsw = config->ram_sramsync;
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Memory::ReadByte(uint32_t addr)
{
	uint32_t index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// ���C��RAM
	if (addr < mem.length) {
		return (uint32_t)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (uint32_t)mem.ipl[addr];
	}

	// IPL�C���[�W or SCSI����
	if (addr >= 0xfc0000) {
		// IPL�C���[�W��
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPL�C���[�W
			addr &= 0x1ffff;
			addr ^= 1;
			return (uint32_t)mem.ipl[addr];
		}
		// SCSI������(�͈̓`�F�b�N)
		if (addr < 0xfc2000) {
			// SCSI����
			addr &= 0x1fff;
			addr ^= 1;
			return (uint32_t)mem.scsi[addr];
		}
		// X68030 IPL�O����
		if (mem.now == X68030) {
			// X68030 IPL�O��
			addr &= 0x1ffff;
			addr ^= 1;
			return (uint32_t)mem.scsi[addr];
		}
		// SCSI�������f���ŁAROM�͈͊O
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (uint32_t)mem.cg[addr];
	}

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (uint32_t)mem.scsi[addr];
		}
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadByte(addr);
		}
	}

	LOG1(Log::Warning, "����`�o�C�g�ǂݍ��� $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Memory::ReadWord(uint32_t addr)
{
	uint32_t data;
	uint32_t index;
uint16_t *ptr;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// CPU����̏ꍇ�͋����ۏ؂���Ă��邪�ADMAC����̏ꍇ�̓`�F�b�N�K�v����
	if (addr & 1) {
		// ��UCPU�֓n��(CPU�o�R��DMA��)
		cpu->AddrErr(addr, TRUE);
		return 0xffff;
	}

	// ���C��RAM
	if (addr < mem.length) {
		ptr = (uint16_t*)(&mem.ram[addr]);
		data = (uint32_t)*ptr;
		return data;
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		ptr = (uint16_t*)(&mem.ipl[addr]);
		data = (uint32_t)*ptr;
		return data;
	}

	// IPL�C���[�W or SCSI����
	if (addr >= 0xfc0000) {
		// IPL�C���[�W��
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPL�C���[�W
			addr &= 0x1ffff;
			ptr = (uint16_t*)(&mem.ipl[addr]);
			data = (uint32_t)*ptr;
			return data;
		}
		// SCSI������(�͈̓`�F�b�N)
		if (addr < 0xfc2000) {
			// SCSI����
			addr &= 0x1fff;
			ptr = (uint16_t*)(&mem.scsi[addr]);
			data = (uint32_t)*ptr;
			return data;
		}
		// X68030 IPL�O����
		if (mem.now == X68030) {
			// X68030 IPL�O��
			addr &= 0x1ffff;
			ptr = (uint16_t*)(&mem.scsi[addr]);
			data = (uint32_t)*ptr;
			return data;
		}
		// SCSI�������f���ŁAROM�͈͊O
		return 0xffff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		ptr = (uint16_t*)(&mem.cg[addr]);
		data = (uint32_t)*ptr;
		return data;
	}

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			ptr = (uint16_t*)(&mem.scsi[addr]);
			data = (uint32_t)*ptr;
			return data;
		}
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadWord(addr);
		}
	}

	// �o�X�G���[
	LOG1(Log::Warning, "����`���[�h�ǂݍ��� $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xffff;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteByte(uint32_t addr, uint32_t data)
{
	uint32_t index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);
	ASSERT(mem.now != None);

	// ���C��RAM
	if (addr < mem.length) {
		mem.ram[addr ^ 1] = (uint8_t)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteByte(addr, data);
			return;
		}
	}

	// �o�X�G���[
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "����`�o�C�g�������� $%06X <- $%02X", addr, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteWord(uint32_t addr, uint32_t data)
{
uint16_t *ptr;
	uint32_t index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x10000);
	ASSERT(mem.now != None);

	// CPU����̏ꍇ�͋����ۏ؂���Ă��邪�ADMAC����̏ꍇ�̓`�F�b�N�K�v����
	if (addr & 1) {
		// ��UCPU�֓n��(CPU�o�R��DMA��)
		cpu->AddrErr(addr, FALSE);
		return;
	}

	// ���C��RAM
	if (addr < mem.length) {
		ptr = (uint16_t*)(&mem.ram[addr]);
		*ptr = (uint16_t)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteWord(addr, data);
			return;
		}
	}

	// �o�X�G���[
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "����`���[�h�������� $%06X <- $%04X", addr, data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
uint32_t FASTCALL Memory::ReadOnly(uint32_t addr) const
{
	uint32_t index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// ���C��RAM
	if (addr < mem.length) {
		return (uint32_t)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (uint32_t)mem.ipl[addr];
	}

	// IPL�C���[�W or SCSI����
	if (addr >= 0xfc0000) {
		// IPL�C���[�W��
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPL�C���[�W
			addr &= 0x1ffff;
			addr ^= 1;
			return (uint32_t)mem.ipl[addr];
		}
		// SCSI������(�͈̓`�F�b�N)
		if (addr < 0xfc2000) {
			// SCSI����
			addr &= 0x1fff;
			addr ^= 1;
			return (uint32_t)mem.scsi[addr];
		}
		// X68030 IPL�O����
		if (mem.now == X68030) {
			// X68030 IPL�O��
			addr &= 0x1ffff;
			addr ^= 1;
			return (uint32_t)mem.scsi[addr];
		}
		// SCSI�������f���ŁAROM�͈͊O
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (uint32_t)mem.cg[addr];
	}

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (uint32_t)mem.scsi[addr];
		}
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadOnly(addr);
		}
	}

	// �}�b�v����Ă��Ȃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g�쐬
//
//---------------------------------------------------------------------------
void FASTCALL Memory::MakeContext(int reset)
{
	ASSERT(this);

	// ���Z�b�g��
	if (reset) {
		// �G���A�Z�b�g�����Z�b�g(CPU::Reset����MakeContext���Ă΂�邽��)
		ASSERT(areaset);
		areaset->Reset();

		// ���Z�b�g��p�R���e�L�X�g($FF00000�`���A$0000000�`�Ɍ�����)
		pCPU->BeginProgramRegion(TRUE);
		pCPU->AddProgramRegion(0x0000, 0xffff, ((uint32_t)mem.ipl) + 0x10000);
		pCPU->EndProgramRegion();

		pCPU->BeginProgramRegion(FALSE);
		pCPU->AddProgramRegion(0x0000, 0xffff, ((uint32_t)mem.ipl) + 0x10000);
		pCPU->EndProgramRegion();

		// �f�[�^�͑S�Ė���
		pCPU->BeginDataRegion(FALSE, FALSE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  FALSE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(FALSE, FALSE, TRUE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  FALSE, TRUE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(FALSE, TRUE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  TRUE, FALSE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(FALSE, TRUE, TRUE);	pCPU->EndDataRegion();
		pCPU->BeginDataRegion(TRUE,  TRUE, TRUE);	pCPU->EndDataRegion();
	} else {
		{
			// �ʏ�R���e�L�X�g - �v���O����(User)
			int area = areaset->GetArea();
			pCPU->BeginProgramRegion(FALSE);	// Program region (User)
			pCPU->AddProgramRegion((area + 1) << 13, mem.length - 1, (unsigned int)mem.ram);
			pCPU->EndProgramRegion();
		}

		{
			// �ʏ�R���e�L�X�g - �v���O����(Super)
			pCPU->BeginProgramRegion(TRUE);	// Program region (Super)
			pCPU->AddProgramRegion(0, mem.length - 1, (unsigned int)mem.ram);
			pCPU->AddProgramRegion(0xfe0000, 0xffffff, ((unsigned int)mem.ipl) - 0xfe0000);	// IPL

			// SCSI�O�t
			if (mem.now == SCSIExt) {
				pCPU->AddProgramRegion(0xea0000, 0xea1fff, ((unsigned int)mem.scsi) - 0xea0000);
			}

			// IPL�C���[�W or SCSI����
			if ((mem.now == SASI) || (mem.now == SCSIExt)) {
				pCPU->AddProgramRegion(0xfc0000, 0xfdffff, ((unsigned int)mem.ipl) - 0xfc0000);	//	IPL Image
			} else {
				// SCSI����
				if(mem.now != X68030) {
					pCPU->AddProgramRegion(0xfc0000, 0xfc1fff, ((unsigned int)mem.scsi) - 0xfc0000);	// SCSI Internal
				} else {
					pCPU->AddProgramRegion(0xfc0000, 0xfdffff, ((unsigned int)mem.scsi) - 0xfc0000);	// X68030 first half
				}
			}

			// �O���t�B�b�NVRAM
			GVRAM *gvram = (GVRAM*)vm->SearchDevice(XM6_MAKEID('G', 'V', 'R', 'M'));
			ASSERT(gvram);
			pCPU->AddProgramRegion(0xc00000, 0xdfffff, ((unsigned int)gvram->GetGVRAM()) - 0xc00000);

			// �e�L�X�gVRAM
			TVRAM* tvram = (TVRAM*)vm->SearchDevice(XM6_MAKEID('T', 'V', 'R', 'M'));
			ASSERT(tvram);
			pCPU->AddProgramRegion(0xe00000, 0xe7ffff, ((unsigned int)tvram->GetTVRAM()) - 0xe00000);

			// SRAM
			ASSERT(sram);
			pCPU->AddProgramRegion(0xed0000, 0xed0000 + (sram->GetSize() << 10) - 1, ((unsigned int)sram->GetSRAM()) - 0xed0000);
			pCPU->EndProgramRegion();
		}

		{
			// �ʏ�R���e�L�X�g - �ǂݏo��(User)
			int area = areaset->GetArea();

			pCPU->BeginDataRegion(FALSE, FALSE, FALSE);		// User, Read, Byte
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ���[�U�A�N�Z�X�\���
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::ReadErrC, NULL);			// �X�[�p�o�C�U���
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::ReadErrC, NULL);			// ���C����������������ԁ{�X�[�p�[�o�C�UI/O���
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::ReadByteC, NULL);					// ���[�UI/O���($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::ReadErrC, NULL);					// �X�[�p�o�C�U���(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();

			pCPU->BeginDataRegion(FALSE, FALSE, TRUE);		// User, Read, Word
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ���[�U�A�N�Z�X�\���
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::ReadErrC, NULL);			// �X�[�p�o�C�U���
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::ReadErrC, NULL);			// ���C����������������ԁ{�X�[�p�[�o�C�UI/O���
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::ReadWordC, NULL);					// ���[�UI/O���($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::ReadErrC, NULL);					// �X�[�p�o�C�U���(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();

			pCPU->BeginDataRegion(FALSE, TRUE, FALSE);		// User, Write, Byte
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ���[�U�A�N�Z�X�\���
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::WriteErrC, NULL);			// �X�[�p�o�C�U���
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::WriteErrC, NULL);			// ���C����������������ԁ{�X�[�p�[�o�C�UI/O���
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::WriteByteC, NULL);				// ���[�UI/O���($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::WriteErrC, NULL);					// �X�[�p�o�C�U���(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();

			pCPU->BeginDataRegion(FALSE, TRUE, TRUE);		// User, Write, Word
			pCPU->AddDataRegion((area + 1) << 13, mem.length - 1, NULL, (void*)&mem.ram[(area + 1) << 13]);	// ���[�U�A�N�Z�X�\���
			pCPU->AddDataRegion(0, ((area + 1) << 13) - 1, ::WriteErrC, NULL);			// �X�[�p�o�C�U���
			pCPU->AddDataRegion((mem.size << 20), 0xebffff, ::WriteErrC, NULL);			// ���C����������������ԁ{�X�[�p�[�o�C�UI/O���
			pCPU->AddDataRegion(0xec0000, 0xecffff, ::WriteWordC, NULL);				// ���[�UI/O���($EC0000-$ECFFFF)
			pCPU->AddDataRegion(0xed0000, 0xffffff, ::WriteErrC, NULL);					// �X�[�p�o�C�U���(SRAM,CG,IPL,SCSI)
			pCPU->EndDataRegion();
		}

		{
			// �ʏ�R���e�L�X�g - �ǂݏo��(Super)
			for(int isWord = 0; isWord < 2; ++isWord) {
				pCPU->BeginDataRegion(TRUE, FALSE, (int) isWord);		// Super, Read, {Byte|Word}
				pCPU->AddDataRegion(0, mem.length - 1, NULL, (void*)mem.ram);
				pCPU->AddDataRegion(0xf00000, 0xfbffff, NULL, (void*)mem.cg);			// CG
				pCPU->AddDataRegion(0xfe0000, 0xffffff, NULL, (void*)mem.ipl);			// IPL

				// SCSI�O�t
				if (mem.now == SCSIExt) {
					pCPU->AddDataRegion(0xea0020, 0xea1fff, NULL, (void*)(&mem.scsi[0x20]));
				}

				// IPL�C���[�W or SCSI����
				if ((mem.now == SASI) || (mem.now == SCSIExt)) {
					// IPL�C���[�W
					pCPU->AddDataRegion(0xfc0000, 0xfdffff, NULL, (void*)mem.ipl);
				} else {
					// SCSI����
					if (mem.now != X68030) {
						pCPU->AddDataRegion(0xfc0000, 0xfc1fff, NULL, (void*)mem.scsi);
					} else {
						// X68030 IPL�O��
						pCPU->AddDataRegion(0xfc0000, 0xfdffff, NULL, (void*)mem.scsi);
					}
				}

				// ����ȊO(�O���R�[��)
				if(!isWord) {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::ReadByteC, NULL);
				} else {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::ReadWordC, NULL);
				}
				pCPU->EndDataRegion();
			}
		}

		{
			// �ʏ�R���e�L�X�g - ��������(Super)
			for(int isWord = 0; isWord < 2; ++isWord) {
				pCPU->BeginDataRegion(TRUE, TRUE, (int) isWord);		// Super, Write, {Byte|Word}

				pCPU->AddDataRegion(0, mem.length - 1, NULL, (void*)mem.ram);

				// ����ȊO(�O���R�[��)
				if(!isWord) {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::WriteByteC, NULL);
				} else {
					pCPU->AddDataRegion((mem.size << 20), 0xefffff, ::WriteWordC, NULL);
				}
				pCPU->EndDataRegion();
			}
		}

		// cpu->Release��Y�ꂸ��
		cpu->Release();
	}
}

//---------------------------------------------------------------------------
//
//	IPL�o�[�W�����`�F�b�N
//	��IPL��version1.00(87/05/07)�ł��邩�ۂ����`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL Memory::CheckIPL() const
{
	ASSERT(this);
	ASSERT(mem.now != None);

	// ���݃`�F�b�N
	if (!mem.ipl) {
		return FALSE;
	}

	// SASI�^�C�v�̏ꍇ�̂݃`�F�b�N����
	if (mem.now != SASI) {
		return TRUE;
	}

	// ���t(BCD)���`�F�b�N
	if (mem.ipl[0x1000a] != 0x87) {
		return FALSE;
	}
	if (mem.ipl[0x1000c] != 0x07) {
		return FALSE;
	}
	if (mem.ipl[0x1000d] != 0x05) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CG�`�F�b�N
//	��8x8�h�b�g�t�H���g(�S�@�틤��)��Sum,Xor�Ń`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL Memory::CheckCG() const
{
	uint8_t add;
	uint8_t eor;
	uint8_t *ptr;
	int i;

	ASSERT(this);
	ASSERT(mem.now != None);

	// ���݃`�F�b�N
	if (!mem.cg) {
		return FALSE;
	}

	// �����ݒ�
	add = 0;
	eor = 0;
	ptr = &mem.cg[0x3a800];

	// ADD, XOR���[�v
	for (i=0; i<0x1000; i++) {
		add = (uint8_t)(add + *ptr);
		eor ^= *ptr;
		ptr++;
	}

	// �`�F�b�N(XVI�ł̎����l)
	if ((add != 0xec) || (eor != 0x84)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CG�擾
//
//---------------------------------------------------------------------------
const uint8_t* FASTCALL Memory::GetCG() const
{
	ASSERT(this);
	ASSERT(mem.cg);

	return mem.cg;
}

//---------------------------------------------------------------------------
//
//	SCSI�擾
//
//---------------------------------------------------------------------------
const uint8_t* FASTCALL Memory::GetSCSI() const
{
	ASSERT(this);
	ASSERT(mem.scsi);

	return mem.scsi;
}

//---------------------------------------------------------------------------
//
//	IPL�擾
//
//---------------------------------------------------------------------------
const uint8_t* FASTCALL Memory::GetIPL() const
{
	ASSERT(this);
	ASSERT(mem.ipl);

	return mem.ipl;
}

extern "C" unsigned int MemDecodeTable[384] = { 0 };								// �������f�R�[�h�e�[�u��
extern "C" unsigned int EventTable[32] = { 0 };									
extern "C" unsigned int MemoryPtr =  0;										
extern "C" unsigned int EventNum = 0;										
/*
;
; �f�[�^�G���A (8KB�P��)
;
; 0	MEMORY
; 1	GVRAM
; 2	TVRAM
; 3	CRTC
; 4	VC
; 5	DMAC
; 6	AREA
; 7	MFP
; 8	RTC
; 9	PRN
; 10	SYSPORT
; 11	OPM
; 12	ADPCM
; 13	FDC
; 14	SASI
; 15	SCC
; 16	PPI
; 17	IOSC
; 18	WINDRV
; 19	SCSI
; 20	MIDI
; 21	SPR
; 22	MERCURY
; 23	NEPTUNE
; 24	SRAM
;
*/
extern "C" uint32_t MemDecodeData[] = {
// $C00000 (GVRAM)
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
// $E00000 (TVRAM)
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
			2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
// $E80000 (CRTC - IOSC)
			3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,
// $E9E000 (WINDRV)
			18,
// $EA0000 (SCSI)
			19,
// $EA2000 (RESERVE)
			0,0,0,0,0,0,
// $EAE000 (MIDI)
			20,
// $EB0000 (SPRITE)
			21,21,21,21,21,21,21,21,
// $EC0000 (USER)
			0,0,0,0,0,0,
// $ECC000 (MERCURY)
			22,
// $ECE000 (NEPTUNE)
			23,
// $ED0000 (SRAM)
			24,24,24,24,24,24,24,24,
// $EE0000 (RESERVE)
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// �������f�R�[�_������
extern "C" void MemInitDecode(Memory *mem, MemDevice* list[]) {
	unsigned int* ebx = (unsigned int*) &MemDecodeTable[0];
	const unsigned int* edx = (unsigned int*) &MemDecodeData[0];

	MemoryPtr = (unsigned int) mem;

	for(int i = 0; i < 384; ++i) {
		unsigned int eax = *edx;
		edx += 1;
		unsigned int edi = (unsigned int) list[eax];

		*ebx = edi;
		ebx += 1;
	}
}


// �o�C�g�ǂݍ���
//	; EAX	uint32_t�߂�l
//	; ECX	this
//	; EDX	�A�h���X

#pragma runtime_checks("scu", off)
extern "C" __declspec(naked) void ReadByteC(uint32_t addr) {
	static unsigned int k = 0;
	__asm {
		mov k, edx
	}

	if(k >= 0x00c00000) {
		((MemDevice*)MemDecodeTable[(k-0x00c00000)>>13])->ReadByte(k);
	} else {
		((MemDevice*)MemoryPtr)->ReadByte(k);
	}

	__asm {
		ret
	}
}

// ���[�h�ǂݍ���
//	; EAX	uint32_t�߂�l
//	; ECX	this
//	; EDX	�A�h���X
extern "C" __declspec(naked) void ReadWordC(uint32_t addr) {
	static unsigned int k = 0;
	__asm {
		mov k, edx
	}

	if(k >= 0x00c00000) {
		((MemDevice*)MemDecodeTable[(k-0x00c00000)>>13])->ReadWord(k);
	} else {
		((MemDevice*)MemoryPtr)->ReadWord(k);
	}

	__asm {
		ret
	}
}

//	�o�C�g��������
//	;
//	; EBX	�f�[�^
//	; ECX	this
//	; EDX	�A�h���X
extern "C" __declspec(naked) void WriteByteC(uint32_t addr, uint32_t data) {
	static unsigned int k = 0;
	static unsigned int j = 0;
	__asm {
		mov k, edx
		mov j, ebx
	}

	if(k >= 0x00c00000) {
		((MemDevice*)MemDecodeTable[(k-0x00c00000)>>13])->WriteByte(k, j);
	} else {
		((MemDevice*)MemoryPtr)->WriteByte(k, j);
	}

	__asm {
		ret
	}
}

//	���[�h��������
//	;
//	; EBX	�f�[�^
//	; ECX	this
//	; EDX	�A�h���X
extern "C" __declspec(naked) void WriteWordC(uint32_t addr, uint32_t data) {
	static unsigned int k = 0;
	static unsigned int j = 0;
	__asm {
		mov k, edx
		mov j, ebx
	}

	if(k >= 0x00c00000) {
		((MemDevice*)MemDecodeTable[(k-0x00c00000)>>13])->WriteWord(k, j);
	} else {
		((MemDevice*)MemoryPtr)->WriteWord(k, j);
	}

	__asm {
		ret
	}
}

//	�o�X�G���[�ǂݍ���
//	; EDX	�A�h���X
extern "C" __declspec(naked) void ReadErrC(uint32_t addr) {
	static unsigned int k = 0;
	__asm {
		mov k, edx
	}

	ReadBusErr(k);

	__asm {
		ret
	}
}

// �o�X�G���[��������
//	; EBX	�f�[�^
//	; EDX	�A�h���X
extern "C" __declspec(naked) void WriteErrC(uint32_t addr, uint32_t data) {
	static unsigned int k = 0;
	__asm {
		mov k, edx
	}

	WriteBusErr(k);

	__asm {
		ret
	}
}
#pragma runtime_checks("scu", restore)


// �C�x���g�Q �w��
extern "C" void NotifyEvent(Event *first) {
	Event* esi = first;
	unsigned int ecx = 0;
	unsigned int* edi = &EventTable[0];

	while(esi != 0) {
		*edi = (unsigned int) esi;
		ecx += 1;

		esi = esi->GetNextEvent();
		edi += 1;
	}
	EventNum = ecx;
}

// �C�x���g�Q �ŏ��̂��̂�T��
extern "C" uint32_t GetMinEvent(uint32_t hus) {
	unsigned int eax = hus;

	const unsigned int* esi = &EventTable[0];
	for(unsigned int i = 0, n = EventNum; i < n; ++i) {
		unsigned int edi = *esi;
		esi += 1;
		unsigned int edx = * (unsigned int*) (edi + 4);
		if(edx == 0) {
			edx = eax;
		}
		if(edx < eax) {
			eax = edx;
		}
	}

	return eax;
}

// �C�x���g�Q ���Z�����s
extern "C" int SubExecEvent(uint32_t hus) {
	unsigned int edi = hus;
	const unsigned int* esi = &EventTable[0];

	for(unsigned i = 0, n = EventNum; i < n; ++i) {
	//	loop:
		unsigned int ebp = *esi;
		esi += 1;
		unsigned int eax = * (unsigned int*) (ebp + 8);		// Event::event_t.time
		if(eax != 0) {
			* (int*) (ebp+4) -= edi;		// Event::event_t.remain
			if(*(int*) (ebp+4) <= 0) {		// Event::event_t.remain
				// exec
				*(int*)(ebp+4) = (int) (eax);	// 	Event::event_t.remain

				Event*	pe = (Event*) ebp;
				Device* pd = pe->GetDevice();

				unsigned int eax = pd->Callback(pe);
				if(eax == 0) {
					// ; ������(time�����remain��0�N���A)
					// .disable:
					* (unsigned int*) (ebp+8) = 0;		// Event::event_t.time
					* (unsigned int*) (ebp+4) = 0;		// Event::event_t.remain
				}
			}
		}
	//	next:
	}

	return FALSE;
}
