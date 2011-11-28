//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ���z�}�V���R�A �A�Z���u���T�u ]
//
//---------------------------------------------------------------------------

#if !defined (core_asm_h)
#define core_asm_h

//#if _MSC_VER >= 1200

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	�v���g�^�C�v�錾
//
//---------------------------------------------------------------------------
void MemInitDecode(Memory *mem, MemDevice* list[]);
										// �������f�R�[�_������
void ReadByteC(uint32_t addr);
										// �o�C�g�ǂݍ���
void ReadWordC(uint32_t addr);
										// ���[�h�ǂݍ���
void WriteByteC(uint32_t addr, uint32_t data);
										// �o�C�g��������
void WriteWordC(uint32_t addr, uint32_t data);
										// ���[�h��������
void ReadErrC(uint32_t addr);
										// �o�X�G���[�ǂݍ���
void WriteErrC(uint32_t addr, uint32_t data);
										// �o�X�G���[��������
void NotifyEvent(Event *first);
										// �C�x���g�Q �w��
uint32_t GetMinEvent(uint32_t hus);
										// �C�x���g�Q �ŏ��̂��̂�T��
int SubExecEvent(uint32_t hus);
										// �C�x���g�Q ���Z�����s
extern unsigned int MemDecodeTable[];
										// �������f�R�[�h�e�[�u��

#if defined(__cplusplus)
}
#endif	//__cplusplus

//#endif	// _MSC_VER
#endif	// mem_asm_h
