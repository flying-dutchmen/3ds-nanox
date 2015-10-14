/**
 * Microwindows Nintendo 3DS Mouse Driver
 *  Copyright (c) 2015 Kenny D. Lee
 * Portions Copyright (c) 2011 Derek Carter
 * Portions Copyright (c) 1999, 2000, 2002, 2003 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 */
#include <3ds.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

#define KEY_ALL_DIR   ( KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT )
#define KEY_ALL_ABXY  ( KEY_A | KEY_B | KEY_X | KEY_Y )
#define KEY_ALL_LR    ( KEY_L | KEY_R )
#define KEY_ALL_FRONT ( KEY_ALL_ABXY | KEY_TOUCH | KEY_ALL_DIR )
#define KEY_ALL       ( KEY_ALL_FRONT | KEY_ALL_LR )

/* local routines*/
static int  	N3DS_Open(MOUSEDEVICE *pmd);
static void 	N3DS_Close(void);
static int  	N3DS_GetButtonInfo(void);
static void	N3DS_GetDefaultAccel(int *pscale,int *pthresh);
static int  	N3DS_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bptr);
static int	N3DS_Poll(void);

touchPosition cur_pos;
touchPosition new_pos;
PAD_KEY cur_keypad;
PAD_KEY new_keypad;

MOUSEDEVICE mousedev = {
	N3DS_Open,
	N3DS_Close,
	N3DS_GetButtonInfo,
	N3DS_GetDefaultAccel,
	N3DS_Read,
	N3DS_Poll,
    MOUSE_NORMAL    /* flags*/
};

int N3DS_keypress;  /* for keyboard driver */ 
/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int
N3DS_Open(MOUSEDEVICE *pmd)
{
  
  touchRead( &cur_pos );
  cur_keypad = hidKeysHeld();
  return 1;
}

/*
 * Close the mouse device.
 */
static void
N3DS_Close(void)
{

}

/*
 * Get mouse buttons supported
 */
static int
N3DS_GetButtonInfo(void)
{
  // Supports left and right buttons
  return MWBUTTON_L | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
N3DS_GetDefaultAccel(int *pscale,int *pthresh)
{
  *pscale = SCALE;
  *pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 * status 0 = error
 * status 1 = relative
 * status 2 = exact
 * status 3 = don't move mouse
 */
static int
N3DS_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
  int delta_keypad = (new_keypad ^ cur_keypad );

  *bp = 0;
  *dx = 0;
  *dy = 0;
  *dz = 0;

  // If touch is being pressed right now
  if ( new_keypad & KEY_TOUCH )
    {
      touchRead( &new_pos );
      // On touching the mouse reset current to new
      if (!(delta_keypad & KEY_TOUCH))
	{
	  scanKeys();
	  if (keysHeld() & KEY_TOUCH)
	    {
	      *dx = new_pos.px - cur_pos.px;
	      *dy = new_pos.py - cur_pos.py;
	    }
	}
      cur_pos = new_pos;
    }
  if (new_keypad & KEY_UP)    *dy -= 1;
  if (new_keypad & KEY_DOWN)  *dy += 1;
  if (new_keypad & KEY_LEFT)  *dx -= 1;
  if (new_keypad & KEY_RIGHT) *dx += 1;
  if (new_keypad & KEY_L)
    {
      *bp = MWBUTTON_L;
    }
  if (new_keypad & KEY_R)
    {
      *bp = MWBUTTON_R;
    }
  
  cur_keypad = new_keypad;

// reset keypad
 new_keypad = 0;
  return 1;
}

/*
 * Poll for events
 * 1 = activity
 * 0 = nothing
 */

static int
N3DS_Poll(void)
{
  hidScanInput();
  scanKeys();
  new_keypad = hidKeysHeld();

  // Check for touch changes and movement
  if ( ( ( new_keypad ^ cur_keypad ) & KEY_ALL_FRONT ) ||
       ( new_keypad & KEY_ALL_FRONT ) )
  {
    return 1;
  }
  
  return 0;
}
