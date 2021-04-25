/***********************************************************************************

  Module :	CUSBManager.cpp

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
#include "CTypes.h"
#include <pspusb.h>
#include <pspusbstor.h>
#include "CUSBManager.h"
#include "CFrameWork.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//*************************************************************************************
//
//*************************************************************************************
bool	CUSBManager::Open()
{
#ifdef KERNEL_MODE
	//
	//	Start necessary drivers
	//
	CFrameWork::LoadAndStartModule( "flash0:/kd/semawm.prx", CFrameWork::KERNEL_PARTITION );
	CFrameWork::LoadAndStartModule( "flash0:/kd/usbstor.prx", CFrameWork::KERNEL_PARTITION );
	CFrameWork::LoadAndStartModule( "flash0:/kd/usbstormgr.prx", CFrameWork::KERNEL_PARTITION );
	CFrameWork::LoadAndStartModule( "flash0:/kd/usbstorms.prx", CFrameWork::KERNEL_PARTITION );
	CFrameWork::LoadAndStartModule( "flash0:/kd/usbstorboot.prx", CFrameWork::KERNEL_PARTITION );

	//
	//	Setup USB drivers
	//
	if ( sceUsbStart( PSP_USBBUS_DRIVERNAME, 0, 0 ) != 0 )
	{
		ASSERT( 0, "Error starting USB Bus driver\n" );

		return false;
	}

	if ( sceUsbStart( PSP_USBSTOR_DRIVERNAME, 0, 0 ) != 0 )
	{
		ASSERT( 0, "Error starting USB Mass Storage driver\n " );

		return false;
	}

	if ( sceUsbstorBootSetCapacity( 0x800000 ) != 0 )
	{
		ASSERT( 0, "Error setting capacity with USB Mass Storage driver\n" );

		return false;
	}
#endif	// #ifdef KERNEL_MODE
	return true;
}

//*************************************************************************************
//
//*************************************************************************************
void	CUSBManager::Close()
{
#ifdef KERNEL_MODE
	if ( IsActive() == true )
	{
		if ( Deactivate() == false )
		{
			ASSERT( 0, "Error closing USB connection" );
		}
	}

	if ( sceUsbStop( PSP_USBSTOR_DRIVERNAME, 0, 0 ) != 0 )
	{
		ASSERT( 0, "Error stopping USB Mass Storage driver\n" );
	}

	if ( sceUsbStop( PSP_USBBUS_DRIVERNAME, 0, 0 ) != 0 )
	{
		ASSERT( 0, "Error stopping USB BUS driver\n" );
	}
#endif	// #ifdef KERNEL_MODE
}

//*************************************************************************************
//
//*************************************************************************************
bool	CUSBManager::Activate()
{
#ifdef KERNEL_MODE

	if ( sceUsbGetState() & PSP_USB_ACTIVATED )
	{
		return false;
	}

	return ( sceUsbActivate( 0x1c8 ) == 0 );

#else	// #ifdef KERNEL_MODE

	return false;

#endif	// #ifdef KERNEL_MODE
}

//*************************************************************************************
//
//*************************************************************************************
bool	CUSBManager::Deactivate()
{
#ifdef KERNEL_MODE

	if ( sceUsbGetState() & PSP_USB_ACTIVATED )
	{
		return ( sceUsbDeactivate( 0x1c8 ) == 0 );
	}

	return false;

#else	// #ifdef KERNEL_MODE

	return true;

#endif	// #ifdef KERNEL_MODE
}

//*************************************************************************************
//
//*************************************************************************************
bool	CUSBManager::IsActive()
{
#ifdef KERNEL_MODE

	return ( sceUsbGetState() & PSP_USB_ACTIVATED );

#else	// #ifdef KERNEL_MODE

	return false;

#endif	// #ifdef KERNEL_MODE
}

//**********************************************************************************
//
//**********************************************************************************
bool	CUSBManager::CableConnected()
{
#ifdef KERNEL_MODE

	return ( sceUsbGetState() & PSP_USB_CABLE_CONNECTED );

#else	// #ifdef KERNEL_MODE

	return false;

#endif	// #ifdef KERNEL_MODE
}

//**********************************************************************************
//
//**********************************************************************************
bool	CUSBManager::ConnectionEstablished()
{
#ifdef KERNEL_MODE

	return ( sceUsbGetState() & PSP_USB_CONNECTION_ESTABLISHED );

#else	// #ifdef KERNEL_MODE

	return false;

#endif	// #ifdef KERNEL_MODE
}

//*******************************  END OF FILE  ************************************
