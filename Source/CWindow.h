/***********************************************************************************

  Module :	CWindow.h

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

#ifndef CWINDOW_H_
#define CWINDOW_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CString.h"
#include "CProcess.h"
#include "CSizedItem.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CWindowItem;

typedef std::list< CWindowItem * >	CWindowItemList;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
class CWindow : public CProcess, public CSizedItem
{
	public:

		static void		Open();
		static void		Close();

		static void		RenderAll();

		static void		SetVisibleAll( bool visible );

		static void		DrawWindow( const V2 & pos, const V2 & size, ARGB color );

	public:

		CWindow();

		virtual void	Render();
		virtual void	Process();
		virtual void	ProcessInput();

		ARGB			GetColor();
		ARGB			GetTextColor();
		ARGB			GetTitleColor();

		void			AddItem( CWindowItem * const p_item );
		void			RemoveItem( CWindowItem * const p_item );
		void			ClearItems();

		const CString &	GetTitle() const;
		void			SetTitle( const CString & title );

		bool			HasFocus() const;
		void			SetFocus( bool focus );

		bool			IsVisible() const;
		void			SetVisible( bool visible );

	protected:

		virtual ~CWindow();

	protected:

		CWindowItemList	m_ItemList;
		CString			m_szTitle;

		bool			m_bInFocus;
		bool			m_bVisible;

		static bool		s_bVisible;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CWINDOW_H_ */
