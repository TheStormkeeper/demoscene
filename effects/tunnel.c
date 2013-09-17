#include <math.h>

#include "p61/p61.h"

#include "std/debug.h"
#include "std/memory.h"
#include "std/resource.h"

#include "gfx/blit.h"
#include "gfx/hsl.h"
#include "gfx/line.h"
#include "gfx/palette.h"
#include "tools/frame.h"
#include "tools/loopevent.h"

#include "system/c2p.h"
#include "system/display.h"
#include "system/fileio.h"
#include "system/vblank.h"

#include "uvmap/generate.h"
#include "uvmap/render.h"

const int WIDTH = 320;
const int HEIGHT = 256;
const int DEPTH = 8;

/*
 * Set up resources.
 */
void AddInitialResources() {
#if 0
  ResAdd("Module", ReadFileToChipMem("data/tempest-acidjazzed_evening.p61"));
#endif
  ResAdd("Texture", NewPixBufFromFile("data/texture-01.8"));
  ResAdd("TexturePal", NewPaletteFromFile("data/texture-01.pal"));
  ResAdd("CreditsImg", NewPixBufFromFile("data/code.8"));
  ResAdd("CreditsPal", NewPaletteFromFile("data/code.pal"));
  ResAdd("WhelpzImg", NewPixBufFromFile("data/whelpz.8"));
  ResAdd("WhelpzPal", NewPaletteFromFile("data/whelpz.pal"));
  ResAdd("TunnelMap", NewUVMap(WIDTH, HEIGHT, UV_FAST, 256, 256));
  ResAdd("Canvas", NewPixBuf(PIXBUF_CLUT, WIDTH, HEIGHT));
}

/*
 * Set up display function.
 */
bool SetupDisplay() {
  return InitDisplay(WIDTH, HEIGHT, DEPTH);
}

/*
 * Set up effect function.
 */
void SetupEffect() {
  static TunnelPetalsT petals = { 3, 0.333333f, 0.33333f };

  UVMapGenerateTunnel(R_("TunnelMap"), 32.0f, 1, 16.0f / 9.0f, 0.5f, 0.5f, &petals);

  LinkPalettes(R_("TexturePal"), R_("WhelpzPal"), R_("CreditsPal"), NULL);
  LoadPalette(R_("TexturePal"));

  ResAdd("EffectPal", MemClone(R_("TexturePal")));

  PixBufRemap(R_("CreditsImg"), R_("CreditsPal"));
  PixBufRemap(R_("WhelpzImg"), R_("WhelpzPal"));
  PixBufSetBlitMode(R_("CreditsImg"), BLIT_TRANSPARENT);
  PixBufSetBlitMode(R_("WhelpzImg"), BLIT_TRANSPARENT);

#if 0
  P61_Init(R_("Module"), NULL, NULL);
  P61_ControlBlock.Play = 1;
#endif
}

/*
 * Tear down effect function.
 */
void TearDownEffect() {
  P61_End();

  UnlinkPalettes(R_("TexturePal"));
}

/*
 * Effect rendering functions.
 */
typedef void (*PaletteFunctorT)(int frameNumber, HSL *hsl);

void CyclicHue(int frameNumber, HSL *hsl) {
  hsl->h += (float)(frameNumber & 255) / 256.0f;

  if (hsl->h > 1.0f)
    hsl->h -= 1.0f;
}

void PulsingSaturation(int frameNumber, HSL *hsl) {
  float s = sin(frameNumber * 3.14159265f / 50.0f) * 1.00f;
  float change = (s > 0.0f) ? (1.0f - hsl->s) : (hsl->s);

  hsl->s += change * s;
}

void PulsingLuminosity(int frameNumber, HSL *hsl) {
  float s = sin(frameNumber * 3.14159265f / 12.5f) * 0.66f;
  float change = (s > 0.0f) ? (1.0f - hsl->l) : (hsl->l);

  hsl->l += change * s;
}

static PaletteFunctorT PalEffects[] = {
  CyclicHue,
  PulsingSaturation,
  PulsingLuminosity
};

void PaletteEffect(int frameNumber, PaletteT *src, PaletteT *dst, PaletteFunctorT *func) {
  while (src) {
    int i;

    if (!(dst && func && src->count == dst->count && src->start == dst->start))
      break;

    for (i = 0; i < src->count; i++) {
      HSL hsl;

      RGB2HSL(&src->colors[i], &hsl);
      (*func)(frameNumber, &hsl);
      HSL2RGB(&hsl, &dst->colors[i]);
    }

    src = src->next;
    dst = dst->next;
    func++;
  }
}

void RenderTunnel(int frameNumber) {
  PixBufT *canvas = R_("Canvas");

  UVMapSetOffset(R_("TunnelMap"), 0, frameNumber);
  UVMapSetTexture(R_("TunnelMap"), R_("Texture"));
  UVMapRender(R_("TunnelMap"), canvas);

  PixBufBlit(canvas, 0, 137, R_("WhelpzImg"), NULL);

  {
    float rad = (float)(frameNumber % 150) / 150 * 2 * M_PI;
    int w = sin(rad) * 80;
    PixBufBlitScaled(canvas, 200 + (80 - abs(w)) / 2, 20, w, 33, R_("CreditsImg"));
  }

  c2p1x1_8_c5_bm(canvas->data, GetCurrentBitMap(), WIDTH, HEIGHT, 0, 0);
}

/*
 * Main loop.
 */
void MainLoop() {
  SetVBlankCounter(0);

  do {
    int frameNumber = GetVBlankCounter();

    PaletteEffect(frameNumber, R_("TexturePal"), R_("EffectPal"), PalEffects);
    LoadPalette(R_("EffectPal"));

    RenderTunnel(frameNumber);
    RenderFrameNumber(frameNumber);

    DisplaySwap();
  } while (ReadLoopEvent() != LOOP_EXIT);
}
