/* MEMORY LAYOUT ASSUMPTIONS

The "chaff" area must be located at the start of Y scratch RAM, 0x20081000: see
the macro getchaffaddress.

The stack must be located at the end of Y scratch RAM: see the memory
wiping at the end of ctr_crypt_s where memory between the start of Y
scratch RAM and the stack pointer is overwritten.
*/

.syntax unified
.cpu cortex-m33
.thumb

#include "pico/aes.h"
#include "hardware/platform_defs.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/clocks.h"
#include "hardware/regs/sha256.h"
#include "hardware/regs/resets.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/trng.h"
#include "hardware/rcp.h"

#if PICO_AES_HARDENING
@                                             Number of calls to gen_rand_sha[_nonpres]
#define RND_COUNT_decrypt                     394                           // From pico_aes_decrypt_internal up to call to ctr_crypt_s
#define RND_COUNT_ctr_crypt_s_init            (17 + 32 * CT_BPERM)          // Init phase of ctr_crypt_s
#define RND_COUNT_ctr_crypt_mainloop_A        (4 + ST_VPERM + ST_SHAREC)
#define RND_COUNT_refreshchaff_and_lfsr       2
#define RND_COUNT_remap                       2
#define RND_COUNT_decryption_end              3
#endif

.global pico_aes_decrypt_internal
.global chaff

.extern pico_aes_lock_key

@ RCP macros

#define CTAG0  0x2a
#define CTAG1  0x2b
#define CTAG2  0x2c
#define CTAG3  0x2d
#define CTAG4  0x2e
#define CTAG5  0x30
#define CTAG6  0x31
#define CTAG7  0x32
#define CTAG8  0x33
#define CTAG9  0x34
#define CTAG10 0x35
#define CTAG11 0x36
#define CTAG12 0x37
#define CTAG13 0x38
#define CTAG14 0x39
#define CTAG15 0x3a
#define CTAG16 0x3b
#define CTAG17 0x3c
#define CTAG18 0x3d
#define CTAG19 0x3e
#define CTAG20 0x3f
#define CTAG21 0x29

@ number of blocks from the TRNG processed to initialise rstate_sha
#define TRNG_BLOCKS 25

@ if GEN_RAND_SHA==0 then we don't call the counting version
#if PICO_AES_HARDENING && GEN_RAND_SHA
.macro check_rnd_count count
.if !(\count & 0xffffff00)
movs r1, #\count
.else
ldr r1, =\count
.endif
movs r0, #(\count & 1) ^ 1
bl check_rnd_count_func
rcp_iequal_nodelay r1, r0
#if PICO_AES_DOUBLE_HARDENING
rcp_iequal_nodelay r0, r1
#endif
.endm

@ r1 has the expected count
@ Trashes r0
.macro check_rnd_count_dynamic
mov r0, sp
bl check_rnd_count_func
rcp_iequal_nodelay r1, r0
#if PICO_AES_DOUBLE_HARDENING
rcp_iequal_nodelay r0, r1
#endif
.endm

.macro reset_rnd_count
bl reset_rnd_count_func
.endm

.macro reset_rnd_count_checked
@ This version verifies that the count was actually reset
uxtb r0, r1
bl reset_rnd_count_func
ldr r0, [r0]
bics r1, #0xff00ff
rcp_iequal_nodelay r1, r0
.endm

#else
.macro check_rnd_count count
.endm
.macro reset_rnd_count
.endm
.macro reset_rnd_count_checked
.endm
#endif
@ The lower jitterpriorty is, the more the jitter
.macro SET_COUNT n,jitterpriority
.if RC_COUNT
.if RC_JITTER > \jitterpriority
 rcp_count_set \n + PICO_AES256_RCP_COUNT_DELTA
.else
 rcp_count_set_nodelay \n + PICO_AES256_RCP_COUNT_DELTA
.endif
.endif
.endm

.macro CHK_COUNT n,jitterpriority
.if RC_COUNT
.if RC_JITTER > \jitterpriority
 rcp_count_check \n + PICO_AES256_RCP_COUNT_DELTA
.else
 rcp_count_check_nodelay \n + PICO_AES256_RCP_COUNT_DELTA
.endif
.endif
.endm

.macro GET_CANARY rx,tag,jitterpriority
.if RC_CANARY
.if RC_JITTER > \jitterpriority
 rcp_canary_get \rx,\tag
.else
 rcp_canary_get_nodelay \rx,\tag
.endif
.endif
.endm

.macro CHK_CANARY rx,tag,jitterpriority
.if RC_CANARY
.if RC_JITTER > \jitterpriority
 rcp_canary_check \rx,\tag
.else
 rcp_canary_check_nodelay \rx,\tag
.endif
.endif
.endm

@ Clear internal stripe load registers, and r0-r3
@ 0 <= offset <= 32
.macro clear03 offset=0
 getchaffaddress r0,\offset
 ldmia r0,{r0-r3}
.endm

.macro clear03_preserve_r3 offset=0
 getchaffaddress r0,\offset
 ldmia r0!,{r1-r2}
 ldmia r0!,{r1-r2}
.endm

.macro clear01 offset=0
 getchaffaddress r0,\offset
 ldmia r0,{r0,r1}
.endm

@ Put workspace in the second scratch area
@ The "a"=allocatable attribute (and possibly the %progbits attribute) are necessary to store the murmur3 constants,
@ otherwise they may end up silently replaced with 0 or 0xffffffff
#if PICO_AES_SINGLE_USE
.section .scratch_y.aes,"aw",%progbits
#else
.section .aes_scratch_y.aes,"aw",%progbits
#endif

aes_workspace_start:

@ chaff has to be at the start of scratch_y = 0x20081000 because this is assumed by the following macro, getchaffaddress
@ We need to set the chaff address directly with MOVs, rather than setting it with a load as normal, because at the point
@ the macro is called we have just done a load of a sensitive value at a known memory offset mod 16, and the idea is that
@ the next load is going to be of a random number (in the "chaff" memory) at that same offset mod 16, so we can't afford
@ to do a ldr \rx, =0x20081000 + \offset first, as this will load a non-random value from an uncontrolled memory location mod 16.
@ Ideally we'd avoid the magic number 0x2008100 by using, ADR \rx, chaff+\offset, but the linker does not support this.
.macro getchaffaddress rx,offset=0
 mov \rx,#(0x1000+\offset)
 movt \rx,#0x2008
.endm
chaff:
.space 48

.balign 16
rkey_s:                      @ round key shares: 600 bytes = 15 rounds * 2 shares * (4+1) words
                             @ see comment at init_key_4way for description of layout and meaning of rkey_s
.space 600
rkey4way:                    @ scratch area for init_key_4way; could overlap this with other scratch space if need to save space
.space 128
.if CT_BPERM
bperm_rand:                  @ 32 half words that define the oblivious permutation of blocks
.space 64
.endif

.balign 16
permscratch:                 @ Must be 0 mod 16; 16 bytes of scratch space to store permutation(s)
perm16:
.space 16
@ Scratch space of 32 bytes used both by init_key_sbox and map_sbox_s
.balign 16
fourway:                     @ Must be 0 mod 16
shareA:                      @ 0 mod 16
.space 20                    @ Only need 16 bytes, but choosing shareB!=shareA mod 16
shareB:                      @ 4 mod 16
.space 20
shareC:                      @ 8 mod 16
.space 4
statevperm:                  @ 12 mod 16
.space 4                     @ vperm state rotation: only last two bits are operational; other bits random
RKshareC:                    @ Round key common share C; see comment at init_key_4way for explanation
.space 4
RKshareCchange:              @ Temporary used by ref_roundkey_shares_s
.space 4
IV0:                         @ 2-way share of IV for block 0
.space 36                    @ Considering IV0 as a word pointer, the format is IV = IV0[0,1,2,3] ^ (IV0[5,6,7,8],ror#16)
                             @ The gap at IV0[4] is to defeat unsharing by internal striped memory registers
                             @ I.e., there are implicit XORs IV0[0]^IV0[4], IV0[1]^IV0[5], ..., that the 1 word offset renders useless

@ Regardless of configuration, the code uses a single 256-entry LUT,
@ which is a simple S-box table.
@ The LUT is represented as two shares, lut_a and lut_b,
@ whose values must be EORed. Furthermore, the contents of each share are
@ scambled according to a 4-byte "map". The map comprises two bytes that
@ are EORed into the addressing of the share, and two bytes that are
@ EORed into the data read back from the share. Performing a lookup
@ of a value x involves computing
@ lut_a[x ^ a₀ ^ a₁] ^ c₀ ^ c₁ ^ lut_b[x ^ b₀ ^ b₁] ^ d₀ ^ d₁
@ where a₀, a₁, c₀ and c₁ are the "map" of the lut_a share and
@ b₀, b₁, d₀ and d₁ are the "map" of the lut_b share.
@ In practice the result of a lookup is itself represented in two
@ shares, namely
@ lut_a[x ^ a₀ ^ a₁] ^ c₀ ^ d₀  and
@ lut_b[x ^ b₀ ^ b₁] ^ c₁ ^ d₁
.balign 16
lut_a:                       @ LUT share A (must be 0 mod 16 so that init_key_sbox knows how to mask the lookup)
.byte 0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76
.byte 0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0
.byte 0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15
.byte 0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75
.byte 0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84
.byte 0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf
.byte 0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8
.byte 0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2
.byte 0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73
.byte 0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb
.byte 0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79
.byte 0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08
.byte 0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a
.byte 0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e
.byte 0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf
.byte 0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
lut_a_map:                   @ the current scrambling of lut_a; not particularly secret since it can be deduced from the contents of lut_a and lut_b
.space 4
.space 4                     @ align to 8 mod 16
lut_b:                       @ LUT share B (must be 8 mod 16 so that init_key_sbox knows how to mask the lookup)
.space 256
lut_b_map:
.space 4
.space 4                     @ align to multiple of 8

.balign 16
rstate_all_start:            @ Mark start of RNG data to allow selective memory wipe
rstate_sha:                  @ 128-bit SHA random state, to be initialised to TRNG bytes; zeroth byte must be initialised to zero
.space 16
jstate:                      @ 32-bit jitter state
.space 4
rstate_lfsr:                 @ 32-bit LFSR random state and constant used to step it
.space 4
.word 0x1d872b41             @ constant that defines a maximal-length LFSR
rstate_count:
.space 4
rstate_all_end:              @ Mark end of RNG data to allow selective memory wipe

.if CT_BPERM
.balign 16
murmur3_constants:           @ Five constants used in murmur3_32 hash
.word 0xcc9e2d51
.word 0x1b873593
.word 0xe6546b64
.word 0x85ebca6b
.word 0xc2b2ae35
.endif

aes_workspace_end:

@ Initialisation code in main .text section
.section RAM_SECTION_NAME(aes),"ax",%progbits

@ The following is copied from the A2 boot ROM code at src/main/arm/varm_boot_path.c with adjustments.
@ We feed a stream of bits from the TRNG into the SHA hardware accelerator to generate some
@ random numbers.
@ Trashes r0-r6
.balign 4
init_rstate:
 CHK_COUNT 24,6
 ldr r4,=TRNG_BASE+TRNG_RNG_IMR_OFFSET
 ldr r5,=SHA256_BASE
 movs r1,#1
 str r1,[r4,#TRNG_TRNG_SW_RESET_OFFSET     -TRNG_RNG_IMR_OFFSET]
 ldr r6,[r4,#TRNG_TRNG_SW_RESET_OFFSET     -TRNG_RNG_IMR_OFFSET]     @ reads as 0
 movw r1,#SHA256_CSR_RESET|SHA256_CSR_START_BITS                     @ initialise SHA internal state by writing START bit
 str r1,[r5,#SHA256_CSR_OFFSET]
 str r6,[r4,#TRNG_SAMPLE_CNT1_OFFSET       -TRNG_RNG_IMR_OFFSET]
#if PICO_AES_HARDENING
 movs r3, #0
#endif
 movs r6,#TRNG_BLOCKS*2+1                                            @ odd so that we break out of the loop half-way through loading the SHA hardware, giving
                                                                     @ time for previous SHA computation to complete
2:
 movs r1,#0xff                                                       @ TRNG setup is inside loop in case it is skipped.
 str r1,[r4,#TRNG_TRNG_DEBUG_CONTROL_OFFSET-TRNG_RNG_IMR_OFFSET]     @ disable checks and bypass decorrelators,to stream raw TRNG ROSC samples
 str r1,[r4,#TRNG_RND_SOURCE_ENABLE_OFFSET -TRNG_RNG_IMR_OFFSET]     @ start ROSC if it is not already started
 str r1,[r4,#TRNG_RNG_ICR_OFFSET           -TRNG_RNG_IMR_OFFSET]     @ clear all interrupts (including EHR_VLD)
 adds r0,r4,#TRNG_EHR_DATA0_OFFSET         -TRNG_RNG_IMR_OFFSET
1:
 ldr r1,[r4,#TRNG_TRNG_BUSY_OFFSET         -TRNG_RNG_IMR_OFFSET]                                                      @ wait for 192 ROSC samples to fill EHR,should take constant time
 cmp r1,#0
 bne 1b
 subs r6,#1                                                          @ done?
 beq 3f
 movs r1,#8
1:
 ldmia r0!,{r2}                                                      @ copy 6 EHR words to SHA-256, plus garbage (RND_SOURCE_ENABLE and SAMPLE_CNT1)
 str r2,[r5,#SHA256_WDATA_OFFSET]                                    @ for a total of half a SHA-256 block
#if PICO_AES_HARDENING
 adds r3,#1
#endif
 subs r1,#1
 bne 1b
#if PICO_AES_HARDENING
 ldr r1, =TRNG_BASE+TRNG_EHR_DATA0_OFFSET+32
 rcp_iequal_nodelay r0, r1
#endif
 ldr r2,[r5,#SHA256_SUM0_OFFSET]                                     @ TRNG is now sampling again; use some SHA bits to modulate the chain length
 str r2,[r4,#TRNG_TRNG_CONFIG_OFFSET       -TRNG_RNG_IMR_OFFSET]
 b.n 2b

3:
#if PICO_AES_HARDENING
 movs r2, #(TRNG_BLOCKS*2) * 8
 rcp_iequal_nodelay r2, r3
#endif
#if PICO_AES_HARDENING
@ good test that we are dealing with real hardware
 ldr r2,[r5,#SHA256_CSR_OFFSET]
 movw r1,#SHA256_CSR_RESET
 rcp_iequal_nodelay r1, r2
 rcp_iequal_nodelay r2, r1
#endif
 CHK_COUNT 25,6
 str r1,[r4,#TRNG_TRNG_CONFIG_OFFSET       -TRNG_RNG_IMR_OFFSET]     @ turn off rand source and wipe SHA bits left in TRNG config; r1=0
 str r1,[r4,#TRNG_RND_SOURCE_ENABLE_OFFSET -TRNG_RNG_IMR_OFFSET]
 adds r5,r5,#SHA256_SUM0_OFFSET
@ r5=SHA256 SUM0 register (r5+4=SUM1, r4+8=SUM2, etc)
 ldmia r5,{r0-r3}  @ load first 4 words of the 8 word SHA256 output
 ldr r6,=rstate_sha
@ r5=SHA256 SUM0 register (r5+4=SUM1, r4+8=SUM2, etc), r6=rstate_sha
 stmia r6,{r0-r3}
 CHK_COUNT 26,6
 movs r0,#0
#if !PICO_AES_HARDENING
 strb r0,[r6]      @ make sure rstate_sha[0] has byte 0 set to 0, representing "out of data"
#else
 str  r0,[r6]      @ make sure rstate_sha[0] has word 0 set to 0, representing "out of data" (24-31) and 0 numbers generated (0-23)
#endif

@ try to find a non-zero initialiser to create a non-degenerate LFSR random state
 ldr r1,[r5,#16]   @ SHA SUM4
 cbnz r1,1f        @ is word 4 non-zero? then use it
 ldr r1,[r5,#20]   @ SHA SUM5
 cbnz r1,1f        @ otherwise, is word 5 non-zero? use it
 mov r1,r6         @ give up and use the address of rstate_sha (which is non-zero); this can't really happen (2^{-64} probability)
1:
 str r1,[r6,#rstate_lfsr-rstate_sha]

@ try to find a non-zero initialiser to create a non-degenerate ROSC random state
 ldr r1,[r5,#24]   @ SHA SUM6
 cbnz r1,1f        @ is word 6 non-zero? then use it
 ldr r1,[r5,#28]   @ SHA SUM7
 cbnz r1,1f        @ otherwise, is word 7 non-zero? use it
 mov r1,r6         @ give up and use the address of rstate_sha (which is non-zero); this can't really happen (2^{-64} probability)
1:
 ldr r2,=ROSC_RANDOM_OFFSET+ROSC_BASE
 str r1,[r2,#0]    @ Initialise ROSC LFSR
 CHK_COUNT 27,6
#if PICO_AES_HARDENING
 ldr r3,=ROSC_RANDOM_OFFSET+ROSC_BASE
 cbnz r1, 1f
 rcp_panic
1:
 ldr r3, [r3]
 rcp_iequal_nodelay r1, r3
#endif

.if GEN_RAND_SHA
.if SH_JITTER
 movs r2,#0
 str r2,[r6,#jstate-rstate_sha]
.endif
.endif

 CHK_COUNT 28,6
 bx r14

.thumb_func
pico_aes_decrypt_internal:
@ r0=4-way key, r1=IV_shareA, r2=IV_shareB, r3=message buffer, [sp]=number of blocks
 ldr r12,[sp]               @ Pop 5th argument in r12 (which we are allowed to treat as scratch according to AAPCS)
 push {r14}
 GET_CANARY r14,CTAG3,6
#if !PICO_AES_CALLER_INIT_RCP_COUNT
 SET_COUNT 23,6
#endif
 push {r4-r11,r14}
 push {r0-r3,r12}            @ Save the five arguments
 bl reset_sha_trng
 bl init_rstate
@ randomly re-share the LUT contents
 ldr r4,=lut_a
 mov r5,#64                  @ 64 words = 256 bytes
1:
 bl gen_rand_sha_nonpres
 ldr r6,[r4,#lut_b-lut_a]    @ EOR a random word into both shares
 eors r6,r6,r0
@if r0 is not EORed into only one share, then the LUT won't be right
 str r6,[r4,#lut_b-lut_a]
 ldr r6,[r4]
#if PICO_AES_HARDENING
 eors r7,r6,r0
 eors r8,r7,r6
 rcp_iequal_nodelay r8, r0
 stmia r4!,{r7}
#else
 eors r6,r6,r0
 stmia r4!,{r6}
#endif
 subs r5,r5,#1
 bne 1b
#if PICO_AES_HARDENING
 ldr r5,=lut_a + 256
 rcp_iequal_nodelay r4, r5
#endif
 CHK_COUNT 29,6
#if PICO_AES_HARDENING
@check again as this is quite important
 rcp_iequal_nodelay r5, r4
#endif
 bl remap                    @ scramble the LUTs
 pop {r0}                    @ pointer to 4way key data
 bl init_key_4way
 // todo alex this may trash r12; is that ok?
 bl pico_aes_lock_key
 CHK_COUNT 32,6
 pop {r0-r3}                 @ r0=IV_shareA, r1=IV_shareB, r2=message, r3=num blocks
 bl ctr_crypt_s
 bl randomisechaff
 clear03
 pop {r4-r11,r14}
 CHK_CANARY r14,CTAG3,6
 pop {r15}

.thumb_func
reset_sha_trng:
 GET_CANARY r0,CTAG19,0
 ldr r1,=RESETS_BASE+RESETS_RESET_OFFSET
 ldr r2,[r1]
 ldr r3,=#RESETS_RESET_SHA256_BITS|RESETS_RESET_TRNG_BITS
 orrs r2,r2,r3
 str r2,[r1]       @ reset the SHA hardware and the TRNG hardware
 CHK_COUNT 23,6
 bics r2,r2,r3
 str r2,[r1]       @ release the reset
 CHK_CANARY r0,CTAG19,0
 bx r14

@ Put AES core code in first scratch area
.section .scratch_x.aes,"ax",%progbits

@ if GEN_RAND_SHA==0 then we don't call the counting version
#if PICO_AES_HARDENING && GEN_RAND_SHA
check_rnd_count_func:
@ NOTE: we don't bother with a canary here as we don't write anything
 ldr r0,=rstate_sha
 ldr r0, [r0]
 rsbs r0,r0,#0   @ Negate bottom 24 bits to get the number of calls to gen_rand_sha[_nonpres] since the last reset
 bfc r0,#24,#8   @
 bx r14

reset_rnd_count_func:
 push {lr}
 GET_CANARY lr,CTAG11,0
 ldr r0,=rstate_sha
 ldrb r1, [r0, #3]
 orrs r1, #1
 lsls r1, #24
 str r1, [r0]
 CHK_CANARY lr,CTAG11,0
 pop {pc}
#endif

.if GEN_RAND_SHA
@ we need SHA256_SUM0_OFFSET==8 (see note below)
.if SHA256_SUM0_OFFSET!=8
.err
.endif

@ Return single random word in r0
@ Preserves r1-r13
.balign 4
gen_rand_sha:
 push {r1-r3,lr}
 GET_CANARY r1,CTAG1,2
 push {r1}
.if SH_JITTER
 ldr r2,=rstate_sha
 ldr r0,[r2,#jstate-rstate_sha]
 lsls r3,r0,#30
 lsrs r3,#28
 movs r1,#1
 lsls r3,r1,r3           @ 1<<(4*(r0&3))
 udiv r3,r3,r1           @ Takes constant + (r0&3) cycles
 lsrs r0,r0,#2
 bne 1f
 bl gen_rand_sha_nonpres
 ldr r2,=rstate_sha
#if PICO_AES_HARDENING
 ldr r1,[r2]             @ Make this (SH_JITTER) not affect rnd_count
 adds r1,r1,#1           @ (compensating for call to gen_rand_sha_nonpres which decrements the count by 1)
 str r1,[r2]             @ The purpose is to simplify check_rnd_count calls, and to avoid having to reset jstate frequently
#endif
1:
 str r0,[r2,#jstate-rstate_sha]
.endif
 bl gen_rand_sha_nonpres
 pop {r1}
 CHK_CANARY r1,CTAG1,0
 pop {r1-r3,pc}

@ Return single random word in r0
@ Trashes r1-r3
.balign 4
gen_rand_sha_nonpres:
 push {lr}
 GET_CANARY lr,CTAG18,0
 ldr r2,=rstate_sha
#if !PICO_AES_HARDENING
 ldr r3,=SHA256_BASE
 ldrb r1,[r2]                @ get word counter from bottom byte of rstate_sha[] (offset into SUM registers)
 subs r0,r1,#4               @ decrement it to previous SUM register
 ble 1f                      @ if the offset was 4 or less we have run out of SUM register values
 strb r0,[r2]                @ save updated SUM register offset in bottom byte of rstate_sha[]
 ldr r0,[r3,r1]              @ read value from SUM register: note that this relies on SHA256_SUM0_OFFSET==8
#else
 ldr r3,=SHA256_BASE
 ldr r1,[r2]                 @ get word counter (8) : rand counter (24) from first word of rstate_sha[] (offset into SUM registers)
 lsls r0, r1, #1             @ clear C (also set N which may force us down BLE path on skip of the sub below)
 sbcs r0,r1,#0x04000000      @ decrement word counter for previous SUM register (and decrement rand counter due to C == 0)
 str r0,[r2]                 @ save updated worder counter / rand_counter in bottom word of rstate_sha[]
 asrs r1, r0, #24
 ble 1f                      @ if the offset was 4 or less we have run out of SUM register values

 ldr r2,=SHA256_BASE + 4
 adds r2, r1
 adds r1, r3, r0, asr #24
 ldr r0, [r2], #-4
 rcp_iequal_nodelay r1, r2
#endif
 b gen_rand_sha_nonpres_exit
1:
@ [CK_JITTER code was here]
 movs r0,#SHA256_SUM6_OFFSET+1
#if !PICO_AES_HARDENING
 strb r0,[r2]                @ reset word counter: the +1 is compensated for later
#else
 strb r0,[r2,#3]             @ reset word counter: the +1 is compensated for later
#endif
 movw r1,#(1<<SHA256_CSR_BSWAP_LSB)+(1<<SHA256_CSR_START_LSB)
 str r1,[r3,#SHA256_CSR_OFFSET]        @ start SHA256 hardware
 movs r0,#3                  @ take four words from rstate_sha, incrementing as we go
 ldr r1,[r2]
#if !PICO_AES_HARDENING
 adds r1,r1,#255             @ overall this adds 256 to the value in rstate_sha and resets the bottom byte to SHA256_SUM6_OFFSET
#else
 adds r1,r1,#0xff000000      @ overall this adds 256 to the value in rstate_sha and resets the bottom byte to SHA256_SUM6_OFFSET
#endif
1:
 str r1,[r2],#4
 str r1,[r3,#SHA256_WDATA_OFFSET]
 cbz r0,3f
 ldr r1,[r2]
 adcs r1,r1,#0
 sub r0,r0,#1                @ preserve the carry
 b 1b
3:
 movs r1,#0x80               @ End of message bit (with byte-swapped endianity) = start of message padding
 str r1,[r3,#SHA256_WDATA_OFFSET]
 movs r1,#9
1:
 str r0,[r3,#SHA256_WDATA_OFFSET]
 subs r1,r1,#1
 bpl 1b

 lsls r2, r1, #31             @ Specifies message length = 128 bits (with byte-swapped endianity) (i.e. 0x80000000)
 str r2,[r3,#SHA256_WDATA_OFFSET]
1:
 ldr r0,[r3,#SHA256_CSR_OFFSET]
#if PICO_AES_HARDENING
 asrs r2, #1
#endif
 lsrs r0,r0,#SHA256_CSR_SUM_VLD_LSB+1
 bcc 1b                      @ wait for hardware to finish
#if PICO_AES_HARDENING
 @ r1 is -1 from loop above
 @ r2 is asr-ed right from 0x8000000. emperically it takes more than 6 loops, so we should have multiple 1s in the high bits
 @    note also that if 0x80000000 was not set above correctly, r2 might not be negative
 asrs r2, #26
 @ BEWARE this will fail if you step thru the above loop in the debugger as it will finish too quickly!
 rcp_iequal_nodelay r1, r2
#endif
 ldr r0,[r3,#SHA256_SUM7_OFFSET]
gen_rand_sha_nonpres_exit:
 CHK_CANARY lr,CTAG18,0
 pop {pc}
.endif

@ simple LFSR rand versions
@ return a random number in r0
@ This version preserves all r1-r13
@ 23 or 24 cycles including branch = 23 or 24 cycles/word
@ (would be 20 or 21 cycles if written out)
.balign 4
.thumb_func
.if !GEN_RAND_SHA
gen_rand_sha:
gen_rand_lfsr:               @ Not used
 push {r14}
 GET_CANARY r14,CTAG2,2
 push {r1-r3,r14}
 bl gen_rand_lfsr_nonpres
 pop {r1,r3,r14}
 CHK_CANARY r14,CTAG2,0
 pop {r15}
.endif

@ Trashes r1,r2,r3
@ 12 cycles including branch = 12 cycles/word
.balign 4
.if !GEN_RAND_SHA
gen_rand_sha_nonpres:
.endif
gen_rand_lfsr_nonpres:
 GET_CANARY r3,CTAG10,0
 ldr r2,=rstate_lfsr
 ldmia r2,{r0-r1}           @ r0=state_in, r1=0x1d872b41=constant for a maximum-length sequence
 and r1,r1,r0,asr#31        @ will we be shifting out a 1? keep the constant, otherwise 0
 eors r0,r1,r0,lsl#1
#if PICO_AES_HARDENING
@ Basically r3 &= (r0 ? 0xfffffffff : 0) but still potentially perturbing r3 unless the and is skipped
 clz r1, r0
 subs r1, #32
 asrs r1, #5
 ands r3, r1
#endif
 str r0,[r2]
 CHK_CANARY r3,CTAG10,0
 bx r14

.macro loadlfsr
 ldr r2,=rstate_lfsr
 ldmia r2,{r0-r1}           @ r0=lfsr_state, r1=lfsr_const=0x1d872b41 for a maximum-length sequence
.endm

.macro steplfsr
 ands r3,r1,r0,asr#31       @ will we be shifting out a 1? keep the constant, otherwise 0
 eors r0,r3,r0,lsl#1
.endm

.macro steplfsr_check
 steplfsr
 bne steplfsr_check\@
 rcp_panic
 steplfsr_check\@:
.endm

.macro savelfsr
 str r0,[r2]
.endm

.ltorg

.balign 4
.thumb_func
makesmallperm:
@ Make a uniformly random permutation of R0 bytes and stores the resulting byte array at R1
@ Should be very uniform up to R0=10; maybe 11 or 12 are also OK. (10! << 2^32)
@ To make it valid up to R0=256, move the bl gen_rand_sha inside the loop
@ Uses inside-out method (slightly more efficient variant of Fisher-Yates)
@ Trashes r0-r3

 push {r14}
 GET_CANARY r14,CTAG4,6
 push {r4-r6,r14}
 movs r4,r1
 movs r6,r0
 movs r1,#0
 movs r2,#1
 bl gen_rand_sha

1:
@ r1,r2=i,i+1,   i=0, 2, 4, ...
 cmp r1,r6
 beq 2f

 umull r0,r3,r0,r2
 ldrb r5,[r4,r3]
 strb r5,[r4,r1]
 strb r1,[r4,r3]
 adds r1,r1,#2

@ r2,r1=i,i+1,   i=1, 3, 5, ...
 cmp r2,r6
 beq 2f

 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 strb r2,[r4,r3]
 adds r2,r2,#2

 b 1b

2:
 pop {r4-r6,r14}
 CHK_CANARY r14,CTAG4,6
 pop {r15}

.balign 4
.thumb_func
makeperm16:
@ Make a random permutation of 16 things using the inside-out method (slightly more efficient variant of Fisher-Yates)
@ Store it in the 16 bytes at perm16
@ More efficient than calling makeperm with R0=16, R1=perm16 - fewer calls to gen_rand_sha
@ Trashes r0-r5

 GET_CANARY r0,CTAG5,1
 push {r0,r14}
 ldr r4,=perm16
 bl gen_rand_sha_nonpres

@ i=0
 movs r1,#0
 movs r2,#1       @ r1,r2=i,i+1
 strb r1,[r4]

@ i=1
 adds r1,r1,#2    @ r1,r2=i+1,i
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 strb r2,[r4,r3]

1:
@ i=2, 4, 6, 8
 adds r2,r2,#2    @ r1,r2=i,i+1
 umull r0,r3,r0,r2
 ldrb r5,[r4,r3]
 strb r5,[r4,r1]
 strb r1,[r4,r3]

@ i=3, 5, 7, 9
 adds r1,r1,#2    @ r1,r2=i+1,i
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 cmp r1,#10
 strb r2,[r4,r3]
 bne 1b

@ refresh random number after extracting 10! from it
@ 10! and 16!/10! are both much less than 2^32, so the permutation will be extremely close to uniform
 bl gen_rand_sha

1:
@ i=10, 12, 14
 adds r2,r2,#2    @ r1,r2=i,i+1
 umull r0,r3,r0,r2
 ldrb r5,[r4,r3]
 strb r5,[r4,r1]
 strb r1,[r4,r3]

@ i=11, 13, 15
 adds r1,r1,#2    @ r1,r2=i+1,i
 umull r0,r3,r0,r1
 ldrb r5,[r4,r3]
 strb r5,[r4,r2]
 cmp r1,#16
 strb r2,[r4,r3]
 bne 1b

 pop {r0,r14}
 CHK_CANARY r0,CTAG5,4
 bx r14

.balign 4
.thumb_func
remap:
@ do a random remap of the LUTs
@ preserves r0-r11; trashes r12
 GET_CANARY r12,CTAG6,6
 push {r0-r12,r14}
 bl gen_rand_sha_nonpres
 ldr r1,=lut_a
 bl remap_1
 bl gen_rand_sha_nonpres
 ldr r1,=lut_b
 bl remap_1
 pop {r0-r12,r14}
 CHK_CANARY r12,CTAG6,6
 bx r14

remap_1:
@ r0: B0:xa B1:xb B2:ya B3:yb
@ r1: array of 256 bytes, followed by a 4-byte map
@ shuffle LUT share array such that new[i]=old[i^xa^xb]^ya^yb, update map according to r0
 GET_CANARY r6,CTAG7,6
 push {r6,r14}
 mov r14,0x01010101
 ubfx r6,r0,#16,#8
 ubfx r7,r0,#24,#8
 mul r6,r6,r14               @ data remaps ya and yb, byte replicated
 mul r7,r7,r14
 movw r10,#0x1010
 and r10,r10,r0,lsl#3        @ 0/16 in each byte of r10 from b1 and b9 of r0, ready for rotates by 0 or 16
 mov r3,#0x7f7f7f7f
 ubfx r2,r0,#0,#1
 lsl r11,r3,r2               @ 0x7f or 0xfe in each byte of r11, ready for sel of rev16
 ubfx r2,r0,#8,#1
 lsl r12,r3,r2
 ldr r2,[r1,#0x100]          @ old map
 eors r2,r2,r0
 str r2,[r1,#0x100]          @ updated map
 // todo graham; what is the effect of not doing the whole loop - is it broken if you just do some?
 mov r2,#252                 @ loop over entries
1:
 ldr r4,[r1,r2]
 eor r3,r2,r0
 eor r3,r3,r0,ror#8
 and r3,r3,#0xfc             @ r3=remapped address r2
 ldr r5,[r1,r3]
 eors r5,r5,r6               @ remap data; ensure case x==0 works by doing both remaps on same side
 eors r5,r5,r7
 lsr r8,r10,#8
 ror r5,r5,r8                @ ROR#16 is the same as eor of address with 2
 ror r5,r5,r10
 rev16 r8,r5                 @ REV16 is the same as eor of address with 1
 uadd8 r9,r11,r11
 sel r5,r8,r5
 rev16 r8,r5
 uadd8 r9,r12,r12
 sel r5,r8,r5
 mul r8,r14,r2
 mul r9,r14,r3
 usub8 r8,r8,r9              @ bytewise comparison of original address and remapped address, both byte replicated
 sel r8,r4,r5                @ swap r4 and r5 as necessary in constant time
 str r8,[r1,r2]              @ write possibly swapped values back
 sel r8,r5,r4
 str r8,[r1,r3]
 subs r2,r2,#4
 bpl 1b
 pop {r6,r14}
 CHK_CANARY r6,CTAG7,6
 bx r14

.if RK_ROR

@ "refresh" shares of rkeys by random eor into both shares of each word, and also randomise the single word RKshareC
@ Trashes r0-r12
@ If i = word number 0..3,
@ Aptr=memory word pointer to block of 20 bytes containing H&V-rotated share A roundkey (similarly B), then
@ vpermA=Aptr[4]>>30, vpermB=Bptr[4]>>30, and
@ roundkey shareA(i) = Aptr[i+vpermA mod 4] ror ((i+vpermA mod 4)^th byte of Aptr[4])
@ roundkey shareB(i) = Bptr[i+vpermB mod 4] ror ((i+vpermB mod 4)^th byte of Bptr[4])+16

.macro ref_roundkey_shares_s_impl
 ldr r4,=rkey_s
 loadlfsr
 steplfsr_check              @ r0=change in RKshareC
 ldr r2,=RKshareCchange
 str r0,[r2]
 ldr r3,=RKshareC
 ldr r5,[r3]
 eors r5,r5,r0
 str r5,[r3]
 @ r0=lfsr_state, r1=lfsr_const, r4=roundkey_ptr, r11=roundcounter

ref_roundkey_shares_s_loop:
 ldmia r4!,{r5-r8,r10}       @ r5-r8 = rkey shareA, r10=X_A=vperm+rotations of rkey shareA

 ldr r12,[r4,#16]            @ r12 = X_B=vperm+rotations of rkey shareB
 mov r2,r12,lsr#30           @ r2 = vpermB
 sub r9,r2,r10,lsr#30        @ r9 = vpermB - vpermA (|junk)
 mov r2,r9,lsl#3             @ r2 = 8*(vpermB - vpermA) mod 32
 mov r12,r12,ror r2
 usub8 r12,r10,r12           @ r12 = rotsA - (rotsB ror r2)

 @ r2,r3,r10=workspace, r0=lfsr_state, r1=lfsr_const, r4=roundkeyB_ptr, r5-r8=roundkeyA, r9=vpermdiff, r10=rotsA, r11=roundcounter, r12=rotdiff
 steplfsr; eors r5,r5,r0; ands r9,r9,#3; ldr r3,[r4,r9,lsl#2]; ror r2,r0,r12; eors r3,r3,r2,ror#16; mov r12,r12,ror#8; str r3,[r4,r9,lsl#2]; adds r9,r9,#1
 steplfsr; eors r6,r6,r0; ands r9,r9,#3; ldr r3,[r4,r9,lsl#2]; ror r2,r0,r12; eors r3,r3,r2,ror#16; mov r12,r12,ror#8; str r3,[r4,r9,lsl#2]; adds r9,r9,#1
 steplfsr; eors r7,r7,r0; ands r9,r9,#3; ldr r3,[r4,r9,lsl#2]; ror r2,r0,r12; eors r3,r3,r2,ror#16; mov r12,r12,ror#8; str r3,[r4,r9,lsl#2]; adds r9,r9,#1
 steplfsr_check; eors r8,r8,r0; ands r9,r9,#3; ldr r3,[r4,r9,lsl#2]; ror r2,r0,r12; eors r3,r3,r2,ror#16;                    str r3,[r4,r9,lsl#2]

 ldr r3,=RKshareCchange
 ldr r3,[r3]
 movs r2,#0
 usub8 r10,r2,r10
 ror r2,r3,r10; mov r10,r10,ror#8; eors r5,r5,r2
 ror r2,r3,r10; mov r10,r10,ror#8; eors r6,r6,r2
 ror r2,r3,r10; mov r10,r10,ror#8; eors r7,r7,r2
 ror r2,r3,r10;                    eors r8,r8,r2

 subs r4,r4,#20
 stmia r4,{r5-r8}
 adds r4,r4,#40
 subs r11,r11,#1

 bne ref_roundkey_shares_s_loop
#if PICO_AES_HARDENING
 ldr r5,=rkey_s + 40 * 15
 rcp_iequal_nodelay r4, r5
#endif
 ldr r2,=rstate_lfsr         @ restore rstate_lfsr
 savelfsr                    @ Save lfsr_state
 clear03 24
.endm

.else // RK_ROR

@ "refresh" shares of rkeys by random eor into both shares of each word, and also randomise the single word RKshareC
@ Trashes r0-r11
.macro ref_roundkey_shares_s_impl
 ldr r4,=rkey_s
 loadlfsr
 steplfsr_check              @ r0=change in RKshareC
 ldr r3,=RKshareC
 ldr r5,[r3]
 eors r5,r5,r0
 str r5,[r3]
 mov r10,r0
ref_roundkey_shares_s_loop:
 ldmia r4!,{r5-r9}           @ r5-r8 = rkey shareA with vperm r9

 @ clear03: would need to do this with, say r2,r3,r12 (reloading r2 later)

 ldr r3,[r4,#16]             @ rkey shareB has a vperm of r10>>30
 movs r3,r3,lsr#30
 sub r9,r3,r9,lsr#30         @ r9 = vperm_B - vperm_A (|junk)
 @ r3,r12=workspace, r0=lfsr_state, r1=lfsr_const, r2=rstate_lfsr, r4=roundkeyB_ptr, r5-r8=roundkeyA, r9=vpermdiff, r10=RKshareCchange, r11=roundcounter

 steplfsr; eors r5,r5,r0; and r9,r9,#3; eors r5,r5,r10; ldr r3,[r4,r9,lsl#2]; eors r3,r3,r0,ror#16; str r3,[r4,r9,lsl#2]; adds r9,r9,#1
 steplfsr; eors r6,r6,r0; and r9,r9,#3; eors r6,r6,r10; ldr r3,[r4,r9,lsl#2]; eors r3,r3,r0,ror#16; str r3,[r4,r9,lsl#2]; adds r9,r9,#1
 steplfsr; eors r7,r7,r0; and r9,r9,#3; eors r7,r7,r10; ldr r3,[r4,r9,lsl#2]; eors r3,r3,r0,ror#16; str r3,[r4,r9,lsl#2]; adds r9,r9,#1
 steplfsr_check; eors r8,r8,r0; and r9,r9,#3; eors r8,r8,r10; ldr r3,[r4,r9,lsl#2]; eors r3,r3,r0,ror#16; str r3,[r4,r9,lsl#2]

 subs r4,r4,#20
 stmia r4,{r5-r8}
 adds r4,r4,#40
 subs r11,r11,#1

 @ clear03: would need to do this with, say r3,r5-r8

 bne ref_roundkey_shares_s_loop
 savelfsr
 clear03 24
#if PICO_AES_HARDENING
 ldr r5,=rkey_s + 40 * 15
 rcp_iequal_nodelay r4, r5
#endif
.endm
.endif

.if PICO_AES_INLINE_REF_ROUNDKEY_SHARES_S
.macro inline_ref_roundkey_shares_s
ref_roundkey_shares_s_starts:
 mov r11,#15                 @ there are 15 expanded keys
 ref_roundkey_shares_s_impl
ref_roundkey_shares_s_end:
.endm
.else
.balign 4
.thumb_func
ref_roundkey_shares_s:
 mov r11,#15                 @ there are 15 expanded keys
ref_roundkey_shares_s_test:  @ entry point for test code to do fewer than 15 rounds
 push {lr}
 GET_CANARY lr,CTAG8,6
 ref_roundkey_shares_s_impl
 CHK_CANARY lr,CTAG8,6
 pop {pc}
.endif

.if RK_ROR

@ Rotates roundkey vperms and RK_ROR rotations by random amounts
@ Trashes r0-r10
@ If i = word number 0..3,
@ Aptr=memory word pointer to block of 20 bytes containing H&V-rotated share A roundkey (similarly B), then
@ vpermA=Aptr[4]>>30, vpermB=Bptr[4]>>30, and
@ roundkey shareA(i) = Aptr[i+vpermA mod 4] ror ((i+vpermA mod 4)^th byte of Aptr[4])
@ roundkey shareB(i) = Bptr[i+vpermB mod 4] ror ((i+vpermB mod 4)^th byte of Bptr[4])+16
.macro ref_roundkey_hvperms_s_impl
 ldr r10,=rkey_s
ref_roundkey_hvperms_s_loop:
 bl gen_rand_lfsr_nonpres     @ r0=new vperm high|rotations
 ldmia r10,{r2-r5,r9}         @ r2-r5=roundkey share A/B, r9=old vperm high|rotations
 str r0,[r10,#16]
 mov r8,r0,lsr#30             @ r8=new vperm low
 sub r6,r8,r9,lsr#30          @ r6=(new vperm low)-(old vperm low) | junk
 mov r8,r6,lsl#3              @ r8=8*((new vperm low)-(old vperm low)) mod 32
 mov r0,r0,ror r8
 usub8 r0,r9,r0               @ i^th byte of r0 = (i^th byte of old rotations) - ((i+newvperm-oldvperm)^th byte of new rotations)
 movs r2,r2,ror r0; ands r6,r6,#3; str r2,[r10,r6,lsl#2]; movs r0,r0,ror#8; adds r6,r6,#1
 movs r3,r3,ror r0; ands r6,r6,#3; str r3,[r10,r6,lsl#2]; movs r0,r0,ror#8; adds r6,r6,#1
 movs r4,r4,ror r0; ands r6,r6,#3; str r4,[r10,r6,lsl#2]; movs r0,r0,ror#8; adds r6,r6,#1
 movs r5,r5,ror r0; ands r6,r6,#3; str r5,[r10,r6,lsl#2]
 adds r10,r10,#20
 adds r7,r7,#1
 cmp r7, #30
 bne ref_roundkey_hvperms_s_loop
 clear03 28
.endm

.else

@ Rotates roundkey vperms by random amounts
@ Trashes r0-r9
.macro ref_roundkey_hvperms_s_impl
 bl gen_rand_lfsr_nonpres
 ldr r1,=rkey_s
ref_roundkey_hvperms_s_loop:
 cmp r7,#15
 bne 2f
@ Get a new random r0 after using 15 x 2 bits of the original one
@ Note that the junk bits (2-31) in the vperms are not adjusted independently, but that's no big loss,
@ and the gain is only calling gen_rand_lfsr twice instead of 30 times.
 push {r1}; bl gen_rand_lfsr_nonpres; pop {r1}
 2:
 ldmia r1,{r2-r5,r9}    @ roundkey share A/B=r2-r5, vperm=r9 (including junk bits)
 mov r8,r9,lsr#30       @ r8=old vperm (low)
 add r6,r9,r0           @ r6=new vperm (high) | new junk
 str r6,[r1,#16]
 rsb  r6,r8,r6,lsr#30   @ r6=(new vperm low)-(old vperm low) | junk bits
 ands r6,r6,#3; str r2,[r1,r6,lsl#2]; adds r6,r6,#1
 ands r6,r6,#3; str r3,[r1,r6,lsl#2]; adds r6,r6,#1
 ands r6,r6,#3; str r4,[r1,r6,lsl#2]; adds r6,r6,#1
 ands r6,r6,#3; str r5,[r1,r6,lsl#2]
 adds r1,r1,#20
 movs r0,r0,ror#2
 adds r7,r7,#1
 cmp r7, #30
 bne ref_roundkey_hvperms_s_loop
 clear03 28
.endm
.endif

.if PICO_AES_INLINE_REF_ROUNDKEY_HVPERMS_S
.macro inline_ref_roundkey_hvperms_s
ref_roundkey_hvperms_s_starts:
 movs r7,#0
 ref_roundkey_hvperms_s_impl
ref_roundkey_hvperms_s_end:
.endm
.else
.balign 4
.thumb_func
ref_roundkey_hvperms_s:
 movs r7,#0
ref_roundkey_hvperms_s_test:  @ entry point for test code to do fewer than 15 rounds
 GET_CANARY r0,CTAG9,6
 push {r0, lr}
 ref_roundkey_hvperms_s_impl
 pop {r0}
 CHK_CANARY r0,CTAG9,6
 pop {pc}
.endif

.ltorg

.if ST_VPERM
.balign 4
.thumb_func
@ Cycle share registers r4-r7, r8-r11 (r4->r5-r6->r7->r4 etc.) by an addtional amount
@ given in the bottom two bits of R0 and update the rotation recorded at statevperm.
@ On entry R1 must point to statevperm.
@ Trashes r0-r3,r12
@ Maintains r4=rorig(4+(-!r1)%4), r5=rorig(4+(1-!r1)%4), ...
@           r8=rorig(8+(-!r1)%4), r9=rorig(8+(1-!r1)%4), ...
@ Note: only low 2 bits of !r1 are used. The rest are random to add to the noise.
addstatevperm:
 push {r14}
 GET_CANARY r14,CTAG20,0
 ldr r2,[r1]
 adds r2,r2,r0
 str r2,[r1]

 ldr r1,=shareA
 ands r0,r0,#3; str r4,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r5,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r6,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r7,[r1,r0,lsl#2]; adds r0,r0,#1
 ldmia r1,{r4-r7}

 getchaffaddress r12          @ Overwrite temporary storage with random numbers
 ldmia r12!,{r2,r3}
 stmia r1!,{r2,r3}
 ldmia r12!,{r2,r3}
 stmia r1!,{r2,r3}

 ldr r1,=shareB
 ands r0,r0,#3; str r8, [r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r9, [r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r10,[r1,r0,lsl#2]; adds r0,r0,#1
 ands r0,r0,#3; str r11,[r1,r0,lsl#2]; adds r0,r0,#1
 ldmia r1,{r8-r11}

 getchaffaddress r0,16        @ Overwrite temporary storage with random numbers
 ldmia r0!,{r2,r3}
 stmia r1!,{r2,r3}
 ldmia r0!,{r2,r3}
 stmia r1!,{r2,r3}

addstatevperm_exit:           @ label exit point to be to able to specify to analysis code
 CHK_CANARY r14,CTAG20,0
 pop {pc}
.endif

@ Conjugate lut_a, lut_b with (state) shareC
@ I.e., EOR the input and output with shareC.
@ We need to pick one input for each share A and B, and one output for ONE of the shares A and B
@ Arbitrarily choosing a0, b1 and d0
.balign 4
conjshareC:
 push {r14}
 GET_CANARY r14,CTAG21,0
.if ST_SHAREC
 ldr r1,=shareA
 ldr r0,[r1, #shareC-shareA]   @ Get shareC as a word (all bytes the same)
 ldr r1,=lut_a                 @ Need to EOR share C into inputs of both lut_a and lut_b, and one of their outputs...
 ldr r2,[r1,#0x100]
 eors r2,r2,r0,lsr#24
 str r2,[r1,#0x100]
 movs r0,r0,lsr#16
 ldr r1,=lut_b                 @ ... (continued) Here we're EORing share C into a0, b1 and d0.
 ldr r2,[r1,#0x100]
 eors r2,r2,r0,lsl#8
 str r2,[r1,#0x100]
.endif
 CHK_CANARY r14,CTAG21,0
 pop {pc}

.macro shift_rows_s_impl
@ First "rotate" the two most-significant bytes of the state by two registers
@ Trashes r0-r3
@ Slightly faster (but not shorter?) with ubfx/bfi
 eors r0,r4,r6               @ ta=state[0]^state[2]; ta&=0xffff0000; state[0]^=ta; state[2]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r4,r4,r0
 eors r6,r6,r0
 eors r0,r5,r7               @ ta=state[1]^state[3]; ta&=0xffff0000; state[1]^=ta; state[3]^=ta;
 lsrs r0,r0,#16
 lsls r0,r0,#16
 eors r5,r5,r0
 eors r7,r7,r0
@ next "rotate" the two odd-significance bytes of the state by one register
 eors r1,r7,r4               @ tb=state[3]^state[0]; tb&=0xff00ff00;
 ands r1,r1,#0xff00ff00
 eors r0,r4,r5               @ ta=state[0]^state[1]; ta&=0xff00ff00; state[0]^=ta;
 ands r0,r0,#0xff00ff00
 eors r4,r4,r0
 eors r0,r5,r6               @ ta=state[1]^state[2]; ta&=0xff00ff00; state[1]^=ta;
 ands r0,r0,#0xff00ff00
 eors r5,r5,r0
 eors r0,r6,r7               @ ta=state[2]^state[3]; ta&=0xff00ff00; state[2]^=ta;
 ands r0,r0,#0xff00ff00
 eors r6,r6,r0
 eors r7,r7,r1               @                                       state[3]^=tb;
@ repeat for other share, conjugated by ror#16
 clear01                     @ barrier
 eors r0,r8,r10              @ ta=state[0]^state[2]; ta&=0x0000ffff; state[0]^=ta; state[2]^=ta;
 lsls r0,r0,#16
 lsrs r0,r0,#16
 eors r8,r8,r0
 eors r10,r10,r0
 eors r0,r9,r11              @ ta=state[1]^state[3]; ta&=0x0000ffff; state[1]^=ta; state[3]^=ta;
 lsls r0,r0,#16
 lsrs r0,r0,#16
 eors r9,r9,r0
 eors r11,r11,r0
 eors r1,r11,r8              @ tb=state[3]^state[0]; tb&=0xff00ff00;
 ands r1,r1,#0xff00ff00
 eors r0,r8,r9               @ ta=state[0]^state[1]; ta&=0xff00ff00; state[0]^=ta;
 ands r0,r0,#0xff00ff00
 eors r8,r8,r0
 eors r0,r9,r10              @ ta=state[1]^state[2]; ta&=0xff00ff00; state[1]^=ta;
 ands r0,r0,#0xff00ff00
 eors r9,r9,r0
 eors r0,r10,r11             @ ta=state[2]^state[3]; ta&=0xff00ff00; state[2]^=ta;
 ands r0,r0,#0xff00ff00
 eors r10,r10,r0

 eors r11,r11,r1             @                                       state[3]^=tb;

 clear01                     @ barrier
.endm

.if PICO_AES_INLINE_SHIFT_ROWS_S
.macro inline_shift_rows_s
shift_rows_s_starts:
 shift_rows_s_impl
shift_rows_s_end:
.endm
.else
.balign 4
.thumb_func
@ Not going to use canaries here as it doesn't write anything - could be use to perturb register values, but not super worried about that yet
shift_rows_s:
 shift_rows_s_impl
 bx r14
.endif

@ multiply polynomial over GF(2⁸) by c(x) = 0x03x³ + 0x01x² + 0x01x + 0x02 modulo x⁴+1
@ r0x00 is a register holding 0x00000000;  r0x1b is a register holding 0x1b1b1b1b
.macro mixcol rx,rt,ru,r0x00,r0x1b
                             @ let rx=(a,b,c,d)
 uadd8 \rt,\rx,\rx           @ MSB of each byte into the GE flags
 sel \ru,\r0x1b,\r0x00       @ get bytewise correction for bytewise field multiplication by 2
 eors \rt,\rt,\ru            @ (2a,2b,2c,2d)

 eors \ru,\rt,\rx            @ (3a,3b,3c,3d)
 eors \rt,\rt,\rx,ror#24     @ (2a+b,2b+c,2c+d,2d+a)
 eors \rt,\rt,\rx,ror#16     @ (2a+b+c,2b+c+d,2c+d+a,2d+a+b)
 eors \rx,\rt,\ru,ror#8      @ (2a+b+c+3d,2b+c+d+3a,2c+d+a+3b,2d+a+b+3c)
.endm

@ multiply polynomial over GF(2⁸) by d(x) = 0x0Bx³ + 0x0Dx² + 0x09x + 0x0E modulo x⁴+1; c(x)d(x)=1 modulo x⁴+1
.macro invmixcol rx,rt,ru,rv,rw,r0x00,r0x1b
 uadd8 \rt,\rx,\rx           @ field multiplication by 2 as above
 sel \rw,\r0x1b,\r0x00
 eors \rt,\rt,\rw            @ 2x
 uadd8 \ru,\rt,\rt
 sel \rw,\r0x1b,\r0x00
 eors \ru,\ru,\rw            @ 4x
 uadd8 \rv,\ru,\ru
 sel \rw,\r0x1b,\r0x00
 eors \rv,\rv,\rw            @ 8x

 eors \rx,\rx,\rv            @ 9x
 eors \rw,\rx,\rt            @ 11x
 eors \rw,\rw,\rx,ror#16     @ 11x ^ 9x ROL #16
 eors \rx,\rx,\ru            @ 13x
 eors \rw,\rw,\rx,ror#8      @ 11x ^ 9x ROL #16 ^ 13x ROL #24
 eors \rt,\rt,\ru            @ 6x
 eors \rt,\rt,\rv            @ 14x
 eors \rx,\rt,\rw,ror#8      @ 14x ^ 9x ROL #8 ^ 13x ROL #16 ^ 11x ROL #24
.endm

.balign 4
.thumb_func
@ Not going to use canaries here as it doesn't write anything - could be use to perturb register values, but not super worried about that yet
@ Trashes r0-r3,r12
mix_cols_s:
 mov r2,#0x00000000
 mov r3,#0x1b1b1b1b
 mixcol r4 ,r0,r1,r2,r3      @ apply mixcol to each state word
 mixcol r5 ,r0,r1,r2,r3
 mixcol r6 ,r0,r1,r2,r3
 mixcol r7 ,r0,r1,r2,r3
 ldr r12,=chaff
 ldmia r12!,{r0,r1}          @ overwrite sensitive shareA-related quantities r0,r1 with random numbers
 mixcol r8 ,r0,r1,r2,r3
 mixcol r9 ,r0,r1,r2,r3
 mixcol r10,r0,r1,r2,r3
 mixcol r11,r0,r1,r2,r3
 ldmia r12!,{r0,r1}          @ overwrite  sensitive shareB-related quantities r0,r1 with random numbers
 bx r14

@ Lookup each byte of a word, Rtarg, in a table and replace Rtarg with the result (used for SBOX lookups)
.macro subbytes Rtarg,Rtable,Rspare0,Rspare1,Rspare2,Rspare3
 ubfx \Rspare0,\Rtarg,#0,  #8
 ubfx \Rspare1,\Rtarg,#8,  #8
 ubfx \Rspare2,\Rtarg,#16, #8
 ubfx \Rspare3,\Rtarg,#24, #8

 ldrb \Rspare0,[\Rtable,\Rspare0]
 ldrb \Rspare1,[\Rtable,\Rspare1]
 ldrb \Rspare2,[\Rtable,\Rspare2]
 ldrb \Rspare3,[\Rtable,\Rspare3]
 orr \Rspare0,\Rspare0,\Rspare1,lsl#8
 orr \Rspare2,\Rspare2,\Rspare3,lsl#8
 orr \Rtarg,\Rspare0,\Rspare2,lsl#16
.endm

@ map all bytes of the state through the split LUT, lut_a and lut_b
@ Trashes r0-r3,r12
.macro map_sbox_s_impl
 ldr r0,=shareA                 @ Write out state share A to memory
@ stmia r0,{r4-r7}              @ Used to do a STM
 getchaffaddress r1
 ldr r2,[r1]
 str r4,[r0]                    @ Interperse with dummy writes to prevent implicit broadcasting of HW(ShareA_word0^ShareA_word1)+cyclic perms,
 str r2,[r1]                    @ which arise due to internal write buffer. Such a quantity could (without such interspersing) be paired
 str r5,[r0,#4]                 @ via 2nd order with its share B counterpart, resulting in broadcasting HW(word0^word1)+cyclic.
 str r2,[r1]                    @ shareC doesn't guard against this, because word0^shareC^word1^shareC=word0^word1.
 str r6,[r0,#8]                 @ Broadcasting of HW(ShareA_word0)+cyclic on the other hand is not prevented by interspersing, but
 str r2,[r1]                    @ it isn't useful at 2nd order because shareC kills its relationship with HW(ShareB_word0)+cyclic.
 str r7,[r0,#12]
 str r2,[r1]

 ldr r0,=shareB                 @ Write out state share B to memory
 stmia r0,{r8-r11}              @ Not essential to intersperse share B too because i0B^i1B etc should have nothing in share A to couple with

 bl makeperm16                  @ Rebuild random 16-way permutation. Maybe do this less frequently
@ Now combine state shares A and B and apply the split sbox to each byte, in the order given by the above random permutation

 bl gen_rand_sha_nonpres
 mov r11,r0
 ldr r8,=lut_a
 ldr r9,=lut_b
 ldr r0,[r8,#0x100]             @ R0 = a0 | a1<<8 | c0<<16 | c1<<24   (lut_a_map)
 eors r3,r0,r0,lsr#8            @ R3 = a0^a1 | junk
 uxtb r10,r3
 ldr r1,[r9,#0x100]             @ R1 = b0 | b1<<8 | d0<<16 | d1<<24   (lut_b_map)
 eors r1,r0,r1
 eors r2,r1,r1,lsr#8
 movs r12,r1,lsr#16             @ R12 = c0^d0 | (c1^d1)<<8
 bfi r12,r2,#16,#8              @ R12 = c0^d0 | (c1^d1)<<8 | (a0^a1^b0^b1)<<16

 ldr r4,=perm16
 ldr r5,=shareA
 ldr r6,=shareB
 movs r1,#0;movs r2,#0;movs r3,#0
@ Using r0=loop counter, r4=perm16, r5=shareA, r6=shareB, r8=lut_a, r9=lut_b, r10=a0^a1, r11=Random, r12=(c0^d0) | (c1^d1)<<8 | (a0^a1^b0^b1)<<16
 movs r0,#15
1:                              @ (Ordering instructions to minimise result delays)
 ldrb r1,[r4,r0]                @ r1 = perm[r0]
 mov  r11,r11,ror#11            @ Rotate random 32 bits to present a new low 8 bits
 eors r7,r1,#2                  @ r7 = perm[r0]^2
 ldrb r2,[r5,r1]                @ r2 = shareA[perm[r0]]
 eor  r11,r11,r2,ror#8          @ Transfer some of the share-randomness of the input to the output (the share-randomness would otherwise be lost/wasted)
 ldrb r3,[r6,r7]                @ r3 = shareB[perm[r0]^2]
 eor  r2,r2,r10                 @ r2 = shareA[perm[r0]]^a0^a1
 eors r2,r2,r3                  @ r2 = shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]
 ldrb r3,[r8,r2]                @ r3 = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]
 eor  r2,r2,r12,lsr#16          @ r2 = shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]
 eor  r3,r3,r12                 @ r3 = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]^c0^d0 | (junk<<8)
 eor  r3,r3,r11                 @ r3 = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]^c0^d0^rand | (junk<<8)
 strb r3,[r5,r1]                @ shareA'[perm[r0]] = lut_a[shareA[perm[r0]]^a0^a1^shareB[perm[r0]^2]]^c0^d0^rand
 ldrb r3,[r9,r2]                @ r3 = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]
 subs r0,r0,#1
 eor  r3,r3,r11                 @ r3 = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]^rand
 eor  r3,r3,r12,lsr#8           @ r3 = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]^rand^c1^d1 | (junk<<8)
 strb r3,[r6,r7]                @ shareB'[perm[r0]^2] = lut_b[shareA[perm[r0]]^b0^b1^shareB[perm[r0]^2]]^rand^c1^d1
 bpl 1b
 clear03 8                      @ barrier

 ldmia r6,{r8-r11}              @ Read state share B back from memory
 clear03 12                     @ barrier
 getchaffaddress r0,16
 bfi r0,r5,#0,#4                @ match chaff pointer (r0) to share A location (R5) mod 16
 @ldmia r5,{r4-r7}               @ Read state share A back from memory
 @clear03 16                     @ barrier
 ldr r4,[r5]                    @ Intersperse barriers to prevent HW(o0^o1)+cyclic being broadcast; see similar note re stores at the start of map_sbox_s
 ldr r1,[r0]
 ldr r6,[r5,#8]
 ldr r1,[r0,#8]
 ldr r7,[r5,#12]
 ldr r1,[r0,#12]
 ldr r5,[r5,#4]                 @ Do r5 last because it's the address register
 ldr r1,[r0,#4]

@ Refresh state shares because luts only give imperfect share-by-value
@ Probably not necessary now that we use R11 and input-reuse random resharing during the sbox operation (though the R11 bytes are not fully independent)
@ loadlfsr
@ steplfsr; eors r4,r4,r0; mov r12,#0; eor r8,r8,r0,ror#16              @ Barriers between each pair of eors to prevent implicit r4^r8 etc
@ steplfsr; eors r5,r5,r0; mov r12,#0; eor r9,r9,r0,ror#16
@ steplfsr; eors r6,r6,r0; mov r12,#0; eor r10,r10,r0,ror#16
@ steplfsr; eors r7,r7,r0; mov r12,#0; eor r11,r11,r0,ror#16
@ savelfsr
.endm

.if PICO_AES_INLINE_MAP_SBOX_S
.macro inline_map_sbox_s
map_sbox_s_starts:
 // push {lr}
 map_sbox_s_impl
 // pop {lr}
map_sbox_s_end:
.endm
.else
.balign 4
.thumb_func
map_sbox_s:
 GET_CANARY r12,CTAG12,3
 push {r12,r14}

 map_sbox_s_impl

 pop {r12,r14}
 CHK_CANARY r12,CTAG12,5
 bx r14
.endif

.ltorg

.balign 4
.thumb_func
randomisechaff:
@ Randomise 48 bytes of chaff values (random load values)
@ Uses 12 bytes of permscratch
@ Trashes r0-3
 GET_CANARY r0,CTAG13,6
 push {r0,r14}
 movs r0,#12
 ldr r1,=permscratch
 bl makesmallperm           @ Store the random words in a random order to make 2nd order attacks harder
 movs r1,#11
1:
 push {r1}
 bl gen_rand_sha_nonpres
 pop {r1}
 ldr r2,=permscratch
 ldrb r2,[r2,r1]
 getchaffaddress r3
 str r0,[r3,r2,lsl#2]
 subs r1,r1,#1
 bpl 1b
 pop {r0,r14}
 CHK_CANARY r0,CTAG13,6
 bx r14

.balign 4
refreshchaff_and_lfsr:
@ Update 48 bytes of chaff values (random load values) using faster RNG than used for randomisechaff
@ Re-randomise LFSR with SHA
@ Uses 12 bytes of permscratch
@ Trashes r0-3,12
 GET_CANARY r0,CTAG14,6
 push {r0,r14}

@ Refresh LFSR using SHA to make it harder to reverse-engineer LFSR sequence
 bl gen_rand_sha_nonpres
 ldr r1,=rstate_lfsr
 ldr r2,[r1]
1:
 adds r2,r2,r0
@ note that r2 should not be 0 on entry, so both
@ r2 + r0, and r2 + r0 + r0 on the next loop should not both be 0
@ if they are, we will loop
 beq 1b           @ Don't update LFSR state to 0
#if PICO_AES_HARDENING
 beq 1b
#endif
 str r2,[r1]

@ Choose a random order to update chaff words to make 2nd order attacks harder
 movs r0,#12
 ldr r1,=permscratch
 bl makesmallperm

 movs r1,#11
1:
 push {r1}
 bl gen_rand_lfsr_nonpres
 pop {r1}
 ldr r2,=permscratch
 ldr r3,=chaff
 ldrb r2,[r2,r1]
 ldr r12,[r3,r2,lsl#2]
 add r0,r0,r12
 str r0,[r3,r2,lsl#2]
 subs r1,r1,#1
 bpl 1b
 pop {r0,r14}
 CHK_CANARY r0,CTAG14,6
 bx r14

.balign 4
.thumb_func
@ Do sbox on the four bytes of the 4-way share r4-r7
@ Trashes r0,r8-r12
init_key_sbox:
 GET_CANARY r12,CTAG15,6
 push {r1-r3,r12,r14}
 bl gen_rand_sha_nonpres; mov r8,r0
 bl gen_rand_sha_nonpres; mov r9,r0
 bl gen_rand_sha_nonpres; mov r10,r0
 bl gen_rand_sha_nonpres; mov r11,r0
 ldr r0,=fourway                @ Write out 4-way share to memory
 stmia r0,{r8-r11}              @ Save random values first to obscure saving of state
 stmia r0,{r4-r7}
 movs r4,#0                     @ Clear r4-r7 so that they don't interact with makesmallperm
 movs r5,#0
 movs r6,#0
 movs r7,#0

 bl randomisechaff              @ Randomise block of memory mainly used for obscuring loads

 movs r0,#4
 ldr r1,=permscratch
 bl makesmallperm               @ Build random 4-way permutation determining order of bytes to be SBOXed
 ldr r1,=permscratch            @ Write out random addresses in advance to save two registers (reusing permscratch)
 ldr r4,[r1]
 ldr r0,=fourway
 uxtab r5,r0,r4
 uxtab r6,r0,r4,ror#8
 uxtab r7,r0,r4,ror#16
 uxtab r8,r0,r4,ror#24
 stmia r1,{r5-r8}               @ Store at r1=permscratch: fourway+perm[0], fourway+perm[1], fourway+perm[2], fourway+perm[3]

 bl gen_rand_sha                @ Save some randomness for the resharing operation later
 movs r7,r0
 bl gen_rand_sha
 movs r8,r0

 ldr r2,=lut_a
 ldr r3,=lut_b
 ldr r0,[r2,#0x100]             @ R0 = a0 | a1<<8 | c0<<16 | c1<<24   (lut_a_map)
 eors r10,r0,r0,lsr#8
 uxtb r10,r10                   @ R10 = a0^a1
 ldr r1,[r3,#0x100]             @ R1 = b0 | b1<<8 | d0<<16 | d1<<24   (lut_b_map)
 eors r1,r0,r1
 eors r4,r1,r1,lsr#8
 uxtb r11,r4                    @ R11 = a0^a1^b0^b1
 eor r10,r10,r11,lsl#8          @ R10 = a0^a1 | (a0^a1^b0^b1)<<8
 movs r12,r1,ror#16             @ R12 = c0^d0 | (c1^d1)<<8 | junk<<16 | junk<<24

 ldr r1,=permscratch
 ldr r11,=chaff
@ Using r1=permutedfourwaypointer, r2=lut_a, r3=lut_b, r7,r8=randomness, r10=(a0^a1)|(a0^a1^b0^b1)<<8, r11=chaff, r12=(c0^d0)|(c1^d1)<<8|junk
1:
 ands r5,r1,#12
 adds r5,r11,r5                 @ Align chaff address to r1
 ldr  r6,[r1],#4                @ r6 = fourway + perm[i] (i=0-3, loop iteration)
 ldr  r5,[r5]                   @ Random load to mask previous load

 ands r9,r6,#12
 add  r9,r11,r9                 @ r9 = chaff address aligned to (r6 bic 3) mod 16
 ldrb r4,[r6,#0]
 ldr  r14,[r9,#0]               @ Random load to mask previous load
 eor  r4,r4,r10
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31

 ldrb r5,[r6,#4]
 ldr  r14,[r9,#4]               @ Random load to mask previous load
 eors r4,r4,r5
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31

 ldrb r5,[r6,#8]
 ldr  r14,[r9,#8]               @ Random load to mask previous load
 eors r4,r4,r5
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31

 ldrb r5,[r6,#12]
 ldr  r14,[r9,#12]              @ Random load to mask previous load
 eors r4,r4,r5                  @ r4 = unsharedbyte[perm[i]]^a0^a1 | junk
 eor  r4,r4,r14,lsl#8           @ Add in some junk in bits 8-31

 ands r14,r4,#255
 ldrb r5,[r2,r14]               @ r5 = lut_a[unsharedbyte[perm[i]]^a0^a1]
 and  r14,r4,#15
 add  r14,r14,#32
 ldrb r14,[r11,r14]             @ Random load to mask previous load (r2 and r11 are both 0 mod 16)
 eors r5,r5,r12                 @ r5 = lut_a[unsharedbyte[perm[i]]^a0^a1]^c0^d0 | junk<<8 | junk<<16 | junk<<24
@ split r5 into two shares and store at [r6,#0] and [r6,#4]
 strb r7,[r6,#0]
 eors r5,r5,r7
 strb r5,[r6,#4]

 mov r5,r10,lsr#8               @ r5=a0^a1^b0^b1
 ldr  r14,[r11,#44]             @ Need to eor into a random destination register
 eors r14,r4,r5                 @ r14 = unsharedbyte[perm[i]]^b0^b1 | junk<<8
 and r14,r14,#255

 ldrb r5,[r3,r14]               @ r5 = lut_b[unsharedbyte[perm[i]]^b0^b1]
 and  r14,r14,#15
 add  r4,r11,#24
 ldrb r14,[r4,r14]              @ Random load to mask previous load (r3==8 and r11==0 mod 16)
 eor  r5,r5,r12,ror#8           @ r5 = lut_b[unsharedbyte[perm[i]]^b0^b1]^c1^d1 | junk<<8 | junk<<16 | junk<<24
@ split r5 into two shares and store at [r6,#8] and [r6,#12]
 strb r8,[r6,#8]
 eors r5,r5,r8
 strb r5,[r6,#12]

 movs r7,r7,ror#8
 movs r8,r8,ror#8

 tst r1,#12                     @ This does 4 loop iterations because permscratch is guaranteed to be 0 mod 16
 bne 1b

 ldr r0,=fourway
 ldmia r0,{r4-r7}               @ Load SBOXed values back into register r4-r7
 ldmia r11,{r8-r12,r14}         @ Random load to mask previous load and to obfuscate registers

 pop {r1-r3,r12,r14}
 CHK_CANARY r12,CTAG15,6
 bx r14

.balign 4
.thumb_func
@ r1 = pointer to 4 x 4-way share (16 words); left unchanged
@ r3 = rkey_s+40*roundkeynumber; advanced by 40
@ Trashes r8-r12
@ If i = word number 0..3,
@ Aptr=memory word pointer to block of 20 bytes containing H&V-rotated share A roundkey (similarly B), then
@ vpermA=Aptr[4]>>30, vpermB=Bptr[4]>>30, and
@ roundkey shareA(i) = Aptr[i+vpermA mod 4] ror #((i+vpermA mod 4)^th byte of Aptr[4])
@ roundkey shareB(i) = Bptr[i+vpermB mod 4] ror #((i+vpermB mod 4)^th byte of Bptr[4])+16
storeroundkey:
 GET_CANARY r8,CTAG16,6
 push {r2,r8,r14}

@ eor two 4-way share components to make a component of a 2-way share
@ Note that we load from 4-way share at a random address then convert to 2-way share and
@ store at a fixed address, rather than the other way around, so that 2-way shares are obscured
@ by vperm (we don't know which 2-way share is being processed at a particular point in time).
@ And (if RK_ROR) we rotate first before EORing down to 2-way, so there is never an unrotated 2-way share

 bl gen_rand_sha             @ Get r0 = vperm for shareA of the round key
 str r0,[r3,#16]
 mov r8,r0,lsr#30
 rsb r8,r8,#0                @ r8=-vperm
.if RK_ROR
 movs r2,#0
 usub8 r2,r2,r0              @ r2=-hperms
.endif
 mov r9,#4
1:
 and r8,r8,#3
 adds r0,r1,r8,lsl#4

 ldmia r0,{r10,r11}
.if RK_ROR
 mov r10,r10,ror r2
 mov r11,r11,ror r2
 movs r2,r2,ror#8
.endif
 eor r10,r10,r11
 str r10,[r3],#4
 add r8,r8,#1
 subs r9,r9,#1
 bne 1b

 adds r1,r1,#8
 adds r3,r3,#4               @ skip over vperm (already stored)

 bl gen_rand_sha             @ Get r0 = vperm for shareB of the round key
 str r0,[r3,#16]
 mov r8,r0,lsr#30
 rsb r8,r8,#0                @ r8=-vperm
.if RK_ROR
 movs r2,#0
 usub8 r2,r2,r0              @ r2=-hperms
.endif
 mov r9,#4
 ldr r12,=RKshareC
 ldr r12,[r12]
1:
 and r8,r8,#3
 adds r0,r1,r8,lsl#4
 ldmia r0,{r10,r11}
 eor r10,r10,r12             @ Mix in RKshareC into round key shareB
.if RK_ROR
 mov r10,r10,ror r2
 mov r11,r11,ror r2
 movs r2,r2,ror#8
.endif
 mov r10,r10,ror#16
 mov r11,r11,ror#16
 eor r10,r10,r11
 str r10,[r3],#4
 add r8,r8,#1
 subs r9,r9,#1
 bne 1b

 subs r1,r1,#8               @ Restore r1 = (r1 on entry)
 adds r3,r3,#4               @ Set     r3 = (r3 on entry) + 40

 pop {r2,r8,r14}
 CHK_CANARY r8,CTAG16,6
 bx r14

.balign 4
.thumb_func
init_key_4way:
@ On entry, r0 points to 4-way shared raw key data (64 bytes, 64 byte gap for FIB workaround, then other 64 bytes)
@ The format is a0 b0 c0 d0 a1 b1 c1 d1 ... a7 b7 c7 d7
@ That is, each word, K, of the original 256-bit key is expanded into four words whose exclusive OR is K.
@
@ On exit, rkeys_s, a 40*15=600-byte region, is filled as follows.
@ Each of the 15 round keys is represented as two 5-word regions rka[0..4] and rkb[0..4],
@ each of which consists of 4 words of round key followed by a word encoding vperm and rotation (RK_ROR) information.
@ In addition a common share word, RKshareC, is set randomly.
@ For a given round, rk[i] = the i^th word of the actual round key is given by:
@ vpermA=rka[4]>>30
@ vpermB=rkb[4]>>30
@ rka_unrot[i] = rka[i+vpermA mod 4] ror #((i+vpermA mod 4)^th byte of rka[4])
@ rkb_unrot[i] = rkb[i+vpermB mod 4] ror #((i+vpermB mod 4)^th byte of rkb[4])+16
@ rk[i] = rka_unrot[i] ^ rkb_unrot[i] ^ RKshareC

 GET_CANARY r12,CTAG17,6
 push {r0-r12,r14}

@ Transfer 4-way key into local workspace, rerandomising the shares
 mov r5,r0                   @ r5=4-way key input
 bl randomisechaff
 ldr r6,=rkey4way
 movs r7,#8
1:
#if PICO_AES_FIB_WORKAROUND
 cmp r7,#4
 bne 2f
 adds r5,#64                @ Skip 64 byte gap for FIB workaround
2:
#endif
 ldmia r5!,{r1-r4}
 bl gen_rand_sha; eors r1,r1,r0; eors r4,r4,r0
 bl gen_rand_sha; eors r2,r2,r0; eors r4,r4,r0
 bl gen_rand_sha; eors r3,r3,r0; eors r4,r4,r0
 stmia r6!,{r1-r4}
 subs r7,r7,#1
 bne 1b

@ Now raw key is stored in rkey4way[], construct 2-way share in rkey_s[] for
@ the 128-bit roundkeys 0 and 1, then expand from 2 to 15 roundkeys.
 bl gen_rand_sha_nonpres
 ldr r12,=RKshareC
 str r0,[r12]                @ Make RKshareC random word
 ldr r3,=rkey_s              @ r3=rkey_s
 ldr r1,=rkey4way            @ r1=rkey4way
 bl storeroundkey            @ Store round key 0 and advance r3 by 40
 adds r1,r1,#64
 bl storeroundkey            @ Store round key 1 and advance r3 by 40
 adds r1,r1,#48
 ldmia r1!,{r4-r7}           @ r4-r7 = 4-way share of previous round key word
                             @ r1=rkey4way+128 on entry to main loop
 movs r2,#0                  @ r2=word counter (0-51), offset from word 8

@ Note that r1-r3 are not sensitive values, so it's safe to stack
@ them and conditionally branch on them.

@ rkey4way = 8 x 4 consecutive 4-way share words as cyclic buffer of
@   Rounds 0,1     Rounds 2,3            Rounds 12,13       Round 14
@   a0 b0 c0 d0 -> a8 b8 c8 d8 -> ... -> a48 b48 c48 d48 -> a56 b56 c56 d56
@   a1 b1 c1 d1 -> a9 b9 c9 d9           a49 b49 c49 d49    a57 b57 c57 d57
@   a2 b2 c2 d2    etc                   a50 b50 c50 d50    a58 b58 c58 d58
@   a3 b3 c3 d3                          a51 b51 c51 d51    a59 b59 c59 d59
@   a4 b4 c4 d4                          a52 b52 c52 d52    ===============
@   a5 b5 c5 d5                          a53 b53 c53 d53
@   a6 b6 c6 d6                          a54 b54 c54 d54
@   a7 b7 c7 d7                          a55 b55 c55 d55

init_key_expandloop:
@ r1 = pointer past one of eight 4-way shares of a roundkey word in the above cyclic buffer (r1=rkey4way+16i for i=1,...,8)
@ r2 = round key word counter (0-51), offset from word 8 (counting expanded roundkey words)
@ r3 = pointer to rkey_s+40*roundnumber = rkey_s+40*(2+[r2/4])
@ r4-r7 = 4-way share of previous roundkey word

 tst r2,#7
 bne 1f
 subs r1,r1,#128             @ Every 8th word, reset cyclic buffer pointer and do ROTWORD
 movs r4,r4,ror#8
 movs r5,r5,ror#8
 movs r6,r6,ror#8
 movs r7,r7,ror#8
1:

 tst r2,#3
 bne 1f
 bl init_key_sbox            @ Every 4th word, do SUBBYTES (sbox) on r4-r7
1:

 tst r2,#7
 bne 1f
 movs r0,r2,lsr#3
 mov r8,#1
 movs r8,r8,lsl r0
 eors r4,r4,r8               @ Every 8th word, add in round constant
1:

 ldmia r1,{r8-r11}           @ eor with key from two rounds ago and advance r1 by 16
 eors r4,r4,r8
 eors r5,r5,r9
 eors r6,r6,r10
 eors r7,r7,r11
 stmia r1!,{r4-r7}

 add r2,r2,#1
 tst r2,#3
 bne 1f
 subs r1,r1,#64
 bl storeroundkey            @ Store round key 1+r2/4 and advance r3 by 40
 adds r1,r1,#64
1:

 cmp r2,#52
 bne init_key_expandloop

 CHK_COUNT 30,6
 pop {r0-r12,r14}
 CHK_CANARY r12,CTAG17,6
 bx r14

.ltorg

@ Add the round key shares pointed to by r12 into the state shares
@ Trashes r0-r3
.balign 4
addrkey_s:

 ldr r0,=chaff               @ guaranteed 0 mod 16
.if ST_VPERM
 ldr r3,=statevperm
 ldr r3,[r3]                 @ r3=vperm state rotation in bottom two bits
 ldr r2,[r0,#12]             @ barrier load
.else
 movs r3,#0
.endif
 bfi r0,r12,#0,#4            @ match chaff pointer (r0) to roundkey ptr (r12) mod 16
 ldr r1,[r12,#16]            @ r1=vperm key rotation in top two bits
 ldr r2,[r0,#16]             @ barrier load

 rsb r2,r3,r1,lsr#30         @ r2=vpermkeyrot-vpermstaterot
@ Read shareA of roundkey, offset by vpermkeyrot-vpermstaterot, and eor it into shareA of state, offset by -vpermstaterot
@ r1=rkeyArotdata, r2=vpermkeyrot-vpermstaterot, r3=statevperm, r4-r11=state, r12=roundkeyAptr
.if RK_ROR
 movs r0,r2,lsl#3
 movs r1,r1,ror r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; adds r2,r2,#1;                   rors r0,r0,r1; eors r4,r4,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; adds r2,r2,#1; movs r1,r1,ror#8; rors r0,r0,r1; eors r5,r5,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; adds r2,r2,#1; movs r1,r1,ror#8; rors r0,r0,r1; eors r6,r6,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2];                movs r1,r1,ror#8; rors r0,r0,r1; eors r7,r7,r0
.else
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; adds r2,r2,#1; eors r4,r4,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; adds r2,r2,#1; eors r5,r5,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; adds r2,r2,#1; eors r6,r6,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2];                eors r7,r7,r0
.endif
 clear03_preserve_r3
 add r12,r12,#20
 @ r0=chaff+16, r3=statevperm, r4-r11=state, r12=roundkeyBptr

 bfi r0,r12,#0,#4            @ match chaff pointer (r0) to roundkey ptr (r12) mod 16
 ldr r1,[r12,#16]            @ r1=vperm key rotation in top two bits
 ldr r2,[r0,#16]             @ barrier load
 rsb r2,r3,r1,lsr#30         @ r2=vpermkeyrot-vpermstaterot
 ldr r3,=RKshareC            @ r3=common round key shareC
 bfi r0,r3,#0,#4
 ldr r3,[r3]
 ldr r0,[r0]                 @ barrier load

@ Read shareB of roundkey, offset by vpermkeyrot-vpermstaterot, and eor it into shareB of state, offset by -vpermstaterot
@ r1=rkeyBrotdata, r2=vpermkeyrot-vpermstaterot, r3=RKshareC, r4-r11=state, r12=roundkeyB ptr
.if RK_ROR
 movs r0,r2,lsl#3
 movs r1,r1,ror r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r8,r8,r3,ror#16;   adds r2,r2,#1;                   rors r0,r0,r1; eor r8,r8,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r9,r9,r3,ror#16;   adds r2,r2,#1; movs r1,r1,ror#8; rors r0,r0,r1; eor r9,r9,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r10,r10,r3,ror#16; adds r2,r2,#1; movs r1,r1,ror#8; rors r0,r0,r1; eor r10,r10,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r11,r11,r3,ror#16;                movs r1,r1,ror#8; rors r0,r0,r1; eor r11,r11,r0
.else
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r8,r8,r3,ror#16;   adds r2,r2,#1; eors r8,r8,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r9,r9,r3,ror#16;   adds r2,r2,#1; eors r9,r9,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r10,r10,r3,ror#16; adds r2,r2,#1; eors r10,r10,r0
 ands r2,r2,#3; ldr r0,[r12,r2,lsl#2]; eor r11,r11,r3,ror#16;                eors r11,r11,r0
.endif
 clear03
 bx r14

.balign 4
.thumb_func
@ de/encrypt data in place
@ r0: ivec
@ r1: buf
@ r2: n, number of blocks, n>0
.if CT_BPERM
@ In AES-CTR each block can be independently en/decrypted as the encryption only depends on the IV,
@ the key, and the block number. We can therefore process them in any order, and using a
@ random order helps to defeat attacks that work on the output of the AES, since an attacker
@ wouldn't know what plaintext or ciphertext corresponds to a particular instruction.
.endif

ctr_crypt_s:
@ r0=IV_shareA, r1=IV_shareB, r2=cipher/plaintext buffer, r3=number of blocks
 GET_CANARY r12,CTAG0,6
 push {r0-r12,r14}           @ save all registers so that when we restore we overwrite any secrets

 push {r0-r3}

@ Count should be initted in pico_aes_decrypt_internal - ctr_crypt_s shouldn't be called directly
@ #if !CALLER_INIT_RCP_COUNT
@  SET_COUNT 33,6
@ #endif

.if CT_BPERM
@ Initialise 32 random numbers (which fit in half-words)
@ r3=number of blocks
 ldr r4,=bperm_rand
 movs r5,#32
1:
 bl gen_rand_sha
 umull r0,r2,r0,r3        @ Random number between 0 and n-1 (n=#blocks)
 strh r2,[r4],#2
 subs r5,r5,#1
 bne 1b
.endif

 bl randomisechaff

@ Refresh IVshareA and IVshareB, convert to ror#16 format and store the result at IV0
@ Not doing shareC or state vperm at this point
 pop {r0}
 ldmia r0,{r4-r7}         @ r4-r7 = IVshareA
 clear03 16
 pop {r1}
 ldmia r1,{r8-r11}        @ r8-r11 = IVshareB
 clear03 32
 bl gen_rand_sha_nonpres; eors r4,r4,r0; movs r1,#0; mov r8, r8, ror#16; eor r8, r8, r0,ror#16   @ Barriers between shares to prevent implicit r4^r8 etc
 bl gen_rand_sha_nonpres; eors r5,r5,r0; movs r1,#0; mov r9, r9, ror#16; eor r9, r9, r0,ror#16
 bl gen_rand_sha_nonpres; eors r6,r6,r0; movs r1,#0; mov r10,r10,ror#16; eor r10,r10,r0,ror#16
 bl gen_rand_sha_nonpres; eors r7,r7,r0; movs r1,#0; mov r11,r11,ror#16; eor r11,r11,r0,ror#16
 ldr r0,=IV0
 stmia r0!,{r4-r7}
 adds r1,r0,#4
 stmia r1,{r8-r11}
@ "Decommission" IV0 so that it doesn't get stacked
#if 1 // approved by Alex - no side channel leakage it seems
#if PICO_AES_HARDENING
 // if this is skipped, r4 is likely random, so more 1 in 4 chance that ldmia will trap
 // in any case very unlikely to load useful data below (and presuambly the faulting address is uninteresting
 // since it is already XORed with random data above)
 movs r0, #32
 // note if r1 is unset, then we are reading from lut_a
 movs r1, #0
 ldmia r1!, {r4, r5, r6, r7, r8, r9, r10, r11}
 rcp_iequal_nodelay r0, r1
#else
 movs r0, #0
 ldmia r0, {r4, r5, r6, r7, r8, r9, r10, r11}
#endif
#else
 bl gen_rand_sha_nonpres; movs r4,r0
 bl gen_rand_sha_nonpres; movs r5,r0
 bl gen_rand_sha_nonpres; movs r6,r0
 bl gen_rand_sha_nonpres; movs r7,r0
 bl gen_rand_sha_nonpres; mov  r8,r0
 bl gen_rand_sha_nonpres; mov  r9,r0
 bl gen_rand_sha_nonpres; mov r10,r0
 bl gen_rand_sha_nonpres; mov r11,r0
#endif
@ Trashes r0, r1
 check_rnd_count (RND_COUNT_decrypt+RND_COUNT_ctr_crypt_s_init)
 pop {r1,r2}
@ r1=cipher/plaintext buffer, r2=number of blocks

 movs r3,#0
 CHK_COUNT 33,6

ctr_crypt_mainloop:
 SET_COUNT 80,6
@ r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter

@ Do as much preparatory stuff as possible that doesn't involve the IV (to reduce interaction with it)
 push {r1-r3}
@ It's OK for execution time to depend on the block counter r3 ("public"), but not the block number (secret)

@ Trashes r0, r1
 reset_rnd_count_checked

 // no point in having a branch if we should never take it (hardening/size fail)
#if REFCHAFF_PERIOD != 1
 tst r3,#(REFCHAFF_PERIOD-1)
 bne 1f
#endif
 bl refreshchaff_and_lfsr
1:
 ldr r3,[sp,#8]             @ get block count off the stack
 // no point in having a branch if we should never take it (hardening/size fail)
#if REMAP_PERIOD != 1
 tst r3,#(REMAP_PERIOD-1)
 bne 1f
#endif
 bl remap                    @ shuffle the LUTs; this preserves R3
1:

 CHK_COUNT 80,6
 ldr r0,[sp,#8]             @ get block count off the stack
#if PICO_AES_HARDENING
@ We check the random counts here. Note we start with the combined count and subtract, just because
@ it might make it marginally more difficult to get the right answer if skipping multiple instructions
 movs r1, #(RND_COUNT_remap + RND_COUNT_refreshchaff_and_lfsr)
#if REMAP_PERIOD != 1
 tst r0, #(REMAP_PERIOD-1)
 it ne
 subne r1, #RND_COUNT_remap
#endif
#if REFCHAFF_PERIOD != 1
 tst r0, #(REFCHAFF_PERIOD-1)
 it ne
 subne r1, #RND_COUNT_refreshchaff_and_lfsr
#endif
@ r0=block count, r1=expected sha rand count, r3=block count
 rcp_iequal_nodelay r0, r3
@ r1=expected sha rand count, r3=block count
 check_rnd_count_dynamic
#endif // PICO_AES_HARDENING
@ r3=block count

@ No point in having a branch if we should never take it (hardening/size fail)
#if REFROUNDKEYSHARES_PERIOD != 1
#if PICO_AES_HARDENING
// we want to check that we are calling enough
#warning REFROUNDKEYSHARES_PERIOD check needs hardening
#endif
 tst r3,#(REFROUNDKEYSHARES_PERIOD-1)
 bne skip_ref_roundkey_shares_s
#endif
#if PICO_AES_INLINE_REF_ROUNDKEY_SHARES_S
 inline_ref_roundkey_shares_s
#else
#if PICO_AES_HARDENING
 // todo graham we could remove this for space, as I don't think r4 and r5 are equal
@ Make sure r4 != r5 on entry to ref_roundkey_shares_s
 subs r4, r5, #1
#endif
 bl ref_roundkey_shares_s    @ refresh the round key shares
#if PICO_AES_HARDENING
@ r4 and r5 are set equal by ref_roundkey_shares (note we don't do a rnd_check as no sha random numbers are generated)
 rcp_iequal_nodelay r4, r5
#endif
#endif
skip_ref_roundkey_shares_s:

#if REFROUNDKEYHVPERMS_PERIOD != 1
#if PICO_AES_HARDENING
// we want to check that we are calling enough
#warning REFROUNDKEYHVPERMS_PERIOD check needs hardening
#endif
 ldr r3,[sp,#8]             @ get block count off the stack
 tst r3,#(REFROUNDKEYHVPERMS_PERIOD-1)
 bne skip_ref_roundkey_hvperm_s
#endif
#if PICO_AES_INLINE_REF_ROUNDKEY_HVPERMS_S
 inline_ref_roundkey_hvperms_s
#else
 bl ref_roundkey_hvperms_s   @ refresh the round key vperms
#if PICO_AES_HARDENING
 movs r0, #30
@ r7 should be 30 on exit from ref_roundkey_hvperms_s
 rcp_iequal_nodelay r0, r7
#endif
#endif
skip_ref_roundkey_hvperms_s:

 CHK_COUNT 81,6

@ Trashes r0, r1
 reset_rnd_count
 pop {r1-r3}
@ r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter

@ Now calculate r12 = block number-to-be-deciphered from r3 = block counter
.if CT_BPERM
@ Use a "swap-or-not" method to generate an "oblivious" permutation; see makeperm.py version 7
 push {r1}
 ldr r0,=murmur3_constants
 ldmia r0,{r9-r12,r14}       @ load five murmur3_32 hash constants
 ldr r0,=bperm_rand
 movs r1,#31
 movs r4,r3                  @ r4=i
1:
 ldrh r5,[r0],#2             @ r5=k
 subs r5,r5,r4               @ r5=k-i
 ands r6,r2,r5,asr#31        @ r6=n*(k-i<0)
 adds r5,r5,r6               @ r5=j=(k-i)%n
 adds r6,r4,r5               @ r6=i+j
 subs r7,r4,r5               @ r7=i-j
 and  r8,r7,r7,asr#31        @ r8=min(i-j,0)
 sub  r7,r7,r8,lsl#1         @ r7=|i-j|
 mla  r6,r6,r2,r7            @ r6=n(i+j)+|i-j|, encodes the unordered pair {i,j}
 eors r6,r6,r1,lsl#27        @ mix with swap-or-not round counter to get different hash functions
@ Now do murmur3_32 hash of r6
 mul  r6,r6,r9
 movs r6,r6,ror#17
 mul  r6,r6,r10
 movs r6,r6,ror#19
 adds r6,r6,r6,lsl#2
 add  r6,r6,r11
 eors r6,r6,#4
 eors r6,r6,r6,lsr#16
 mul  r6,r6,r12
 eors r6,r6,r6,lsr#13
 mul  r6,r6,r14
 eors r6,r6,r6,lsr#16        @ not actually used here
@ Now set i to j, conditional on the top bit of r6
 subs r7,r5,r4               @ r7=j-i
 ands r7,r7,r6,asr#31        @ r7=(j-i)*(top bit of r6)
 adds r4,r4,r7               @ r4=j if top bit of r6, else i
 subs r1,r1,#1
 bpl 1b
 // tooo loop check
 pop {r1}
 mov r12,r4
.else
 mov r12,r3
.endif
 CHK_COUNT 82,6

@ r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter (monotonic), r12=block number (block to be deciphered)
 push {r1-r3,r12}
@ r4-r11 = IV0, r12=block number

processIV:                   @ non-target label to assist power analysis
 ldr r8,=IV0
 ldmia r8,{r4-r7}            @ load IV0_A
 clear03 16
 add r8,r8,#20
 ldmia r8,{r8-r11}           @ load IV0_B
 clear03 32
 rev r0,r12
 eor r7,r7,r0                @ XOR in block number to IV0. IV(block n) = IV0 ^ n, cf standard CTR mode IV0 + n.
                             @ XOR (vs addition) is compatible with XOR-shares, so stealthier/simpler because don't have to unshare to work out IV(block n)
@ r4-r11 = IV for the current block
 CHK_COUNT 83,6
.if ST_SHAREC
 bl gen_rand_sha_nonpres     @ Create state share C; all bytes the same
 ands r0,r0,#255
 orrs r0,r0,r0,lsl#8
 orrs r12,r0,r0,lsl#16
 ldr r1,=shareC
 str r12,[r1]
.else
 movs r12,#0
.endif
@ r4-r11 = IV for the current block w/o shareC, r12=shareC
@ refresh state shares and mix in shareC
 bl gen_rand_sha_nonpres; eors r4,r4,r0; eor r4,r4,r12; movs r1,#0; eor r8, r8, r0,ror#16   @ Barriers between shares to prevent implicit r4^r8 etc
 bl gen_rand_sha_nonpres; eors r5,r5,r0; eor r5,r5,r12; movs r1,#0; eor r9, r9, r0,ror#16
 bl gen_rand_sha_nonpres; eors r6,r6,r0; eor r6,r6,r12; movs r1,#0; eor r10,r10,r0,ror#16
 bl gen_rand_sha_nonpres; eors r7,r7,r0; eor r7,r7,r12; movs r1,#0; eor r11,r11,r0,ror#16
.if ST_VPERM
 bl gen_rand_sha_nonpres
 ldr r1,=statevperm
 movs r2,#0
 str r2,[r1]
 bl addstatevperm            @ Initialise state vperm (use SHA RNG to start with, later refreshes are with LFSR RNG)
#if PICO_AES_HARDENING
 // r1 is set to lut_b by addstatevperm
 ldr r0, =shareB + 0x10
 rcp_iequal_nodelay r0, r1
#endif
.endif

@ Trashes r0, r1
 check_rnd_count RND_COUNT_ctr_crypt_mainloop_A
 CHK_COUNT 84,6
.if ST_SHAREC                @ Avoid func call if the func is empty
 bl conjshareC               @ Add the effect of shareC to lut_a, lut_b
#if PICO_AES_HARDENING
 // r1 is set to lut_b by conjshare
 ldr r2,=lut_b
 rcp_iequal_nodelay r1, r2
#endif
.endif
 // todo graham remove this count
 CHK_COUNT 85,6
@ now perform the 15 encryption rounds on (key, state=IV+x)
@ here r4-r7, r8-r11: state
 mov r2,#0                   @ round counter
rounds_s_mainloop:
@ Trashes r0, r1
 reset_rnd_count_checked
 ldr r12,=rkey_s
 add r12,r12,r2,lsl#5        @ pointer to key shares for this round
 add r12,r12,r2,lsl#3
 push {r2}                   @ save round count
 bl addrkey_s
.if PICO_AES_INLINE_MAP_SBOX_S
 inline_map_sbox_s
.else
 bl map_sbox_s
.endif
.if PICO_AES_INLINE_SHIFT_ROWS_S
 inline_shift_rows_s
.else
 bl shift_rows_s
.endif
.if ST_VPERM
 ldr r2,[sp]                @ peek at stack to get round count
 cmp r2,#NUMREFSTATEVPERM
 bcs 1f
 bl gen_rand_lfsr_nonpres
 ldr r1,=statevperm
 bl addstatevperm            @ V shuffle of r4-r11
#if PICO_AES_HARDENING
 // r1 is set to lut_b by addstatevperm
 ldr r2, =shareB + 0x10
 rcp_iequal_nodelay r1, r2
#endif
1:
.endif
 pop {r2}
 adds r2,r2,#1               @ increment round counter
 cmp r2,#14
 beq 2f                      @ break from loop? (last round has no mix_cols)
 push {r2}
 bl mix_cols_s
 pop {r2}
 b rounds_s_mainloop
2:
#if PICO_AES_HARDENING
 movs r1, #14
 rcp_iequal_nodelay r1, r2
#endif
 CHK_COUNT 86,6
 ldr r12,=rkey_s+14*40       @ final round key shares
 // todo graham check this is called
 bl addrkey_s
 CHK_COUNT 87,6
.if ST_SHAREC                @ Avoid func call if the func is empty
 // todo alex, i assume that skipping this will cause bad things to happen anyway?
 bl conjshareC               @ Undo the effect of shareC from lut_a, lut_b
.endif
 CHK_COUNT 88,6
.if ST_VPERM
@ Undo the effects of vperm rotation recorded in statevperm
 ldr r1,=statevperm
 ldr r2,[r1]
 rsbs r0,r2,#0
@ We don't check this is called since failing to undo this is probably going to break decryption
// todo alex is this fair?
 bl addstatevperm
.endif

 pop {r1-r3,r12}
 push {r1,r3}
@ r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter, r12=block to be deciphered

decryption_start:
@ Decrypt ciphertext using AES output in shares: r4-r11
.if ST_SHAREC
 ldr r0,=shareC
 ldr r0,[r0]
.else
 movs r0,#0
.endif
 ldr r14,=chaff
@ r0=shareC, r1=cipher/plaintext buffer, r2=number of blocks, r3=free, r4-r11=stateA/B, r12=block to be deciphered, r14=chaff
 CHK_COUNT 89,6
 add r1,r1,r12,lsl#4         @ Temporarily r1 points to block-to-be-deciphered
 ldr r3,[r1]                 @ r3=ciphertext word
 eors r3,r3,r4               @ r3=r3^shareA
 ldr r4,[r14]                @ barrier load
 eor r3,r3,r8,ror#16         @ r3=r3^shareB
 eors r3,r3,r0               @ r3=r3^shareC
 str r3,[r1]                 @ plaintext word=r3
 ldr r3,[r1,#4]              @ and similarly for words 1,2,3 of block...
 ldr r4,[r14,#4]
 eors r3,r3,r5
 eor r3,r3,r9,ror#16
 eors r3,r3,r0
 str r3,[r1,#4]
 ldr r3,[r1,#8]
 ldr r4,[r14,#8]
 eors r3,r3,r6
 eor r3,r3,r10,ror#16
 eors r3,r3,r0
 str r3,[r1,#8]
 ldr r3,[r1,#12]
 ldr r4,[r14,#12]
 eors r3,r3,r7
 eor r3,r3,r11,ror#16
 eors r3,r3,r0
 str r3,[r1,#12]

 CHK_COUNT 90,6

@ Trashes r0, r1
 check_rnd_count RND_COUNT_decryption_end

 pop {r1,r3}                  @ Restore r1 to point to start of buffer
                              @ Restore block counter
@ r1=cipher/plaintext buffer, r2=number of blocks, r3=block counter
decryption_end:

 adds r3,r3,#1
 cmp r3,r2
 CHK_COUNT 91,6
 bne ctr_crypt_mainloop
 // todo alex, is this necessary - if you don't do the right number of loops, you ain't gonna get far?
#if PICO_AES_HARDENING
 rcp_iequal_nodelay r2, r3
#endif

#if WIPE_MEMORY
@ Wipe memory from aes_workspace_start up to the stack pointer
@ First fill everything (except the RNG state itself) with random numbers to avoid any possibly useful power signals
 ldr r4,=aes_workspace_start
 add r5, r4, #rstate_all_start - aes_workspace_start
#if PICO_AES_HARDENING
 ldr r7,=aes_workspace_start
 add r6, r4, #rstate_all_start - aes_workspace_start
 rcp_iequal_nodelay r4, r7
#endif
#if PICO_AES_HARDENING
 // todo alex, is this necessary - if you don't do the right number of loops, you ain't gonna get far?
@ Recheck of above
 rcp_iequal_nodelay r3, r2
#endif
1:
 bl gen_rand_sha_nonpres
 stmia r4!,{r0}
 cmp r4,r5
 bcc 1b
#if PICO_AES_HARDENING
 rcp_iequal_nodelay r4, r6
 mov r6,sp
#endif
 // not if this load is skpped, then we are just erasing from where we left off before
.if rstate_all_end <= rstate_all_start
.err
.endif
 ldr r4,=rstate_all_end
 mov r5,sp                  @ gcc arm assembler says cmp r4,sp is deprecated, so use another register
1:
 bl gen_rand_sha_nonpres
 stmia r4!,{r0}
 cmp r4,r5
 bcc 1b
#if PICO_AES_HARDENING
 rcp_iequal_nodelay r4, r6
#endif

@ Then fill everything with zeros so as not to leave behind clues about the RNG state
 ldr r4,=aes_workspace_start
 movs r0,#0
 mov r5,sp
1:
 stmia r4!,{r0}
 cmp r4,r5
 bcc 1b
#if PICO_AES_HARDENING
 rcp_iequal_nodelay r4, r6
#endif
#endif

.if GEN_RAND_SHA
 SET_COUNT 23,6
 bl reset_sha_trng           @ clear out the SHA hardware
.endif
 pop {r0-r12,r14}
 CHK_CANARY r12,CTAG0,6
 bx r14
