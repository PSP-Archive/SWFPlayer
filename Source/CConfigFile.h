/***********************************************************************************

  Module :	CConfigFile.h

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

#ifndef CCONFIGFILE_H_
#define CCONFIGFILE_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CString.h"
#include "TinyXML.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CConfigVarBase;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************

//**********************************************************************************
//**********************************************************************************
//
//	Config file reader class
//
//**********************************************************************************
//**********************************************************************************
class CConfigFile
{
	protected:

		typedef std::vector< CConfigVarBase * >	CVarList;

	public:

		static void				Open();
		static void				Close();
		static void				Save();

		static void				AddVar( CConfigVarBase * const p_var );

	protected:

		CConfigFile();
		~CConfigFile();

	private:

		TiXmlDocument *			m_pDocument;

	private:

		static CVarList *		s_pVarList;
		static CConfigFile *	s_pInstance;
};


//**********************************************************************************
//**********************************************************************************
//
//	Config var class
//
//**********************************************************************************
//**********************************************************************************
class CConfigVarBase
{
	public:

		CConfigVarBase( const CString & name )
		:	m_szName( name )
		{
			CConfigFile::AddVar( this );
		}

		virtual ~CConfigVarBase()
		{
		}

		virtual const CString &	GetName() const
		{
			return m_szName;
		}

		virtual void			Set( const CString & value ) = 0;
		virtual void			Save( TiXmlElement * const p_element ) = 0;

	protected:

		CString					m_szName;
};

//**********************************************************************************
//
//**********************************************************************************
class CVarString : public CConfigVarBase
{
	public:

		CVarString( const CString & name, const CString & default_value )
		:	CConfigVarBase( name )
		,	m_szValue( default_value )
		{
		}

		const CString &	Get() const
		{
			return m_szValue;
		}

		virtual void	Set( const CString & value )
		{
			m_szValue = value;
		}

		const CString &	operator * ()
		{
			return m_szValue;
		}

		const CString &	operator = ( const CString & rhs )
		{
			m_szValue = rhs;

			return m_szValue;
		}

		virtual void	Save( TiXmlElement * const p_element )
		{
			p_element->SetAttribute( m_szName, m_szValue );
		}

	private:

		CString		m_szValue;
};

//**********************************************************************************
//
//**********************************************************************************
class CVarFloat : public CConfigVarBase
{
	public:

		CVarFloat( const CString & name, const float & default_value )
		:	CConfigVarBase( name )
		,	m_szValue( default_value )
		{
		}

		const float &	Get() const
		{
			return m_szValue;
		}

		virtual void	Set( const CString & value )
		{
			m_szValue = static_cast< float >( atof( value ) );
		}

		const float &	operator * ()
		{
			return m_szValue;
		}

		const float &	operator = ( const float & rhs )
		{
			m_szValue = rhs;

			return m_szValue;
		}

		virtual void	Save( TiXmlElement * const p_element )
		{
			CString	value;

			value.Printf( "%.2f", m_szValue );

			p_element->SetAttribute( m_szName, value );
		}

	private:

		float		m_szValue;
};

//**********************************************************************************
//
//**********************************************************************************
class CVarInt : public CConfigVarBase
{
	public:

		CVarInt( const CString & name, const int & default_value )
		:	CConfigVarBase( name )
		,	m_szValue( default_value )
		{
		}

		const int &	Get() const
		{
			return m_szValue;
		}

		virtual void	Set( const CString & value )
		{
			m_szValue = atoi( value );
		}

		const int &	operator * ()
		{
			return m_szValue;
		}

		const int &	operator = ( const int & rhs )
		{
			m_szValue = rhs;

			return m_szValue;
		}

		virtual void	Save( TiXmlElement * const p_element )
		{
			CString	value;

			value.Printf( "%d", m_szValue );

			p_element->SetAttribute( m_szName, value );
		}

	private:

		int		m_szValue;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CCONFIGFILE_H_ */
