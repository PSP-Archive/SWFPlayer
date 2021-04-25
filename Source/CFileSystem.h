/***********************************************************************************

  Module :	CFileSystem.h

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

#ifndef CFILESYSTEM_H_
#define CFILESYSTEM_H_

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
enum eAttributeFlags
{
	AF_READ_ONLY	= ( 1 << 0 ),
	AF_HIDDEN		= ( 1 << 1 ),
	AF_UNKNOWN1		= ( 1 << 2 ),
	AF_UNKNOWN2		= ( 1 << 3 ),
	AF_ARCHIVE		= ( 1 << 5 ),
	AF_DIRECTORY	= ( 1 << 12 ),
	AF_WLAN			= ( 1 << 29 ),
	AF_BACK_BUTTON	= ( 1 << 30 ),
	AF_DRIVE		= ( 1 << 31 ),
};

struct sDevCtl
{
	s32	max_clusters;
	s32	free_clusters;
	s32	max_sectors;	// ???
	s32	sector_size;
	s32	sector_count;
};

struct sDevCommand
{
	sDevCtl *	p_dev_inf;
};

struct sDirEntry
{
	CString		m_szFileName;
	SceIoStat	m_Stats;

	bool		IsFile() const			{ return ( IsDrive() == false && IsBackButton() == false && IsDirectory() == false && IsWLAN() == false ); }
	bool		IsWLAN() const			{ return ( m_Stats.st_mode & AF_WLAN ); }
	bool		IsDrive() const			{ return ( m_Stats.st_mode & AF_DRIVE ); }
	bool		IsBackButton() const	{ return ( m_Stats.st_mode & AF_BACK_BUTTON ); }
	bool		IsDirectory() const		{ return ( ( m_Stats.st_mode & AF_DIRECTORY ) & FIO_S_IFMT ); }
};

typedef std::list< sDirEntry >	CFileList;
typedef SceUID					FIND_FILE_HANDLE;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
class CFile
{
	public:

		friend class CFileSystem;

	public:

		bool						Read( void * p_address, u32 length, u32 * p_nbytes_read = NULL ) const;
		bool						Write( const void * p_address, u32 length, u32 * p_nbytes_written = NULL ) const;

		bool						IsEOF() const;

		u32							GetLength() const;

		u32							Tell() const;
		bool						Seek( const u32 offset, const u32 origin ) const;

		char						FGetC() const;

		CString						GetExtension() const;

		bool						IsReadOnly() const;
		bool						IsWritable() const;

		operator FILE * () const;

	protected:

		CFile( const CString & filename, FILE * const p_handle );
		~CFile();

	private:

		CString						m_szFilename;
		FILE *						m_pHandle;
		u32							m_Length;
};

//*************************************************************************************
//
//*************************************************************************************
class CFileSystem
{
	public:

		static bool					SetRoot( const CString & directory );

		static CFile *				Open( const CString & filename, const char * const p_open_flags );
		static void					Close( CFile * const p_file );

		static bool					FileExists( const CString & filename );
		static bool					DirectoryExists( const CString & directory );

		static bool					GetDirectoryFiles( const CString & directory, CFileList & dir_files );

		static bool					FindFirstFile( const CString & path, FIND_FILE_HANDLE & handle );
		static bool					FindNextFile( sDirEntry & dir_entry, FIND_FILE_HANDLE handle );
		static bool					FindCloseFile( FIND_FILE_HANDLE handle );

		static bool					CopyFile( const CString & src_file, const CString & dst_file, float progress_inc = 0.f );
		static bool					CopyDirectory( const CString & src_dir, const CString & dst_dir, float progress_inc = 0.f );

		static bool					DeleteFile( const CString & filename );
		static bool					DeleteDirectory( const CString & directory );

		static bool					MakeDirectory( const CString & directory );

		static bool					IsUMDInserted();
		static bool					ActivateUMD();

		static bool					Rename( const CString & old_name, const CString & new_name );

		static void					SplitPath( const CString & path, CString * p_drive, CString * p_dir, CString * p_fname, CString * p_ext );

		static bool					MakeReadOnly( const CString & filename );
		static bool					MakeWritable( const CString & filename );

		static CString				MakeFullPath( const CString & filename );

		static CString				GetSizeString( u32 size );
		static CString				GetFileExtension( const CString & filename );

		static bool					HideCorruptFiles();
		static void					SetHideCorruptFiles( bool hide );

		static const CString &		GetWorkingFolder();
		static CString				GetCurrentFolder();

		static s32					GetFreeSpace( const CString & device );

	private:

		static SceIoDirent			s_DirEntry;
		static CString				s_szRootDirectory;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CFILESYSTEM_H_ */
