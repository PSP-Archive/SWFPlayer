/***********************************************************************************

  Module :	CWindow.cpp

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
#include "CWindow.h"
#include "CWindowItem.h"
#include "CGfx.h"
#include "CFrameWork.h"
#include "CFont.h"
#include "CSkinManager.h"
#include "CTextureManager.h"
#include "CRenderable.h"

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
static std::list< CWindow * >	s_RenderList;
bool							CWindow::s_bVisible( true );

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
void	CWindow::Open()
{
	s_RenderList.clear();
	CRenderable::Register( CRenderable::RO_WINDOWS, RenderAll );
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindow::Close()
{
	s_RenderList.clear();
	CRenderable::UnRegister( CRenderable::RO_WINDOWS, RenderAll );
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindow::RenderAll()
{
	if ( s_bVisible == true )
	{
		for ( std::list< CWindow * >::reverse_iterator it = s_RenderList.rbegin(); it != s_RenderList.rend(); ++it )
		{
			if ( ( *it )->IsVisible() == true )
			{
				( *it )->Render();
			}
		}
	}
}

//**********************************************************************************
//	
//**********************************************************************************
void	CWindow::SetVisibleAll( bool visible )
{
	s_bVisible = visible;
}

//*************************************************************************************
//
//*************************************************************************************
CWindow::CWindow()
:	m_szTitle( "" )
,	m_bInFocus( false )
,	m_bVisible( true )
{
	m_ItemList.clear();

	s_RenderList.push_back( this );
}

//*************************************************************************************
//
//*************************************************************************************
CWindow::~CWindow()
{
	while ( m_ItemList.empty() == false )
	{
		CWindowItem *	p_item( m_ItemList.back() );

		delete p_item;
	}

	s_RenderList.remove( this );
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::Render()
{
	V2			pos( GetScreenPos() );
	const V2	size( GetSize() );
	const ARGB	color( GetColor() );
	const ARGB	title_color( GetTitleColor() );
	const V2	title_offset( CSkinManager::GetV2( "window", "title_offset", V2( 0.f, 0.f ) ) );

	DrawWindow( pos, size, color );

	const V2	title_size( CFont::GetDefaultFont()->GetStringSize( m_szTitle ) * GetScale() );

	if ( title_size.x > ( size.x - 32.f ) )
	{
		V2	clip_size( size );

		clip_size.x -= 32.f;

		clip_size.y = title_size.y;

		pos += title_offset;
		pos.x += 16.f;

		CGfx::SetClipRegion( pos, clip_size );

		CFont::GetDefaultFont()->Print( m_szTitle, pos, title_color, GetScale() );

		CGfx::DisableClipRegions();
	}
	else
	{
		V2	mid_pos( pos );

		mid_pos.x += ( size.x - title_size.x ) * 0.5f;
		mid_pos.y += title_offset.y;

		CFont::GetDefaultFont()->Print( m_szTitle, mid_pos, title_color, GetScale() );
	}

	for ( CWindowItemList::iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		( *it )->Render();
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::Process()
{
	for ( CWindowItemList::iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		( *it )->Process();
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::ProcessInput()
{
	if ( HasFocus() == true )
	{
		for ( CWindowItemList::iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
		{
			if ( ( *it )->HasFocus() == true )
			{
				( *it )->ProcessInput();
			}
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::AddItem( CWindowItem * const p_item )
{
	p_item->SetParent( this );

	m_ItemList.push_back( p_item );
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindow::RemoveItem( CWindowItem * const p_item )
{
	m_ItemList.remove( p_item );
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::ClearItems()
{
	m_ItemList.clear();
}

//*************************************************************************************
//
//*************************************************************************************
const CString &	CWindow::GetTitle() const
{
	return m_szTitle;
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::SetTitle( const CString & title )
{
	m_szTitle = title;
}

//*************************************************************************************
//
//*************************************************************************************
bool	CWindow::HasFocus() const
{
	return m_bInFocus;
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindow::SetFocus( bool focus )
{
	m_bInFocus = focus;

	if ( focus == true )
	{
		s_RenderList.remove( this );
		s_RenderList.push_front( this );
	}
	else
	{
		s_RenderList.remove( this );
		s_RenderList.push_back( this );
	}

	for ( CWindowItemList::iterator it = m_ItemList.begin(); it != m_ItemList.end(); ++it )
	{
		( *it )->SetFocus( focus );
	}
}

//**********************************************************************************
//
//**********************************************************************************
bool	CWindow::IsVisible() const
{
	return m_bVisible;
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindow::SetVisible( bool visible )
{
	m_bVisible = visible;
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindow::DrawWindow( const V2 & pos, const V2 & size, ARGB color )
{
	CTexture * const	p_t_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_TOP )->GetTexture() );
	CTexture * const	p_b_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_BOTTOM )->GetTexture() );
	CTexture * const	p_l_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_LEFT )->GetTexture() );
	CTexture * const	p_r_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_RIGHT )->GetTexture() );
	CTexture * const	p_c_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_CENTER )->GetTexture() );
	CTexture * const	p_tl_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_TOP_LEFT )->GetTexture() );
	CTexture * const	p_tr_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_TOP_RIGHT )->GetTexture() );
	CTexture * const	p_br_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_BOTTOM_RIGHT )->GetTexture() );
	CTexture * const	p_bl_tex( CSkinManager::GetComponent( CSkinManager::SC_WND_BOTTOM_LEFT )->GetTexture() );

	CGfx::DrawQuad( p_tl_tex, V2( pos.x, pos.y ), V2( p_tl_tex->m_nWidth, p_tl_tex->m_nHeight ), color );
	CGfx::DrawQuad( p_tr_tex, V2( pos.x + size.x - p_tr_tex->m_nWidth, pos.y ), V2( p_tr_tex->m_nWidth, p_tr_tex->m_nHeight ), color );

	CGfx::DrawQuad( p_bl_tex, V2( pos.x, pos.y + size.y - p_bl_tex->m_nHeight ), V2( p_bl_tex->m_nWidth, p_bl_tex->m_nHeight ), color );
	CGfx::DrawQuad( p_br_tex, V2( pos.x + size.x - p_tr_tex->m_nWidth, pos.y + size.y - p_br_tex->m_nHeight ), V2( p_br_tex->m_nWidth, p_br_tex->m_nHeight ), color );

	CGfx::DrawQuad( p_t_tex, V2( pos.x + p_tl_tex->m_nWidth, pos.y ), V2( size.x - p_tl_tex->m_nWidth - p_tr_tex->m_nWidth, p_t_tex->m_nHeight ), color );
	CGfx::DrawQuad( p_b_tex, V2( pos.x + p_bl_tex->m_nWidth, pos.y + size.y - p_b_tex->m_nHeight ), V2( size.x - p_bl_tex->m_nWidth - p_br_tex->m_nWidth, p_b_tex->m_nHeight ), color );

	CGfx::DrawQuad( p_l_tex, V2( pos.x, pos.y + p_tl_tex->m_nHeight ), V2( p_l_tex->m_nWidth, size.y - p_tl_tex->m_nHeight - p_bl_tex->m_nHeight ), color );
	CGfx::DrawQuad( p_r_tex, V2( pos.x + size.x - p_r_tex->m_nWidth, pos.y + p_tr_tex->m_nHeight ), V2( p_r_tex->m_nWidth, size.y - p_tr_tex->m_nHeight - p_br_tex->m_nHeight ), color );

	CGfx::DrawQuad( p_c_tex, V2( pos.x + p_tl_tex->m_nWidth, pos.y + p_tl_tex->m_nHeight ), V2( size.x - p_tl_tex->m_nWidth - p_br_tex->m_nWidth, size.y - p_tl_tex->m_nHeight - p_br_tex->m_nHeight ), color );
}

//**********************************************************************************
//
//**********************************************************************************
ARGB	CWindow::GetColor()
{
	if ( HasFocus() == true )
	{
		return CSkinManager::GetColor( "window", "color_on", 0xffffffff );
	}
	else
	{
		return CSkinManager::GetColor( "window", "color_off", 0x80808080 );
	}
}

//**********************************************************************************
//
//**********************************************************************************
ARGB	CWindow::GetTextColor()
{
	if ( HasFocus() == true )
	{
		return CSkinManager::GetColor( "window", "text_color_on", 0xffffffff );
	}
	else
	{
		return CSkinManager::GetColor( "window", "text_color_off", 0x80808080 );
	}
}

//**********************************************************************************
//
//**********************************************************************************
ARGB	CWindow::GetTitleColor()
{
	if ( HasFocus() == true )
	{
		return CSkinManager::GetColor( "window", "title_color_on", 0xffffffff );
	}
	else
	{
		return CSkinManager::GetColor( "window", "title_color_off", 0x80808080 );
	}
}

//*******************************  END OF FILE  ************************************
