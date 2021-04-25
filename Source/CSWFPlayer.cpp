/***********************************************************************************

  Module :	CSWFPlayer.cpp

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
#include "CSWFPlayer.h"
#include "CFrameWork.h"
#include "CFileHandler.h"
#include "CGfx.h"
#include "CInput.h"
#include "CHUD.h"
#include "CConfigFile.h"
#include "CSWFFileHandler.h"

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
CSWFPlayer *		CSWFPlayer::s_pInstance( NULL );

static CVarString	CVAR_SRC_PATH( "src_path", "" );

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
void	CSWFPlayer::Create()
{
	ASSERT( s_pInstance == NULL, "Instance already created" );

	s_pInstance = new CSWFPlayer();
}

//**********************************************************************************
//
//**********************************************************************************
void	CSWFPlayer::Destroy()
{
	SAFE_DELETE( s_pInstance );
}

//**********************************************************************************
//
//**********************************************************************************
CSWFPlayer *	CSWFPlayer::Get()
{
	ASSERT( s_pInstance != NULL, "Trying to access a NULL file assistant" );

	return s_pInstance;
}

//**********************************************************************************
//
//**********************************************************************************
CSWFPlayer::CSWFPlayer()
:	m_pSrcList( NULL )
,	m_FadeInLevel( 0.f )
{
	//
	//	Create the HUD
	//
	CHUD::Create();

	//
	//	Register the file extensions we're interested in
	//
	CFileHandler::RegisterExtension( "swf", CSWFFileHandler::Execute, CSWFFileHandler::Information );

	//
	//	Create the directory window
	//
	m_pSrcList = new CDirectoryList();

	m_pSrcList->SetPos( CSkinManager::GetV2( "directory_list", "src_list_pos_on", V2( 16.f, 16.f ) ) );
	m_pSrcList->SetSize( CSkinManager::GetV2( "directory_list", "src_list_size", V2( 320.f, 220.f ) ) );
	m_pSrcList->SetFocus( true );
	m_pSrcList->SetPath( CFileSystem::GetCurrentFolder() + "/" );

	if ( CVAR_SRC_PATH.Get().IsEmpty() == false )		m_pSrcList->SetPath( CVAR_SRC_PATH.Get() );
	else												m_pSrcList->SetDrive( CDirectoryList::ALL_DRIVES );

	//
	//	Set the buttons
	//
	CHUD::SetButton( CHUD::BUTTON_START, "Options" );
	CHUD::SetButton( CHUD::BUTTON_SELECT, "Reboot" );
}

//**********************************************************************************
//
//**********************************************************************************
CSWFPlayer::~CSWFPlayer()
{
	SAFE_DELETE( m_pSrcList );

	//
	//	Shut the HUD down
	//
	CHUD::Destroy();
}

//**********************************************************************************
//
//**********************************************************************************
void	CSWFPlayer::Process()
{
	//
	//	Constantly update the src path
	//
	if ( m_pSrcList->CanSavePath() )
	{
		CVAR_SRC_PATH = m_pSrcList->GetCurrentPath();
	}

	//
	//	Fade the screen in
	//
	m_FadeInLevel += 6.f;

	if ( m_FadeInLevel > 255.f )
	{
		CGfx::SetFadeLevel( 255 );
	}
	else
	{
		CGfx::SetFadeLevel( static_cast< u8 >( m_FadeInLevel ) );
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CSWFPlayer::ProcessInput()
{
}

//**********************************************************************************
//
//**********************************************************************************
CDirectoryList *	CSWFPlayer::GetSrcList() const
{
	return m_pSrcList;
}

//**********************************************************************************
//
//**********************************************************************************
CDirectoryList *	CSWFPlayer::GetFocusList() const
{
	return m_pSrcList;
}

//**********************************************************************************
//
//**********************************************************************************
void	CSWFPlayer::SetListFocus( bool focus )
{
	m_pSrcList->SetFocus( focus );
}

//*******************************  END OF FILE  ************************************
