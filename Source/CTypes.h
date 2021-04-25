/***********************************************************************************

  Module :	CTypes.h

  Description :

  Last Modified $Date: $

  $Revision: $

  Copyright (C) 2006 - PSPHacks.net Development Team

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  Contact information
  PSPHacks.net Development Team <webmaster@psphacks.net>
  71M - 71M@Orange.net

***********************************************************************************/

#ifndef CTYPES_H_
#define CTYPES_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <pspsdk.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <psptypes.h>
#include <pspsysmem.h>
#include <psploadexec.h>
#include <psprtc.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psphprm.h>
#include <pspiofilemgr.h>
#include <pspumd.h>
#include <psppower.h>
#include <pspwlan.h>
#include <psputility.h>
#include <sys/unistd.h>
#include <list>
#include <vector>
#include "CMath.h"
#include "CVector.h"
#include "CAssert.h"

//**********************************************************************************
//   Macros
//**********************************************************************************
#define SAFE_DELETE( x )		\
{								\
	delete x;					\
	x = NULL;					\
}

#define SAFE_RDELETE( x )		\
{								\
	delete [] x;				\
	x = NULL;					\
}

#define MAKE_RGB( r, g, b )		( ( 1 << 15 ) | ( ( b >> 3 ) << 10 ) | ( ( g >> 3 ) << 5 ) | ( ( r >> 3 ) << 0 ) )
#define MAKE_ARGB( a, r, g, b )	( ( a << 24 ) | ( b << 16 ) | ( g << 8 ) | ( r << 0 ) )

//**********************************************************************************
//   Types
//**********************************************************************************
struct ARGB
{
	ARGB() : color( 0x00000000 )	{}
	ARGB( u32 col ) : color( col )	{}
	ARGB( u8 r, u8 g, u8 b ) : color( MAKE_ARGB( 255, r, g, b ) )		{}
	ARGB( u8 a, u8 r, u8 g, u8 b ) : color( MAKE_ARGB( a, r, g, b ) )	{}

	union
	{
		u32	color;

		struct
		{
			u8	r, g, b, a;
		};
	};
};

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************


#endif /* CTYPES_H_ */
