/***********************************************************************************

  Module :	CFileHandler.h

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

#ifndef CFILEHANDLER_H_
#define CFILEHANDLER_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CSkinManager.h"
#include "CFileSystem.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
struct sFileExtensionInfo
{
	CString							m_szExecutionName;
	CSkinManager::eSkinComponent	m_Icon;
};

typedef	bool						( * ExecutionCallback )( const CString & filename, bool kiosk_mode );
typedef	const sFileExtensionInfo &	( * InformationCallback )( const CString & filename );
typedef	void						( * BackgroundImageCallback )( const CString & filename );

struct sFileHandlerInfo
{
	CString						m_szExtension;
	ExecutionCallback			m_ExecuteCallback;
	InformationCallback			m_InformationCallback;
	BackgroundImageCallback		m_BackgroundCallback;
};

typedef std::list< sFileHandlerInfo * >	CFileHandleList;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CFileHandler
{
	public:

		static bool					Open();
		static void					Close();

		static void					RegisterExtension( const CString & extension,
													   ExecutionCallback execute_callback,
													   InformationCallback info_callback,
													   BackgroundImageCallback background_callback = NULL );

		static sFileHandlerInfo *	FindHandler( const CString & file_name );

		static void					SetFileList( const CFileList & file_list );
		static bool					MultiSelection();
		static const CString &		GetFile();
		static const CString &		GetPrevFile();
		static const CString &		GetNextFile();

	private:

		static CFileList			s_FileList;
		static CFileList::iterator	s_CurrentFile;
		static CFileHandleList		s_HandleList;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CFILEHANDLER_H_ */
