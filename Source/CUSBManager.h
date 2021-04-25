/***********************************************************************************

  Module :	CUSBManager.h

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

#ifndef CUSBMANAGER_H_
#define CUSBMANAGER_H_

//**********************************************************************************
//   Include Files
//**********************************************************************************

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
class CUSBManager
{
	public:

		// Initialise the USB manager
		static bool		Open();

		// Shutdown the USB manager
		static void		Close();

		// Activate the USB connection, returns true if successful
		static bool		Activate();

		// Deactivate the USB connection, returns true if successful
		static bool		Deactivate();

		// Returns true if the USB connection is active
		static bool		IsActive();

		// Returns true if the USB cable is plugged in
		static bool		CableConnected();

		// Returns true if the USB cable is plugged in
		static bool		ConnectionEstablished();
};

//**********************************************************************************
//   Externs
//**********************************************************************************

//**********************************************************************************
//   Prototypes
//**********************************************************************************

#endif /* CUSBMANAGER_H_ */
