;
; X68000 EMULATOR "XM6"
;
; Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
; [ MFC アセンブラサブ ]
;

;
; 外部宣言
;
		section	.text align=16
		bits	32

		global	_IsMMXSupport
		global	_IsCMOVSupport
		global	_SoundMMX
		global	_SoundEMMS

;
; MMXサポートチェック
;
; BOOL IsMMXSupport(void)
;
_IsMMXSupport:
		pushad
; CPUIDの有無をチェック
		pushfd
		pop	eax
		xor	eax,00200000h
		push	eax
		popfd
		pushfd
		pop	ebx
		cmp	eax,ebx
		jnz	.error
; CPUIDの機能フラグサポートをチェック
		xor	eax,eax
		cpuid
		cmp	eax,0
		jz	.error
; MMXテクノロジのサポートをチェック
		mov	eax,1
		cpuid
		and	edx,00800000h
		jz	.error
; MMXあり
		popad
		mov	eax,1
		ret
; MMXなし
.error:
		popad
		xor	eax,eax
		ret

;
; CMOVサポートチェック
;
; BOOL IsCMOVSupport(void)
;
_IsCMOVSupport:
		pushad
; CPUIDの有無をチェック
		pushfd
		pop	eax
		xor	eax,00200000h
		push	eax
		popfd
		pushfd
		pop	ebx
		cmp	eax,ebx
		jnz	.error
; CPUIDの機能フラグサポートをチェック
		xor	eax,eax
		cpuid
		cmp	eax,0
		jz	.error
; CMOVのサポートをチェック
		mov	eax,1
		cpuid
		and	edx,00008000h
		jz	.error
; CMOVあり
		popad
		mov	eax,1
		ret
; CMOVなし
.error:
		popad
		xor	eax,eax
		ret

;
; サウンドサンプルサイジング(MMX)
;
; void SoundMMX(DWORD *pSrc, WORD *pDst, int nBytes)
;
_SoundMMX:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; バッファ取得(pSrc, pDst)
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
; 回数取得(nBytes)
		mov	ecx,[ebp+16]
; long(128バイト単位)
		cmp	ecx,128
		jnc	.longchk
		jmp	.shortchk
; 回数セット
.longchk:
		mov	eax,ecx
		shr	eax,7
		and	ecx,byte 127
; ループ
.longlp:
		movq	mm0,[esi]
		movq	mm1,[esi+16]
		movq	mm2,[esi+32]
		movq	mm3,[esi+48]
		packssdw mm0,[esi+8]
		packssdw mm1,[esi+24]
		packssdw mm2,[esi+40]
		packssdw mm3,[esi+56]
		movq	[edi],mm0
		movq	[edi+8],mm1
		movq	[edi+16],mm2
		movq	[edi+24],mm3
;
		movq	mm0,[esi+64]
		movq	mm1,[esi+80]
		movq	mm2,[esi+96]
		movq	mm3,[esi+112]
		packssdw mm0,[esi+72]
		packssdw mm1,[esi+88]
		packssdw mm2,[esi+104]
		packssdw mm3,[esi+120]
		movq	[edi+32],mm0
		movq	[edi+40],mm1
		movq	[edi+48],mm2
		movq	[edi+56],mm3
;
		movq	mm0,[esi+128]
		movq	mm1,[esi+144]
		movq	mm2,[esi+160]
		movq	mm3,[esi+176]
		packssdw mm0,[esi+136]
		packssdw mm1,[esi+152]
		packssdw mm2,[esi+168]
		packssdw mm3,[esi+184]
		movq	[edi+64],mm0
		movq	[edi+72],mm1
		movq	[edi+80],mm2
		movq	[edi+88],mm3
;
		movq	mm0,[esi+192]
		movq	mm1,[esi+208]
		movq	mm2,[esi+224]
		movq	mm3,[esi+240]
		packssdw mm0,[esi+200]
		packssdw mm1,[esi+216]
		packssdw mm2,[esi+232]
		packssdw mm3,[esi+248]
		movq	[edi+96],mm0
		movq	[edi+104],mm1
		movq	[edi+112],mm2
		movq	[edi+120],mm3
; 次へ
		add	esi,256
		add	edi,128
		dec	eax
		jnz	.longlp
; short(16バイト単位)
.shortchk:
		cmp	ecx,byte 16
		jc	.normalchk
		mov	eax,ecx
		shr	eax,4
		and	ecx,byte 15
; ループ
.shortlp:
		movq	mm0,[esi]
		movq	mm1,[esi+16]
		packssdw mm0,[esi+8]
		packssdw mm1,[esi+24]
		movq	[edi],mm0
		movq	[edi+8],mm1
; 次へ
		add	esi,byte 32
		add	edi,byte 16
		dec	eax
		jnz	.shortlp
; normal(MMXを使用しない)
.normalchk:
		shr	ecx,1
		or	ecx,ecx
		jz	.exit
		mov	ebx,00007fffh
		mov	edx,0ffff8000h
; ループ(分岐するケースは少ないと判断。CMOVは使わない)
.normallp:
		mov	eax,[esi]
		cmp	eax,ebx
		jg	.over
		cmp	eax,edx
		jl	.under
; セットして次へ
.next:
		mov	[edi],ax
		add	esi,byte 4
		add	edi,byte 2
		dec	ecx
		jnz	.normallp
; 終了
.exit:
; EMMSは行わない(SoundEMMSに任せる)
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; オーバーフロー
.over:
		mov	eax,ebx
		jmp	.next
; アンダーフロー
.under:
		mov	eax,edx
		jmp	.next

;
; サウンドサンプルサイジング(EMMS)
;
; void SoundEMMS()
;
_SoundEMMS:
		emms
		ret
