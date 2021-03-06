/***********************************************************************************

  Module :	CWindowItem.cpp

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
#include "CWindowItem.h"
#include "CWindow.h"
#include "CGfx.h"
#include "CFrameWork.h"
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

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
CWindowItem::CWindowItem()
:	m_pParent( NULL )
,	m_bInFocus( false )
{
}

//*************************************************************************************
//
//*************************************************************************************
CWindowItem::~CWindowItem()
{
	m_pParent->RemoveItem( this );
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowItem::Render()
{
	CGfx::DrawQuad( GetScreenPos(), GetSize(), 0xff808080 );
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowItem::Process()
{
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowItem::ProcessInput()
{
}

//**********************************************************************************
//
//**********************************************************************************
CWindow *	CWindowItem::GetParent() const
{
	return m_pParent;
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowItem::SetParent( CWindow * const p_parent )
{
	m_pParent = p_parent;
}

//*************************************************************************************
//
//*************************************************************************************
bool	CWindowItem::HasFocus() const
{
	return m_bInFocus;
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowItem::SetFocus( bool focus )
{
	m_bInFocus = focus;
}

//*************************************************************************************
//
//*************************************************************************************
V2	CWindowItem::GetScreenPos() const
{
	V2	pos( GetPos() * GetScale() );

	if ( m_pParent != NULL )
	{
		pos += m_pParent->GetScreenPos();
	}

	return pos;
}

//**********************************************************************************
//
//**********************************************************************************
float	CWindowItem::GetScale() const
{
	float	scale( m_fScale );

	if ( m_pParent != NULL )
	{
		scale *= m_pParent->GetScale();
	}

	return scale;
}

//**********************************************************************************
//
//**********************************************************************************
ARGB	CWindowItem::GetColor() const
{
	if ( HasFocus() == true )
	{
		return CSkinManager::GetColor( "window_item", "text_color_on", 0xffffffff );
	}
	else
	{
		return CSkinManager::GetColor( "window_item", "text_color_off", 0x80ffffff );
	}
}

//*******************************  END OF FILE  ************************************
