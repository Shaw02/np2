;=======================================================================
;	OPN(A)	Process
;
;	this file written for Microsoft Macro Assembler version 9.00
;			and Microsoft Visual C++.net 2008
;
;		re-maked by S.W.
;			2009.11.21	ちょっとだけpipe-lineの最適化
;
;=======================================================================
	.586p
	.model	flat,c

;---------------
;定数定義
FMDIV_BITS		equ		8
FMDIV_ENT		equ		(1 shl FMDIV_BITS)
FMVOL_SFTBIT		equ		4

SIN_BITS		equ		11
EVC_BITS		equ		10
ENV_BITS		equ		16
KF_BITS			equ		6
FREQ_BITS		equ		20
ENVTBL_BIT		equ		14
SINTBL_BIT		equ		14

TL_BITS			equ		(FREQ_BITS+2)
OPM_OUTSB		equ		(TL_BITS + 2 - 16)

SIN_ENT			equ		(1 shl SIN_BITS)
EVC_ENT			equ		(1 shl EVC_BITS)

EC_ATTACK		equ		0
EC_DECAY		equ		(EVC_ENT shl ENV_BITS)
EC_OFF			equ		((2 * EVC_ENT) shl ENV_BITS)

EM_ATTACK		equ		4
EM_DECAY1		equ		3
EM_DECAY2		equ		2
EM_RELEASE		equ		1
EM_OFF			equ		0


;---------------
;構造体定義
slot_t			struc	
detune1			dd	?		; 00
totallevel		dd	?		; 04
decaylevel		dd	?		; 08
attack			dd	?		; 0c
decay1			dd	?		; 10
decay2			dd	?		; 14
release			dd	?		; 18
freq_cnt		dd	?		; 1c
freq_inc		dd	?		; 20
multiple		dd	?		; 24
keyscale		db	?		; 28
env_mode		db	?		; 29
envraito		db	?		; 2a
ssgeg1			db	?		; 2b
env_cnt			dd	?		; 2c
env_end			dd	?		; 30
env_inc			dd	?		; 34
env_inc_attack		dd	?		; 38
env_inc_decay1		dd	?		; 3c
env_inc_decay2		dd	?		; 40
env_inc_rel		dd	?		; 44
slot_t			ends

ch_t			struc	
slot			db	(sizeof(slot_t) * 4)	dup(?)
algorithm		db	?
feedback		db	?
playing			db	?
outslot			db	?
op1fb			dd	?
connect1		dd	?
connect3		dd	?
connect2		dd	?
connect4		dd	?
keynote			dd	4	dup(?)
keyfunc			db	4	dup(?)
kcode			db	4	dup(?)
pan			db	?
extop			db	?
stereo			db	2	dup(?)
ch_t			ends

opngen_t		struc 
playchannels		dd	?
playing			dd	?
feedback2		dd	?
feedback3		dd	?
feedback4		dd	?
outdl			dd	?
outdc			dd	?
outdr			dd	?
calcremain		dd	?
keyreg			db	12	dup(?)
opngen_t		ends

opncfg_t		struc 
calc1024		dd	?
fmvol			dd	?
ratebit			dd	?
vr_en			dd	?
vr_l			dd	?
vr_r			dd	?
sintable		dd	SIN_ENT		dup(?)
envtable		dd	EVC_ENT		dup(?)
envcurve		dd	(EVC_ENT*2 + 1)	dup(?)
opncfg_t		ends

;---------------
;外部宣言

	extern	C	opngen	:opngen_t
	extern	C	opnch	:ch_t			;FM音源の構造体
	extern	C	opncfg	:opncfg_t		

	extern	C	sinshift:byte
	extern	C	envshift:byte

;ENVCURVE	equ	(opncfg + opncfg_t.envcurve)
;SINTABLE	equ	(opncfg + opncfg_t.sintable)
;ENVTABLE	equ	(opncfg + opncfg_t.envtable)

;---------------
;プロトタイプ宣言

@opngen_getpcm@12	proto	near	syscall,	ptStream:DWORD
@opngen_getpcmvr@12	proto	near	syscall,	ptStream:DWORD

.code



;===============================================================
;		オペレータ
;---------------------------------------------------------------
;◆引数
;	eax	スロットへの入力
;	edx	音量（TL込み）
;	edi	スロットの構造体
;◆返り値
;	eax	スロットの出力
;◆レジスタ
;	esi	ch
;	edi	ch->slot
;===============================================================
op_out	macro
	add	eax, (slot_t ptr [edi]).freq_cnt	;(u)
	shr	eax, (FREQ_BITS - SIN_BITS)		;(u) shrはu-pipe限定
	and	eax, (SIN_ENT - 1)			;(u)
	mov	cl, [sinshift + eax]			;(v)
	mov	eax, [opncfg + eax*4].sintable		;(u)
	add	cl, [envshift + edx]			;(v)
	imul	eax, [opncfg + edx*4].envtable		;(np) ペアリング不可
	sar	eax, cl					;(np) ペアリング不可
	endm
;===============================================================
;		エンベロープ計算
;---------------------------------------------------------------
;◆引数
;	edi	スロットの構造体
;◆返り値
;	edx	音量（TL込み）
;◆レジスタ
;	esi	ch
;	edi	ch->slot
;===============================================================
calcenv	macro	p3
							;()内は、CPU(586以降)のpipe。
	;Freqency & Envlop
	mov	eax, (slot_t ptr [edi]).env_cnt		;(u)
	mov	edx, (slot_t ptr [edi]).freq_inc	;(v)
	add	eax, (slot_t ptr [edi]).env_inc		;(u)
	add	(slot_t ptr [edi]).freq_cnt, edx	;(v)

	;フェーズ チェンジ
	.if	( eax >= dword ptr (slot_t ptr [edi]).env_end )
	   mov	dl, (slot_t ptr [edi]).env_mode
	   .if		(dl == EM_ATTACK)		;AR(4)
		mov	(slot_t ptr [edi]).env_mode, EM_DECAY1	;(u)
		mov	eax, (slot_t ptr [edi]).decaylevel	;(v)
		mov	edx, (slot_t ptr [edi]).env_inc_decay1	;(u)
		mov	(slot_t ptr [edi]).env_end, eax		;(v)
		mov	(slot_t ptr [edi]).env_inc, edx		;(u)
		mov	eax, EC_DECAY				;(v)
	   .elseif	(dl == EM_DECAY1)		;DR(3)
		mov	(slot_t ptr [edi]).env_mode, EM_DECAY2	;(u)
		mov	edx, (slot_t ptr [edi]).env_inc_decay2	;(v)
		mov	(slot_t ptr [edi]).env_end, EC_OFF	;(u)
		mov	(slot_t ptr [edi]).env_inc, edx		;(v)
		mov	eax, (slot_t ptr [edi]).decaylevel	;(u)
	   .else					;SR(2) & RR(1) % OFF(0)
		.if	(dl == EM_RELEASE)
		  mov	(slot_t ptr [edi]).env_mode, EM_OFF	;(u)
		.endif
		and	(ch_t ptr [esi]).playing, (not p3)	;(v)
		xor	eax,eax					;(u)	EC_ATTACK
		mov	(slot_t ptr [edi]).env_end, EC_DECAY	;(v)
		mov	(slot_t ptr [edi]).env_inc, eax		;(u)	0
	   .endif
	.endif
	;音量計算
	mov	(slot_t ptr [edi]).env_cnt, eax			;(u)
	mov	edx, (slot_t ptr [edi]).totallevel		;(v)
	shr	eax, ENV_BITS					;(u)	eax >> ENV_BITS
	sub	edx, [opncfg + eax*4].envcurve			;(u) (直ぐにeaxを使う為、ペアリング不可)
	endm							;でも、次は、jl命令(v-pipe)が来ている。
;===============================================================
;		
;---------------------------------------------------------------
;◆引数
;	ecx	void	*hdl		
;	edx	SINT32	*buf		
;	[ebp-4]	UINT	OPN_LENG	count
;◆返り値
;
;◆レジスタ
;	esi	chの構造体
;	edi	slotの構造体
;===============================================================
;				align	16
@opngen_getpcm@12	proc	near	syscall	public	uses ebx esi edi,
	OPN_LENG:DWORD
	;Local変数
	local	OPN_SAMPL	:DWORD
	local	OPN_SAMPR	:DWORD

;	cmp	OPN_LENG, 0
;	je	og_noupdate
;	cmp	(opngen_t ptr [edi]).playing, 0
;	je	og_noupdate

	lea	edi, [opngen]

   .if	((dword ptr (opngen_t ptr [edi]).playing) != 0)

	lea	esi, [edx]			;*buf
	mov	ebx, (opngen_t ptr [edi]).calcremain

	.while	(OPN_LENG > 0)
	   mov	eax, ebx
	   imul	ebx, (opngen_t ptr [edi]).outdl
	   mov	OPN_SAMPL, ebx				;OPN_SAMPL = opngen.outdl * opngen.calcremain
	   mov	ebx, FMDIV_ENT
	   sub	ebx, eax
	   imul	eax, (opngen_t ptr [edi]).outdr
	   mov	OPN_SAMPR, eax				;OPN_SAMPL = opngen.outdl * opngen.calcremain
	   push	esi
	   .repeat
		xor	eax,eax
		lea	esi, [opnch]
		mov	(opngen_t ptr [edi]).calcremain, ebx
		mov	(opngen_t ptr [edi]).playing, eax
		mov	(opngen_t ptr [edi]).outdl, eax
		mov	(opngen_t ptr [edi]).outdc, eax
		mov	(opngen_t ptr [edi]).outdr, eax
		mov	ch, byte ptr ((opngen_t ptr [edi]).playchannels)
		push	edi
		.while	(ch>0)
		   mov	cl, (ch_t ptr [esi]).outslot
		   .if	(cl & (byte ptr (ch_t ptr [esi]).playing))
			xor	eax,eax
			lea	edi, [esi]
			mov	[opngen].feedback2, eax
			mov	[opngen].feedback3, eax
			mov	[opngen].feedback4, eax
			calcenv	1		; slot1 calculate
			jl	og_calcslot3
			mov	cl, (ch_t ptr [esi]).feedback
			.if	(cl == 0)
				xor	eax, eax			; without feedback
				op_out
			.else
				mov	eax, (ch_t ptr [esi]).op1fb	; with feedback
				mov	ebx, eax
				shr	eax, cl
				op_out
				mov	(ch_t ptr [esi]).op1fb, eax
				add	eax, ebx
				sar	eax, 1
			.endif
			.if	(byte ptr ((ch_t ptr [esi]).algorithm) == 5)
				mov	[opngen].feedback2, eax		; case ALG == 5
				mov	[opngen].feedback3, eax
				mov	[opngen].feedback4, eax
			.else
				mov	ebx, (ch_t ptr [esi]).connect1	; case ALG != 5
				add	[ebx], eax
			.endif
og_calcslot3:		
			add	edi, sizeof(slot_t)		; slot3 calculate
			calcenv	2
			jl	og_calcslot2
			mov	eax, [opngen].feedback2
			op_out
			mov	ebx, (ch_t ptr [esi]).connect2
			add	[ebx], eax
og_calcslot2:		add	edi, sizeof(slot_t)		; slot2 calculate
			calcenv	4
			jl	og_calcslot4
			mov	eax, [opngen].feedback3
			op_out
			mov	ebx, (ch_t ptr [esi]).connect3
			add	[ebx], eax
og_calcslot4:		add	edi, sizeof(slot_t)		; slot4 calculate
			calcenv	8
			jl	og_calcsloted
			mov	eax, [opngen].feedback4
			op_out
			mov	ebx, (ch_t ptr [esi]).connect4
			add	[ebx], eax
og_calcsloted:		inc	[opngen].playing
		   .endif
		   add	esi, sizeof(ch_t)
		   dec	ch
		.endw
		pop	edi
		mov	eax, (opngen_t ptr [edi]).outdc
		add	(opngen_t ptr [edi]).outdl, eax
		add	(opngen_t ptr [edi]).outdr, eax
		sar	(opngen_t ptr [edi]).outdl, FMVOL_SFTBIT
		sar	(opngen_t ptr [edi]).outdr, FMVOL_SFTBIT
		mov	edx, [opncfg].calc1024
		mov	ebx, (opngen_t ptr [edi]).calcremain
		mov	eax, ebx
		sub	ebx, edx
		jbe	og_nextsamp
		mov	(opngen_t ptr [edi]).calcremain, ebx
		mov	eax, edx
		imul	eax, (opngen_t ptr [edi]).outdl
		add	OPN_SAMPL, eax
		imul	edx, (opngen_t ptr [edi]).outdr
		add	OPN_SAMPR, edx
	  .until	0
og_nextsamp:
	   pop	esi
	   neg	ebx
	   mov	(opngen_t ptr [edi]).calcremain, ebx
	   mov	ecx, eax
	   imul	eax, (opngen_t ptr [edi]).outdl
	   add	eax, OPN_SAMPL
	   imul	[opncfg].fmvol

	   add	[esi], edx
	   mov	eax, (opngen_t ptr [edi]).outdr
	   imul	ecx
	   add	eax, OPN_SAMPR
	   imul	[opncfg].fmvol
	   add	[esi+4], edx
	   add	esi, 8
	   dec	OPN_LENG
	.endw

   .endif

og_noupdate:
	ret	4

@opngen_getpcm@12	endp

;===============================================================
;		
;---------------------------------------------------------------
;◆引数
;	ecx	void	*hdl		
;	edx	SINT32	*buf		
;	[ebp-4]	UINT	OPN_LENG	count
;◆返り値
;
;◆レジスタ
;	esi	chの構造体
;	edi	slotの構造体
;===============================================================
;				align	16
@opngen_getpcmvr@12	proc	near	syscall	public	uses	ebx esi edi,
	OPNV_LENG:DWORD
	;Local変数
	local	OPNV_SAMPL	:DWORD
	local	OPNV_SAMPR	:DWORD

	.if	([opncfg].vr_en == 0)
		invoke	@opngen_getpcm@12	,OPNV_LENG
		jmp	ogv_noupdate
	.endif

	cmp	OPNV_LENG,0
	je	ogv_noupdate

		mov	esi, edx
		mov	ebx, [opngen].calcremain

ogv_fmout_st:	mov	eax, ebx
		imul	ebx, [opngen].outdl
		mov	OPNV_SAMPL, ebx
		mov	ebx, FMDIV_ENT
		sub	ebx, eax
		imul	eax, [opngen].outdr
		mov	OPNV_SAMPR, eax
		push	esi

ogv_fmout_lp:	xor	eax, eax
		mov	[opngen].calcremain, ebx
		mov	[opngen].outdl, eax
		mov	[opngen].outdc, eax
		mov	[opngen].outdr, eax
		mov	ch, byte ptr [opngen].playchannels
		lea	edi, [opnch]
ogv_calcch_lp:	xor	eax, eax
		mov	[opngen].feedback2, eax
		mov	[opngen].feedback3, eax
		mov	[opngen].feedback4, eax
		lea	esi, ch_t ptr [edi]
		calcenv	1		; slot1 calculate
		jl	ogv_calcslot3
		mov	cl, (ch_t ptr [esi]).feedback
		test	cl, cl
		je	ogv_nofeed
		mov	eax, (ch_t ptr [esi]).op1fb	; with feedback
		mov	ebx, eax
		shr	eax, cl
		op_out
		mov	(ch_t ptr [esi]).op1fb, eax
		add	eax, ebx
		sar	eax, 1
		jmp	ogv_algchk
ogv_nofeed:	xor	eax, eax			; without feedback
		op_out
ogv_algchk:	cmp	(ch_t ptr [esi]).algorithm, 5
		jne	ogv_calcalg5
		mov	[opngen].feedback2, eax	; case ALG == 5
		mov	[opngen].feedback3, eax
		mov	[opngen].feedback4, eax
		jmp	ogv_calcslot3
ogv_calcalg5:	mov	ebx, (ch_t ptr [esi]).connect1	; case ALG != 5
		add	[ebx], eax
ogv_calcslot3:	add	edi, sizeof(slot_t)		; slot3 calculate
		calcenv	2
		jl	ogv_calcslot2
		mov	eax, [opngen].feedback2
		op_out
		mov	ebx, (ch_t ptr [esi]).connect2
		add	[ebx], eax
ogv_calcslot2:	add	edi, sizeof(slot_t)		; slot2 calculate
		calcenv	4
		jl	ogv_calcslot4
		mov	eax, [opngen].feedback3
		op_out
		mov	ebx, (ch_t ptr [esi]).connect3
		add	[ebx], eax
ogv_calcslot4:	add	edi, sizeof(slot_t)		; slot4 calculate
		calcenv	8
		jl	ogv_calcsloted
		mov	eax, [opngen].feedback4
		op_out
		mov	ebx, (ch_t ptr [esi]).connect4
		add	[ebx], eax
ogv_calcsloted:	add	edi, (sizeof(ch_t) - (sizeof(slot_t) * 3))
		dec	ch
		jne	ogv_calcch_lp

		mov	eax, [opngen].outdl
		mov	edx, [opngen].outdc
		imul	eax, [opncfg].vr_l
		mov	ebx, edx
		sar	eax, 5
		add	ebx, eax
		sar	eax, 2
		add	edx, eax
		mov	eax, [opngen].outdr
		imul	eax, [opncfg].vr_r
		sar	eax, 5
		add	edx, eax
		sar	eax, 2
		add	ebx, eax
		add	[opngen].outdl, edx
		add	[opngen].outdr, ebx
		sar	[opngen].outdl, FMVOL_SFTBIT
		sar	[opngen].outdr, FMVOL_SFTBIT
		mov	edx, [opncfg].calc1024
		mov	ebx, [opngen].calcremain
		mov	eax, ebx
		sub	ebx, edx
		jbe	ogv_nextsamp
		mov	[opngen].calcremain, ebx
		mov	eax, edx
		imul	eax, [opngen].outdl
		add	OPNV_SAMPL, eax
		imul	edx, [opngen].outdr
		add	OPNV_SAMPR, edx
		jmp	ogv_fmout_lp
ogv_nextsamp:	
		pop	esi
		neg	ebx
		mov	[opngen].calcremain, ebx
		mov	ecx, eax
		imul	eax, [opngen].outdl
		add	eax, OPNV_SAMPL
		imul	[opncfg].fmvol

		add	[esi], edx
		mov	eax, [opngen].outdr
		imul	ecx
		add	eax, OPNV_SAMPR
		imul	[opncfg].fmvol
		add	[esi+4], edx
		add	esi, 8
		dec	OPNV_LENG
		jne	ogv_fmout_st

ogv_noupdate:	ret	4

@opngen_getpcmvr@12	endp
	end
