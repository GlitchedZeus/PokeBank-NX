/* Independent research harness; no vendor implementation copied.
 * Reference API: FlagBrew/memecrypto bd3b16ee2c12bd9ee353cf3098cb998e7880638c.
 * Not a game-written save, not evidence of retail acceptance. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "memecrypto.h"
int main(void) {
    unsigned char zero[128] = {0}, a[128], b[128], input[128], sig[128], decoded[128];
    memset(a, 0xA5, sizeof a); memset(b, 0x5A, sizeof b);
    reverseCrypt(zero, a); reverseCrypt(zero, b);
    int untouched_a = 1, untouched_b = 1;
    for (int i=0; i<128; ++i) { untouched_a &= a[i]==0xA5; untouched_b &= b[i]==0x5A; }
    printf("reverseCrypt(zero): sentinel_A_unchanged=%d sentinel_B_unchanged=%d\n", untouched_a, untouched_b);
    /* This verifies only observable no-output behavior, not uninitialized reads.
       MemorySanitizer/Valgrind is required for direct uninitialized-read evidence. */
    assert(untouched_a && untouched_b);
    for (int tail=0; tail<2; ++tail) {
        memset(input, tail ? 0xA5 : 0, sizeof input);
        for (int i=0; i<32; ++i) input[i]=(unsigned char)(i*7+3);
        assert(memecrypto_sign(input,sig,128));
        assert(memecrypto_verify(sig,decoded,128));
        assert(memcmp(input,decoded,128)==0);
        printf("signed_payload_%s_tail: verifier_accepts=1\n",tail?"nonzero":"zero");
        sig[0]^=1;
        assert(!memecrypto_verify(sig,decoded,128));
    }
    puts("PASS: two payload constructions verify; acceptance by the game remains UNKNOWN");
    return 0;
}
