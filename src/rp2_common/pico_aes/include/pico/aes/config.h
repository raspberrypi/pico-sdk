#pragma once

// These options (up to long /////////////// line) should be enabled because the security risk of not using them is too high
// or because the time cost is very low so you may as well have them.
// They can be set to 0 for analysis or testing purposes.

#ifndef GEN_RAND_SHA
#define GEN_RAND_SHA         1         // use SHA256 hardware to generate some random numbers
#endif
                                       // Some RNG calls are hard coded to LFSR RNG, others to SHA RNG
                                       // Setting GEN_RAND_SHA to 0 has the effect of redirecting the latter to LFSR RNG
#ifndef ST_SHAREC
#define ST_SHAREC            1         // This creates a partial extra share at almost no extra cost
#endif
#ifndef ST_VPERM
#define ST_VPERM             1         // insert random vertical permutations in state during de/encryption?
#endif
#ifndef CT_BPERM
#define CT_BPERM             1         // process blocks in a random order in counter mode?
#endif
#ifndef RK_ROR
#define RK_ROR               1         // store round key shares with random rotations within each word
#endif

#ifndef WIPE_MEMORY
#define WIPE_MEMORY          1         // Wipe memory after decryption
#endif

// The following options should be enabled to increase resistance to glitching attacks.

#ifndef RC_CANARY
#define RC_CANARY            1         // use rcp_canary feature
#endif
#ifndef RC_COUNT
#define RC_COUNT             1         // use rcp_count feature
#endif

// Although jitter/timing-variation may be circumventable in theory, in practice
// randomising the timing of operations can make side-channel attacks very much more
// effort to carry out. These can be disabled for analysis or testing purposes.
// It is advisable to use a least one form of jitter.

// RC_JITTER is quite slow, and is probably the most predictable of the three, so it is disabled by default.
// (Leaving it as an option because it's just possible that the large delays it produces are advantageous in defeating certain side-channel attacks.)
#ifndef RC_JITTER
#define RC_JITTER            0         // 0-7. Higher = more jitter. Governs use of random-delay versions of RCP instructions.
#endif

#ifndef SH_JITTER
#define SH_JITTER            1         // Insert random delays, tagged onto SHA RNG
#endif

#ifndef CK_JITTER
#define CK_JITTER            1         // Use the ROSC clock to make ARM timings unpredictable
#endif

#ifndef INLINE_REF_ROUNDKEY_SHARES_S
#define INLINE_REF_ROUNDKEY_SHARES_S 0
#endif

#ifndef INLINE_REF_ROUNDKEY_HVPERMS_S
#define INLINE_REF_ROUNDKEY_HVPERMS_S 0
#endif

#ifndef INLINE_SHIFT_ROWS_S
#define INLINE_SHIFT_ROWS_S 0
#endif

#ifndef INLINE_MAP_SBOX_S
#define INLINE_MAP_SBOX_S 0
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The following options can be adjusted, affecting the performance/security tradeoff

// Period = X means that the operation in question occurs every X blocks, so higher = more performance and lower security.
// No point in making them more than 16 or so, since the time taken by the subroutines would be negligible.
// These must be a power of 2. Timings as of commit 82d31652
// 
//                                        Baseline time per 16-byte block = 14109 (with no jitter)         cycles
#ifndef REFCHAFF_PERIOD
#define REFCHAFF_PERIOD             1     // Extra cost per 16-byte block =   474/REFCHAFF_PERIOD          cycles
#endif
#ifndef REMAP_PERIOD
#define REMAP_PERIOD                4     // Extra cost per 16-byte block =  4148/REMAP_PERIOD             cycles
#endif
#ifndef REFROUNDKEYSHARES_PERIOD
#define REFROUNDKEYSHARES_PERIOD    1     // Extra cost per 16-byte block =  1304/REFROUNDKEYSHARES_PERIOD cycles
#endif
#ifndef REFROUNDKEYHVPERMS_PERIOD
#define REFROUNDKEYHVPERMS_PERIOD   1     // Extra cost per 16-byte block =  1486/REFROUNDKEYVPERM_PERIOD  cycles
#endif

// Setting NUMREFSTATEVPERM to X means that state vperm refreshing happens on the first X AES rounds only,
// so lower = more performance and lower security.
// The rationale for doing it this way is that later rounds should be protected by CT_BPERM.
// NUMREFSTATEVPERM can be from 0 to 14.
#ifndef NUMREFSTATEVPERM
#define NUMREFSTATEVPERM            7     // Extra cost per 16-byte block =  61*NUMREFSTATEVPERM cycles
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_NUM_BLOCKS 32768

#if SH_JITTER && !GEN_RAND_SHA
#error GEN_RAND_SHA must be set if you want to use SH_JITTER
#endif
