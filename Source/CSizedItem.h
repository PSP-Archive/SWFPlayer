/***********************************************************************************

  Module :	CSizedItem.h

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

#ifndef CSIZEDITEM_H_
#define CSIZEDITEM_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CSizedItem
{
	public:

		CSizedItem();
		virtual ~CSizedItem();

		const V2 &			GetPos() const;
		virtual void		SetPos( const V2 & pos );

		virtual V2			GetSize() const;
		virtual void		SetSize( const V2 & size );

		virtual V2			GetScreenPos() const;

		virtual float		GetScale() const;
		void				SetScale( float scale );

	protected:

		V2					m_Pos;
		V2					m_Size;

		float				m_fScale;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CSIZEDITEM_H_ */
