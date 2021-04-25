/***********************************************************************************

  Module :	CDirectoryList.h

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

#ifndef CDIRECTORYLIST_H_
#define CDIRECTORYLIST_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CWindow.h"
#include "CFileSystem.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CWindowTable;
class CWindowTableItem;
class CDirectoryListItem;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CDirectoryList : public CWindow
{
	public:

		enum eDrive
		{
			ALL_DRIVES,
			DRIVE_FLASH0,
			DRIVE_FLASH1,
			DRIVE_MS0,
			DRIVE_UMD,

			MAX_DRIVES
		};

		enum eDriveInfo
		{
			DRIVE_ID,
			DRIVE_NAME,

			MAX_DRIVE_INFO
		};

	public:

		CDirectoryList();
		~CDirectoryList();

		virtual void			SetFocus( bool focus );
		virtual void			SetSize( const V2 & size );

		void					SetDrive( eDrive drive );
		void					SetPath( const CString & path );
		void					RefreshList( bool maintain_selection );

		virtual void			Process();
		virtual void			ProcessInput();

		const CString &			GetCurrentPath() const;
		CString					GetFullPathFileName( const CString & file_name );

		bool					AnyItemsSelected();
		void					GetSelectedFiles( CFileList & list, bool full_path );
		void					ClearSelectedFiles();

		bool					ReadOnly() const;
		bool					IsDriveList() const;

		static const CString &	GetDriveInfo( eDrive drive, eDriveInfo info );

		bool					CanSavePath() const;

	protected:

		CString					GetPreviousDir();

		void					Selection( CDirectoryListItem * const p_item );
		static void				SelectionCallback( CWindowTableItem * const p_item, u32 item_no );

	private:

		u32						m_nRefreshTimer;

		CWindowTable *			m_pTable;

		CString					m_szDirName;

		static const CString	s_szDriveNames[ MAX_DRIVES ][ MAX_DRIVE_INFO ];
};


//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CDIRECTORYLIST_H_ */
