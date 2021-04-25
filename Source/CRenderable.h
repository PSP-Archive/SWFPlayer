/***********************************************************************************

  Module :	CRenderable.h

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

#ifndef CRENDERABLE_H_
#define CRENDERABLE_H_

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
typedef void	( * RenderCallback )();

typedef std::list< RenderCallback >	CRenderCallbackList;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CRenderable
{
	public:

		enum eRenderOrder
		{
			RO_BACKGROUND,
			RO_WINDOWS,
			RO_HUD,

			MAX_RENDER_ORDER
		};

	public:

		static void					Open();
		static void					Close();

		static void					Render();

		static void					Register( eRenderOrder order, RenderCallback callback );
		static void					UnRegister( eRenderOrder order, RenderCallback callback );

	private:

		static CRenderCallbackList	s_RenderList[ MAX_RENDER_ORDER ];
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CRENDERABLE_H_ */
