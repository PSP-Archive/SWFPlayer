/***********************************************************************************

  Module :	CGfx.h

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

#ifndef CGFX_H_
#define CGFX_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CTexture;

struct sVertexColor
{
	ARGB	color;
	V3		pos;
};

struct sVertexColorNormal
{
	ARGB	color;
	V3		normal;
	V3		pos;
};

struct sVertexTexturedColor
{
	V2		uv;
	ARGB	color;
	V3		pos;
};

struct sVertexTexturedColorNormal
{
	V2		uv;
	ARGB	color;
	V3		normal;
	V3		pos;
};

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CGfx
{
	public:

		// Initialise the GPU
		static bool			Open();

		// Shutdown the GPU
		static void			Close();

		// Prepares the draw list for use
		static void			BeginRender();

		// Terminates the current draw list
		static void			EndRender();

		// Clears the screen to the specified ARGB color
		static void			ClearScreen( u32 color );

		// Swap the draw list buffers
		static void			SwapBuffers();

		// Returns the address of the current draw list
		static u32 *		GetDrawList();

		// Draw a gouraud shaded quad
		static void			DrawQuad( const V2 & pos, const V2 & dimension, ARGB color );
		static void			DrawQuad( const V2 & pos, const V2 & dimension, ARGB color0, ARGB color1, ARGB color2, ARGB color3 );

		static void			DrawQuad( const CTexture * const p_texture, const V2 & pos, const V2 & dimension, ARGB color );
		static void			DrawQuad( const CTexture * const p_texture, const V2 & pos, const V2 & dimension, ARGB color0, ARGB color1, ARGB color2, ARGB color3 );

		// Draw a sprite list
		static void			DrawSprite( const sVertexColor * const p_poly_list, u32 point_count );
		static void			DrawSprite( const CTexture * const p_texture, const sVertexTexturedColor * const p_poly_list, u32 point_count );

		// Returns a GPU poly list
		static void			GetPolyList( u32 point_count, sVertexColor ** p_poly_list );
		static void			GetPolyList( u32 point_count, sVertexColorNormal ** p_poly_list );
		static void			GetPolyList( u32 point_count, sVertexTexturedColor ** p_poly_list );
		static void			GetPolyList( u32 point_count, sVertexTexturedColorNormal ** p_poly_list );

		// Draw a 2D polygon list
		static void			DrawPoly2D( u32 prim_type, const sVertexColor * const p_poly_list, u32 point_count );
		static void			DrawPoly2D( u32 prim_type, const CTexture * const p_texture, const sVertexTexturedColor * const p_poly_list, u32 point_count );

		// Draw a 2D polygon list
		static void			DrawPoly3D( u32 prim_type, const sVertexColor * const p_poly_list, u32 point_count );
		static void			DrawPoly3D( u32 prim_type, const sVertexColorNormal * const p_poly_list, u32 point_count );
		static void			DrawPoly3D( u32 prim_type, const CTexture * const p_texture, const sVertexTexturedColor * const p_poly_list, u32 point_count );
		static void			DrawPoly3D( u32 prim_type, const CTexture * const p_texture, const sVertexTexturedColorNormal * const p_poly_list, u32 point_count );

		// Set the current texture
		static void			SetTexture( const CTexture * const p_texture );

		// Sets a clip region using the stencil buffer
		static void			SetClipRegion( const V2 & pos, const V2 & size );

		// Sets a clip region using the stencil buffer
		static void			DisableClipRegions();

		// Clears the stencil buffer
		static void			ClearClipRegions();

		// Set the screen fade level 0 - 255, 0 = black
		static void			SetFadeLevel( u8 fade_level );

		// Get the next free VRAM address
		static u8 *			GetFreeVRAM();

		// Allocate and free VRAM
		static u8 *			AllocateVRAM( u32 size );
		static void			FreeVRAM( u32 size );

		// Free all the allocated VRAM
		static void			FreeAllVRAM();

		// Returns the front and back frame buffers
		static u8 *			GetBackBuffer();
		static u8 *			GetFrontBuffer();

	public:

		// Address of free VRAM
		static u8 *			s_pVRAM;

		// Front and back frame buffer addresses
		static u8 *			s_pBackBuffer;
		static u8 *			s_pFrontBuffer;

		// Width and height of the screen
		static const u32	s_ScreenWidth;
		static const u32	s_ScreenHeight;

		// Fade level of the screen
		static u8			s_FadeLevel;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CGFX_H_ */
