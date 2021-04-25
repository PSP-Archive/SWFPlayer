/***********************************************************************************

  Module :	CMath.h

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

#ifndef CMATH_H_
#define CMATH_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include <math.h>
#include <float.h>

//**********************************************************************************
//   Constants
//**********************************************************************************

//
//	Pi related defines
//
#define G_PI					3.14159265358979323846f
#define G_PI_ON_2				(G_PI / 2.f)
#define	G_PI_TIMES_2			(2.f * G_PI)
#define G_3_PI_ON_2				(G_PI + G_PI_ON_2)

//
//	Ranges
//
const float EPSILON				= 0.0001f;

const float VERY_SMALL			= 1.0e-5f;
const float VERY_LOW			= -1.0e20f;
const float VERY_HIGH			= 1.0e20f;

const float ONE_DEGREE			= (0.0174532925199432957692369076848861271344f);
const float INV_ONE_DEGREE		= (1.f / ONE_DEGREE);

//**********************************************************************************
//   Macros
//**********************************************************************************
#define SIN( x )				sinf( x )
#define COS( x )				cosf( x )
#define ACOS( x )				acosf( x )

#define MAX(a,b)				((a)>(b)?(a):(b))
#define MIN(a,b)				((a)<(b)?(a):(b))
#define SETMIN(a,b)				{if((b)>(a)) (a)=(b);}
#define SETMAX(a,b)				{if((b)<(a)) (a)=(b);}
#define SQUARE(x)				((x)*(x))

#define INTER(l,r,rmul)			((l)+((r)-(l))*(rmul))
#define INTER2(l,r,rmul,u,umul)	((l)+((r)-(l))*(rmul)+((u)-(l))*(umul))
#define bound(LOW,VALUE,HIGH)	((VALUE)>(LOW)?((VALUE)<(HIGH)?(VALUE):(HIGH)):(LOW))

#define SQRTF( x )				( float( sqrt( x ) ) )
#define SQRTI( x )				sqrti( x )

#define DEGS_TO_RADS( val )		(val * ONE_DEGREE)
#define RADS_TO_DEGS( val )		(val * INV_ONE_DEGREE)

//**********************************************************************************
//   Types
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
inline float	FABSF( const float x )
{
	s32 * const	t( ( s32 * )&x );
	*t &= 0x7fffffff;
	return x;
}

u32	sqrti( u32 d );

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#include "CVector.h"


#endif /* _MATH_H_ */
