/***********************************************************************************

  Module :	CWindowTable.cpp

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
#include "CWindowTable.h"
#include "CWindow.h"
#include "CGfx.h"
#include "CInput.h"
#include "CFrameWork.h"
#include "CTypes.h"
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
static const s32	PAD_SLOW_REPEAT( 8 );
static const s32	PAD_FAST_REPEAT( 2 );
static const s32	MAX_REPEAT_COUNT( 2 );
static const float	SCROLL_SPEED( 20.f );

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//**********************************************************************************
//
//	Window table item implementation
//
//**********************************************************************************
//**********************************************************************************
CWindowTableItem::CWindowTableItem()
:	m_pParent( NULL )
,	m_pCallback( NULL )
{
}

//**********************************************************************************
//
//**********************************************************************************
CWindowTableItem::~CWindowTableItem()
{
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTableItem::Process()
{
}

//**********************************************************************************
//
//**********************************************************************************
V2	CWindowTableItem::Render( V2 pos, bool highlight, float scroll_offset )
{
	return pos;
}

//**********************************************************************************
//
//**********************************************************************************
V2	CWindowTableItem::GetSize() const
{
	return V2( 0.f, 0.f );
}

//**********************************************************************************
//
//**********************************************************************************
CWindowTable *	CWindowTableItem::GetParent() const
{
	return m_pParent;
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTableItem::SetParent( CWindowTable * const p_parent )
{
	m_pParent = p_parent;
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTableItem::Select( u32 item_no )
{
	if ( m_pCallback != NULL )
	{
		m_pCallback( this, item_no );
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTableItem::SetCallback( SelectCallback callback )
{
	m_pCallback = callback;
}


//**********************************************************************************
//**********************************************************************************
//
//	Window table implementation
//
//**********************************************************************************
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
CWindowTable::CWindowTable( bool wrap_selection )
:	m_nCursorPos( 0 )
,	m_nItemsOffset( 0 )
,	m_nMaxDisplayItems( 0 )
,	m_fScrollOffset( 0.f )
,	m_nRepeatCount( 0 )
,	m_bSizeDirty( true )
,	m_bWrapSelection( wrap_selection )
{
	m_Items.clear();
}

//*************************************************************************************
//
//*************************************************************************************
CWindowTable::~CWindowTable()
{
	ClearItems();
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowTable::Render()
{
	s32			cur_item( 0 );
	s32			num_displayed( 0 );
	V2			pos( GetScreenPos() );
	const ARGB	highlight_color( CSkinManager::GetColor( "window_table", "highlight_color", 0x80404040 ) );

	if ( m_bSizeDirty == true )
	{
		m_bSizeDirty = false;

		CalculateMaxDisplayItems();
	}

	for ( CTableItemList::iterator it = m_Items.begin(); it != m_Items.end(); ++it )
	{
		if ( num_displayed < m_nMaxDisplayItems )
		{
			if ( cur_item >= m_nItemsOffset )
			{
				CWindowTableItem * const	p_item( *it );

				if ( num_displayed != m_nCursorPos )
				{
					pos = p_item->Render( pos, false, 0.f );
				}
				else
				{
					if ( HasFocus() == true )
					{
						CGfx::DrawQuad( pos, V2( GetSize().x, p_item->GetSize().y ), highlight_color );
					}

					pos = p_item->Render( pos, true, m_fScrollOffset * GetScale() );
				}

				++num_displayed;
			}
		}

		++cur_item;
	}

	CGfx::ClearClipRegions();
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowTable::Process()
{
	CapSelection();

	for ( CTableItemList::iterator it = m_Items.begin(); it != m_Items.end(); ++it )
	{
		( *it )->Process();
	}

	//
	//	Update scrolling
	//
	const CWindowTableItem * const	p_item( GetSelectedItem() );

	if ( p_item != NULL )
	{
		if ( ( p_item->GetSize().x > GetSize().x ) && ( HasFocus() == true ) )
		{
			m_fScrollOffset -= SCROLL_SPEED * CFrameWork::GetElapsedTime();

			if ( m_fScrollOffset < -p_item->GetSize().x )
			{
				m_fScrollOffset += p_item->GetSize().x;
			}
		}
		else
		{
			m_fScrollOffset = 0.f;
		}
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::ScrollUp()
{
	--m_nCursorPos;

	CapSelection();
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::ScrollDown()
{
	++m_nCursorPos;

	CapSelection();
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::CapSelection()
{
	if ( m_nCursorPos < 0 )
	{
		m_nCursorPos = 0;

		--m_nItemsOffset;

		if ( m_nItemsOffset < 0 )
		{
			if ( m_bWrapSelection == true )
			{
				m_nCursorPos = ( m_nMaxDisplayItems - 1 );
				m_nItemsOffset = ( m_Items.size() - m_nMaxDisplayItems );
			}
			else
			{
				m_nItemsOffset = 0;
			}
		}
	}

	if ( m_nCursorPos > ( m_nMaxDisplayItems - 1 ) )
	{
		m_nCursorPos = ( m_nMaxDisplayItems - 1 );

		++m_nItemsOffset;

		if ( ( m_nItemsOffset + m_nMaxDisplayItems ) > static_cast< s32 >( m_Items.size() ) )
		{
			if ( m_bWrapSelection == true )
			{
				m_nCursorPos = 0;
				m_nItemsOffset = 0;
			}
			else
			{
				m_nItemsOffset = ( m_Items.size() - m_nMaxDisplayItems );
			}
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowTable::ProcessInput()
{
	const s32	old_cursor_pos( m_nCursorPos );
	const s32	old_item_offset( m_nItemsOffset );
	const s32	repeat_speed( ( m_nRepeatCount > MAX_REPEAT_COUNT ) ? PAD_FAST_REPEAT : PAD_SLOW_REPEAT );

	if ( CInput::IsButtonRepeat( CInput::UP, repeat_speed ) == true )
	{
		ScrollUp();

		++m_nRepeatCount;
	}
	else if ( CInput::IsButtonRepeat( CInput::DOWN, repeat_speed ) == true )
	{
		ScrollDown();

		++m_nRepeatCount;
	}
	else if ( CInput::IsButtonDown( CInput::UP ) == false && CInput::IsButtonDown( CInput::DOWN ) == false )
	{
		m_nRepeatCount = 0;
	}

	if ( old_item_offset != m_nItemsOffset || old_cursor_pos != m_nCursorPos )
	{
		m_fScrollOffset = 0.f;
	}

	if ( CInput::IsButtonClicked( CInput::CROSS ) == true )
	{
		CWindowTableItem * const	p_item( GetSelectedItem() );

		if ( p_item != NULL )
		{
			p_item->Select( m_nItemsOffset + m_nCursorPos );
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowTable::AddItem( CWindowTableItem * const p_item )
{
	m_bSizeDirty = true;

	m_Items.push_back( p_item );

	p_item->SetParent( this );
}

//*************************************************************************************
//
//*************************************************************************************
void	CWindowTable::ClearItems( bool maintain_selection )
{
	if ( maintain_selection == false )
	{
		ResetSelection();
	}

	while ( m_Items.empty() == false )
	{
		CWindowTableItem *	p_item( m_Items.back() );

		delete p_item;

		m_Items.pop_back();
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::Sort( Compare compare_func )
{
	u32							i( 0 );
	const u32					list_size( m_Items.size() );
	CWindowTableItem ** const	p_temp_array( new CWindowTableItem * [ list_size ] );

	if ( p_temp_array != NULL )
	{
		for ( CTableItemList::iterator it = m_Items.begin(); it != m_Items.end(); ++it, ++i )
		{
			p_temp_array[ i ] = ( *it );
		}

		qsort( p_temp_array, list_size, sizeof( CWindowTableItem * ), compare_func );

		m_Items.clear();

		for ( u32 i = 0; i < list_size; ++i )
		{
			AddItem( p_temp_array[ i ] );
		}

		delete p_temp_array;
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::CalculateMaxDisplayItems()
{
	V2			pos( 0.f, 0.f );
	const V2	max_size( GetSize() );

	m_nMaxDisplayItems = 0;

	for ( CTableItemList::iterator it = m_Items.begin(); it != m_Items.end(); ++it )
	{
		pos += ( *it )->GetSize();

		if ( pos.y <= max_size.y )
		{
			++m_nMaxDisplayItems;
		}
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::ResetSelection()
{
	m_nCursorPos = 0;
	m_nItemsOffset = 0;
}

//**********************************************************************************
//
//**********************************************************************************
u32	CWindowTable::GetCurrentSelection()
{
	return ( m_nItemsOffset + m_nCursorPos );
}

//**********************************************************************************
//
//**********************************************************************************
void	CWindowTable::SetCurrentSelection( u32 selection )
{
	while ( selection < GetCurrentSelection() )
	{
		ScrollUp();
	}

	while ( selection > GetCurrentSelection() )
	{
		ScrollDown();
	}
}

//**********************************************************************************
//
//**********************************************************************************
CWindowTableItem *	CWindowTable::GetSelectedItem()
{
	u32			idx( 0 );
	const u32	selected_item( GetCurrentSelection() );

	for ( CTableItemList::iterator it = m_Items.begin(); it != m_Items.end(); ++it, ++idx )
	{
		if ( idx == selected_item )
		{
			return ( *it );
		}
	}

	return NULL;
}

//**********************************************************************************
//
//**********************************************************************************
V2	CWindowTable::GetAllItemsSize()
{
	V2	size( 0.f, 0.f );

	for ( CTableItemList::iterator it = m_Items.begin(); it != m_Items.end(); ++it )
	{
		const V2	item_size( ( *it )->GetSize() );

		size.y += item_size.y;

		if ( item_size.x > size.x )
		{
			size.x = item_size.x;
		}
	}

	return size;
}

//**********************************************************************************
//
//**********************************************************************************
CTableItemList &	CWindowTable::GetItemList()
{
	return m_Items;
}

//*******************************  END OF FILE  ************************************
