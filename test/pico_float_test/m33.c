#include <stdio.h>
#include "pico/stdlib.h"

static int rc=0;

typedef uint32_t ui32;
typedef uint64_t ui64;

extern ui64 __aeabi_dadd(ui64, ui64);
extern ui64 __aeabi_dsub(ui64, ui64);
extern ui64 __aeabi_dmul(ui64, ui64);
extern ui64 ddiv_fast(ui64, ui64);
extern ui64 sqrt_fast(ui64);

#define m33cf_dadd __aeabi_dadd
#define m33cf_dsub __aeabi_dsub
#define m33cf_dmul __aeabi_dmul
#define m33cf_ddiv_fast ddiv_fast
#define m33cf_dsqrt_fast sqrt_fast
static void checkf(ui32 r,ui32 t) {
    static int n=0;
    if(r!=t) {
        printf("M33CF test F%d: expected %08x got %08x\n",n,t,r);
        rc=-1;
    }
    n++;
}

static void checkd(ui64 r,ui64 t) {
    static int n=0;
    if(r!=t) {
        printf("M33CF test D%d: expected %08x%08x got %08x%08x\n",
               n,
               (ui32)(t>>32),(ui32)t,
               (ui32)(r>>32),(ui32)r);
        rc=-1;
    }
    n++;
}

int main() {
    stdio_init_all();
    puts("M33CF function test\n");

#if 0
    checkf(m33cf_fadd(0x4d3c3e6f,0x7617944a),0x7617944a);
    checkf(m33cf_fadd(0x67819fdb,0xe6225dcc),0x675aa843);
    checkf(m33cf_fadd(0x3c9a71a0,0x22d538f1),0x3c9a71a0);
    checkf(m33cf_fadd(0xc37ab23a,0x825141a3),0xc37ab23a);
    checkf(m33cf_fadd(0x98586bf0,0x17d915be),0x97d7c222);
    checkf(m33cf_fadd(0x69572e49,0x4e650317),0x69572e49);
    checkf(m33cf_fadd(0xa67b6fbc,0x266be0aa),0xa478f120);
    checkf(m33cf_fadd(0x01d100ff,0xf3a2011c),0xf3a2011c);
    checkf(m33cf_fadd(0x01f9ed0f,0x81130914),0x01b06885);
    checkf(m33cf_fadd(0xb2ce6eb7,0x325aff32),0xb241de3c);
    checkf(m33cf_fsub(0xc8c2cfa1,0xaca3309c),0xc8c2cfa1);
    checkf(m33cf_fsub(0xe1e2656e,0x311fb841),0xe1e2656e);
    checkf(m33cf_fsub(0xbb5b7ea5,0x2e393a3c),0xbb5b7ea5);
    checkf(m33cf_fsub(0x04047b97,0x07c5cd1a),0x87c4c423);
    checkf(m33cf_fsub(0xda4c2f9b,0xda13370e),0xd963e234);
    checkf(m33cf_fsub(0x06325a1d,0xd9ccae75),0x59ccae75);
    checkf(m33cf_fsub(0x001f6b55,0x71efb885),0xf1efb885);
    checkf(m33cf_fsub(0x3eafa278,0x7d3bc3af),0xfd3bc3af);
    checkf(m33cf_fsub(0xe8203c86,0x3d93556b),0xe8203c86);
    checkf(m33cf_fsub(0xa5f8dbf1,0xc3f00090),0x43f00090);
    checkf(m33cf_fmul(0x8009dc76,0x7cb46e28),0x80000000);
    checkf(m33cf_fmul(0x2e3ba1b3,0x0fa7a191),0x00000000);
    checkf(m33cf_fmul(0xf0c99917,0xf0c7ff9a),0x7f800000);
    checkf(m33cf_fmul(0x41b2a69c,0xc14000be),0xc385fd7a);
    checkf(m33cf_fmul(0x765c15f1,0x0aad9b08),0x4195401c);
    checkf(m33cf_fmul(0xea3a2755,0xcc8ba580),0x774b1767);
    checkf(m33cf_fmul(0xe0368e14,0xbdc000de),0x5e88eb2d);
    checkf(m33cf_fmul(0x832fe859,0x8217ff9f),0x00000000);
    checkf(m33cf_fmul(0x00f19200,0xe3752d1a),0xa4e75b49);
    checkf(m33cf_fmul(0x42d1061e,0x63a8a510),0x6709b2ca);
    checkf(m33cf_fdiv_fast(0xbe891ac7,0x779000b4),0x8673bca3);
    checkf(m33cf_fdiv_fast(0x06cd4e3a,0x8075c830),0xff800000);
    checkf(m33cf_fdiv_fast(0x1fca4821,0x9f084021),0xc03e0865);
    checkf(m33cf_fdiv_fast(0x036f1348,0x715ba87b),0x00000000);
    checkf(m33cf_fdiv_fast(0x403c6064,0xbe000057),0xc1bc5fe4);
    checkf(m33cf_fdiv_fast(0x009bf1e5,0xbe5000a0),0x81bfee10);
    checkf(m33cf_fdiv_fast(0x507bac6d,0x946acd7e),0xfb89326b);
    checkf(m33cf_fdiv_fast(0x0810656a,0x880448ea),0xbf8bb812);
    checkf(m33cf_fdiv_fast(0x16e368e6,0x965dc974),0xc0033ebf);
    checkf(m33cf_fdiv_fast(0x72a7e095,0x6ba01097),0x46863f59);
    checkf(m33cf_fadd(0x83e2ed1d,0x00d5c831),0x83df95fc);
    checkf(m33cf_fadd(0xe4ddd608,0xb9587489),0xe4ddd608);
    checkf(m33cf_fadd(0x6f337823,0xc0679a9f),0x6f337823);
    checkf(m33cf_fadd(0x49a367e6,0x3f4679ba),0x49a367ec);
    checkf(m33cf_fadd(0xd1307f4d,0xaf17b226),0xd1307f4d);
    checkf(m33cf_fadd(0xbdf2106c,0x49a6a874),0x49a6a873);
    checkf(m33cf_fadd(0xf3eacf1b,0x52634887),0xf3eacf1b);
    checkf(m33cf_fadd(0x6f33647b,0x0442ab37),0x6f33647b);
    checkf(m33cf_fadd(0x4cdb9e7a,0x7d301bff),0x7d301bff);
    checkf(m33cf_fadd(0x4e1abca7,0x57441d4c),0x57441d73);
    checkf(m33cf_fsub(0x8391af63,0xfee57d3d),0x7ee57d3d);
    checkf(m33cf_fsub(0x2508aef8,0x1bd43e41),0x2508aedd);
    checkf(m33cf_fsub(0x47fe76b7,0x9131d5b0),0x47fe76b7);
    checkf(m33cf_fsub(0x5d3119e5,0xd16c2f47),0x5d3119e6);
    checkf(m33cf_fsub(0xe39f668c,0x1f0dabef),0xe39f668c);
    checkf(m33cf_fsub(0x1e6d2035,0x3bdab1d6),0xbbdab1d6);
    checkf(m33cf_fsub(0xb5fde6fa,0x1a379480),0xb5fde6fa);
    checkf(m33cf_fsub(0x225bb239,0xfeff8ff3),0x7eff8ff3);
    checkf(m33cf_fsub(0x844054a3,0x2122b279),0xa122b279);
    checkf(m33cf_fsub(0x5a04757e,0x56caed2b),0x5a02dfa4);
    checkf(m33cf_fmul(0x4cb93c51,0xd3c0275f),0xe10b09ba);
    checkf(m33cf_fmul(0x3f390c21,0x64abdb95),0x647873a6);
    checkf(m33cf_fmul(0x56bd6b48,0x04a56d68),0x1bf4ce32);
    checkf(m33cf_fmul(0xa3367c5f,0x59e18108),0xbda0bf4a);
    checkf(m33cf_fmul(0x55939e8d,0x29b3443a),0x3fcebe68);
    checkf(m33cf_fmul(0xd9fe416a,0x4e18e879),0xe897ddba);
    checkf(m33cf_fmul(0x4c0712bd,0xfe8f0d4e),0xff800000);
    checkf(m33cf_fmul(0xbdcb53f9,0x7b5292ea),0xf9a73f92);
    checkf(m33cf_fmul(0x290fc5cc,0x8d7fcbed),0x80000000);
    checkf(m33cf_fmul(0x98abd414,0x595cce6e),0xb29434d0);
    checkf(m33cf_fdiv_fast(0x19fec05d,0x198926d6),0x3fedc0ae);
    checkf(m33cf_fdiv_fast(0xd3f6c6ef,0xeb89ccae),0x27e53a49);
    checkf(m33cf_fdiv_fast(0xe929f69d,0x5dd3860a),0xcacdb375);
    checkf(m33cf_fdiv_fast(0xceef0386,0x3a2a3aa2),0xd433b89f);
    checkf(m33cf_fdiv_fast(0xf3e76a98,0x859a5a67),0x7f800000);
    checkf(m33cf_fdiv_fast(0x2ade013a,0x42dbe607),0x278139dc);
    checkf(m33cf_fdiv_fast(0x7676da5e,0x48e1ff79),0x6d0bcfd4);
    checkf(m33cf_fdiv_fast(0x42a137e4,0x5c054bdf),0x261ad000);
    checkf(m33cf_fdiv_fast(0x358e8442,0x4a4c7e5a),0x2ab269aa);
    checkf(m33cf_fdiv_fast(0xf72429ca,0xdc475d3a),0x5a52cc93);
    checkf(m33cf_fsqrt_fast(0xc100dc14),0xffc00000);
    checkf(m33cf_fsqrt_fast(0x2a2eaa04),0x34d374f7);
    checkf(m33cf_fsqrt_fast(0x6e9ff12a),0x570f151a);
    checkf(m33cf_fsqrt_fast(0xae37c4b7),0xffc00000);
    checkf(m33cf_fsqrt_fast(0x47584a58),0x436b4f1d);
    checkf(m33cf_fsqrt_fast(0xebca1f1e),0xffc00000);
    checkf(m33cf_fsqrt_fast(0xba92914d),0xffc00000);
    checkf(m33cf_fsqrt_fast(0xdb304a6d),0xffc00000);
    checkf(m33cf_fsqrt_fast(0x92a72ec1),0xffc00000);
    checkf(m33cf_fsqrt_fast(0x936f9320),0xffc00000);
#endif

    checkd(m33cf_dadd(0x000000fc75a5900aULL,0x5bc7667ff4f5aed4ULL),0x5bc7667ff4f5aed4ULL);
    checkd(m33cf_dadd(0x3b795f9a7971afcaULL,0x3b795f7a89f1afcaULL),0x3b895f8a81b1afcaULL);
    checkd(m33cf_dadd(0x800c3012be12e063ULL,0x0007ffffffffffb2ULL),0x0000000000000000ULL);
    checkd(m33cf_dadd(0xf2ebeaef727ecb8cULL,0xe919ba322b0d6abeULL),0xf2ebeaef727ecb8cULL);
    checkd(m33cf_dadd(0xf318c8542dbf290fULL,0x7318c84fdc68b825ULL),0xf1f14559c3a80000ULL);
    checkd(m33cf_dadd(0xe9cafb8dc44950d0ULL,0x000081e7baa8971aULL),0xe9cafb8dc44950d0ULL);
    checkd(m33cf_dadd(0xffb51396c4a74144ULL,0x0006ec693b58bedeULL),0xffb51396c4a74144ULL);
    checkd(m33cf_dadd(0x8cb46a68a3f226a7ULL,0xb73c729a98a91e3cULL),0xb73c729a98a91e3cULL);
    checkd(m33cf_dadd(0x16d2eaf7f2850dc1ULL,0x96d2cf06aaf78c08ULL),0x165bf1478d81b900ULL);
    checkd(m33cf_dadd(0x7fecab31e7006bc2ULL,0xffec71829182845fULL),0x7f7cd7aabef3b180ULL);
    checkd(m33cf_dsub(0x7fe80001bfc46633ULL,0xc01c0000dfe2337aULL),0x7fe80001bfc46633ULL);
    checkd(m33cf_dsub(0x956bc258e14cf502ULL,0x956bc257257e217bULL),0x942bbced38700000ULL);
    checkd(m33cf_dsub(0x1304f931164a0dc4ULL,0x1c3725edf809d211ULL),0x9c3725edf809d211ULL);
    checkd(m33cf_dsub(0x8cb8000dd190a395ULL,0x4ae40006e8c85189ULL),0xcae40006e8c85189ULL);
    checkd(m33cf_dsub(0x800000304a84d45aULL,0x0c32000000000026ULL),0x8c32000000000026ULL);
    checkd(m33cf_dsub(0xf2b4699998abeef8ULL,0xeb07d9b004a574ddULL),0xf2b4699998abeef8ULL);
    checkd(m33cf_dsub(0x3fdfe7b9d8d1dbffULL,0x800d7094d9d8d639ULL),0x3fdfe7b9d8d1dbffULL);
    checkd(m33cf_dsub(0xb4e058de3e8b4acaULL,0x803f94b600000001ULL),0xb4e058de3e8b4acaULL);
    checkd(m33cf_dsub(0x8ddfdbe97e292ab1ULL,0x4b9c6fda25efa7e6ULL),0xcb9c6fda25efa7e6ULL);
    checkd(m33cf_dsub(0x9cfebf98a0150dc2ULL,0x1cceb5d8a0150dc3ULL),0x9d014b29da0bd7bdULL);
    checkd(m33cf_dmul(0x7eee03dd737c3204ULL,0xc19afd2262fa8aa9ULL),0xfff0000000000000ULL);
    checkd(m33cf_dmul(0x80760ca8a50d4410ULL,0x807512348901231bULL),0x0000000000000000ULL);
    checkd(m33cf_dmul(0x7ca0d01a0450b53dULL,0x7ca0cf355c70652fULL),0x7ff0000000000000ULL);
    checkd(m33cf_dmul(0x0ea698eb5b8a7c9eULL,0x0d0fffffffffff0fULL),0x0000000000000000ULL);
    checkd(m33cf_dmul(0xba4565707f05766dULL,0x2c5e39f11066eb37ULL),0xa6b435d75666263eULL);
    checkd(m33cf_dmul(0xf2cd3698d8928584ULL,0x8ba3e96d6434a1b9ULL),0x3e822d8426f335d6ULL);
    checkd(m33cf_dmul(0x7b386a737bf6693dULL,0xfff70000000000ecULL),0xffff0000000000ecULL);
    checkd(m33cf_dmul(0x407f0007866747d5ULL,0xe0408003c333a303ULL),0xe0cff80f0cd052ebULL);
    checkd(m33cf_dmul(0x8142be0e6bff63ceULL,0x8135ac3ae2fe4078ULL),0x0000000000000000ULL);
    checkd(m33cf_dmul(0x4d1f3420ca65f2baULL,0x8000282866063841ULL),0x8000000000000000ULL);
    checkd(m33cf_ddiv_fast(0x94885bb8363252b5ULL,0x9484cbb8363252b6ULL),0x3ff2bdae4b39ea58ULL);
    checkd(m33cf_ddiv_fast(0x7112caa0922ef96aULL,0x7112c703422ef96aULL),0x3ff0031472b5f708ULL);
    checkd(m33cf_ddiv_fast(0x803795fc115579cfULL,0x00251828182845f3ULL),0xc001e3ca5352423eULL);
    checkd(m33cf_ddiv_fast(0x9098cf9d1d63a8a6ULL,0x0000bce378acc9edULL),0xfff0000000000000ULL);
    checkd(m33cf_ddiv_fast(0x76eb74d4755d9e90ULL,0x56da74d4755d9e40ULL),0x60009ad22a263295ULL);
    checkd(m33cf_ddiv_fast(0xb7ac4ff81ab5acd6ULL,0x3770052042a6f0c2ULL),0xc02c46e8ee4dcde4ULL);
    checkd(m33cf_ddiv_fast(0x9f7b7105213fc016ULL,0x9f7b7105206fc017ULL),0x3ff00000007946b7ULL);
    checkd(m33cf_ddiv_fast(0x155302277af2c615ULL,0x0052a52fdf6ae145ULL),0x54f04fc7152639a7ULL);
    checkd(m33cf_ddiv_fast(0x55544fdcabf6c324ULL,0x7f398c80174fa929ULL),0x160970d87e92ac22ULL);
    checkd(m33cf_ddiv_fast(0x92a415a76b37d7d2ULL,0x0200640000000000ULL),0xd0939b1df01b2df3ULL);
    checkd(m33cf_dadd(0x3dd4731301475eafULL,0xde02f10226d652e5ULL),0xde02f10226d652e5ULL);
    checkd(m33cf_dadd(0x1a031fc739b5b31cULL,0xe23b24be4b0200bfULL),0xe23b24be4b0200bfULL);
    checkd(m33cf_dadd(0x4baad04027dc41beULL,0xefe184a8db9b5e28ULL),0xefe184a8db9b5e28ULL);
    checkd(m33cf_dadd(0xa97a312516d84984ULL,0xe87c4e53da594b0bULL),0xe87c4e53da594b0bULL);
    checkd(m33cf_dadd(0x65f94cdbf4e1655bULL,0xeb08d78147a5fa0cULL),0xeb08d78147a5fa0cULL);
    checkd(m33cf_dadd(0xd758e560b9f8515eULL,0x40167af8bfabbfc8ULL),0xd758e560b9f8515eULL);
    checkd(m33cf_dadd(0x3177a00dd7f30cfaULL,0x14ce136ab4488ffbULL),0x3177a00dd7f30cfaULL);
    checkd(m33cf_dadd(0xe2de834ef78df600ULL,0xe9709814970e2cb1ULL),0xe9709814970e2cb1ULL);
    checkd(m33cf_dadd(0x88ad647a702f0bedULL,0x2c0af4057a3c4795ULL),0x2c0af4057a3c4795ULL);
    checkd(m33cf_dadd(0xb7dd95bbcbfdd773ULL,0x531893debd8fbf97ULL),0x531893debd8fbf97ULL);
    checkd(m33cf_dsub(0x7ba38f5e7b843460ULL,0xf0b3823de05b22b4ULL),0x7ba38f5e7b843460ULL);
    checkd(m33cf_dsub(0xee5169f2e74a0f18ULL,0xd501efe1254788b3ULL),0xee5169f2e74a0f18ULL);
    checkd(m33cf_dsub(0x69a4707a9a0e8faeULL,0xb2f0883420b659dbULL),0x69a4707a9a0e8faeULL);
    checkd(m33cf_dsub(0x427f8d2ef8b19474ULL,0x3a5d0ad508f37fedULL),0x427f8d2ef8b19474ULL);
    checkd(m33cf_dsub(0x32a9c51a3d565d85ULL,0x86600a23c10b178aULL),0x32a9c51a3d565d85ULL);
    checkd(m33cf_dsub(0x07dac3387f06fa0cULL,0x5c733002a61b620dULL),0xdc733002a61b620dULL);
    checkd(m33cf_dsub(0x43d6c9f6fdb3d1b3ULL,0xba69f6160840b2b1ULL),0x43d6c9f6fdb3d1b3ULL);
    checkd(m33cf_dsub(0x4293bb41f9808440ULL,0x2482605096e85ffeULL),0x4293bb41f9808440ULL);
    checkd(m33cf_dsub(0x83337a590422130bULL,0x7a45cb63b68f1999ULL),0xfa45cb63b68f1999ULL);
    checkd(m33cf_dsub(0x122533912d6612c9ULL,0x1b08519e5f8f78b5ULL),0x9b08519e5f8f78b5ULL);
    checkd(m33cf_dmul(0xd0bd40b470b385cdULL,0xabfa84fccb01cb46ULL),0x3cc83e249b03d547ULL);
    checkd(m33cf_dmul(0x740cc7981c80fddaULL,0x90c53fcc12a6a097ULL),0xc4e31c5c4f5c9ba4ULL);
    checkd(m33cf_dmul(0x43290fa64e05e998ULL,0xad4bb7fa2bb564b9ULL),0xb085b549bd618756ULL);
    checkd(m33cf_dmul(0xf8fd9c8c9b0bd944ULL,0x4f4a1743e96370a8ULL),0xfff0000000000000ULL);
    checkd(m33cf_dmul(0x0c3cdfc8888b644fULL,0x43223d92356f971eULL),0x0f70756f504cf5dcULL);
    checkd(m33cf_dmul(0x640b87064cb6c260ULL,0x5d9aabe9efed6024ULL),0x7ff0000000000000ULL);
    checkd(m33cf_dmul(0x8718f6339a5eb7d5ULL,0x5d05b556aad4f81aULL),0xa430ef060c1c1204ULL);
    checkd(m33cf_dmul(0xd2325605b52e4bbaULL,0x21bef3dbb44cb312ULL),0xb401bc60793a2186ULL);
    checkd(m33cf_dmul(0xa1e31fb000c8768eULL,0x6d10072a62a042f7ULL),0xcf03284086d8733fULL);
    checkd(m33cf_dmul(0xc787a9a862f5b946ULL,0xcbdeeaeafe6b567dULL),0x5376dcc440db5e02ULL);
    checkd(m33cf_ddiv_fast(0xd1ab78cc3a53310bULL,0x871b22cc627440ffULL),0x7ff0000000000000ULL);
    checkd(m33cf_ddiv_fast(0x8c64d3beb627c82bULL,0x8ec3d33d99470d80ULL),0x3d90cf03611e44bcULL);
    checkd(m33cf_ddiv_fast(0xf6bfb08f555b9642ULL,0x51560ce4365f67a5ULL),0xe556fe91c490804dULL);
    checkd(m33cf_ddiv_fast(0xe2a556ddbf80d538ULL,0x48808e2d782a9962ULL),0xda149f9aa72e3465ULL);
    checkd(m33cf_ddiv_fast(0x9646a0875dd00eb0ULL,0x579f2c0d32efbfc3ULL),0x8000000000000000ULL);
    checkd(m33cf_ddiv_fast(0x4eb970540e932394ULL,0xc9bc823d173ca6deULL),0xc4ec8dd8577f1941ULL);
    checkd(m33cf_ddiv_fast(0x26492979e5664695ULL,0x082cff680d6d2530ULL),0x5e0bc467714a1337ULL);
    checkd(m33cf_ddiv_fast(0x12db696de9340939ULL,0xe5632c6e98de9cc0ULL),0x8000000000000000ULL);
    checkd(m33cf_ddiv_fast(0x4af31aa7075beb9dULL,0x13f5ee839770ee58ULL),0x76ebdfd4cfc4635bULL);
    checkd(m33cf_ddiv_fast(0x3e62ca75a07c1ac2ULL,0xb048d234e122da6dULL),0xce0839c35d7657d1ULL);
    checkd(m33cf_dsqrt_fast(0x56d82e2279837d7bULL),0x4b63ab5aaabcb779ULL);
    checkd(m33cf_dsqrt_fast(0xfc6998b66a2a51deULL),0xfff8000000000000ULL);
    checkd(m33cf_dsqrt_fast(0x2c0cc3149408dcc9ULL),0x35fe567d3c418723ULL);
    checkd(m33cf_dsqrt_fast(0x1f71e22e323600e6ULL),0x2fb0ea62598ef070ULL);
    checkd(m33cf_dsqrt_fast(0x71a6cf9286fdea03ULL),0x58cb047c625fe761ULL);
    checkd(m33cf_dsqrt_fast(0x307b55796f027243ULL),0x3834e9a8788bda68ULL);
    checkd(m33cf_dsqrt_fast(0x10518d57b7b32094ULL),0x2820c212d75b45adULL);
    checkd(m33cf_dsqrt_fast(0x85d55765699fc337ULL),0xfff8000000000000ULL);
    checkd(m33cf_dsqrt_fast(0x030a70b7171e4c51ULL),0x217d166dff1d7836ULL);
    checkd(m33cf_dsqrt_fast(0x5284145a8007521fULL),0x493959345ef6a420ULL);

    if(rc==0) puts("Pass");
    else      puts("Fail");
}
