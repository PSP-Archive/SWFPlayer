/***********************************************************************************

  Module :	CMessageBox.cpp

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
#include "CMessageBox.h"
#include "CFont.h"
#include "CGfx.h"
#include "CWindowText.h"
#include "CInput.h"
#include "CFrameWork.h"
#include "CRenderable.h"
#include "CHUD.h"

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
static const V2	BOX_BORDER( 16.f, 16.f );

//**********************************************************************************
//**********************************************************************************
//
//	Message box implementation
//
//**********************************************************************************
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
CMessageBox::CMessageBox()
:	m_pText( new CWindowText( "" ) )
{
	SetFocus( true );

	m_pText->SetFocus( true );

	AddItem( m_pText );

	for ( u32 i = 0; i < MAX_BUTTON_CALLBACKS; ++i )
	{
		m_Callback[ i ] = NULL;
	}

	m_pText->SetPos( BOX_BORDER );
}

//**********************************************************************************
//
//**********************************************************************************
void	CMessageBox::Render()
{
	CGfx::DrawQuad( V2( 0.f, 0.f ), V2( CGfx::s_ScreenWidth, CGfx::s_ScreenHeight ), 0x80000000 );

	CWindow::Render();
}

//**********************************************************************************
//
//**********************************************************************************
void	CMessageBox::ProcessInput()
{
	ButtonCallback	callback( NULL );

	if ( CInput::IsButtonClicked( CInput::CROSS ) == true )
	{
		callback = m_Callback[ CROSS_CALLBACK ];
	}

	if ( CInput::IsButtonClicked( CInput::CIRCLE ) == true )
	{
		callback = m_Callback[ CIRCLE_CALLBACK ];
	}

	if ( CInput::IsButtonClicked( CInput::SQUARE ) == true )
	{
		callback = m_Callback[ SQUARE_CALLBACK ];
	}

	if ( CInput::IsButtonClicked( CInput::TRIANGLE ) == true )
	{
		callback = m_Callback[ TRIANGLE_CALLBACK ];
	}

	if ( callback != NULL )
	{
		Delete();

		callback();

		return;
	}

	CWindow::ProcessInput();
}

//**********************************************************************************
//
//**********************************************************************************
void	CMessageBox::SetText( const CString & text )
{
	m_pText->SetText( text );

	V2	text_size( m_pText->GetSize() + ( BOX_BORDER * 2.f ) );

	if ( text_size.x < CFont::GetDefaultFont()->GetStringSize( m_szTitle ).x + ( 2.f * BOX_BORDER.x ) )
	{
		text_size.x = CFont::GetDefaultFont()->GetStringSize( m_szTitle ).x + ( 2.f * BOX_BORDER.x );
	}

	SetPos( V2( 0.5f * ( CGfx::s_ScreenWidth - text_size.x ), 0.5f * ( CGfx::s_ScreenHeight - text_size.y ) ) );
	SetSize( text_size );
}

//**********************************************************************************
//
//**********************************************************************************
void	CMessageBox::SetCallback( eButton button, ButtonCallback callback )
{
	m_Callback[ button ] = callback;
}


//**********************************************************************************
//**********************************************************************************
//
//	Modal message box implementation
//
//**********************************************************************************
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
CModalMessageBox::CModalMessageBox( const CString & title, const CString & text )
:	m_bExitCross( false )
,	m_bExitCircle( false )
,	m_bExitSquare( false )
,	m_bExitTriangle( false )
,	m_bExitHome( false )
{
	SetText( text );
	SetTitle( title );

	CHUD::SetButtons( "", "", "", "" );
}

//**********************************************************************************
//
//**********************************************************************************
CModalMessageBox::eExitCode	CModalMessageBox::Show()
{
	CProcess::Pause( true );

	while ( 1 )
	{
		eExitCode	exit_code( EXIT_NULL );

		CFrameWork::Process();

		if ( m_bExitCross == true && CInput::IsButtonClicked( CInput::CROSS ) == true )
		{
			exit_code = EXIT_CROSS;
		}

		if ( m_bExitCircle == true && CInput::IsButtonClicked( CInput::CIRCLE ) == true )
		{
			exit_code = EXIT_CIRCLE;
		}

		if ( m_bExitSquare == true && CInput::IsButtonClicked( CInput::SQUARE ) == true )
		{
			exit_code = EXIT_SQUARE;
		}

		if ( m_bExitTriangle == true && CInput::IsButtonClicked( CInput::TRIANGLE ) == true )
		{
			exit_code = EXIT_TRIANGLE;
		}

		if ( m_bExitHome == true && CInput::IsButtonClicked( CInput::HOME ) == true )
		{
			exit_code = EXIT_HOME;
		}

		//
		//	Have we clicked an exit button?
		//
		if ( exit_code != EXIT_NULL )
		{
			SetVisible( false );

			CProcess::Pause( false );

			return exit_code;
		}
	}

	return EXIT_TRIANGLE;
}

//**********************************************************************************
//
//**********************************************************************************
void	CModalMessageBox::ProcessInput()
{
}

//**********************************************************************************
//
//**********************************************************************************
void	CModalMessageBox::AddExitCode( eExitCode code, const CString & text )
{
	switch ( code )
	{
	case EXIT_CIRCLE:
		m_bExitCircle = true;
		CHUD::SetButton( CHUD::BUTTON_CIRCLE, text );
		break;

	case EXIT_CROSS:
		m_bExitCross = true;
		CHUD::SetButton( CHUD::BUTTON_CROSS, text );
		break;

	case EXIT_SQUARE:
		m_bExitSquare = true;
		CHUD::SetButton( CHUD::BUTTON_SQUARE, text );
		break;

	case EXIT_TRIANGLE:
		m_bExitTriangle = true;
		CHUD::SetButton( CHUD::BUTTON_TRIANGLE, text );
		break;

	case EXIT_HOME:
		m_bExitHome = true;

		break;

	default:
		break;
	}
}


//**********************************************************************************
//**********************************************************************************
//
//	Error message box implementation
//
//**********************************************************************************
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
CErrorMessage::CErrorMessage( const CString & error_msg )
:	CModalMessageBox( "ERROR!", error_msg )
{
	AddExitCode( EXIT_CROSS, "Continue" );

	Show();
}


//**********************************************************************************
//**********************************************************************************
//
//	Info message box implementation
//
//**********************************************************************************
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
CInfoMessage::CInfoMessage( const CString & title, const CString & error_msg )
:	CModalMessageBox( title, error_msg )
{
	AddExitCode( EXIT_CROSS, "Ok" );

	Show();
}

//*******************************  END OF FILE  ************************************
