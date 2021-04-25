/***********************************************************************************

  Module :	CString.h

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

#ifndef CSTRING_H_
#define CSTRING_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "CAssert.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

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
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
inline s32 StringLen( const char * p_string )
{
	return static_cast< s32 >( strlen( p_string ) );
}

inline char * StringCopy( char * dest, const char * source )
{
	return strcpy( dest, source );
}

inline char * StringCopy( char * dest, const char * source, u32 len )
{
	return strncpy( dest, source, len );
}

inline char * StringAppend( char * dest, const char * source )
{
	return strcat( dest, source );
}

inline s32 StringCompare( const char * a, const char * b )
{
	return strcmp( a, b );
}

inline s32 StringCompareI( const char * a, const char * b )
{
	return strcmpi( a, b );
}

inline const char * StringFind( const char * source, char c )
{
	return strchr( source, c );
}

inline char * StringFind( char * source, char c )
{
	return strchr( source, c );
}

inline char* StringStr( const char* source, const char* search)
{
	return strstr(source, search);
}

inline s32 StringPrint( char * string, u32 length, const char * format, va_list list )
{
#ifndef WIN32
	return vsnprintf( string, length, format, list );
#else	// #ifndef WIN32
	return 0;
#endif	// #ifndef WIN32
}

char *	skipWS( char * s );
void	trimEndingChar( char * s, char c );
void	trimEndingWS( char * s );
int		strStartsWith( char * s, char * start );
int		endsWith( char * s, char * end );
void	strReplaceChar( char * str, char s, char d );

//*************************************************************************************
//
//*************************************************************************************
template< s32 STRING_LEN, class CType = char > class CStaticString
{
	public:

		CStaticString()
		{
			m_pString[ 0 ] = 0;
		}

		CStaticString( const CStaticString & string )
		{
			Copy( string.m_pString );
		}

		CStaticString( const CType * string )
		{
			Copy( string );
		}

		CStaticString( const CType * p_string1, const CType * p_string2 )
		{
			Copy( p_string1 );

			ASSERT( Length() + StringLen( p_string2 ) <= STRING_LEN, "String append will overflow array" );

			StringAppend( m_pString, p_string2 );
		}

		CStaticString( const CType * string, u32 len )
		{
			SetLeft( string, len );
		}

		CStaticString( s32 val )
		{
			*this = val;
		}


		operator const CType * () const
		{
			return m_pString;
		}

		CStaticString & operator= ( const CStaticString & string )
		{
			Copy( string.m_pString );

			return *this;
		}

		CStaticString &	operator = ( const CType * string )
		{
			Copy( string );

			return *this;
		}

		CStaticString & Reverse()
		{
			const u32	len( Length() );

			for ( u32 i = 0; i < ( len / 2 ); ++i )
			{
				const char	tmp( m_pString[ i ] );

				m_pString[ i ] = m_pString[ len - 1 - i ];
				m_pString[ len - 1 - i ] = tmp;
			}

			return *this;
		}

		CStaticString & operator = ( s32 val )
		{
			char *	t;
			char *	s( m_pString );
			int		mod;

			if ( val < 0 )
			{
				*s++ = '-';
				val = -val;
			}

			t = s;

			while ( val )
			{
				mod = val % 10;
				*t++ = (char)mod + '0';
				val /= 10;
			}

			if ( s == t )
			{
				*t++ = '0';
			}

			*t = '\0';

			Reverse();

			return *this;
		}

		CStaticString &	operator += ( const CType * string )
		{
			return Append( string );
		}

		CStaticString &	operator += ( const CStaticString & string )
		{
			return Append( string );
		}

		CStaticString	operator + ( const CType * const p_string ) const
		{
			CStaticString< STRING_LEN, CType >	temp( m_pString );

			temp.Append( p_string );

			return temp;
		}

		CStaticString	operator + ( const CStaticString & string ) const
		{
			CStaticString< STRING_LEN, CType >	temp( m_pString );

			temp.Append( string );

			return temp;
		}

		bool	operator == ( const CType * string ) const
		{
			return Equals( string );
		}

		bool	operator != ( const CType * string ) const
		{
			return !Equals( string );
		}

		bool	operator > ( const CType * string ) const
		{
			return ( StringCompareI( m_pString, string ) > 0 );
		}

		bool	operator < ( const CType * string ) const
		{
			return ( StringCompareI( m_pString, string ) < 0 );
		}

		const CType *	GetPtr() const
		{
			return m_pString;
		}

		bool	Equals( const CType * string ) const
		{
			return StringCompare( m_pString, string ) == 0;
		}

		bool	IEquals( const CType * string ) const
		{
			return StringCompareI( m_pString, string ) == 0;
		}

		s32		Length() const
		{
			return StringLen( m_pString );
		}

		bool	IsEmpty() const
		{
  			return ( m_pString[ 0 ] == '\0' );
		}

		void	ToUpper()
		{
			strupr( m_pString );
		}

		void	ToLower()
		{
			strlwr( m_pString );
		}

		CType *	Find( const CType * const p_string )
		{
			return StringStr( m_pString, p_string );
		}

		const CType *	Find( const CType * const p_string ) const
		{
			return StringStr( m_pString, p_string );
		}

		const CType *	Find( CType c ) const
		{
			return StringFind( m_pString, c );
		}

		CType *	Find( CType c )
		{
			return StringFind( m_pString, c );
		}

		CStaticString &	Append( const CType * string )
		{
			ASSERT( Length() + StringLen( string ) <= STRING_LEN, "String append will overflow array" );

			StringAppend( m_pString, string );

			return *this;
		}

		void	Clear()
		{
			m_pString[ 0 ] = '\0';
		}

		CStaticString &	Printf( const CType * format, ... )
		{
			s32		chars_written;
			va_list	marker;

			va_start( marker, format );

			chars_written = StringPrint( m_pString, STRING_LEN, format, marker );

			va_end( marker );

			ASSERT( chars_written >= 0, "The formatted output is too long for the buffer" );

			return *this;
		}

	protected:

		void	Copy( const CType * string )
		{
			if ( string == NULL )
			{
				m_pString[ 0 ] = '\0';
			}
			else
			{
//				ASSERT( StringLen( string ) <= STRING_LEN, "Trying to copy a string that is too large" );

				StringCopy( m_pString, string, STRING_LEN );

				m_pString[ STRING_LEN ] = '\0';
			}
		}

	protected:

		CType	m_pString[ STRING_LEN + 1 ];
};

//**********************************************************************************
//
//**********************************************************************************
typedef CStaticString< 512 >	CString;
typedef std::list< CString >	CStringList;


#endif /* CSTRING_H_ */
