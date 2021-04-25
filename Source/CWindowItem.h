/***********************************************************************************

  Module :	CWindowItem.h

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

#ifndef CWINDOWITEM_H_
#define CWINDOWITEM_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CSizedItem.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CWindow;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CWindowItem : public CSizedItem
{
	public:

		CWindowItem();
		virtual ~CWindowItem();

		virtual void		Render();
		virtual void		Process();
		virtual void		ProcessInput();

		virtual float		GetScale() const;

		CWindow *			GetParent() const;
		void				SetParent( CWindow * const p_parent );

		bool				HasFocus() const;
		void				SetFocus( bool focus );

		virtual V2			GetScreenPos() const;

		virtual ARGB		GetColor() const;

	protected:

		CWindow *			m_pParent;

		bool				m_bInFocus;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CWINDOWITEM_H_ */
