/***********************************************************************************

  Module :	CHUD.h

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

#ifndef CHUD_H_
#define CHUD_H_

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
class CProgressBar;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CHUD
{
	public:

		enum eButton
		{
			BUTTON_CROSS,
			BUTTON_CIRCLE,
			BUTTON_SQUARE,
			BUTTON_TRIANGLE,
			BUTTON_START,
			BUTTON_SELECT,

			MAX_BUTTONS
		};

	public:

		static void				Create();
		static void				Destroy();

		static void				Show( bool show );
		static bool				IsVisible();

		static void				SetButton( eButton button, const CString & text );
		static void				SetButtons( const CString & cross, const CString & circle, const CString & square, const CString & triangle );

		static void				ShowProgressBar( bool show, const CString & title );
		static float			GetProgressBar();
		static void				SetProgressBar( float val );

	private:

		CHUD();
		~CHUD();

		static void				Render();
		void					RenderInternal();

	private:

		static bool				s_bVisible;

		static CHUD *			s_pInstance;
		static CString			s_szButtonText[ MAX_BUTTONS ];

		static float			s_fProgressBar;
		static bool				s_bShowProgressBar;
		static CString			s_szProgressTitle;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CHUD_H_ */
