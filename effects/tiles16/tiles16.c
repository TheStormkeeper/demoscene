#include "startup.h"
#include "hardware.h"
#include "coplist.h"
#include "memory.h"
#include "gfx.h"
#include "blitter.h"
#include "random.h"
#include "tasks.h"

int __chipmem = 128 * 1024;

#define WIDTH (320 + 16)
#define HEIGHT (256 + 16)
#define DEPTH 5
#define BPLMOD (WIDTH * (DEPTH - 1) / 8)

#define VTILES (HEIGHT / 16)
#define HTILES (WIDTH / 16)
#define TILESIZE (16 * 16 * DEPTH / 8)

static CopInsT *bplptr[2][DEPTH];
static BitmapT *screen[2];
static CopInsT *bplcon1[2];
static CopListT *cp[2];
static short active;

typedef struct TileSet {
  u_short width, height;
  u_short count;
  void **ptrs;
} TileSetT;

#include "data/MagicLand.c"
#include "data/MagicLand-tiles.c"
#define tileset MagicLand_tiles
#define tilemap_width MagicLand_map_width
#define tilemap_height MagicLand_map_height
#define tilemap MagicLand_map

static void Load(void) {
  tileset.ptrs = MemAlloc(sizeof(void *) * tileset.count, MEMF_PUBLIC);
  {
    short n = tileset.count;
    void *base = tiles.planes[0];
    void **ptrs = tileset.ptrs;
    while (--n >= 0) {
      *ptrs++ = base;
      base += TILESIZE;
    }
  }

  {
    int i;
    for (i = 0; i < tilemap_width * tilemap_height; i++) {
      tilemap[i] <<= 2;
      tilemap[i] |= 3;
    }
  }
}

static void MakeCopperList(CopListT *cp, int i) {
  CopInit(cp);
  CopSetupMode(cp, MODE_LORES, DEPTH);
  CopSetupDisplayWindow(cp, MODE_LORES, X(0), Y(0), WIDTH - 16, HEIGHT - 16);
  CopSetupBitplaneFetch(cp, MODE_LORES, X(-16), WIDTH);
  CopSetupBitplanes(cp, bplptr[i], screen[i], DEPTH);
  bplcon1[i] = CopMove16(cp, bplcon1, 0);
  CopLoadPal(cp, &tiles_pal, 0);
  CopEnd(cp);
}

static void Init(void) {
  /* extra memory for horizontal scrolling */
  short extra = div16(tilemap_width * tileset.width, WIDTH);

  Log("Allocate %d extra lines!\n", extra);

  /* Use interleaved mode to limit number of issued blitter operations. */
  screen[0] = NewBitmapCustom(WIDTH, HEIGHT + extra, DEPTH,
                              BM_CLEAR | BM_DISPLAYABLE | BM_INTERLEAVED);
  screen[1] = NewBitmapCustom(WIDTH, HEIGHT + extra, DEPTH,
                              BM_CLEAR | BM_DISPLAYABLE | BM_INTERLEAVED);

  cp[0] = NewCopList(100);
  MakeCopperList(cp[0], 0);
  cp[1] = NewCopList(100);
  MakeCopperList(cp[1], 1);

  CopListActivate(cp[1]);

  EnableDMA(DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);

  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);

  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

__regargs void TriggerRefresh(short x, short y, short w __unused, short h __unused)
{
  short *map = tilemap;
  int tilemod = tilemap_width - HTILES;

  map += x + (short)y * (short)tilemap_width;

  {
    short j = VTILES - 1;
    do {
      short i = HTILES - 1;

      do {
        *map++ |= 3;
      } while (--i >= 0);

      map += tilemod;
    } while (--j >= 0);
  }
}

#define WAITBLT()                               \
  asm("1: btst #6,%0@(2)\n" /* dmaconr */       \
      "   bnes 1b"                              \
      :: "a" (custom));

static __regargs void UpdateTiles(BitmapT *screen, short x, short y,
                                  volatile struct Custom* const custom asm("a6"))
{
  short *map = tilemap;
  void *ptrs = tileset.ptrs;
  void *dst = screen->planes[0] + (x << 1);
  short size = ((16 * DEPTH) << 6) | 1;
  int tilemod = tilemap_width - HTILES;
  short current = active + 1;

  map += x + (short)y * (short)tilemap_width;

  WAITBLT();

  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltamod = 0;
  custom->bltdmod = (WIDTH - 16) / 8;
  custom->bltcon0 = (SRCA | DEST | A_TO_D);
  custom->bltcon1 = 0;

  {
    short j = VTILES - 1;

    do {
      short i = HTILES - 1;

      do {
        short tile = *map++;

        if (tile & current) {
          void *src = *(void **)(ptrs + (tile & ~3));

          WAITBLT();
          custom->bltapt = src;
          custom->bltdpt = dst;
          custom->bltsize = size;

          map[-1] ^= current;
        }

        dst += 2;
      } while (--i >= 0);

      map += tilemod;
      dst += BPLMOD + 15 * WIDTH * DEPTH / 8;
    } while (--j >= 0);
  }
}

static void Render(void) {
  int lines = ReadLineCounter();
  short t = frameCount;
  short tile = t >> 4;
  short pixel = 15 - (t & 15);

  short x = tile % (tilemap_width - HTILES);
  short y = 35;

  UpdateTiles(screen[active], x, y, custom);

  {
    short i;
    CopInsT **_bplptr = bplptr[active];
    void **_planes = screen[active]->planes;
    int offset = x << 1;

    for (i = 0; i < DEPTH; i++)
      CopInsSet32(_bplptr[i], _planes[i] + offset);
  }
  CopInsSet16(bplcon1[active], pixel | (pixel << 4));
  CopListRun(cp[active]);
  Log("all: %d\n", ReadLineCounter() - lines);

  TaskWait(VBlankEvent);
  active ^= 1;
}

EffectT Effect = { Load, NULL, Init, Kill, Render, NULL };