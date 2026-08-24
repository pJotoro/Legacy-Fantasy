/*
# Small Deflate
`sdefl` is a small bare bone lossless compression library in ANSI C (ISO C90)
which implements the Deflate (RFC 1951) compressed data format specification standard.
It is mainly tuned to get as much speed and compression ratio from as little code
as needed to keep the implementation as concise as possible.

## Features
- Portable single header and source file duo written in ANSI C (ISO C90)
- Dual license with either MIT or public domain
- Small implementation
    - Deflate: 525 LoC
    - Inflate: 500 LoC
- Webassembly:
    - Deflate ~3.7 KB (~2.2KB compressed)
    - Inflate ~3.6 KB (~2.2KB compressed)

## Usage:
This file behaves differently depending on what symbols you define
before including it.

Header-File mode:
If you do not define `SINFL_IMPLEMENTATION` before including this file, it
will operate in header only mode. In this mode it declares all used structs
and the API of the library without including the implementation of the library.

Implementation mode:
If you define `SINFL_IMPLEMENTATION` before including this file, it will
compile the implementation. Make sure that you only include
this file implementation in *one* C or C++ file to prevent collisions.

### Benchmark

| Compressor name         | Compression| Decompress.| Compr. size | Ratio |
| ------------------------| -----------| -----------| ----------- | ----- |
| miniz 1.0 -1            |   122 MB/s |   208 MB/s |    48510028 | 48.51 |
| miniz 1.0 -6            |    27 MB/s |   260 MB/s |    36513697 | 36.51 |
| miniz 1.0 -9            |    23 MB/s |   261 MB/s |    36460101 | 36.46 |
| zlib 1.2.11 -1          |    72 MB/s |   307 MB/s |    42298774 | 42.30 |
| zlib 1.2.11 -6          |    24 MB/s |   313 MB/s |    36548921 | 36.55 |
| zlib 1.2.11 -9          |    20 MB/s |   314 MB/s |    36475792 | 36.48 |
| sdefl 1.0 -0            |   127 MB/s |   355 MB/s |    40004116 | 39.88 |
| sdefl 1.0 -1            |   111 MB/s |   413 MB/s |    38940674 | 38.82 |
| sdefl 1.0 -5            |    45 MB/s |   436 MB/s |    36577183 | 36.46 |
| sdefl 1.0 -7            |    38 MB/s |   432 MB/s |    36523781 | 36.41 |
| libdeflate 1.3 -1       |   147 MB/s |   667 MB/s |    39597378 | 39.60 |
| libdeflate 1.3 -6       |    69 MB/s |   689 MB/s |    36648318 | 36.65 |
| libdeflate 1.3 -9       |    13 MB/s |   672 MB/s |    35197141 | 35.20 |
| libdeflate 1.3 -12      |  8.13 MB/s |   670 MB/s |    35100568 | 35.10 |

### Compression
Results on the [Silesia compression corpus](http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia):

| File    |   Original | `sdefl 0`    | `sdefl 5`  | `sdefl 7`   |
| --------| -----------| -------------| ---------- | ------------|
| dickens | 10.192.446 | 4,260,187    |  3,845,261 |   3,833,657 |
| mozilla | 51.220.480 | 20,774,706   | 19,607,009 |  19,565,867 |
| mr      |  9.970.564 | 3,860,531    |  3,673,460 |   3,665,627 |
| nci     | 33.553.445 | 4,030,283    |  3,094,526 |   3,006,075 |
| ooffice |  6.152.192 | 3,320,063    |  3,186,373 |   3,183,815 |
| osdb    | 10.085.684 | 3,919,646    |  3,649,510 |   3,649,477 |
| reymont |  6.627.202 | 2,263,378    |  1,857,588 |   1,827,237 |
| samba   | 21.606.400 | 6,121,797    |  5,462,670 |   5,450,762 |
| sao     |  7.251.944 | 5,612,421    |  5,485,380 |   5,481,765 |
| webster | 41.458.703 | 13,972,648   | 12,059,432 |  11,991,421 |
| xml     |  5.345.280 | 886,620      |    674,009 |     662,141 |
| x-ray   |  8.474.240 | 6,304,655    |  6,244,779 |   6,244,779 |

## License
```
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020-2023 Micha Mettke
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
```
*/
#ifndef INFL_H_INCLUDED
#define INFL_H_INCLUDED

#define INFL_PRE_TBL_SIZE 128
#define INFL_LIT_TBL_SIZE 1334
#define INFL_OFF_TBL_SIZE 402

typedef struct INFL_Context {
  const uint8_t* bitptr;
  const uint8_t* bitend;
  uint64_t bitbuf;
  int32_t bitcnt;

  uint32_t lits[INFL_LIT_TBL_SIZE];
  uint32_t dsts[INFL_OFF_TBL_SIZE];
} INFL_Context;

extern uint64_t INFL_Inflate(void* out, uint64_t cap, const void* in, uint64_t size);
extern uint64_t INFL_ZInflate(void* out, uint64_t cap, const void* in, uint64_t size);

#endif /* INFL_H_INCLUDED */

#ifdef INFL_IMPLEMENTATION

#include <SDL_stdinc.h>
#include <SDL_assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define INFL_Likely(x)       __builtin_expect((x),1)
#define INFL_Unlikely(x)     __builtin_expect((x),0)
#else
#define INFL_Likely(x)       (x)
#define INFL_Unlikely(x)     (x)
#endif

#ifndef INFL_NO_SIMD
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM64)
  #include <arm_neon.h>
  #define INFL_Char16           uint8x16_t
  #define INFL_Char16_ld(p)     vld1q_u8((const uint8_t*)(p))
  #define INFL_Char16_str(d, v) vst1q_u8((uint8_t*)(d), v)
  #define INFL_Char16_char(c)   vdupq_n_u8(c)
#elif defined(__x86_64__) || defined(_WIN32) || defined(_WIN64)
  #include <emmintrin.h>
  #define INFL_Char16 __m128i
  #define INFL_Char16_ld(p) _mm_loadu_si128((const __m128i* )(void*)(p))
  #define INFL_Char16_str(d,v)  _mm_storeu_si128((__m128i*)(void*)(d), v)
  #define INFL_Char16_char(c) _mm_set1_epi8(c)
#else
  #define INFL_NO_SIMD
#endif
#endif

static int32_t INFL_Bsr(uint32_t n) {
#ifdef _MSC_VER
  uint32_t r = 0;
  _BitScanReverse(&r, n);
  return (int32_t)r;
#elif defined(__GNUC__) || defined(__clang__)
  return 31 - __builtin_clz(n);
#endif
}

static uint64_t INFL_Read64(const void* p) {
  uint64_t n;
  SDL_memcpy(&n, p, 8);
  return n;
}

static void INFL_Copy64(uint8_t** dst, uint8_t** src) {
  SDL_memcpy(*dst, *src, 8);
  *dst += 8, *src += 8;
}

static uint8_t* INFL_Write64(uint8_t* dst, uint64_t w) {
  SDL_memcpy(dst, &w, 8);
  return dst + 8;
}

#ifndef INFL_NO_SIMD
static uint8_t* INFL_Write128(uint8_t* dst, INFL_Char16 w) {
  INFL_Char16_str(dst, w);
  return dst + 8;
}

static void INFL_Copy128(uint8_t** dst, uint8_t** src) {
  INFL_Char16 n = INFL_Char16_ld(*src);
  INFL_Char16_str(*dst, n);
  *dst += 16, *src += 16;
}
#endif

static void INFL_Refill(INFL_Context* s) {
  if (s->bitend - s->bitptr >= 8) {
      // @raysan5: original code, only those 3 lines
      s->bitbuf |= INFL_Read64(s->bitptr) << s->bitcnt;
      s->bitptr += (63 - s->bitcnt) >> 3;
      s->bitcnt |= 56; /* bitcount in range [56,63] */
  } else {
      // @raysan5: added this case when bits remaining < 8
      int32_t bitswant = 63 - s->bitcnt;
      int32_t byteswant = bitswant >> 3;
      int32_t bytesuse = s->bitend - s->bitptr <= byteswant ? (int32_t)(s->bitend - s->bitptr) : byteswant;
      uint64_t n = 0;
      SDL_memcpy(&n, s->bitptr, bytesuse);
      s->bitbuf |= n << s->bitcnt;
      s->bitptr += bytesuse;
      s->bitcnt += bytesuse << 3;
  }
}

static int32_t INFL_Peek(INFL_Context* s, int32_t cnt) {
  SDL_assert(cnt >= 0 && cnt <= 56);
  SDL_assert(cnt <= s->bitcnt);
  return s->bitbuf & ((1ull << cnt) - 1);
}

static void INFL_Eat(INFL_Context* s, int32_t cnt) {
  SDL_assert(cnt <= s->bitcnt);
  s->bitbuf >>= cnt;
  s->bitcnt -= cnt;
}

static int32_t INFL__Get(INFL_Context* s, int32_t cnt) {
  int32_t res = INFL_Peek(s, cnt);
  INFL_Eat(s, cnt);
  return res;
}

static int32_t INFL_Get(INFL_Context* s, int32_t cnt) {
  INFL_Refill(s);
  return INFL__Get(s, cnt);
}

typedef struct INFL_Gen {
  int32_t len;
  int32_t cnt;
  int32_t word;
  int16_t* sorted;
} INFL_Gen;

static int32_t INFL_BuildTbl(INFL_Gen* gen, uint32_t* tbl, int32_t tbl_bits, const int32_t* cnt) {
  int32_t tbl_end = 0;
  while (!(gen->cnt = cnt[gen->len])) {
    ++gen->len;
  }
  tbl_end = 1 << gen->len;
  while (gen->len <= tbl_bits) {
    do {uint32_t bit = 0;
      tbl[gen->word] = (*gen->sorted++ << 16) | gen->len;
      if (gen->word == tbl_end - 1) {
        for (; gen->len < tbl_bits; gen->len++) {
          SDL_memcpy(&tbl[tbl_end], tbl, (uint64_t)tbl_end*  sizeof(tbl[0]));
          tbl_end <<= 1;
        }
        return 1;
      }
      bit = 1 << INFL_Bsr((uint32_t)(gen->word ^ (tbl_end - 1)));
      gen->word &= bit - 1;
      gen->word |= bit;
    } while (--gen->cnt);
    do {
      if (++gen->len <= tbl_bits) {
        SDL_memcpy(&tbl[tbl_end], tbl, (uint64_t)tbl_end*  sizeof(tbl[0]));
        tbl_end <<= 1;
      }
    } while (!(gen->cnt = cnt[gen->len]));
  }
  return 0;
}

static void INFL_BuildSubtbl(INFL_Gen* gen, uint32_t* tbl, int32_t tbl_bits,
                   const int32_t* cnt) {
  int32_t sub_bits = 0;
  int32_t sub_start = 0;
  int32_t sub_prefix = -1;
  int32_t tbl_end = 1 << tbl_bits;
  while (1) {
    uint32_t entry;
    int32_t bit, stride, i;
    /* start new sub-table */
    if ((gen->word & ((1 << tbl_bits)-1)) != sub_prefix) {
      int32_t used = 0;
      sub_prefix = gen->word & ((1 << tbl_bits)-1);
      sub_start = tbl_end;
      sub_bits = gen->len - tbl_bits;
      used = gen->cnt;
      while (used < (1 << sub_bits)) {
        sub_bits++;
        used = (used << 1) + cnt[tbl_bits + sub_bits];
      }
      tbl_end = sub_start + (1 << sub_bits);
      tbl[sub_prefix] = (sub_start << 16) | 0x10 | (sub_bits & 0xf);
    }
    /* fill sub-table */
    entry = (*gen->sorted << 16) | ((gen->len - tbl_bits) & 0xf);
    gen->sorted++;
    i = sub_start + (gen->word >> tbl_bits);
    stride = 1 << (gen->len - tbl_bits);
    do {
      tbl[i] = entry;
      i += stride;
    } while (i < tbl_end);
    if (gen->word == (1 << gen->len)-1) {
      return;
    }
    bit = 1 << INFL_Bsr(gen->word ^ ((1 << gen->len) - 1));
    gen->word &= bit - 1;
    gen->word |= bit;
    gen->cnt--;
    while (!gen->cnt) {
      gen->cnt = cnt[++gen->len];
    }
  }
}

static void INFL_Build(uint32_t* tbl, uint8_t* lens, int32_t tbl_bits, int32_t maxlen,
            int32_t symcnt) {
  int32_t i, used = 0;
  int16_t sort[288];
  int32_t cnt[16] = {0}, off[16]= {0};
  INFL_Gen gen = {0};
  gen.sorted = sort;
  gen.len = 1;

  for (i = 0; i < symcnt; ++i)
    cnt[lens[i]]++;
  off[1] = cnt[0];
  for (i = 1; i < maxlen; ++i) {
    off[i + 1] = off[i] + cnt[i];
    used = (used << 1) + cnt[i];
  }
  used = (used << 1) + cnt[i];
  for (i = 0; i < symcnt; ++i)
    gen.sorted[off[lens[i]]++] = (int16_t)i;
  gen.sorted += off[0];

  if (used < (1 << maxlen)){
    for (i = 0; i < 1 << tbl_bits; ++i)
      tbl[i] = (0 << 16u) | 1;
    return;
  }
  if (!INFL_BuildTbl(&gen, tbl, tbl_bits, cnt)){
    INFL_BuildSubtbl(&gen, tbl, tbl_bits, cnt);
  }
}

static int32_t INFL_Decode(INFL_Context* s, const uint32_t* tbl, int32_t bit_len) {
  int32_t idx = INFL_Peek(s, bit_len);
  uint32_t key = tbl[idx];
  if (key & 0x10) {
    /* sub-table lookup */
    int32_t len = key & 0x0f;
    INFL_Eat(s, bit_len);
    idx = INFL_Peek(s, len);
    key = tbl[((key >> 16) & 0xffff) + (uint32_t)idx];
  }
  INFL_Eat(s, key & 0x0f);
  return (key >> 16) & 0x0fff;
}

static uint64_t INFL_Decompress(uint8_t* out, uint64_t cap, const uint8_t* in, uint64_t size) {
  static const uint8_t order[] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
  static const int16_t dbase[30+2] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
      257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
  static const uint8_t dbits[30+2] = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,
      10,10,11,11,12,12,13,13,0,0};
  static const int16_t lbase[29+2] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,
      43,51,59,67,83,99,115,131,163,195,227,258,0,0};
  static const uint8_t lbits[29+2] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,
      4,4,4,5,5,5,5,0,0,0};

  const uint8_t* oe = out + cap;
  const uint8_t* e = in + size, *o = out;
  typedef enum INFL_States {hdr,stored,fixed,dyn,blk} INFL_States;
  INFL_States state = hdr;
  INFL_Context s = {0};
  int32_t last = 0;

  s.bitptr = in;
  s.bitend = e;
  while (1) {
    switch (state) {
    case hdr: {
      /* block header */
      int32_t type = 0;
      INFL_Refill(&s);
      last = INFL__Get(&s,1);
      type = INFL__Get(&s,2);

      switch (type) {default: return (uint64_t)(out-o);
      case 0x00: state = stored; break;
      case 0x01: state = fixed; break;
      case 0x02: state = dyn; break;}
    } break;
    case stored: {
      /* uncompressed block */
      uint32_t len, nlen;
      INFL__Get(&s,s.bitcnt & 7);
      len = (uint16_t)INFL__Get(&s,16);
      nlen = (uint16_t)INFL__Get(&s,16);
      s.bitptr -= s.bitcnt / 8;
      s.bitbuf = s.bitcnt = 0;

      if ((uint16_t)len != (uint16_t)~nlen)
        return (uint64_t)(out-o);
      if (len > (e - s.bitptr))
        return (uint64_t)(out-o);

      SDL_memcpy(out, s.bitptr, (uint64_t)len);
      s.bitptr += len, out += len;
      if (last) return (uint64_t)(out-o);
      state = hdr;
    } break;
    case fixed: {
      /* fixed huffman codes */
      int32_t n; uint8_t lens[288+32];
      for (n = 0; n <= 143; n++) lens[n] = 8;
      for (n = 144; n <= 255; n++) lens[n] = 9;
      for (n = 256; n <= 279; n++) lens[n] = 7;
      for (n = 280; n <= 287; n++) lens[n] = 8;
      for (n = 0; n < 32; n++) lens[288+n] = 5;

      /* build lit/dist tables */
      INFL_Build(s.lits, lens, 10, 15, 288);
      INFL_Build(s.dsts, lens + 288, 8, 15, 32);
      state = blk;
    } break;
    case dyn: {
      /* dynamic huffman codes */
      int32_t n, i;
      uint32_t hlens[INFL_PRE_TBL_SIZE];
      uint8_t nlens[19] = {0}, lens[288+32];

      INFL_Refill(&s);
      {int32_t nlit = 257 + INFL__Get(&s,5);
      int32_t ndist = 1 + INFL__Get(&s,5);
      int32_t nlen = 4 + INFL__Get(&s,4);
      for (n = 0; n < nlen; n++)
        nlens[order[n]] = (uint8_t)INFL_Get(&s,3);
      INFL_Build(hlens, nlens, 7, 7, 19);

      /* decode code lengths */
      for (n = 0; n < nlit + ndist;) {
        int32_t sym = 0;
        INFL_Refill(&s);
        sym = INFL_Decode(&s, hlens, 7);
        switch (sym) {default: lens[n++] = (uint8_t)sym; break;
        case 16: for (i=3+INFL_Get(&s,2);i;i--,n++) lens[n]=lens[n-1]; break;
        case 17: for (i=3+INFL_Get(&s,3);i;i--,n++) lens[n]=0; break;
        case 18: for (i=11+INFL_Get(&s,7);i;i--,n++) lens[n]=0; break;}
      }
      /* build lit/dist tables */
      INFL_Build(s.lits, lens, 10, 15, nlit);
      INFL_Build(s.dsts, lens + nlit, 8, 15, ndist);
      state = blk;}
    } break;
    case blk: {
      /* decompress block */
      while (1) {
        int32_t sym;
        INFL_Refill(&s);
        sym = INFL_Decode(&s, s.lits, 10);
        if (sym < 256) {
          /* literal */
          if (INFL_Unlikely(out >= oe)) {
            return (uint64_t)(out-o);
          }
          *out++ = (uint8_t)sym;
          sym = INFL_Decode(&s, s.lits, 10);
          if (sym < 256) {
            *out++ = (uint8_t)sym;
            continue;
          }
        }
        if (INFL_Unlikely(sym == 256)) {
          /* end of block */
          if (last) return (uint64_t)(out-o);
          state = hdr;
          break;
        }
        /* match */
        if (sym >= 286) {
          /* length codes 286 and 287 must not appear in compressed data */
          return (uint64_t)(out-o);
        }
        sym -= 257;
        {int32_t len = INFL__Get(&s, lbits[sym]) + lbase[sym];
        int32_t dsym = INFL_Decode(&s, s.dsts, 8);
        int32_t offs = INFL__Get(&s, dbits[dsym]) + dbase[dsym];
        uint8_t* dst = out, *src = out - offs;
        if (INFL_Unlikely(offs > (uint64_t)(out-o))) {
          return (uint64_t)(out-o);
        }
        out = out + len;

#ifndef INFL_NO_SIMD
        if (INFL_Likely(oe - out >= 16 * 3)) {
          if (offs >= 16) {
            /* simd copy match */
            INFL_Copy128(&dst, &src);
            INFL_Copy128(&dst, &src);
            do INFL_Copy128(&dst, &src);
            while (dst < out);
          } else if (offs >= 8) {
            /* word copy match */
            INFL_Copy64(&dst, &src);
            INFL_Copy64(&dst, &src);
            do INFL_Copy64(&dst, &src);
            while (dst < out);
          } else if (offs == 1) {
            /* rle match copying */
            INFL_Char16 w = INFL_Char16_char(src[0]);
            dst = INFL_Write128(dst, w);
            dst = INFL_Write128(dst, w);
            do dst = INFL_Write128(dst, w);
            while (dst < out);
          } else {
            /* byte copy match */
            *dst++ = *src++;
            *dst++ = *src++;
            do *dst++ = *src++;
            while (dst < out);
          }
        }
#else
        if (INFL_Likely(oe - out >= 3 * 8 - 3)) {
          if (offs >= 8) {
            /* word copy match */
            INFL_Copy64(&dst, &src);
            INFL_Copy64(&dst, &src);
            do INFL_Copy64(&dst, &src);
            while (dst < out);
          } else if (offs == 1) {
            /* rle match copying */
            uint32_t c = src[0];
            uint32_t hw = (c << 24u) | (c << 16u) | (c << 8u) | (uint32_t)c;
            uint64_t w = (uint64_t)hw << 32llu | hw;
            dst = INFL_Write64(dst, w);
            dst = INFL_Write64(dst, w);
            do dst = INFL_Write64(dst, w);
            while (dst < out);
          } else {
            /* byte copy match */
            *dst++ = *src++;
            *dst++ = *src++;
            do *dst++ = *src++;
            while (dst < out);
          }
        }
#endif
        else {
          *dst++ = *src++;
          *dst++ = *src++;
          do *dst++ = *src++;
          while (dst < out);
        }}
      }
    } break;}
  }
  return (uint64_t)(out-o);
}

extern uint64_t INFL_Inflate(void* out, uint64_t cap, const void* in, uint64_t size) {
  return INFL_Decompress((uint8_t*)out, cap, (const uint8_t*)in, size);
}

static uint32_t INFL_Adler32(uint32_t adler32, const uint8_t* in, uint64_t in_len) {
  const uint32_t ADLER_MOD = 65521;
  uint32_t s1 = adler32 & 0xffff;
  uint32_t s2 = adler32 >> 16;
  uint64_t blk_len, i;

  blk_len = in_len % 5552;
  while (in_len) {
    for (i=0; i + 7 < blk_len; i += 8) {
      s1 += in[0]; s2 += s1;
      s1 += in[1]; s2 += s1;
      s1 += in[2]; s2 += s1;
      s1 += in[3]; s2 += s1;
      s1 += in[4]; s2 += s1;
      s1 += in[5]; s2 += s1;
      s1 += in[6]; s2 += s1;
      s1 += in[7]; s2 += s1;
      in += 8;
    }
    for (; i < blk_len; ++i)
      s1 += *in++, s2 += s1;
    s1 %= ADLER_MOD; s2 %= ADLER_MOD;
    in_len -= blk_len;
    blk_len = 5552;
  } return (uint32_t)(s2 << 16) + (uint32_t)s1;
}

extern uint64_t INFL_ZInflate(void* out, uint64_t cap, const void* mem, uint64_t size) {
  const uint8_t* in = (const uint8_t*)mem;
  if (size >= 6) {
    const uint8_t* eob = in + size - 4;
    uint64_t n = INFL_Decompress((uint8_t*)out, cap, in + 2u, size);
    uint32_t a = INFL_Adler32(1u, (uint8_t*)out, n);
    uint32_t h = eob[0] << 24 | eob[1] << 16 | eob[2] << 8 | eob[3] << 0;
    return a == h ? n : -1;
  } else {
    return -1;
  }
}
#endif
