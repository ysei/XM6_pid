XM6_pid is X68000 emulator, based on XM6 version 2.05
=====================================================


What is it?
===========

XM6_pid (XM6 Platform Independent Development) is experimental project
to achieve XM6 source code as platform independent.


How to build (Windows)
======================
 - Install Visual Studio 2010 Express Edition
 - Install Latest DirectX SDK
 - Open 00proj.vc10_nomfc/XM6.sln
 - Do "build"


How to run
==========
See origian XM6 document for the detail.

 - You must put CGROM.DAT, IPLROM.DAT in the same directory with xm6.exe.
 - nomfc version has no FDD UI, so you have to write your fdd path in source directly.
   - Find ".xdf" or ".dim" in mfc/mfc_nomfc.cpp and change it as your disk image filename.


TODO
====

 - Reconstruct some sub-system.
   - vm/log
   - assert()
   - vm/cpudebug (Will be obsolete, maybe)
   - vm/rend_asm.asm (Rewrite in C++)
 - Change M68k emulator Starstream (x86 asm) to Musashi (ANSI C).
   - Slower, but more platform independent.
 - Build "minimal" version
   - Use only few libc function, no loading, no input, only pufc output.
 - Build "SDL" version
   - Use SDL and libc functions, have media loading, input, sound output, graphics output.


LICENSE
=======
 - Follow the original license. See "使用規定（ライセンス）" in XM6src.txt.
 - I (Takayuki Matsuoka) do not claim my copyright or any rights for my modifications.
