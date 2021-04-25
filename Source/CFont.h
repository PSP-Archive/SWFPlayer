/***********************************************************************************

  Module :	CFont.h

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

#ifndef CFONT_H_
#define CFONT_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CVector.h"
#include "CString.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CSkinTexture;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CFont
{
	private:

		enum eFontMapDimensions
		{
			FONT_MAP_WIDTH	= 31,
			FONT_MAP_HEIGHT	= 3,
		};

		struct sGlyphData
		{
			u32	x, y;
			u32	m_Width;
		};

	public:

		static void			Open();
		static void			Close();
		static CFont *		Create( const V2 & size, float space_size );

		static CFont *		GetDefaultFont();

	public:

		~CFont();

		void				Print( const CString & string, const V2 & pos, ARGB color, float scale = 1.f );

		V2					GetStringSize( const CString & string ) const;

	private:

		CFont( const V2 & size, float space_size );

		void				CreateGlyphData();

		const sGlyphData *	FindGlyph( u8 glyph ) const;

	public:

		CSkinTexture *		m_pTexture;
		V2					m_Size;
		sGlyphData			m_GlyphData[ 255 ];
		float				m_fSpaceSize;

	private:

		static CFont *		s_pDefaultFont;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CFONT_H_ */
