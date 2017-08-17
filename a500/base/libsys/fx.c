#include "fx.h"

static UWORD _sqrt[65] = {
  0,1,1,2,2,4,5,8,11,16,22,32,45,64,90,128,181,256,362,512,724,1024,1448,2048,2896,4096,5792,8192,11585,16384,23170,32768,46340,
  32768,33276,33776,34269,34755,35235,35708,36174,36635,37090,37540,37984,38423,38858,39287,39712,40132,40548,40960,41367,41771,42170,42566,42959,43347,43733,44115,44493,44869,45241,45611,45977
};

static BYTE _nlz[256] = {
  0,
  1,
  2,2,
  3,3,3,3,
  4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

static inline WORD nlz(ULONG x) {
  ULONG y;
  WORD n = 0;
  if ((y = x >> 16)) { n += 16; x = y; }
  if ((y = x >> 8)) { n += 8; x = y; }
  return n + _nlz[x];
}

__regargs LONG isqrt(LONG val) {
  WORD cnt = nlz(val);
  ULONG t;

  if (cnt <= 6) 
    t = (val << (6 - cnt));
  else
    t = (val >> (cnt - 6));

  t &= 31;
  t += 33;

  t = (_sqrt[cnt] * _sqrt[t]) << 1;

  return t >> 16;
}