/***********************************************************************************

  Module :	CWindowText.cpp

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
#include "CWindowText.h"
#include "CFont.h"

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
CWindowText::CWindowText( const CString & text )
:	m_szText( text )
{
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowText::Render()
{
	CFont::GetDefaultFont()->Print( m_szText, GetScreenPos(), CWindowItem::GetColor() );
}

//*************************************************************************************
//
//*************************************************************************************
const CString &	CWindowText::GetText() const
{
	return m_szText;
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowText::SetText( const CString & text )
{
	m_szText = text;

	SetSize( CFont::GetDefaultFont()->GetStringSize( m_szText ) );
}

//*******************************  END OF FILE  ************************************
