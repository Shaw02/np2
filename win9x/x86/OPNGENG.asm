
	.586
	.model	flat,stdcall

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

	extern	_opngen	:opngen_t
	extern	_opnch	:ch_t			;FM音源の構造体
	extern	_opncfg	:opncfg_t		

	extern	_sinshift:byte
	extern	_envshift:byte

;ENVCURVE	equ	(_opncfg + opncfg_t.envcurve)
;SINTABLE	equ	(_opncfg + opncfg_t.sintable)
;ENVTABLE	equ	(_opncfg + opncfg_t.envtable)

;---------------
;プロトタイプ宣言

@opngen_getpcm@12	proto	near,	ptStream:DWORD
@opngen_getpcmvr@12	proto	near,	ptStream:DWORD

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
	mov	cl, [_envshift + edx]			;(v)
	mov	eax, [_opncfg + eax*4].sintable		;(u)
	add	cl, [_sinshift + eax]			;(v)
	imul	eax, [_opncfg + edx*4].envtable		;(np) ペアリング不可
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
	.if	( eax <= dword ptr (slot_t ptr [edi]).env_end )
	   mov	dl, (slot_t ptr [edi]).env_mode
	   .if		(dl == 4)			;AR(4)
		mov	(slot_t ptr [edi]).env_mode, EM_DECAY1	;(u)
		mov	eax, (slot_t ptr [edi]).decaylevel	;(v)
		mov	edx, (slot_t ptr [edi]).env_inc_decay1	;(u)
		mov	(slot_t ptr [edi]).env_end, eax		;(v)
		mov	(slot_t ptr [edi]).env_inc, edx		;(u)
		mov	eax, EC_DECAY				;(v)
	   .elseif	(dl == 3)			;DR(3)
		mov	(slot_t ptr [edi]).env_mode, EM_DECAY2	;(u)
		mov	edx, (slot_t ptr [edi]).env_inc_decay2	;(v)
		mov	(slot_t ptr [edi]).env_end, EC_OFF	;(u)
		mov	(slot_t ptr [edi]).env_inc, edx		;(v)
		mov	eax, (slot_t ptr [edi]).decaylevel	;(u)
	   .else					;SR(2) & RR(1) % OFF(0)
		.if	(dl == 1)
		  mov	(slot_t ptr [edi]).env_mode, EM_OFF	;(u)
		.endif
		and	(ch_t ptr [esi]).playing, p3		;(v)
		xor	eax,eax					;(u)	EC_ATTACK
		mov	(slot_t ptr [edi]).env_end, EC_DECAY	;(v)
		mov	(slot_t ptr [edi]).env_inc, eax		;(u)	0
	   .endif
	.endif
	;音量計算
	mov	(slot_t ptr [edi]).env_cnt, eax			;(u)
	mov	edx, (slot_t ptr [edi]).totallevel		;(v)
	shr	eax, ENV_BITS					;(u)	eax >> ENV_BITS
	sub	edx, [_opncfg + eax*4].envcurve			;(u) (直ぐにeaxを使う為、ペアリング不可)
	endm							;でも、次は、jl命令(v-pipe)が来ている。
;===============================================================
;		
;---------------------------------------------------------------
;◆引数
;	Sint32	leng	
;		edx	
;◆返り値
;
;◆レジスタ
;	esi	chの構造体
;	edi	slotの構造体
;===============================================================
;				align	16
@opngen_getpcm@12	proc	near	public	uses ebx esi edi,
		OPN_LENG	:DWORD
	;Local変数
	local	OPN_SAMPL	:DWORD
	local	OPN_SAMPR	:DWORD
	local	ptStream	:DWORD

		cmp	OPN_LENG, 0
		je	og_noupdate
		cmp	[_opngen].playing, 0
		je	og_noupdate

;		push	ebx
;		push	esi
;		push	edi
;		push	ebp
;		sub		esp, 8

;OPN_SAMPL	equ	0			;Local
;OPN_SAMPR	equ	4			;Local
;OPN_LENG	equ	16 + 8 + 4		;引数

		mov	ptStream, edx
		mov	ebx, [_opngen].calcremain

		;.while	(OPN_LENG > 0)
og_fmout_st:	mov	eax, ebx
		imul	ebx, [_opngen].outdl
		mov	OPN_SAMPL, ebx
		mov	ebx, FMDIV_ENT
		sub	ebx, eax
		imul	eax, [_opngen].outdr
		mov	OPN_SAMPR, eax

		;.repeat
og_fmout_lp:	
		xor	eax,eax
		mov	[_opngen].calcremain, ebx
		mov	[_opngen].playing, eax
		mov	[_opngen].outdl, eax
		mov	[_opngen].outdc, eax
		mov	[_opngen].outdr, eax
		mov	ch, byte ptr [_opngen].playchannels

		lea	esi, [_opnch]
		;.while	(ch>0)
og_calcch_lp:	
		mov	cl, (ch_t ptr [esi]).outslot
		test	cl, (ch_t ptr [esi]).playing
		je	og_calcch_nt
		xor	eax,eax
		lea	edi, [esi]
		mov	[_opngen].feedback2, eax
		mov	[_opngen].feedback3, eax
		mov	[_opngen].feedback4, eax
		calcenv	1		; slot1 calculate
		jl	og_calcslot3
		mov	cl, (ch_t ptr [esi]).feedback
		test	cl, cl
		je	og_nofeed
		mov	eax, (ch_t ptr [esi]).op1fb	; with feedback
		mov	ebx, eax
		shr	eax, cl
		op_out
		mov	(ch_t ptr [esi]).op1fb, eax
		add	eax, ebx
		sar	eax, 1
		jmp	og_algchk
og_nofeed:	xor	eax, eax			; without feedback
		op_out
og_algchk:	cmp	(ch_t ptr [esi]).algorithm, 5
		jne	og_calcalg5
		mov	[_opngen].feedback2, eax	; case ALG == 5
		mov	[_opngen].feedback3, eax
		mov	[_opngen].feedback4, eax
		jmp	og_calcslot3
og_calcalg5:	mov	ebx, (ch_t ptr [esi]).connect1	; case ALG != 5
		add	[ebx], eax
og_calcslot3:	add	edi, sizeof(slot_t)		; slot3 calculate
		calcenv	2
		jl	og_calcslot2
		mov	eax, [_opngen].feedback2
		op_out
		mov	ebx, (ch_t ptr [esi]).connect2
		add	[ebx], eax
og_calcslot2:	add	edi, sizeof(slot_t)		; slot2 calculate
		calcenv	4
		jl	og_calcslot4
		mov	eax, [_opngen].feedback3
		op_out
		mov	ebx, (ch_t ptr [esi]).connect3
		add	[ebx], eax
og_calcslot4:	add	edi, sizeof(slot_t)		; slot4 calculate
		calcenv	8
		jl	og_calcsloted
		mov	eax, [_opngen].feedback4
		op_out
		mov	ebx, (ch_t ptr [esi]).connect4
		add	[ebx], eax
og_calcsloted:	inc	[_opngen].playing
og_calcch_nt:	add	esi, sizeof(ch_t)
		dec	ch
		jne	og_calcch_lp
		;.endw
		mov	eax, [_opngen].outdc
		add	[_opngen].outdl, eax
		add	[_opngen].outdr, eax
		sar	[_opngen].outdl, FMVOL_SFTBIT
		sar	[_opngen].outdr, FMVOL_SFTBIT
		mov	edx, [_opncfg].calc1024
		mov	ebx, [_opngen].calcremain
		mov	eax, ebx
		sub	ebx, edx
		jbe	og_nextsamp
		;.break	.if ( [_opngen].calcremain =< [_opncfg.calc1024] )
		mov	[_opngen].calcremain, ebx
		mov	eax, edx
		imul	eax, [_opngen].outdl
		add	OPN_SAMPL, eax
		imul	edx, [_opngen].outdr
		add	OPN_SAMPR, edx
		jmp	og_fmout_lp
		;.until	0

og_nextsamp:	neg	ebx
		mov	[_opngen].calcremain, ebx
		mov	ecx, eax
		imul	eax, [_opngen].outdl
		add	eax, OPN_SAMPL
		imul	[_opncfg].fmvol

		mov	esi,ptStream
		add	[esi], edx
		mov	eax, [_opngen].outdr
		imul	ecx
		add	eax, OPN_SAMPR
		imul	[_opncfg].fmvol
		add	[esi+4], edx
		add	ptStream, 8

		dec	OPN_LENG
		jne	og_fmout_st
		;.endw
;		add	esp, 8
;		pop	ebp
;		pop	edi
;		pop	esi
;		pop	ebx
og_noupdate:	ret

;		setenv	envcalc1, envret1, 1
;		setenv	envcalc2, envret2, 2
;		setenv	envcalc3, envret3, 4
;		setenv	envcalc4, envret4, 8

@opngen_getpcm@12	endp

;===============================================================
;		
;---------------------------------------------------------------
;	引数
;
;	返り値
;
;===============================================================
;				align	16
@opngen_getpcmvr@12	proc	near	public	uses	ebx esi edi,
		OPNV_LENG	:DWORD
	;Local変数
	local	OPNV_SAMPL	:DWORD
	local	OPNV_SAMPR	:DWORD
	local	ptStream	:DWORD


	;	cmp	[_opncfg].vr_en, 0
	;	je	@opngen_getpcm@12
		.if	([_opncfg].vr_en == 0)
			invoke	@opngen_getpcm@12	,OPNV_LENG
			jmp	ogv_noupdate
		.endif

		cmp	OPNV_LENG,0
		je	ogv_noupdate

;		push	ebx
;		push	esi
;		push	edi
;		push	ebp
;		sub		esp, 8
;
;OPNV_SAMPL	equ	0
;OPNV_SAMPR	equ	4
;OPNV_LENG	equ	16 + 8 + 4

		mov	ptStream, edx
		mov	ebx, [_opngen].calcremain
ogv_fmout_st:	mov	eax, ebx
		imul	ebx, [_opngen].outdl
		mov	OPNV_SAMPL, ebx
		mov	ebx, FMDIV_ENT
		sub	ebx, eax
		imul	eax, [_opngen].outdr
		mov	OPNV_SAMPR, eax
ogv_fmout_lp:	xor	eax, eax
		mov	[_opngen].calcremain, ebx
		mov	[_opngen].outdl, eax
		mov	[_opngen].outdc, eax
		mov	[_opngen].outdr, eax
		mov	ch, byte ptr [_opngen].playchannels
		lea	edi, [_opnch]
ogv_calcch_lp:	xor	eax, eax
		mov	[_opngen].feedback2, eax
		mov	[_opngen].feedback3, eax
		mov	[_opngen].feedback4, eax
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
		mov	[_opngen].feedback2, eax	; case ALG == 5
		mov	[_opngen].feedback3, eax
		mov	[_opngen].feedback4, eax
		jmp	ogv_calcslot3
ogv_calcalg5:	mov	ebx, (ch_t ptr [esi]).connect1	; case ALG != 5
		add	[ebx], eax
ogv_calcslot3:	add	edi, sizeof(slot_t)		; slot3 calculate
		calcenv	2
		jl	ogv_calcslot2
		mov	eax, [_opngen].feedback2
		op_out
		mov	ebx, (ch_t ptr [esi]).connect2
		add	[ebx], eax
ogv_calcslot2:	add	edi, sizeof(slot_t)		; slot2 calculate
		calcenv	4
		jl	ogv_calcslot4
		mov	eax, [_opngen].feedback3
		op_out
		mov	ebx, (ch_t ptr [esi]).connect3
		add	[ebx], eax
ogv_calcslot4:	add	edi, sizeof(slot_t)		; slot4 calculate
		calcenv	8
		jl	ogv_calcsloted
		mov	eax, [_opngen].feedback4
		op_out
		mov	ebx, (ch_t ptr [esi]).connect4
		add	[ebx], eax
ogv_calcsloted:	add	edi, (sizeof(ch_t) - (sizeof(slot_t) * 3))
		dec	ch
		jne	ogv_calcch_lp
		mov	eax, [_opngen].outdl
		mov	edx, [_opngen].outdc
		imul	eax, [_opncfg].vr_l
		mov	ebx, edx
		sar	eax, 5
		add	ebx, eax
		sar	eax, 2
		add	edx, eax
		mov	eax, [_opngen].outdr
		imul	eax, [_opncfg].vr_r
		sar	eax, 5
		add	edx, eax
		sar	eax, 2
		add	ebx, eax
		add	[_opngen].outdl, edx
		add	[_opngen].outdr, ebx
		sar	[_opngen].outdl, FMVOL_SFTBIT
		sar	[_opngen].outdr, FMVOL_SFTBIT
		mov	edx, [_opncfg].calc1024
		mov	ebx, [_opngen].calcremain
		mov	eax, ebx
		sub	ebx, edx
		jbe	ogv_nextsamp
		mov	[_opngen].calcremain, ebx
		mov	eax, edx
		imul	eax, [_opngen].outdl
		add	OPNV_SAMPL, eax
		imul	edx, [_opngen].outdr
		add	OPNV_SAMPR, edx
		jmp	ogv_fmout_lp
ogv_nextsamp:	neg	ebx
		mov	[_opngen].calcremain, ebx
		mov	ecx, eax
		imul	eax, [_opngen].outdl
		add	eax, OPNV_SAMPL
		imul	[_opncfg].fmvol

		mov	esi,ptStream
		add	[esi], edx
		mov	eax, [_opngen].outdr
		imul	ecx
		add	eax, OPNV_SAMPR
		imul	[_opncfg].fmvol
		add	[esi+4], edx
		add	ptStream, 8
		dec	OPNV_LENG
		jne	ogv_fmout_st

;		add	esp, 8
;		pop	ebp
;		pop	edi
;		pop	esi
;		pop	ebx
ogv_noupdate:	ret

;		setenv	vrenvcalc1, vrenvret1, 1
;		setenv	vrenvcalc2, vrenvret2, 2
;		setenv	vrenvcalc3, vrenvret3, 4
;		setenv	vrenvcalc4, vrenvret4, 8

@opngen_getpcmvr@12	endp
	end
