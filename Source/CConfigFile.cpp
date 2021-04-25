/***********************************************************************************

  Module :	CConfigFile.cpp

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
#include "CConfigFile.h"
#include "TinyXML.h"
#include "CFrameWork.h"
#include "CFileSystem.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************
static const CString	s_szConfigFile( "Data/Config.xml" );

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************
CConfigFile::CVarList *	CConfigFile::s_pVarList( NULL );
CConfigFile *			CConfigFile::s_pInstance( NULL );

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
void	CConfigFile::Open()
{
	ASSERT( s_pInstance == NULL, "CConfigFile is already open!" );

	s_pInstance = new CConfigFile();
}

//**********************************************************************************
//
//**********************************************************************************
void	CConfigFile::Close()
{
	ASSERT( s_pInstance != NULL, "CConfigFile isn't open!" );

	SAFE_DELETE( s_pInstance );
}

//**********************************************************************************
//
//**********************************************************************************
CConfigFile::CConfigFile()
:	m_pDocument( new TiXmlDocument() )
{
	ASSERT( m_pDocument != NULL, "Failed to create the XML document" );

	if ( m_pDocument != NULL )
	{
		if ( m_pDocument->LoadFile( s_szConfigFile ) == true )
		{
			TiXmlHandle		doc_handle( m_pDocument );

			//
			//	Initialise the config vars
			//
			TiXmlElement *	p_options( doc_handle.FirstChild( "SWFPlayer" ).FirstChild( "options" ).Element() );

			if ( p_options != NULL )
			{
				for ( CVarList::iterator it = s_pVarList->begin(); it != s_pVarList->end(); ++it )
				{
					CConfigVarBase *	p_var( *it );
					const char * const	p_attribute( p_options->Attribute( p_var->GetName() ) );

					if ( p_attribute != NULL )
					{
						p_var->Set( p_attribute );
					}
				}
			}
		}
		else
		{
			TRACE( "%s\n", m_pDocument->ErrorDesc() );

			ASSERT( 0, "Failed to load config file!" );
		}
	}
}

//**********************************************************************************
//
//**********************************************************************************
CConfigFile::~CConfigFile()
{
	Save();
}

//**********************************************************************************
//
//**********************************************************************************
void	CConfigFile::Save()
{
	if ( s_pInstance->m_pDocument != NULL )
	{
		TiXmlHandle		doc_handle( s_pInstance->m_pDocument );
		TiXmlElement *	p_options( doc_handle.FirstChild( "SWFPlayer" ).FirstChild( "options" ).Element() );

		if ( p_options != NULL )
		{
			for ( CVarList::iterator it = s_pVarList->begin(); it != s_pVarList->end(); ++it )
			{
				CConfigVarBase *	p_var( *it );

				p_var->Save( p_options );
			}
		}

		if ( CFileSystem::MakeWritable( s_szConfigFile ) == false )
		{
			ASSERT( 0, "Failed to change readonly state of config file!" );
		}

		if ( s_pInstance->m_pDocument->SaveFile( CFileSystem::MakeFullPath( s_szConfigFile ) ) == false )
		{
			ASSERT( 0, "Failed to save config file!" );
		}
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CConfigFile::AddVar( CConfigVarBase * const p_var )
{
	if ( s_pVarList == NULL )
	{
		s_pVarList = new CVarList();
	}

	s_pVarList->push_back( p_var );
}

//*******************************  END OF FILE  ************************************
