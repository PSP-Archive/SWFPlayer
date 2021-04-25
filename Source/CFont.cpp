/***********************************************************************************

  Module :	CFont.cpp

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
#include "CFont.h"
#include "CFrameWork.h"
#include "CTextureManager.h"
#include "CGfx.h"
#include "CSkinManager.h"

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
CFont *		CFont::s_pDefaultFont( NULL );

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
void	CFont::Open()
{
	//
	//	Create the default font
	//
	s_pDefaultFont = new CFont( CSkinManager::GetV2( "font", "size", V2( 7.f, 12.f ) ), CSkinManager::GetFloat( "font", "space_size", 7.f ) );
}

//*************************************************************************************
//
//*************************************************************************************
void	CFont::Close()
{
	SAFE_DELETE( s_pDefaultFont );
}

//*************************************************************************************
//
//*************************************************************************************
CFont *	CFont::Create( const V2 & size, float space_size )
{
	return new CFont( size, space_size );
}

//*************************************************************************************
//
//*************************************************************************************
CFont *	CFont::GetDefaultFont()
{
	ASSERT( s_pDefaultFont != NULL, "Trying to access a NULL font" );

	return s_pDefaultFont;
}

//*************************************************************************************
//
//*************************************************************************************
CFont::CFont( const V2 & size, float space_size )
:	m_Size( size )
,	m_fSpaceSize( space_size )
{
	m_pTexture = CSkinManager::GetComponent( CSkinManager::SC_FONT );

	ASSERT( m_pTexture != NULL, "Couldn't create the font!" );

	CreateGlyphData();
}

//*************************************************************************************
//
//*************************************************************************************
CFont::~CFont()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	CFont::CreateGlyphData()
{
	u8						glyph( 32 );
	const CTexture * const	p_texture( m_pTexture->GetTexture() );
	const u8 * const		p_pixels( p_texture->m_pBuffer );
	const s32				font_width( static_cast< s32 >( m_Size.x ) );
	const s32				font_height( static_cast< s32 >( m_Size.y ) );
	sGlyphData *			p_glyph_data( m_GlyphData );

	for ( u32 y = 0; y < p_texture->m_nHeight; y += font_height )
	{
		for ( u32 x = 0; x < p_texture->m_nWidth; x += font_width )
		{
			s32					glyph_end( 0 );
			s32					glyph_begin( font_width );
			const u8 * const	p_glyph_pix( &p_pixels[ ( x + ( y * p_texture->m_nCanvasWidth ) ) * 4 ] );

			for ( s32 gx = 0; gx < font_width; ++gx )
			{
				for ( s32 gy = 0; gy < font_height; ++gy )
				{
					const u32	offset( ( gx + ( gy * p_texture->m_nCanvasWidth ) ) * 4 );

					if ( p_glyph_pix[ offset + 0 ] != 0 ||
						 p_glyph_pix[ offset + 1 ] != 0 ||
						 p_glyph_pix[ offset + 2 ] != 0 ||
						 p_glyph_pix[ offset + 3 ] != 0 )
					{
						if ( gx <= glyph_begin )
						{
							glyph_begin = gx;
						}

						if ( gx >= glyph_end )
						{
							glyph_end = gx;
						}

						break;
					}
				}
			}

			if ( glyph_end == 0 )
			{
				glyph_end = font_width;
			}

			if ( glyph_begin == font_width )
			{
				glyph_begin = 0;
			}

			p_glyph_data->x = x + glyph_begin;
			p_glyph_data->y = y;
			p_glyph_data->m_Width = ( glyph_end - glyph_begin ) + 1;

			++glyph;
			++p_glyph_data;
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
const CFont::sGlyphData *	CFont::FindGlyph( u8 glyph ) const
{
	if ( glyph < 0x20 )
	{
		return &m_GlyphData[ 0x21 ];
	}

	return &m_GlyphData[ glyph - 0x20 ];
}

//*************************************************************************************
//
//*************************************************************************************
void	CFont::Print( const CString & string, const V2 & pos, ARGB color, float scale )
{
	const u8 *				p_char( ( const u8 * )( string.GetPtr() ) );
	const V2				orig_pos( static_cast< float >( static_cast< s32 >( pos.x ) ), static_cast< float >( static_cast< s32 >( pos.y ) ) );
	V2						text_pos( orig_pos );
	u32						sprite_count( 0 );
	sVertexTexturedColor *	p_vert;
	sVertexTexturedColor *	p_dst;
	const CTexture * const	p_texture( m_pTexture->GetTexture() );

	CGfx::GetPolyList( 2 * string.Length(), &p_vert );

	p_dst = p_vert;

	for ( s32 i = 0; i < string.Length(); ++i )
	{
		switch ( *p_char )
		{
		case ' ':
			{
				text_pos.x += m_fSpaceSize;
			}
			break;

		case '\t':
			{
				text_pos.x += 8 * m_fSpaceSize;
			}
			break;

		case '\n':
			{
				text_pos.x = orig_pos.x;
				text_pos.y += ( ( m_Size.y + 1 ) * scale );
			}
			break;

		default:
			{
				const sGlyphData * const	p_glyph_data( FindGlyph( *p_char ) );

				if ( p_glyph_data != NULL )
				{
					p_dst->uv.x = p_glyph_data->x;
					p_dst->uv.y = p_glyph_data->y;
					p_dst->color = color;
					p_dst->pos.x = text_pos.x;
					p_dst->pos.y = text_pos.y;
					p_dst->pos.z = 0.f;
					++p_dst;

					p_dst->uv.x = p_glyph_data->x + p_glyph_data->m_Width;
					p_dst->uv.y = p_glyph_data->y + m_Size.y;
					p_dst->color = color;
					p_dst->pos.x = text_pos.x + ( p_glyph_data->m_Width * scale );
					p_dst->pos.y = text_pos.y + ( m_Size.y * scale );
					p_dst->pos.z = 0.f;
					++p_dst;

					text_pos.x += ( ( p_glyph_data->m_Width + 1 ) * scale );

					sprite_count += 2;
				}
				else
				{
					BREAK_POINT( "Couldn't find glyph!" );

					text_pos.x += ( m_Size.x * scale );
				}
			}
			break;
		}

		p_char++;
	}

	CGfx::DrawSprite( p_texture, p_vert, sprite_count );
}

//*************************************************************************************
//
//*************************************************************************************
V2	CFont::GetStringSize( const CString & string ) const
{
	const u8 *	p_char( ( const u8 * )( string.GetPtr() ) );
	V2			size( 0.f, m_Size.y );
	float		last_x( 0.f );

	for ( s32 i = 0; i < string.Length(); ++i )
	{
		switch ( *p_char )
		{
		case ' ':
			{
				size.x += m_fSpaceSize;
			}
			break;

		case '\t':
			{
				size.x += 8 * m_fSpaceSize;
			}
			break;

		case '\n':
			{
				if ( last_x < size.x )
				{
					last_x = size.x;
				}

				size.x = 0.f;
				size.y += ( m_Size.y + 1 );
			}
			break;

		default:
			{
				const sGlyphData * const	p_glyph_data( FindGlyph( *p_char ) );

				if ( p_glyph_data != NULL )
				{
					size.x += ( p_glyph_data->m_Width + 1 );
				}
			}
			break;
		}

		p_char++;
	}

	if ( last_x > size.x )
	{
		size.x = last_x;
	}

	return size;
}

//*******************************  END OF FILE  ************************************
