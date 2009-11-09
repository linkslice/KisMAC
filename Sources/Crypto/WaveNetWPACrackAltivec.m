/*
        
        File:			WaveNetWPACrackAltivec.m
        Program:		KisMAC
		Author:			Michael Rossberg
						mick@binaervarianz.de
		Description:	KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.

    KisMAC is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation;

    KisMAC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with KisMAC; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#import "WaveNetWPACrackAltivec.h"
#import "WaveHelper.h"
#import "WaveClient.h"
#import "WPA.h"
#import <openssl/md5.h>
#import <openssl/sha.h>
#include <Accelerate/Accelerate.h>

struct clientData {
    UInt8 ptkInput[WPA_NONCE_LENGTH+WPA_NONCE_LENGTH+12];
    const UInt8 *mic;
    const UInt8 *data;
    UInt32 dataLen;
    NSString *clientID;
    int wpaKeyCipher;
};


#define SHA1_MAC_LEN 20

typedef struct {
    UInt32 state[5];
} SHA1_CTX;

typedef union {
    vUInt32 v[5];
    unsigned int i[5][4];
} SHA1_CTX_A;

#pragma mark-
#pragma mark Macros for SHA1 Altivec optimized
#pragma mark-

#if defined(__i386__)
    //not sure what this first one should really be?
    #define vec_step(a) 8
    #define vec_add(a, b) _mm_add_epi32(a, b)
    #define vec_and(a, b) _mm_and_si128(a, b)
    #define vec_xor(a, b) _mm_xor_si128(a, b)
    #define vec_or(a, b)  _mm_or_si128(a, b)
//this is not right but at least we can compile it!
    #define vec_rl(a, b)  vL128Rotate(a, b)
#endif

#define blkA0(i) buf[i].v

#define blkA(i) (buf[i & 15].v = vec_rl(vec_xor(buf[(i + 13) & 15].v, vec_xor(buf[(i + 8) & 15].v, vec_xor(buf[(i + 2) & 15].v, buf[i & 15].v))), one))

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0A(v,w,x,y,z,i) \
       z = vec_add(z, vec_add(vec_xor(vec_and(w, vec_xor(x, y)), y), vec_add(blkA0(i), vec_add(const0, vec_rl(v, five))))); \
       w = vec_rl(w, thirty);
#define R1A(v,w,x,y,z,i) \
       z = vec_add(z, vec_add(vec_xor(vec_and(w, vec_xor(x, y)), y), vec_add(blkA(i), vec_add(const0, vec_rl(v, five))))); \
       w = vec_rl(w, thirty);
#define R2A(v,w,x,y,z,i) \
        z = vec_add(z, vec_add((vec_xor(w, vec_xor(x, y))), vec_add(blkA(i), vec_add(const2, vec_rl(v, five))))); \
        w = vec_rl(w, thirty);
#define R3A(v,w,x,y,z,i) \
       z = vec_add(z, vec_add(vec_or(vec_and(vec_or(w, x), y), vec_and(w, x)), vec_add(blkA(i), vec_add(const3, vec_rl(v, five))))); \
       w = vec_rl(w, thirty);
#define R4A(v,w,x,y,z,i) \
       z = vec_add(z, vec_add((vec_xor(vec_xor(w,x), y)), vec_add(blkA(i), vec_add(const4, vec_rl(v, five))))); \
       w = vec_rl(w, thirty);

#pragma mark-
#pragma mark SHA1 functions
#pragma mark-

inline void SHA1TransformAltivec(vUInt32 state[5], unsigned long buffer[4][16]) {
        union vec {
            UInt32 scalars[ vec_step(vUInt32) ];
            vUInt32 v;
        };
        union vec buf[16];
        vUInt32  a, b, c, d, e, const0, const2, const3, const4, thirty, five, one;
        int i;
        
        const0 = (vUInt32){(0x5A827999)};
        const2 = (vUInt32){(0x6ED9EBA1)};
        const3 = (vUInt32){(0x8F1BBCDC)};
        const4 = (vUInt32){(0xCA62C1D6)};
        thirty = (vUInt32){(30)};
        five   = (vUInt32){(5)};
        one    = (vUInt32){(1)};
        
        for (i = 0; i < 16; i++) {
            buf[i].scalars[0] = buffer[0][i];
            buf[i].scalars[1] = buffer[1][i];
            buf[i].scalars[2] = buffer[2][i];
            buf[i].scalars[3] = buffer[3][i];
        }
        
        a = state[0];
        b = state[1];
        c = state[2];
        d = state[3];
        e = state[4];
        
	/* 4 round.vs of 20 operations each. Loop unrolled. */
	R0A(a,b,c,d,e, 0); R0A(e,a,b,c,d, 1); R0A(d,e,a,b,c, 2); R0A(c,d,e,a,b, 3);
	R0A(b,c,d,e,a, 4); R0A(a,b,c,d,e, 5); R0A(e,a,b,c,d, 6); R0A(d,e,a,b,c, 7);
	R0A(c,d,e,a,b, 8); R0A(b,c,d,e,a, 9); R0A(a,b,c,d,e,10); R0A(e,a,b,c,d,11);
	R0A(d,e,a,b,c,12); R0A(c,d,e,a,b,13); R0A(b,c,d,e,a,14); R0A(a,b,c,d,e,15);
	R1A(e,a,b,c,d,16); R1A(d,e,a,b,c,17); R1A(c,d,e,a,b,18); R1A(b,c,d,e,a,19);
	R2A(a,b,c,d,e,20); R2A(e,a,b,c,d,21); R2A(d,e,a,b,c,22); R2A(c,d,e,a,b,23);
	R2A(b,c,d,e,a,24); R2A(a,b,c,d,e,25); R2A(e,a,b,c,d,26); R2A(d,e,a,b,c,27);
	R2A(c,d,e,a,b,28); R2A(b,c,d,e,a,29); R2A(a,b,c,d,e,30); R2A(e,a,b,c,d,31);
	R2A(d,e,a,b,c,32); R2A(c,d,e,a,b,33); R2A(b,c,d,e,a,34); R2A(a,b,c,d,e,35);
	R2A(e,a,b,c,d,36); R2A(d,e,a,b,c,37); R2A(c,d,e,a,b,38); R2A(b,c,d,e,a,39);
	R3A(a,b,c,d,e,40); R3A(e,a,b,c,d,41); R3A(d,e,a,b,c,42); R3A(c,d,e,a,b,43);
	R3A(b,c,d,e,a,44); R3A(a,b,c,d,e,45); R3A(e,a,b,c,d,46); R3A(d,e,a,b,c,47);
	R3A(c,d,e,a,b,48); R3A(b,c,d,e,a,49); R3A(a,b,c,d,e,50); R3A(e,a,b,c,d,51);
	R3A(d,e,a,b,c,52); R3A(c,d,e,a,b,53); R3A(b,c,d,e,a,54); R3A(a,b,c,d,e,55);
	R3A(e,a,b,c,d,56); R3A(d,e,a,b,c,57); R3A(c,d,e,a,b,58); R3A(b,c,d,e,a,59);
	R4A(a,b,c,d,e,60); R4A(e,a,b,c,d,61); R4A(d,e,a,b,c,62); R4A(c,d,e,a,b,63);
	R4A(b,c,d,e,a,64); R4A(a,b,c,d,e,65); R4A(e,a,b,c,d,66); R4A(d,e,a,b,c,67);
	R4A(c,d,e,a,b,68); R4A(b,c,d,e,a,69); R4A(a,b,c,d,e,70); R4A(e,a,b,c,d,71);
	R4A(d,e,a,b,c,72); R4A(c,d,e,a,b,73); R4A(b,c,d,e,a,74); R4A(a,b,c,d,e,75);
	R4A(e,a,b,c,d,76); R4A(d,e,a,b,c,77); R4A(c,d,e,a,b,78); R4A(b,c,d,e,a,79);
	/* Add the working vars back into conte.vxt.state[] */


        state[0] = vec_add(state[0], a);
        state[1] = vec_add(state[1], b);
        state[2] = vec_add(state[2], c);
        state[3] = vec_add(state[3], d);
        state[4] = vec_add(state[4], e);
}

/* SHA1InitAndUpdateFistSmall64 - Initialize new context And fillup 64*/
inline void SHA1InitWithStatic64Altivec(SHA1_CTX_A* state, unsigned long buffer[4][16]) {
	state->v[0] = (vUInt32){(0x67452301)};
    state->v[1] = (vUInt32){(0xEFCDAB89)};
    state->v[2] = (vUInt32){(0x98BADCFE)};
    state->v[3] = (vUInt32){(0x10325476)};
    state->v[4] = (vUInt32){(0xC3D2E1F0)};
    SHA1TransformAltivec(state->v, buffer);
}

unsigned long altivec20ByteBuffer[4][16];

/* Add padding and return the message digest. */
inline void SHA1FinalFastWith20ByteDataAltivec(unsigned char digest[4][20], SHA1_CTX_A* state, unsigned char data[4][20]) {
	UInt32 i;
        
        memcpy(&altivec20ByteBuffer[0][0], &data[0][0], 20);
        memcpy(&altivec20ByteBuffer[1][0], &data[1][0], 20);
        memcpy(&altivec20ByteBuffer[2][0], &data[2][0], 20);
        memcpy(&altivec20ByteBuffer[3][0], &data[3][0], 20);
        
        SHA1TransformAltivec(state->v, altivec20ByteBuffer);

	for (i = 0; i < 20; i++) {
		digest[0][i] = (unsigned char)
			((state->i[i >> 2][0] >> ((3 - (i & 3)) * 8)) & 255);
		digest[1][i] = (unsigned char)
			((state->i[i >> 2][1] >> ((3 - (i & 3)) * 8)) & 255);
		digest[2][i] = (unsigned char)
			((state->i[i >> 2][2] >> ((3 - (i & 3)) * 8)) & 255);
		digest[3][i] = (unsigned char)
			((state->i[i >> 2][3] >> ((3 - (i & 3)) * 8)) & 255);
	}
}

inline void prepared_hmac_sha1Altivec(const SHA1_CTX_A *k_ipad, const SHA1_CTX_A *k_opad, unsigned char digest[4][20]) {
    SHA1_CTX_A ipad, opad; 

    memcpy(&ipad, k_ipad, sizeof(ipad));
    memcpy(&opad, k_opad, sizeof(opad));
    
    /* perform inner SHA1*/
    SHA1FinalFastWith20ByteDataAltivec(digest, &ipad, digest); /* finish up 1st pass */ 
    
    /* perform outer SHA1 */ 
    SHA1FinalFastWith20ByteDataAltivec(digest, &opad, digest); /* finish up 2nd pass */
}

inline void fastFAltivec(unsigned char password[4][64], int pwdLen[4], const unsigned char *ssid, int ssidlength, const SHA1_CTX_A *ipadContext, const SHA1_CTX_A *opadContext, int count, unsigned char output[4][40]) {
    unsigned char digest[4][20], digest1[4][64];
    int i, j, k, offset; 
    
    offset = SHA_DIGEST_LENGTH * (count - 1);
    
    /* U1 = PRF(P, S || int(i)) */ 
    for (i = 0; i < 4; i++) {
        memcpy(digest1[i], ssid, ssidlength);
        digest1[i][ssidlength]   = 0;   
        digest1[i][ssidlength+1] = 0; 
        digest1[i][ssidlength+2] = 0;
        digest1[i][ssidlength+3] = (unsigned char)(count & 0xff); 
        
        fast_hmac_sha1(digest1[i], ssidlength+4, password[i], pwdLen[i], digest[i]);

        /* output = U1 */ 
        memcpy(&output[i][offset], digest[i], SHA_DIGEST_LENGTH);
    }
    
    offset /= 4;
    
    for (i = 1; i < 4096; i++) { 
        /* Un = PRF(P, Un-1) */ 
        prepared_hmac_sha1Altivec(ipadContext, opadContext, digest); 
    
        /* output = output xor Un */
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 5; k++) {
                ((int*)output[j])[k+offset] ^= ((int*)digest[j])[k];
            }
        }
    }
} 

inline void fastWP_passwordHashAltivec(unsigned char password[4][64], const unsigned char *ssid, int ssidlength, unsigned char output[4][40]) { 
    union pad {
        unsigned long i[4][16];
        unsigned char c[4][64];
    };
    union pad ipad; /* inner padding - key XORd with ipad */ 
    union pad opad; /* outer padding - key XORd with opad */
    SHA1_CTX_A ipadContext, opadContext;
    int i, j, pwdLen[4];
    
    for(j = 0; j < 4; j++) {
        pwdLen[j] = strlen((char *)password[j]);
        
        /* XOR key with ipad and opad values */ 
        for (i = 0; i < pwdLen[j]; i++) { 
            ipad.c[j][i] = password[j][i] ^ 0x36; 
            opad.c[j][i] = password[j][i] ^ 0x5c;
        } 

        memset(&ipad.c[j][pwdLen[j]], 0x36, 64 - pwdLen[j]); 
        memset(&opad.c[j][pwdLen[j]], 0x5c, 64 - pwdLen[j]); 
    }
    
    /*initialize altivec final unit*/
    memset(&altivec20ByteBuffer[0][6], 0, 36);
    altivec20ByteBuffer[0][5]  = 0x80000000;
    altivec20ByteBuffer[0][15] = 0x000002A0;

    memcpy(&altivec20ByteBuffer[1][5], &altivec20ByteBuffer[0][5], 44);
    memcpy(&altivec20ByteBuffer[2][5], &altivec20ByteBuffer[0][5], 44);
    memcpy(&altivec20ByteBuffer[3][5], &altivec20ByteBuffer[0][5], 44);

    /*init ipad & opad (password specific */
    SHA1InitWithStatic64Altivec(&ipadContext, ipad.i);
    SHA1InitWithStatic64Altivec(&opadContext, opad.i);
 
    /* generate PMK */
    fastFAltivec(password, pwdLen, ssid, ssidlength, &ipadContext, &opadContext, 1, output);
    fastFAltivec(password, pwdLen, ssid, ssidlength, &ipadContext, &opadContext, 2, output); 
} 

@implementation WaveNet(WPACrackAltivecExtension)

- (BOOL)crackWPAWithWordlistAltivec:(NSString*)wordlist andImportController:(ImportController*)im {
    unsigned char wrd[4][64];
    const char *ssid;
    FILE* fptr;
    unsigned int i, j, k, l, words, ssidLength, keys, curKey;
    UInt8 pmk[4][40], ptk[64], digest[16];
    struct clientData *c;
    WaveClient *wc;
    const UInt8 *anonce, *snonce;
    UInt8 prefix[] = "Pairwise key expansion";

    fptr = fopen([wordlist UTF8String], "r");
    if (!fptr) return NO;
    
    keys = 0;
    for (i = 0; i < [aClientKeys count]; i++) {
        if ([[aClients objectForKey:[aClientKeys objectAtIndex:i]] eapolDataAvailable])
            keys++;
    }
    
    NSAssert(keys!=0, @"There must be more keys");
    
    curKey = 0;
    c = malloc(keys * sizeof(struct clientData));
    
    for (i = 0; i < [aClientKeys count]; i++) {
        wc = [aClients objectForKey:[aClientKeys objectAtIndex:i]];
        if ([wc eapolDataAvailable]) {
            if ([[wc ID] isEqualToString: _BSSID]) {
                keys--;
            } else {
                if (memcmp(_rawBSSID, [[wc rawID] bytes], 6)>0) {
                    memcpy(&c[curKey].ptkInput[0], [[wc rawID] bytes] , 6);
                    memcpy(&c[curKey].ptkInput[6], _rawBSSID, 6);
                } else {
                    memcpy(&c[curKey].ptkInput[0], _rawBSSID, 6);
                    memcpy(&c[curKey].ptkInput[6], [[wc rawID] bytes] , 6);
                }
                
                anonce = [[wc aNonce] bytes]; 
                snonce = [[wc sNonce] bytes];
                if (memcmp(anonce, snonce, WPA_NONCE_LENGTH)>0) {
                    memcpy(&c[curKey].ptkInput[12],                     snonce, WPA_NONCE_LENGTH);
                    memcpy(&c[curKey].ptkInput[12 + WPA_NONCE_LENGTH],  anonce, WPA_NONCE_LENGTH);
                } else {
                    memcpy(&c[curKey].ptkInput[12],                     anonce, WPA_NONCE_LENGTH);
                    memcpy(&c[curKey].ptkInput[12 + WPA_NONCE_LENGTH],  snonce, WPA_NONCE_LENGTH);
                }

                c[curKey].data          = [[wc eapolPacket] bytes];
                c[curKey].dataLen       = [[wc eapolPacket] length];
                c[curKey].mic           = [[wc eapolMIC]    bytes];
                c[curKey].clientID      = [wc ID];
                c[curKey].wpaKeyCipher  = [wc wpaKeyCipher];
                curKey++;
            }
        }
    }

    words = 0;
    wrd[0][63]=0;
    wrd[1][63]=0;
    wrd[2][63]=0;
    wrd[3][63]=0;

    ssid = [_SSID UTF8String];
    ssidLength = [_SSID lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    
    float theTime, prevTime = clock() / (float)CLK_TCK;
    do {
        k = 0;
        do {
            if (feof(fptr)) break;

            fgets((char *)wrd[k], 64, fptr);
            i = strlen((char *)wrd[k]) - 1;
            wrd[k][i--] = 0;
            if (wrd[k][i]=='\r') wrd[k][i] = 0;
            
            words++;

            if (words % 500 == 0) {
                theTime =clock() / (float)CLK_TCK;
                [im setStatusField:[NSString stringWithFormat:@"%d words tested    %.2f/second", words, 500 / (theTime - prevTime)]];
                prevTime = theTime;
            }

            if (i < 8 || i > 63) continue; //passwords must be shorter than 63 signs
            
            for(j = 0; j < i; j++)
                if ((wrd[k][j] < 32) || (wrd[k][j] > 126)) break;
            if (j!=i) continue;
            
            k++;
        } while(k<4);

        fastWP_passwordHashAltivec(wrd, ssid, ssidLength, pmk);
        for (l = 0; l < k; l++) {
            for (curKey = 0; curKey < keys; curKey++) {
        
            PRF(pmk[l], 32, prefix, strlen((char *)prefix), c[curKey].ptkInput, WPA_NONCE_LENGTH*2 + 12, ptk, 16);
                if (c[curKey].wpaKeyCipher == 1)
                    fast_hmac_md5(c[curKey].data, c[curKey].dataLen, ptk, 16, digest);
                else
                    fast_hmac_sha1(c[curKey].data, c[curKey].dataLen, ptk, 16, digest);
                if (memcmp(digest, c[curKey].mic, 16) == 0) {
                    _password = [[NSString stringWithFormat:@"%s for Client %@", wrd[l], c[curKey].clientID] retain];
                    fclose(fptr);
                    free(c);
                    NSLog(@"Cracking was successful. Password is <%s> for Client %@", wrd[l], c[curKey].clientID);
                    return YES;
                }
            }
        }
    } while (![im canceled] && (k==4));
    
    free(c);
    fclose(fptr);
    
    _crackErrorString = [NSLocalizedString(@"The key was none of the tested passwords.", @"Error description for WPA crack.") retain];
    return NO;
}

@end