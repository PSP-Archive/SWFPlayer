/***********************************************************************************

  Module :	CTextureManager.cpp

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
#include "CTextureManager.h"
#include "CFileSystem.h"
#include "CFrameWork.h"
#include "CGfx.h"

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

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//*************************************************************************************
//
//	CTexture manager implementation
//
//*************************************************************************************
//*************************************************************************************

//*************************************************************************************
//
//*************************************************************************************
CTexture *	CTextureManager::Create( const CString & filename, bool warn, bool resize )
{
	CTexture *		p_texture( NULL );
	CFile * const	p_file( CFileSystem::Open( filename, "rb" ) );

	if ( p_file != NULL )
	{
		const CString	extension( p_file->GetExtension() );

		if ( extension == "png" || extension == "PNG" )
		{
			p_texture = ReadPNG( p_file );
		}
		else
		{
			TRACE( "Unrecognised image extension %s\n", extension.GetPtr() );
			ASSERT( 0, "" );
		}

		CFileSystem::Close( p_file );
	}

	if ( p_texture == NULL )
	{
		if ( warn == true )
		{
			TRACE( "Failed to load image %s\n", filename.GetPtr() );
			ASSERT( 0, "" );
		}
	}
	else
	{
		if ( resize == true )
		{
			p_texture->Resize();
		}
	}

	return p_texture;
}


//*************************************************************************************
//*************************************************************************************
//
//	CTexture implementation
//
//*************************************************************************************
//*************************************************************************************

//*************************************************************************************
//
//*************************************************************************************
CTexture::CTexture()
:	m_nPixelSize( 0 )
,	m_nPixelFormat( 0 )
,	m_nWidth( 0 )
,	m_nHeight( 0 )
,	m_nCanvasWidth( 0 )
,	m_nCanvasHeight( 0 )
,	m_pBuffer( NULL )
,	m_pOriginalAddress( NULL )
,	m_pVRAM( NULL )
{
}

//*************************************************************************************
//
//*************************************************************************************
CTexture::~CTexture()
{
	SAFE_RDELETE( m_pOriginalAddress );

	if ( m_pVRAM != NULL )
	{
		CGfx::FreeVRAM( m_nCanvasWidth * m_nHeight * 4 );
	}
}

//**********************************************************************************
//
//**********************************************************************************
bool	CTexture::Init( u32 pix_fmt, u32 width, u32 height )
{
	//
	//	Destroy the previous pixel data
	//
	SAFE_RDELETE( m_pOriginalAddress );

	if ( m_pVRAM != NULL )
	{
		m_pVRAM = NULL;

		CGfx::FreeVRAM( m_nCanvasWidth * m_nHeight * 4 );
	}

	//
	//	Set the member variables
	//
	m_nPixelFormat = pix_fmt;
	m_nWidth = width;
	m_nHeight = height;
	m_nCanvasWidth = 1;
	m_nCanvasHeight = 1;

	//
	//	Find out the size of a single pixel
	//
	switch ( pix_fmt )
	{
	case GU_PSM_5650:	m_nPixelSize = 2;		break;		// - Hicolor, 16-bit
	case GU_PSM_5551:	m_nPixelSize = 2;		break;		// - Hicolor, 16-bit
	case GU_PSM_4444:	m_nPixelSize = 2;		break;		// - Hicolor, 16-bit
	case GU_PSM_8888:	m_nPixelSize = 4;		break;		// - Truecolor, 32-bit
	case GU_PSM_T4:		m_nPixelSize = 1;		break;		// - Indexed, 4-bit (2 pixels per byte)
	case GU_PSM_T8:		m_nPixelSize = 1;		break;		// - Indexed, 8-bit
	default:			ASSERT( 0, "Unknown pixel format!" );	break;
	}

	//
	//	Round the width and height up to a power of 2
	//
	while ( m_nCanvasWidth < width )
	{
		m_nCanvasWidth <<= 1;
	}

	while ( m_nCanvasHeight < height )
	{
		m_nCanvasHeight <<= 1;
	}

	//
	//	Align the memory to 16 bytes
	//
	m_pOriginalAddress = new u8[ ( m_nPixelSize * m_nCanvasWidth * m_nCanvasHeight ) + 16 ];
	m_pBuffer = reinterpret_cast< u8 * >( ( ( reinterpret_cast< u32 >( m_pOriginalAddress ) + 16 ) & 0xfffffff0 ) );

	ASSERT( m_pOriginalAddress != NULL, "Out of memory!" );

	if ( m_pOriginalAddress != NULL )
	{
		memset( m_pBuffer, 0x00, m_nPixelSize * m_nCanvasWidth * m_nCanvasHeight );
	}

	return ( m_pOriginalAddress != NULL );
}

//*************************************************************************************
//
//*************************************************************************************
void	CTexture::Save( const CString & file_name )
{
	CFile * const	p_file( CFileSystem::Open( file_name, "wb" ) );

	ASSERT( p_file != NULL, "Failed to open texture file" );

	if ( p_file->Write( m_pBuffer, m_nPixelSize * m_nCanvasWidth * m_nCanvasHeight ) == false )
	{
		ASSERT( 0, "Failed to save texture" );
	}

	CFileSystem::Close( p_file );
}

//**********************************************************************************
//	
//**********************************************************************************
void	CTexture::Activate() const
{
	sceGuTexMode( m_nPixelFormat, 0, 0, 0 );

	if ( m_pVRAM != NULL )
	{
		sceGuTexImage( 0, m_nCanvasWidth, m_nCanvasHeight, m_nCanvasWidth, m_pVRAM );
	}
	else
	{
		sceGuTexImage( 0, m_nCanvasWidth, m_nCanvasHeight, m_nCanvasWidth, m_pBuffer );
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CTexture::Upload()
{
	sceKernelDcacheWritebackAll();

	m_pVRAM = CGfx::AllocateVRAM( m_nCanvasWidth * m_nHeight * m_nPixelSize );

	sceGuStart( GU_DIRECT, CGfx::GetDrawList() );

	sceGuCopyImage( m_nPixelFormat, 0, 0, m_nWidth, m_nHeight, m_nCanvasWidth, m_pBuffer, 0, 0, m_nCanvasWidth, m_pVRAM );

	sceGuFinish();
	sceGuSync( 0, 0 );

	SAFE_DELETE( m_pOriginalAddress );
	m_pBuffer = NULL;
}

//**********************************************************************************
//
//**********************************************************************************
void	swizzle_fast( u8* out, const u8* in, unsigned int width, unsigned int height )
{
	unsigned int blockx, blocky;
	unsigned int j;

	unsigned int width_blocks = (width / 16);
	unsigned int height_blocks = (height / 8);

	unsigned int src_pitch = (width-16)/4;
	unsigned int src_row = width * 8;

	const u8* ysrc = in;
	u32* dst = (u32*)out;

	for (blocky = 0; blocky < height_blocks; ++blocky)
	{
		const u8* xsrc = ysrc;
		for (blockx = 0; blockx < width_blocks; ++blockx)
		{
			const u32* src = (u32*)xsrc;
			for (j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}

void	CTexture::Swizzle()
{
	//
	//	Create the swizzle buffer
	//
	if ( m_pBuffer != NULL )
	{
		const u32	width( m_nCanvasWidth * m_nPixelSize );
		const u32	texture_size( width * m_nCanvasHeight );
		u8 *		p_buffer( new u8[ texture_size ] );

		swizzle_fast( p_buffer, m_pBuffer, m_nCanvasWidth, m_nCanvasHeight );

		memcpy( m_pBuffer, p_buffer, texture_size );

		SAFE_RDELETE( p_buffer );
	}
}

//**********************************************************************************
//	
//**********************************************************************************
void	CTexture::Resize()
{
	if ( m_pBuffer != NULL )
	{
		static const u32	DIVISION( 2 );
		const u32			new_width( m_nCanvasWidth / DIVISION );
		const u32			new_height( m_nCanvasHeight / DIVISION );
		u8 * const			p_new_address( new u8[ ( m_nPixelSize * new_width * new_height ) + 16 ] );
		u8 * const			p_buffer( reinterpret_cast< u8 * >( ( ( reinterpret_cast< u32 >( p_new_address ) + 16 ) & 0xfffffff0 ) ) );
		const u8 *			src( m_pBuffer );
		u8 *				dst( p_buffer );

		for ( u32 y = 0; y < new_height; ++y )
		{
			for ( u32 x = 0; x < new_width; ++x )
			{
				for ( u32 p = 0; p < m_nPixelSize; ++p )
				{
					*dst++ = *src++;
				}

				src += m_nPixelSize * ( DIVISION - 1 );
			}

			src += m_nPixelSize * m_nCanvasWidth * ( DIVISION - 1 );
		}

		m_nWidth /= DIVISION;
		m_nHeight /= DIVISION;
		m_nCanvasWidth /= DIVISION;
		m_nCanvasHeight /= DIVISION;

		SAFE_RDELETE( m_pOriginalAddress );

		m_pBuffer = p_buffer;
		m_pOriginalAddress = p_new_address;

		m_bResized = true;
	}
}

//*******************************  END OF FILE  ************************************
