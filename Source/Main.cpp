/***********************************************************************************

  Module :	Main.cpp

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
#include "CFrameWork.h"
#include "CSWFPlayer.h"
#include "CFileSystem.h"
#include "CConfigFile.h"
#include "CSWFFileHandler.h"
#include "CGfx.h"

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
static CVarString	CVAR_PLAY_SWF( "play_swf", "" );

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
int	main( int argc, char * argv[] )
{
	//
	//	Open the framework
	//
	if ( CFrameWork::Open() == true )
	{
		//
		//	Set the working folder
		//
		if ( argc > 0 )
		{
			CString	drive, dir;

			CFileSystem::SplitPath( CString( argv[ 1 ] ), &drive, &dir, NULL, NULL );

			drive += dir;

			CFileSystem::SetRoot( drive );
		}

		//
		//	Create the application
		//
		CSWFPlayer::Create();

		//
		//	Do we want to automatically play an swf file?
		//
		if ( CVAR_PLAY_SWF.Get().IsEmpty() == false )
		{
			CGfx::SetFadeLevel( 255 );

			CSWFFileHandler::Execute( CVAR_PLAY_SWF.Get(), true );
		}

		//
		//	The main loop
		//
		while ( CFrameWork::Process() == true )
		{
		}

		//
		//	Close the application
		//
		CSWFPlayer::Destroy();

		//
		//	Close down the framework
		//
		CFrameWork::Close();
	}
	else
	{
		ASSERT( 0, "CFrameWork::Open()" );
	}

	return 0;
}

//*******************************  END OF FILE  ************************************

