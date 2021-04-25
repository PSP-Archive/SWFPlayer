/***********************************************************************************

  Module :	PNGReader.cpp

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
#include "CFrameWork.h"
#include "CTextureManager.h"
#include "CFileSystem.h"
#include <png.h>

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
//
//*************************************************************************************
CTexture *	CTextureManager::ReadPNG( CFile * const p_file )
{
	const size_t	nSigSize( 8 );
	u8				signature[ nSigSize ];
	CTexture *		p_texture( NULL );

	if ( p_file->Read( signature, sizeof( u8 ) * nSigSize ) == false )
	{
		return NULL;
	}

	if ( png_check_sig( signature, nSigSize ) == false )
	{
		return NULL;
	}

	png_struct *	pPngStruct( png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL ) );

	if ( pPngStruct == NULL)
	{
		return NULL;
	}

	png_info *	pPngInfo( png_create_info_struct( pPngStruct ) );

	if ( pPngInfo == NULL )
	{
		png_destroy_read_struct( &pPngStruct, NULL, NULL );

		return NULL;
	}

	if ( setjmp( pPngStruct->jmpbuf ) != 0 )
	{
		png_destroy_read_struct( &pPngStruct, NULL, NULL );

		return NULL;
	}

	png_init_io( pPngStruct, *p_file );
	png_set_sig_bytes( pPngStruct, nSigSize );
	png_read_png( pPngStruct, pPngInfo, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL );

	png_uint_32 width = pPngInfo->width;
	png_uint_32 height = pPngInfo->height;
	int color_type = pPngInfo->color_type;

	png_byte **pRowTable = pPngInfo->row_pointers;
	unsigned char r=0, g=0, b=0, a=0;

	p_texture = new CTexture();

	if ( p_texture != NULL )
	{
		if ( p_texture->Init( GU_PSM_8888, width, height ) == false )
		{
			SAFE_DELETE( p_texture );
		}
		else
		{
			u8 *	p_dest( p_texture->m_pBuffer );

			for ( u32 y = 0; y < height; ++y )
			{
				const png_byte *	pRow( pRowTable[ y ] );

				for ( u32 x = 0; x < width; ++x )
				{
					switch ( color_type )
					{
					case PNG_COLOR_TYPE_GRAY:
						r = g = b = *pRow++;
						if ( r == 0 && g == 0 && b == 0 )	a = 0x00;
						else								a = 0xff;
						break;
					case PNG_COLOR_TYPE_GRAY_ALPHA:
						r = g = b = *pRow++;
						if ( r == 0 && g == 0 && b == 0 )	a = 0x00;
						else								a = 0xff;
						pRow++;
						break;
					case PNG_COLOR_TYPE_RGB:
						b = *pRow++;
						g = *pRow++;
						r = *pRow++;
						if ( r == 0 && g == 0 && b == 0 )	a = 0x00;
						else								a = 0xff;
						break;
					case PNG_COLOR_TYPE_RGB_ALPHA:
						b = *pRow++;
						g = *pRow++;
						r = *pRow++;
						a = *pRow++;
						break;
					}

					p_dest[ ( x << 2 ) + 0 ] = r;
					p_dest[ ( x << 2 ) + 1 ] = g;
					p_dest[ ( x << 2 ) + 2 ] = b;
					p_dest[ ( x << 2 ) + 3 ] = a;
				}

				p_dest += ( p_texture->m_nCanvasWidth << 2 );
			}
		}
	}

	png_destroy_read_struct( &pPngStruct, &pPngInfo, NULL );

	return p_texture;
}

//*******************************  END OF FILE  ************************************
