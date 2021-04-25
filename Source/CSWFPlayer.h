/***********************************************************************************

  Module :	CSWFPlayer.h

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

#ifndef CSWFPLAYER_H_
#define CSWFPLAYER_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CProcess.h"
#include "CDirectoryList.h"

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
class CSWFPlayer : public CProcess
{
	public:

		static void				Create();
		static void				Destroy();

		static CSWFPlayer *		Get();

	public:

		virtual void			Process();
		virtual void			ProcessInput();

		CDirectoryList *		GetSrcList() const;
		CDirectoryList *		GetFocusList() const;

		void					SetListFocus( bool focus );

	private:

		CSWFPlayer();
		~CSWFPlayer();

	private:

		CDirectoryList *		m_pSrcList;

		float					m_FadeInLevel;

	private:

		static CSWFPlayer *		s_pInstance;
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CSWFPLAYER_H_ */
