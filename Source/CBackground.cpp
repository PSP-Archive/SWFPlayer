/***********************************************************************************

  Module :	CBackground.cpp

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
#include "CTypes.h"
#include "CBackground.h"
#include "CGfx.h"
#include "CRenderable.h"
#include "CMath.h"
#include "CFrameWork.h"
#include "CInput.h"
#include "CString.h"
#include "CTextureManager.h"
#include "CSkinManager.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************
static const u32	NUM_STRIPS( 4 );

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************
static const float	SECTION_WIDTH( 32.f );
static const float	SECTION_HEIGHT1( 12.f );
static const float	SECTION_HEIGHT2( 4.f );
static const ARGB	COLOR0( 0x00ffffff );
static const ARGB	COLOR1( 0x30ffffff );
static const ARGB	COLOR2( 0x20ffffff );
static const ARGB	COLOR3( 0x00808080 );

static bool			s_bVisible( true );
static bool			s_bPBPFadeIn( false );
static s32			s_PBPFadeAlpha( 0 );
static CTexture *	s_pBackgroundTexture( NULL );

static sStripInfo	s_StripInfo[ NUM_STRIPS ] =
{
	{
		0.f,			// m_SinPos
		0.001f,			// m_SinSpeed
		0.15f,			// m_SinPitch
		32.f,			// m_SinYSize
		80.f,			// m_SinYOffset
		false,			// m_bFlipped
	},
	{
		0.9f,			// m_SinPos
		-0.0005f,		// m_SinSpeed
		0.08f,			// m_SinPitch
		32.f,			// m_SinYSize
		80.f,			// m_SinYOffset
		false,			// m_bFlipped
	},
	{
		2.f,			// m_SinPos
		0.002f,			// m_SinSpeed
		0.1f,			// m_SinPitch
		32.f,			// m_SinYSize
		200.f,			// m_SinYOffset
		true,			// m_bFlipped
	},
	{
		0.2f,			// m_SinPos
		-0.0006f,		// m_SinSpeed
		0.2f,			// m_SinPitch
		16.f,			// m_SinYSize
		200.f,			// m_SinYOffset
		true,			// m_bFlipped
	},
};


//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
void	CBackground::Open()
{
	SetVisible( true );

	CRenderable::Register( CRenderable::RO_BACKGROUND, Render );

	for ( u32 i = 0; i < NUM_STRIPS; ++i )
	{
		s_StripInfo[ i ].m_SinPos += ( rand() % 2000 ) / 1000.f;
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CBackground::Close()
{
	CRenderable::UnRegister( CRenderable::RO_BACKGROUND, Render );
}

//**********************************************************************************
//	
//**********************************************************************************
void	CBackground::SetVisible( bool visible )
{
	s_bVisible = visible;
}

//**********************************************************************************
//
//**********************************************************************************
void	CBackground::Render()
{
	if ( s_bVisible == true )
	{
		if ( s_bPBPFadeIn == true )
		{
			s_PBPFadeAlpha += 8;

			SETMAX( s_PBPFadeAlpha, 0xff );
		}
		else
		{
			s_PBPFadeAlpha -= 8;

			SETMIN( s_PBPFadeAlpha, 0x00 );
		}

		CTexture *	p_background_texture( s_pBackgroundTexture );

		if ( CSkinManager::GetComponent( CSkinManager::SC_BACKGROUND )->GetTexture() != NULL )
		{
			p_background_texture = CSkinManager::GetComponent( CSkinManager::SC_BACKGROUND )->GetTexture();
		}

		if ( p_background_texture == NULL )
		{
			CGfx::DrawQuad( V2( 0.f, 0.f ), V2( CGfx::s_ScreenWidth, CGfx::s_ScreenHeight ), 0x00000000 );
		}
		else
		{
			CGfx::DrawQuad( p_background_texture, V2( 0.f, 0.f ), V2( CGfx::s_ScreenWidth, CGfx::s_ScreenHeight ), ARGB( 0xff - s_PBPFadeAlpha, 0xff, 0xff, 0xff ) );
		}

		if ( CSkinManager::GetInt( "background", "background_stripes", 1 ) == 1 )
		{
			for ( u32 i = 0; i < NUM_STRIPS; ++i )
			{
				RenderStrip( s_StripInfo[ i ] );
			}
		}
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CBackground::RenderStrip( sStripInfo & info )
{
	sVertexColor *	p_verts0( NULL );
	sVertexColor *	p_verts1( NULL );
	const u32		num_points( ( 2 * static_cast< u32 >( CGfx::s_ScreenWidth / SECTION_WIDTH ) ) + 2 );

	CGfx::GetPolyList( num_points, &p_verts0 );
	CGfx::GetPolyList( num_points, &p_verts1 );

	for ( u32 i = 0; i < num_points; i += 2 )
	{
		const float	sin_pos( info.m_SinPos + ( i * info.m_SinPitch ) );
		const float	y_pos( info.m_SinYOffset + ( info.m_SinYSize * SIN( sin_pos ) ) );

		if ( info.m_bFlipped == true )
		{
			p_verts0[ i + 0 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts0[ i + 0 ].pos.y = y_pos;
			p_verts0[ i + 0 ].pos.z = 0.f;
			p_verts0[ i + 0 ].color = COLOR0;

			p_verts0[ i + 1 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts0[ i + 1 ].pos.y = y_pos + SECTION_HEIGHT1;
			p_verts0[ i + 1 ].pos.z = 0.f;
			p_verts0[ i + 1 ].color = COLOR1;

			p_verts1[ i + 0 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts1[ i + 0 ].pos.y = y_pos + SECTION_HEIGHT1;
			p_verts1[ i + 0 ].pos.z = 0.f;
			p_verts1[ i + 0 ].color = COLOR2;

			p_verts1[ i + 1 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts1[ i + 1 ].pos.y = y_pos + SECTION_HEIGHT1 + SECTION_HEIGHT2;
			p_verts1[ i + 1 ].pos.z = 0.f;
			p_verts1[ i + 1 ].color = COLOR3;
		}
		else
		{
			p_verts0[ i + 0 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts0[ i + 0 ].pos.y = y_pos;
			p_verts0[ i + 0 ].pos.z = 0.f;
			p_verts0[ i + 0 ].color = COLOR3;

			p_verts0[ i + 1 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts0[ i + 1 ].pos.y = y_pos + SECTION_HEIGHT2;
			p_verts0[ i + 1 ].pos.z = 0.f;
			p_verts0[ i + 1 ].color = COLOR2;

			p_verts1[ i + 0 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts1[ i + 0 ].pos.y = y_pos + SECTION_HEIGHT2;
			p_verts1[ i + 0 ].pos.z = 0.f;
			p_verts1[ i + 0 ].color = COLOR1;

			p_verts1[ i + 1 ].pos.x = ( i / 2 ) * SECTION_WIDTH;
			p_verts1[ i + 1 ].pos.y = y_pos + SECTION_HEIGHT2 + SECTION_HEIGHT1;
			p_verts1[ i + 1 ].pos.z = 0.f;
			p_verts1[ i + 1 ].color = COLOR0;
		}
	}

	CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, p_verts0, num_points );
	CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, p_verts1, num_points );

	info.m_SinPos += info.m_SinSpeed;
}

//**********************************************************************************
//
//**********************************************************************************
void	CBackground::SetBackgroundTexture( CTexture * const p_texture )
{
	if ( p_texture == NULL )
	{
		s_bPBPFadeIn = false;
	}
	else
	{
		s_bPBPFadeIn = true;
	}
}

//*******************************  END OF FILE  ************************************
