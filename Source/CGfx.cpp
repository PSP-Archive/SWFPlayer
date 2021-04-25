/***********************************************************************************

  Module :	CGfx.cpp

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

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CGfx.h"
#include "CFrameWork.h"
#include "CTextureManager.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************
u8 *				CGfx::s_pVRAM( NULL );
u8 *				CGfx::s_pBackBuffer( NULL );
u8 *				CGfx::s_pFrontBuffer( NULL );
const u32			CGfx::s_ScreenWidth( 480 );
const u32			CGfx::s_ScreenHeight( 272 );
u8					CGfx::s_FadeLevel( 0 );

static const u32	BUF_WIDTH( 512 );

static u32 __attribute__( ( aligned( 16 ) ) )	s_DrawList[ 0x80000 ];

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//	Initialise the GPU
//*************************************************************************************
bool	CGfx::Open()
{
	sceGuInit();
	sceGuStart( GU_DIRECT, GetDrawList() );

	u8 *	p_vram( 0x00000000 );

	s_pBackBuffer = p_vram;
	sceGuDrawBuffer( GU_PSM_8888, s_pBackBuffer, BUF_WIDTH );

	p_vram += BUF_WIDTH * s_ScreenHeight * 4;
	s_pFrontBuffer = p_vram;
	sceGuDispBuffer( s_ScreenWidth, s_ScreenHeight, s_pFrontBuffer, BUF_WIDTH );

	sceGuClear( GU_COLOR_BUFFER_BIT | GU_STENCIL_BUFFER_BIT );

	sceGuOffset( 2048 - ( s_ScreenWidth / 2 ), 2048 - ( s_ScreenHeight / 2 ) );
	sceGuViewport( 2048, 2048, s_ScreenWidth, s_ScreenHeight );

	sceGuScissor( 0, 0, s_ScreenWidth, s_ScreenHeight );
	sceGuEnable( GU_SCISSOR_TEST );

	sceGuAlphaFunc( GU_GREATER, 0, 0xff );
	sceGuEnable( GU_ALPHA_TEST );

	sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 );
	sceGuEnable( GU_BLEND );

	sceGuShadeModel( GU_SMOOTH );
	sceGuEnable( GU_TEXTURE_2D );

	sceGuDepthMask( GU_TRUE );
	sceGuDepthFunc( GU_ALWAYS );
	sceGuDisable( GU_STENCIL_TEST );

//	sceGuEnable( GU_CULL_FACE );
//	sceGuFrontFace( GU_CCW );

	//
	//	Flush the new GPU settings
	//
	sceGuFinish();
	sceGuSync( 0, 0 );

	sceDisplayWaitVblankStart();
	sceGuDisplay( GU_TRUE );

	//
	//	Clear all the GPU matrices
	//
	ScePspFMatrix4 mtx;

	mtx.x.x = 1.f;	mtx.x.y = 0.f;	mtx.x.z = 0.f;	mtx.x.w = 0.f;
	mtx.y.x = 0.f;	mtx.y.y = 1.f;	mtx.y.z = 0.f;	mtx.y.w = 0.f;
	mtx.z.x = 0.f;	mtx.z.y = 0.f;	mtx.z.z = 1.f;	mtx.z.w = 0.f;
	mtx.w.x = 0.f;	mtx.w.y = 0.f;	mtx.w.z = 0.f;	mtx.w.w = 1.f;

	sceGuSetMatrix( GU_VIEW,		&mtx );
	sceGuSetMatrix( GU_MODEL,		&mtx );
	sceGuSetMatrix( GU_PROJECTION,	&mtx );

	//
	//	Free all of the VRAM
	//
	FreeAllVRAM();

	return true;
}

//*************************************************************************************
//	Shutdown the GPU
//*************************************************************************************
void	CGfx::Close()
{
}

//*************************************************************************************
//	Kick off a draw list
//*************************************************************************************
void	CGfx::BeginRender()
{
	sceGuStart( GU_DIRECT, GetDrawList() );
}

//*************************************************************************************
//	End the drawlist and send it to the GPU
//*************************************************************************************
void	CGfx::EndRender()
{
	if ( s_FadeLevel < 255 )
	{
		sceGuDisable( GU_CULL_FACE );
		CGfx::DrawQuad( V2( 0.f, 0.f ), V2( s_ScreenWidth, s_ScreenHeight ), ARGB( 255 - s_FadeLevel, 0, 0, 0 ) );
	}

	sceGuFinish();
	sceGuSync( 0, 0 );
}

//*************************************************************************************
//	Clear the screen to the specified color
//*************************************************************************************
void	CGfx::ClearScreen( u32 color )
{
	sceGuClearColor( color );
	sceGuClearStencil( 0 );
	sceGuClear( GU_COLOR_BUFFER_BIT | GU_STENCIL_BUFFER_BIT );
}

//*************************************************************************************
//	Swap the display buffers
//*************************************************************************************
void	CGfx::SwapBuffers()
{
	sceDisplayWaitVblankStart();
	sceGuSwapBuffers();

	u8 *	p_temp( s_pBackBuffer );

	s_pBackBuffer = s_pFrontBuffer;
	s_pFrontBuffer = p_temp;
}

//*************************************************************************************
//	Returns a pointer to the display list
//*************************************************************************************
u32 *	CGfx::GetDrawList()
{
	return s_DrawList;
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawQuad( const V2 & pos, const V2 & dimension, ARGB color )
{
	DrawQuad( pos, dimension, color, color, color, color );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawQuad( const V2 & pos, const V2 & dimension, ARGB color0, ARGB color1, ARGB color2, ARGB color3 )
{
	sVertexColor *	p_vert;

	GetPolyList( 4, &p_vert );

	p_vert[ 0 ].color = color0.color;
	p_vert[ 0 ].pos.x = pos.x;
	p_vert[ 0 ].pos.y = pos.y;
	p_vert[ 0 ].pos.z = 0.f;

	p_vert[ 1 ].color = color1.color;
	p_vert[ 1 ].pos.x = pos.x + dimension.x;
	p_vert[ 1 ].pos.y = pos.y;
	p_vert[ 1 ].pos.z = 0.f;

	p_vert[ 2 ].color = color2.color;
	p_vert[ 2 ].pos.x = pos.x;
	p_vert[ 2 ].pos.y = pos.y + dimension.y;
	p_vert[ 2 ].pos.z = 0.f;

	p_vert[ 3 ].color = color3.color;
	p_vert[ 3 ].pos.x = pos.x + dimension.x;
	p_vert[ 3 ].pos.y = pos.y + dimension.y;
	p_vert[ 3 ].pos.z = 0.f;

	DrawPoly2D( GU_TRIANGLE_STRIP, p_vert, 4 );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawQuad( const CTexture * const p_texture, const V2 & pos, const V2 & dimension, ARGB color )
{
	DrawQuad( p_texture, pos, dimension, color, color, color, color );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawQuad( const CTexture * const p_texture, const V2 & pos, const V2 & dimension, ARGB color0, ARGB color1, ARGB color2, ARGB color3 )
{
	sVertexTexturedColor *	p_vert;

	GetPolyList( 4, &p_vert );

	p_vert[ 0 ].uv.x = 0.f;
	p_vert[ 0 ].uv.y = 0.f;
	p_vert[ 0 ].color = color0.color;
	p_vert[ 0 ].pos.x = pos.x;
	p_vert[ 0 ].pos.y = pos.y;
	p_vert[ 0 ].pos.z = 0.f;

	p_vert[ 1 ].uv.x = p_texture->m_nWidth;
	p_vert[ 1 ].uv.y = 0.f;
	p_vert[ 1 ].color = color1.color;
	p_vert[ 1 ].pos.x = pos.x + dimension.x;
	p_vert[ 1 ].pos.y = pos.y;
	p_vert[ 1 ].pos.z = 0.f;

	p_vert[ 2 ].uv.x = 0.f;
	p_vert[ 2 ].uv.y = p_texture->m_nHeight;
	p_vert[ 2 ].color = color2.color;
	p_vert[ 2 ].pos.x = pos.x;
	p_vert[ 2 ].pos.y = pos.y + dimension.y;
	p_vert[ 2 ].pos.z = 0.f;

	p_vert[ 3 ].uv.x = p_texture->m_nWidth;
	p_vert[ 3 ].uv.y = p_texture->m_nHeight;
	p_vert[ 3 ].color = color3.color;
	p_vert[ 3 ].pos.x = pos.x + dimension.x;
	p_vert[ 3 ].pos.y = pos.y + dimension.y;
	p_vert[ 3 ].pos.z = 0.f;

	DrawPoly2D( GU_TRIANGLE_STRIP, p_texture, p_vert, 4 );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::GetPolyList( u32 point_count, sVertexColor ** p_poly_list )
{
	*p_poly_list = ( sVertexColor * )sceGuGetMemory( point_count * sizeof( sVertexColor ) );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::GetPolyList( u32 point_count, sVertexTexturedColor ** p_poly_list )
{
	*p_poly_list = ( sVertexTexturedColor * )sceGuGetMemory( point_count * sizeof( sVertexTexturedColor ) );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawPoly2D( u32 prim_type, const sVertexColor * const p_poly_list, u32 point_count )
{
	sceGuDisable( GU_TEXTURE_2D );
	sceGuShadeModel( GU_SMOOTH );
	sceGuAmbientColor( 0xffffffff );
	sceGuDrawArray( prim_type, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, point_count, 0, p_poly_list );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawPoly2D( u32 prim_type, const CTexture * const p_texture, const sVertexTexturedColor * const p_poly_list, u32 point_count )
{
	SetTexture( p_texture );
	sceGuShadeModel( GU_SMOOTH );
	sceGuAmbientColor( 0xffffffff );
	sceGuDrawArray( prim_type, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, point_count, 0, p_poly_list );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawPoly3D( u32 prim_type, const sVertexColor * const p_poly_list, u32 point_count )
{
	sceGuDisable( GU_TEXTURE_2D );
	sceGuShadeModel( GU_SMOOTH );
	sceGuDrawArray( prim_type, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, point_count, 0, p_poly_list );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawPoly3D( u32 prim_type, const sVertexColorNormal * const p_poly_list, u32 point_count )
{
	sceGuDisable( GU_TEXTURE_2D );
	sceGuShadeModel( GU_SMOOTH );
	sceGuDrawArray( prim_type, GU_NORMAL_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, point_count, 0, p_poly_list );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawPoly3D( u32 prim_type, const CTexture * const p_texture, const sVertexTexturedColor * const p_poly_list, u32 point_count )
{
	SetTexture( p_texture );
	sceGuShadeModel( GU_SMOOTH );
	sceGuDrawArray( prim_type, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, point_count, 0, p_poly_list );
}

//*************************************************************************************
//
//*************************************************************************************
void	CGfx::DrawPoly3D( u32 prim_type, const CTexture * const p_texture, const sVertexTexturedColorNormal * const p_poly_list, u32 point_count )
{
	SetTexture( p_texture );
	sceGuShadeModel( GU_SMOOTH );
	sceGuDrawArray( prim_type, GU_NORMAL_32BITF | GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, point_count, 0, p_poly_list );
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::DrawSprite( const sVertexColor * const p_poly_list, u32 point_count )
{
	sceGuDisable( GU_TEXTURE_2D );
	sceGuShadeModel( GU_SMOOTH );
	sceGuAmbientColor( 0xffffffff );
	sceGuDrawArray( GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, point_count, 0, p_poly_list );
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::DrawSprite( const CTexture * const p_texture, const sVertexTexturedColor * const p_poly_list, u32 point_count )
{
	SetTexture( p_texture );
	sceGuShadeModel( GU_SMOOTH );
	sceGuAmbientColor( 0xffffffff );
	sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, point_count, 0, p_poly_list );
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::SetClipRegion( const V2 & pos, const V2 & size )
{
	sceGuEnable( GU_STENCIL_TEST );					// Stencil test
//	sceGuDepthMask( GU_TRUE );
	sceGuStencilFunc( GU_ALWAYS, 1, 1 );			// always set 1 bit in 1 bit mask
	sceGuStencilOp( GU_KEEP, GU_KEEP, GU_REPLACE );	// keep value on failed test (fail and zfail) and replace on pass
//	sceGuDepthMask( GU_FALSE );

	DrawQuad( pos, size, 0x01ffffff );

	sceGuStencilFunc( GU_EQUAL, 1, 1 );				// allow drawing where stencil is 1
	sceGuStencilOp( GU_KEEP, GU_KEEP, GU_KEEP );	// keep the stencil buffer unchanged
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::DisableClipRegions()
{
	sceGuDisable( GU_STENCIL_TEST );
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::ClearClipRegions()
{
	sceGuClearStencil( 0 );
	sceGuClear( GU_STENCIL_BUFFER_BIT );

	EndRender();
	BeginRender();
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::SetTexture( const CTexture * const p_texture )
{
	sceGuEnable( GU_TEXTURE_2D );
	sceGuTexFunc( GU_TFX_MODULATE, GU_TCC_RGBA );
	sceGuTexWrap( GU_CLAMP, GU_CLAMP );
	sceGuTexFilter( GU_LINEAR, GU_LINEAR );
	sceGuTexScale( 1.0f, 1.0f );
	sceGuTexOffset( 0.0f, 0.0f );

	p_texture->Activate();
}

//**********************************************************************************
//
//**********************************************************************************
void	CGfx::SetFadeLevel( u8 fade_level )
{
	s_FadeLevel = fade_level;
}

//**********************************************************************************
//	Get the next free VRAM address
//**********************************************************************************
u8 *	CGfx::GetFreeVRAM()
{
	return s_pVRAM;
}

//**********************************************************************************
//	Allocate VRAM
//**********************************************************************************
u8 *	CGfx::AllocateVRAM( u32 size )
{
	u8 *	p_vram( s_pVRAM );

	s_pVRAM += size;

	return p_vram;
}

//**********************************************************************************
//	
//**********************************************************************************
void	CGfx::FreeVRAM( u32 size )
{
	s_pVRAM -= size;
}

//**********************************************************************************
//	Free all the allocated VRAM
//**********************************************************************************
void	CGfx::FreeAllVRAM()
{
	s_pVRAM = ( ( u8 * )sceGeEdramGetAddr() );
	s_pVRAM += BUF_WIDTH * s_ScreenHeight * 4 * 2;	// 2 frame buffers
	//s_pVRAM += BUF_WIDTH * s_ScreenHeight * 4;		// Z-Buffer
}

//**********************************************************************************
//	
//**********************************************************************************
u8 *	CGfx::GetBackBuffer()
{
	return s_pBackBuffer;
}

//**********************************************************************************
//	
//**********************************************************************************
u8 *	CGfx::GetFrontBuffer()
{
	return s_pFrontBuffer;
}

//*******************************  END OF FILE  ************************************
