/***********************************************************************************

  Module :	CProcess.h

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

#ifndef CPROCESS_H_
#define CPROCESS_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"

//**********************************************************************************
//   Macros
//**********************************************************************************

//**********************************************************************************
//   Types
//**********************************************************************************
class CProcess;

typedef std::list< CProcess * >	CProcessList;

//**********************************************************************************
//   Constants
//**********************************************************************************

//**********************************************************************************
//   Class definitions
//**********************************************************************************
class CProcess
{
	public:

		static void				Open();
		static void				Close();
		static void				ProcessList();

		static void				Pause( bool pause );
		static bool				IsPaused();

	public:

		CProcess();

		virtual void			Delete();

		void					MoveToBackOfList();
		void					MoveToFrontOfList();

		virtual void			Process() = 0;
		virtual void			ProcessInput() = 0;

	protected:

		virtual ~CProcess();

	private:

		static bool				s_bPaused;
		static CProcessList		s_ProcessList;
		static CProcessList		s_NewProcessList;
		static CProcessList		s_DeletedProcessList;
		static CProcessList		s_MoveToBackProcessList;
		static CProcessList		s_MoveToFrontProcessList;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CPROCESS_H_ */
