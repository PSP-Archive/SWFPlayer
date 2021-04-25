/***********************************************************************************

  Module :	CWindowTable.h

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

#ifndef CWINDOWTABLE_H_
#define CWINDOWTABLE_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CWindowItem.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CWindowTable;
class CWindowTableItem;

typedef std::list< CWindowTableItem * >	CTableItemList;

typedef	int		( * Compare )( const void * arg1, const void * arg2 );
typedef	void	( * SelectCallback )( CWindowTableItem * p_item, u32 item_no );


//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
class CWindowTableItem
{
	public:

		CWindowTableItem();
		virtual ~CWindowTableItem();

		virtual void		Process();
		virtual V2			Render( V2 pos, bool highlight, float scroll_offset );

		virtual V2			GetSize() const;

		CWindowTable *		GetParent() const;
		void				SetParent( CWindowTable * const p_parent );

		void				Select( u32 item_no );
		void				SetCallback( SelectCallback callback );

	protected:

		CWindowTable *		m_pParent;
		SelectCallback		m_pCallback;
};

//*************************************************************************************
//
//*************************************************************************************
class CWindowTable : public CWindowItem
{
	public:

		CWindowTable( bool wrap_selection );
		~CWindowTable();

		virtual void			Render();
		virtual void			Process();
		virtual void			ProcessInput();

		void					AddItem( CWindowTableItem * const p_item );
		void					ClearItems( bool maintain_selection = false );
		CTableItemList &		GetItemList();

		void					Sort( Compare compare_func );

		void					ResetSelection();
		u32						GetCurrentSelection();
		void					SetCurrentSelection( u32 selection );
		CWindowTableItem *		GetSelectedItem();

		V2						GetAllItemsSize();

		void					ScrollUp();
		void					ScrollDown();

	protected:

		void					CapSelection();
		void					CalculateMaxDisplayItems();

	protected:

		CTableItemList			m_Items;

		s32						m_nCursorPos;
		s32						m_nItemsOffset;
		s32						m_nMaxDisplayItems;
		float					m_fScrollOffset;
		s32						m_nRepeatCount;

		bool					m_bSizeDirty;
		bool					m_bWrapSelection;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CWINDOWTABLE_H_ */
