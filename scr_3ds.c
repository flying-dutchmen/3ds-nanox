/**
 * 3DS True Color 888 screen driver for Microwindows
 * addapted Derek Carter's :: scr_nds.c
 * too be debuged & revised (c) 2015 Kenny D. Lee
 *
 * This is for use in driving the screen in true color modes
 */
#include <3ds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"

/* Used to identify the Alpha bit of the packed color pixel */
#define ALPHA_BIT (1 << 15)

u8* screenBottom = 0;

/* VGA driver entry points*/
static PSD  n3ds888_open(PSD psd);
static void n3ds888_close(PSD psd);

static void n3ds888_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);

static void n3ds888_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void n3ds888_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);

static MWBOOL n3ds888_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
		int linelen,int size,void *addr);
static void NULL_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
		MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op) {}
static PSD  NULL_allocatememgc(PSD psd) { return NULL; }
static MWBOOL NULL_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
			int linelen,int size,void *addr) { return 0; }
static void NULL_freememgc(PSD mempsd) {}

SUBDRIVER subdrivernds;
SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	n3ds888_open,
	n3ds888_close,
	n3ds888_setpalette,       
	n3ds888_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,        
	n3ds888_update, //NULL,				/* Update*/
	NULL				/* PreSelect*/

};
/* operating mode*/
static MWBOOL VGAMODE = TRUE;	/* ega or vga screen rows*/

/* int10 functions*/
#define FNGR640x480	0x0012	/* function for graphics mode 640x480x16*/
#define FNGR640x350	0x0010	/* function for graphics mode 640x350x16*/
#define FNTEXT		0x0003	/* function for 80x25 text mode*/

static PSD
n3ds888_open(PSD psd)
{
  PSUBDRIVER subdriver;

  /* init driver variables depending on ega/vga mode*/
  psd->xres = psd->xvirtres = 240;
  psd->yres = psd->yvirtres = 340;
  psd->planes = 1;
  psd->bpp = 24;
  psd->ncolors = psd->bpp >= 24? (1 << 24): (1 << psd->bpp);
  psd->pixtype = MWPF_TRUECOLOR888;
  psd->portrait = MWPORTRAIT_NONE;
  psd->data_format = set_data_format(psd);

  /* Calculate the correct size and pitch from xres, yres and bpp*/
  GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp,
		   &psd->size, &psd->pitch);

  psd->flags = PSF_SCREEN;

  /*
   * set and initialize subdriver into screen driver
   * psd->size is calculated by subdriver init
   */
  
  // Currently attempting to use FB16 subdriver
  subdriver = select_fb_subdriver(psd);

  // Check that a valid subdriver exists
  if (!subdriver) 
    {
      EPRINTF("No driver for screen bpp %d\n", psd->bpp);
      return NULL;
    }
 
  psd->orgsubdriver = subdriver;

  set_subdriver(psd, subdriver);

//FB_BOT_1
//  psd->addr = (void*) 0x0008CA00//launcher 0x202118E0;

	srvInit();			// mandatory
	aptInit();			// mandatory
	hidInit(NULL);	// input (buttons, screen)

  gfxInit();
//  screenBottom=0x48F000;
//  screenBottom=gfxGetFramebuffer(GFX_BOTTOM, GFX_BOTTOM, NULL, NULL);

  psd->addr = (void*) gfxGetFramebuffer(GFX_BOTTOM, GFX_BOTTOM, NULL, NULL);
  printf('nano-x --> n3ds888_open \n');
  gfxFlushBuffers();


//    0x1E6000-0x22C500 -- top screen 3D left framebuffer 0(240x400x3) (The "3D right first-framebuf" addr stored in the LCD register is set to this, when the 3D is set to "off")
//    0x22C800-0x272D00 -- top screen 3D left framebuffer 1(240x400x3)
//    0x273000-0x2B9500 -- top screen 3D right framebuffer 0(240x400x3)
//    0x2B9800-0x2FFD00 -- top screen 3D right framebuffer 1(240x400x3)
//    0x48F000-0x4C7400 -- bottom screen framebuffer 0(240x320x3)
//    0x4C7800-0x4FF800 -- bottom screen framebuffer 1(240x320x3) 

  return psd;
}

static void
n3ds888_close(PSD psd)
{
 // powerOff(POWER_LCD);
  gfxExit();
}


void DrawPixel(uint8_t* screen, short x, short y, uint32_t colour)
{
       int h=240;
       u32 v=(h-1-y+x*h)*3;
        screen[v]=colour & 0xFF;         //blue
        screen[v+1]=(colour>>8) & 0xFF;  //green
        screen[v+2]=(colour>>16) & 0xFF; //red
}

/* update framebuffer*/
static void
n3ds888_update(PSD psd, MWCOORD destx, MWCOORD desty, MWCOORD width, MWCOORD height)
{
	if (!width)
		width = psd->xres;
	if (!height)
		height = psd->yres;

/*
typedef unsigned char *		ADDR8;
typedef unsigned short *	ADDR16;
typedef uint32_t *		ADDR32;
*/

	MWCOORD x,y;
	MWPIXELVAL c;

/* got to read from psd->addr and write with DrawPixel()*/

	//if (!((width == 1) || (height == 1))) return;
	//printf("U: %d %d %d %d ",destx,desty,width,height);

/*
if (psd->pixtype == MWPF_TRUECOLOR332)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				c = addr[x];
				DrawPixel(psd->addr, destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		}
	}
else if ((psd->pixtype == MWPF_TRUECOLOR565) || (psd->pixtype == MWPF_TRUECOLOR555))
	{	
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 1);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*2; x++) {
				MWPIXELVAL c = ((unsigned short *)addr)[x]; 
				DrawPixel(psd->addr, destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		}
	}

else /* 
if (psd->pixtype == MWPF_TRUECOLOR888)
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx * 3;
		unsigned int extra = psd->pitch - width * 3;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*3; x++) {
				MWPIXELVAL c = RGB2PIXEL888(addr[2], addr[1], addr[0]);
				DrawPixel(psd->addr, destx+x, desty+y, c);
				addr += 3;
			}
			addr += extra;
		}
	}
else if (((MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)) & (psd->bpp != 8))
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + (destx << 2);
		for (y = 0; y < height; y++) {
			for (x = 0; x < width*4; x++) {				
				MWPIXELVAL c = ((unsigned short *)addr)[x]; //MWPIXELVAL=uint32_t				
				DrawPixel(psd->addr, destx+x, desty+y, c);
	    	}
			addr += psd->pitch;
		}
	}
else /* MWPF_PALETTE*/
	{
		unsigned char *addr = psd->addr + desty * psd->pitch + destx;
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				MWPIXELVAL c = addr[x];
				DrawPixel(psd->addr, destx+x, desty+y, c); 
			}
			addr += psd->pitch;
		}
	}
		gfxFlushBuffers();
//		gfxSwapBuffers();
}

static void
n3ds888_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
  psi->rows = psd->yvirtres;
  psi->cols = psd->xvirtres;
  psi->planes = psd->planes;
  psi->bpp = psd->bpp;
  psi->ncolors = psd->ncolors;
  psi->pixtype = psd->pixtype;
  psi->fonts = NUMBER_FONTS;
  psi->portrait = psd->portrait;
  psi->fbdriver = TRUE;	/* running fb driver, can direct map*/
  psi->pixtype = psd->pixtype;
  switch (psd->pixtype) 
    {
    case MWPF_TRUECOLOR8888:
    case MWPF_TRUECOLOR888:
      psi->rmask 	= 0xff0000;
      psi->gmask 	= 0x00ff00;
      psi->bmask	= 0x0000ff;
      break;
    case MWPF_TRUECOLOR565:
      psi->rmask 	= 0xf800;
      psi->gmask 	= 0x07e0;
      psi->bmask	= 0x001f;
      break;
    case MWPF_TRUECOLOR555:
      psi->rmask 	= 0x7c00;
      psi->gmask 	= 0x03e0;
      psi->bmask	= 0x001f;
      break;
    case MWPF_TRUECOLOR332:
      psi->rmask 	= 0xe0;
      psi->gmask 	= 0x1c;
      psi->bmask	= 0x03;
      break;
    case MWPF_PALETTE:
    default:
      psi->rmask 	= 0xff;
      psi->gmask 	= 0xff;
      psi->bmask	= 0xff;
      break;
    }
  
  if(psd->yvirtres > 480) {
    /* SVGA 800x600*/
    psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
    psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
  } else if(psd->yvirtres > 350) {
    /* VGA 640x480*/
    psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
    psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
  } else if(psd->yvirtres <= 240) {
    /* half VGA 640x240 */
    psi->xdpcm = 14;        /* assumes screen width of 24 cm*/ 
    psi->ydpcm =  5;
  } else {
    /* EGA 640x350*/
    psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
    psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
  }
}

static void
n3ds888_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	/* not yet implemented, no colour palette assumed*/
}

