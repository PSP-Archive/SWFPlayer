/***********************************************************************************

  Module :	CAssert.h

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

#ifndef CASSERT_H_
#define CASSERT_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************

//**********************************************************************************
//   Macros
//**********************************************************************************
#ifndef	_DEBUG

#define	TRACE
#define	QASSERT( cond )
#define	ASSERT( cond, msg )
#define	BREAK_POINT					sizeof
#define	FUNCTION_LOG
#define	LOG_LINE
#define CHECK_ERROR( func )			func
#define DISPLAY_ERROR( ecode )

#else	// #ifdef	_DEBUG

#define TRACE					CAssert::Message

#define	QASSERT( cond )																				\
{																									\
	static bool	ignore = false;																		\
	if ( ( cond ) == false && ignore == false )														\
	{																								\
		ignore = CAssert::AssertMessage( "ASSERT: %s - line: %d file: %s", #cond, __LINE__, __FILE__ );	\
	}																								\
}

#define	ASSERT( cond, msg )																					\
{																											\
	static bool	ignore = false;																				\
	if ( ( cond ) == false && ignore == false )																\
	{																										\
		ignore = CAssert::AssertMessage( "ASSERT: %s, %s - line: %d file: %s", #cond, msg, __LINE__, __FILE__ );	\
	}																										\
}

#define	BREAK_POINT				CAssert::AssertMessage

#define	FUNCTION_LOG			CAssert::AssertMessage( "FUNCTION: %s, line: %d file: %s", __FUNCTION__, __LINE__, __FILE__ );

#define	LOG_LINE				CAssert::Message( "line: %d file: %s\n", __LINE__, __FILE__ )

#define CHECK_ERROR( func )					\
{											\
	const int	code( func );				\
											\
	if ( code < 0 )							\
	{										\
		CAssert::DisplayErrorCode( code );	\
	}										\
}

#define DISPLAY_ERROR( ecode )	CAssert::DisplayErrorCode( ecode )

#endif	// #ifdef	_DEBUG

//**********************************************************************************
//   Types
//**********************************************************************************

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//**********************************************************************************
//	Assert message class definition
//**********************************************************************************
class CAssert
{
	public:

		// Open the ASSERT system
		static void		Open();

		// Close the ASSERT system
		static void		Close();

		// Displays an simple message on screen
		static void		Message( const char * fmt, ... );

		// Displays an assert message
		static bool		AssertMessage( const char * fmt, ... );

		// Display an error code as a string
		static void		DisplayErrorCode( int code );
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CASSERT_H_ */
