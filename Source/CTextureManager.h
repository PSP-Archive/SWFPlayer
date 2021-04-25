/***********************************************************************************

  Module :	CTextureManager.h

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

#ifndef CTEXTUREMANAGER_H_
#define CTEXTUREMANAGER_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CString.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CFile;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
class CTexture
{
	public:

		CTexture();
		~CTexture();

		bool	Init( u32 pix_fmt, u32 width, u32 height );

		void	Save( const CString & file_name );

		void	Upload();
		void	Swizzle();
		void	Resize();
		void	Activate() const;

	public:

		u32		m_nPixelSize;
		u32		m_nPixelFormat;
		u32		m_nWidth;
		u32		m_nHeight;
		u32		m_nCanvasWidth;
		u32		m_nCanvasHeight;
		u8 *	m_pBuffer;
		u8 *	m_pOriginalAddress;
		u8 *	m_pVRAM;
		bool	m_bResized;
};

//*************************************************************************************
//
//*************************************************************************************
class CTextureManager
{
	public:

		static CTexture *	Create( const CString & filename, bool resize, bool warn = false );

	private:

		static CTexture *	ReadPNG( CFile * const p_file );
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CTEXTUREMANAGER_H_ */
