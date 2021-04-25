/***********************************************************************************

  Module :	CMessageBox.h

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

#ifndef CMESSAGEBOX_H_
#define CMESSAGEBOX_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CWindow.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CWindowText;

typedef	void	( * ButtonCallback )();

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CMessageBox : public CWindow
{
	public:

		enum eButton
		{
			CROSS_CALLBACK,
			CIRCLE_CALLBACK,
			SQUARE_CALLBACK,
			TRIANGLE_CALLBACK,

			MAX_BUTTON_CALLBACKS
		};

	public:

		CMessageBox();

		virtual void		Render();
		virtual void		ProcessInput();

		void				SetText( const CString & text );
		void				SetCallback( eButton button, ButtonCallback callback );

	private:

		CWindowText *		m_pText;
		ButtonCallback		m_Callback[ MAX_BUTTON_CALLBACKS ];
};

//**********************************************************************************
//
//**********************************************************************************
class CModalMessageBox : public CMessageBox
 {
	public:

		enum eExitCode
		{
			EXIT_NULL,
			EXIT_CROSS,
			EXIT_CIRCLE,
			EXIT_SQUARE,
			EXIT_TRIANGLE,
			EXIT_HOME,
		};

		CModalMessageBox( const CString & title, const CString & text );

		eExitCode			Show();

		void				AddExitCode( eExitCode code, const CString & text );

		virtual void		ProcessInput();

	private:

		bool				m_bExitCross;
		bool				m_bExitCircle;
		bool				m_bExitSquare;
		bool				m_bExitTriangle;
		bool				m_bExitHome;
};

//**********************************************************************************
//
//**********************************************************************************
class CErrorMessage : public CModalMessageBox
{
	public:

		CErrorMessage( const CString & error_msg );
};

//**********************************************************************************
//
//**********************************************************************************
class CInfoMessage : public CModalMessageBox
{
	public:

		CInfoMessage( const CString & title, const CString & error_msg );
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CMESSAGEBOX_H_ */
