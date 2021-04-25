/***********************************************************************************

  Module :	CAssert.cpp

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
#include "CAssert.h"
#include "CInput.h"
#include "CGfx.h"
#include "CFont.h"
#include "CString.h"
#include "CFileSystem.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************
static const CString	s_szLogFile( "ms0:/SWFPlayer_log.txt" );

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************
static CFile *	s_pLogFile( NULL );
static char		s_MsgBuffer[ 2048 ];

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//	
//**********************************************************************************
void	CAssert::Open()
{
	const int	file_handle( sceIoOpen( s_szLogFile, PSP_O_CREAT | PSP_O_TRUNC | PSP_O_WRONLY, 0777 ) );

	if ( file_handle >= 0 )
	{
		sceIoClose( file_handle );
	}

#if defined( USE_LOG_FILE )

	s_pLogFile = CFileSystem::Open( s_szLogFile, "w" );

	if ( s_pLogFile != NULL )
	{
		fprintf( *s_pLogFile, "************* LOG START **************\n\n" );
	}

#endif	// #if defined( USE_LOG_FILE )
}

//**********************************************************************************
//	
//**********************************************************************************
void	CAssert::Close()
{
#if defined( USE_LOG_FILE )

	if ( s_pLogFile != NULL )
	{
		fprintf( *s_pLogFile, "\n\n************* LOG END **************\n" );

		CFileSystem::Close( s_pLogFile );

		s_pLogFile = NULL;
	}

#endif	// #if defined( USE_LOG_FILE )
}

//**********************************************************************************
//
//**********************************************************************************
static u32	s_MessageNum( 0 );

void	CAssert::Message( const char * fmt, ... )
{
	CInput::Process();

	if ( CInput::IsButtonDown( CInput::LTRIGGER ) == true )
	{
		return;
	}

	va_list	marker;

	va_start( marker, fmt );

	vsprintf( s_MsgBuffer, fmt, marker );

	va_end( marker );

	pspDebugScreenPrintf( "%d - ", s_MessageNum );
	pspDebugScreenPrintf( s_MsgBuffer );

#if defined( USE_LOG_FILE )

	if ( s_pLogFile != NULL )
	{
		fprintf( *s_pLogFile, s_MsgBuffer );
	}

#endif	// #if defined( USE_LOG_FILE )

	++s_MessageNum;
}

//**********************************************************************************
//
//**********************************************************************************
bool	CAssert::AssertMessage( const char * fmt, ... )
{
	if ( CInput::IsButtonDown( CInput::RTRIGGER ) == true )
	{
		return true;
	}

	bool	quit( false );
	bool	ignore( false );

	va_list	marker;

	va_start( marker, fmt );

	vsprintf( s_MsgBuffer, fmt, marker );

	va_end( marker );

	pspDebugScreenPrintf( s_MsgBuffer );
	pspDebugScreenPrintf( "\nPress TRIANGLE to continue\n" );

#if defined( USE_LOG_FILE )

	if ( s_pLogFile != NULL )
	{
		fprintf( *s_pLogFile, s_MsgBuffer );
	}

#endif	// #if defined( USE_LOG_FILE )

	while ( quit == false )
	{
		CInput::Process();

		if ( CInput::IsButtonClicked( CInput::TRIANGLE ) == true )
		{
			quit = true;
		}

		if ( CInput::IsButtonClicked( CInput::SQUARE ) == true )
		{
			quit = true;
			ignore = true;
		}

		CGfx::SwapBuffers();
	}

	return ignore;
}

//**********************************************************************************
//
//**********************************************************************************
void	CAssert::DisplayErrorCode( int code )
{
	switch ( code )
	{
	case SCE_KERNEL_ERROR_OK:								Message( "SCE_KERNEL_ERROR_OK	 " );	break;
	case SCE_KERNEL_ERROR_ERROR:							Message( "SCE_KERNEL_ERROR_ERROR	 " );	break;
	case SCE_KERNEL_ERROR_NOTIMP:							Message( "SCE_KERNEL_ERROR_NOTIMP	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_EXPCODE:					Message( "SCE_KERNEL_ERROR_ILLEGAL_EXPCODE	" );	break;
	case SCE_KERNEL_ERROR_EXPHANDLER_NOUSE:					Message( "SCE_KERNEL_ERROR_EXPHANDLER_NOUSE	" );	break;
	case SCE_KERNEL_ERROR_EXPHANDLER_USED:					Message( "SCE_KERNEL_ERROR_EXPHANDLER_USED	" );	break;
	case SCE_KERNEL_ERROR_SYCALLTABLE_NOUSED:				Message( "SCE_KERNEL_ERROR_SYCALLTABLE_NOUSED	" );	break;
	case SCE_KERNEL_ERROR_SYCALLTABLE_USED:					Message( "SCE_KERNEL_ERROR_SYCALLTABLE_USED	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_SYSCALLTABLE:				Message( "SCE_KERNEL_ERROR_ILLEGAL_SYSCALLTABLE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_PRIMARY_SYSCALL_NUMBER:	Message( "SCE_KERNEL_ERROR_ILLEGAL_PRIMARY_SYSCALL_NUMBER	" );	break;
	case SCE_KERNEL_ERROR_PRIMARY_SYSCALL_NUMBER_INUSE:		Message( "SCE_KERNEL_ERROR_PRIMARY_SYSCALL_NUMBER_INUSE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_CONTEXT:					Message( "SCE_KERNEL_ERROR_ILLEGAL_CONTEXT	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_INTRCODE:					Message( "SCE_KERNEL_ERROR_ILLEGAL_INTRCODE	" );	break;
	case SCE_KERNEL_ERROR_CPUDI:							Message( "SCE_KERNEL_ERROR_CPUDI	" );	break;
	case SCE_KERNEL_ERROR_FOUND_HANDLER:					Message( "SCE_KERNEL_ERROR_FOUND_HANDLER	" );	break;
	case SCE_KERNEL_ERROR_NOTFOUND_HANDLER:					Message( "SCE_KERNEL_ERROR_NOTFOUND_HANDLER	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_INTRLEVEL:				Message( "SCE_KERNEL_ERROR_ILLEGAL_INTRLEVEL	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_ADDRESS:					Message( "SCE_KERNEL_ERROR_ILLEGAL_ADDRESS	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_INTRPARAM:				Message( "SCE_KERNEL_ERROR_ILLEGAL_INTRPARAM	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_STACK_ADDRESS:			Message( "SCE_KERNEL_ERROR_ILLEGAL_STACK_ADDRESS	" );	break;
	case SCE_KERNEL_ERROR_ALREADY_STACK_SET:				Message( "SCE_KERNEL_ERROR_ALREADY_STACK_SET	" );	break;
	case SCE_KERNEL_ERROR_NO_TIMER:							Message( "SCE_KERNEL_ERROR_NO_TIMER	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_TIMERID:					Message( "SCE_KERNEL_ERROR_ILLEGAL_TIMERID	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_SOURCE:					Message( "SCE_KERNEL_ERROR_ILLEGAL_SOURCE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_PRESCALE:					Message( "SCE_KERNEL_ERROR_ILLEGAL_PRESCALE	" );	break;
	case SCE_KERNEL_ERROR_TIMER_BUSY:						Message( "SCE_KERNEL_ERROR_TIMER_BUSY	" );	break;
	case SCE_KERNEL_ERROR_TIMER_NOT_SETUP:					Message( "SCE_KERNEL_ERROR_TIMER_NOT_SETUP	" );	break;
	case SCE_KERNEL_ERROR_TIMER_NOT_INUSE:					Message( "SCE_KERNEL_ERROR_TIMER_NOT_INUSE	" );	break;
	case SCE_KERNEL_ERROR_UNIT_USED:						Message( "SCE_KERNEL_ERROR_UNIT_USED	" );	break;
	case SCE_KERNEL_ERROR_UNIT_NOUSE:						Message( "SCE_KERNEL_ERROR_UNIT_NOUSE	" );	break;
	case SCE_KERNEL_ERROR_NO_ROMDIR:						Message( "SCE_KERNEL_ERROR_NO_ROMDIR	" );	break;
	case SCE_KERNEL_ERROR_IDTYPE_EXIST:						Message( "SCE_KERNEL_ERROR_IDTYPE_EXIST	" );	break;
	case SCE_KERNEL_ERROR_IDTYPE_NOT_EXIST:					Message( "SCE_KERNEL_ERROR_IDTYPE_NOT_EXIST	" );	break;
	case SCE_KERNEL_ERROR_IDTYPE_NOT_EMPTY:					Message( "SCE_KERNEL_ERROR_IDTYPE_NOT_EMPTY	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_UID:						Message( "SCE_KERNEL_ERROR_UNKNOWN_UID	" );	break;
	case SCE_KERNEL_ERROR_UNMATCH_UID_TYPE:					Message( "SCE_KERNEL_ERROR_UNMATCH_UID_TYPE	" );	break;
	case SCE_KERNEL_ERROR_ID_NOT_EXIST:						Message( "SCE_KERNEL_ERROR_ID_NOT_EXIST	" );	break;
	case SCE_KERNEL_ERROR_NOT_FOUND_UIDFUNC:				Message( "SCE_KERNEL_ERROR_NOT_FOUND_UIDFUNC	" );	break;
	case SCE_KERNEL_ERROR_UID_ALREADY_HOLDER:				Message( "SCE_KERNEL_ERROR_UID_ALREADY_HOLDER	" );	break;
	case SCE_KERNEL_ERROR_UID_NOT_HOLDER:					Message( "SCE_KERNEL_ERROR_UID_NOT_HOLDER	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_PERM:						Message( "SCE_KERNEL_ERROR_ILLEGAL_PERM	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT:					Message( "SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_ADDR:						Message( "SCE_KERNEL_ERROR_ILLEGAL_ADDR	" );	break;
	case SCE_KERNEL_ERROR_OUT_OF_RANGE:						Message( "SCE_KERNEL_ERROR_OUT_OF_RANGE	" );	break;
	case SCE_KERNEL_ERROR_MEM_RANGE_OVERLAP:				Message( "SCE_KERNEL_ERROR_MEM_RANGE_OVERLAP	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_PARTITION:				Message( "SCE_KERNEL_ERROR_ILLEGAL_PARTITION	" );	break;
	case SCE_KERNEL_ERROR_PARTITION_INUSE:					Message( "SCE_KERNEL_ERROR_PARTITION_INUSE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCKTYPE:				Message( "SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCKTYPE	" );	break;
	case SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED:			Message( "SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED	" );	break;
	case SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_LOCKED:			Message( "SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_LOCKED	" );	break;
	case SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_FAILED:			Message( "SCE_KERNEL_ERROR_MEMBLOCK_RESIZE_FAILED	" );	break;
	case SCE_KERNEL_ERROR_HEAPBLOCK_ALLOC_FAILED:			Message( "SCE_KERNEL_ERROR_HEAPBLOCK_ALLOC_FAILED	" );	break;
	case SCE_KERNEL_ERROR_HEAP_ALLOC_FAILED:				Message( "SCE_KERNEL_ERROR_HEAP_ALLOC_FAILED	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID:					Message( "SCE_KERNEL_ERROR_ILLEGAL_CHUNK_ID	" );	break;
	case SCE_KERNEL_ERROR_NOCHUNK:							Message( "SCE_KERNEL_ERROR_NOCHUNK	" );	break;
	case SCE_KERNEL_ERROR_NO_FREECHUNK:						Message( "SCE_KERNEL_ERROR_NO_FREECHUNK	" );	break;
	case SCE_KERNEL_ERROR_LINKERR:							Message( "SCE_KERNEL_ERROR_LINKERR	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_OBJECT:					Message( "SCE_KERNEL_ERROR_ILLEGAL_OBJECT	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_MODULE:					Message( "SCE_KERNEL_ERROR_UNKNOWN_MODULE	" );	break;
	case SCE_KERNEL_ERROR_NOFILE:							Message( "SCE_KERNEL_ERROR_NOFILE	" );	break;
	case SCE_KERNEL_ERROR_FILEERR:							Message( "SCE_KERNEL_ERROR_FILEERR	" );	break;
	case SCE_KERNEL_ERROR_MEMINUSE:							Message( "SCE_KERNEL_ERROR_MEMINUSE	" );	break;
	case SCE_KERNEL_ERROR_PARTITION_MISMATCH:				Message( "SCE_KERNEL_ERROR_PARTITION_MISMATCH	" );	break;
	case SCE_KERNEL_ERROR_ALREADY_STARTED:					Message( "SCE_KERNEL_ERROR_ALREADY_STARTED	" );	break;
	case SCE_KERNEL_ERROR_NOT_STARTED:						Message( "SCE_KERNEL_ERROR_NOT_STARTED	" );	break;
	case SCE_KERNEL_ERROR_ALREADY_STOPPED:					Message( "SCE_KERNEL_ERROR_ALREADY_STOPPED	" );	break;
	case SCE_KERNEL_ERROR_CAN_NOT_STOP:						Message( "SCE_KERNEL_ERROR_CAN_NOT_STOP	" );	break;
	case SCE_KERNEL_ERROR_NOT_STOPPED:						Message( "SCE_KERNEL_ERROR_NOT_STOPPED	" );	break;
	case SCE_KERNEL_ERROR_NOT_REMOVABLE:					Message( "SCE_KERNEL_ERROR_NOT_REMOVABLE	" );	break;
	case SCE_KERNEL_ERROR_EXCLUSIVE_LOAD:					Message( "SCE_KERNEL_ERROR_EXCLUSIVE_LOAD	" );	break;
	case SCE_KERNEL_ERROR_LIBRARY_NOT_YET_LINKED:			Message( "SCE_KERNEL_ERROR_LIBRARY_NOT_YET_LINKED	" );	break;
	case SCE_KERNEL_ERROR_LIBRARY_FOUND:					Message( "SCE_KERNEL_ERROR_LIBRARY_FOUND	" );	break;
	case SCE_KERNEL_ERROR_LIBRARY_NOTFOUND:					Message( "SCE_KERNEL_ERROR_LIBRARY_NOTFOUND	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_LIBRARY:					Message( "SCE_KERNEL_ERROR_ILLEGAL_LIBRARY	" );	break;
	case SCE_KERNEL_ERROR_LIBRARY_INUSE:					Message( "SCE_KERNEL_ERROR_LIBRARY_INUSE	" );	break;
	case SCE_KERNEL_ERROR_ALREADY_STOPPING:					Message( "SCE_KERNEL_ERROR_ALREADY_STOPPING	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_OFFSET:					Message( "SCE_KERNEL_ERROR_ILLEGAL_OFFSET	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_POSITION:					Message( "SCE_KERNEL_ERROR_ILLEGAL_POSITION	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_ACCESS:					Message( "SCE_KERNEL_ERROR_ILLEGAL_ACCESS	" );	break;
	case SCE_KERNEL_ERROR_MODULE_MGR_BUSY:					Message( "SCE_KERNEL_ERROR_MODULE_MGR_BUSY	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_FLAG:						Message( "SCE_KERNEL_ERROR_ILLEGAL_FLAG	" );	break;
	case SCE_KERNEL_ERROR_CANNOT_GET_MODULELIST:			Message( "SCE_KERNEL_ERROR_CANNOT_GET_MODULELIST	" );	break;
	case SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE:		Message( "SCE_KERNEL_ERROR_PROHIBIT_LOADMODULE_DEVICE	" );	break;
	case SCE_KERNEL_ERROR_PROHIBIT_LOADEXEC_DEVICE:			Message( "SCE_KERNEL_ERROR_PROHIBIT_LOADEXEC_DEVICE	" );	break;
	case SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE:				Message( "SCE_KERNEL_ERROR_UNSUPPORTED_PRX_TYPE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL:				Message( "SCE_KERNEL_ERROR_ILLEGAL_PERM_CALL	" );	break;
	case SCE_KERNEL_ERROR_CANNOT_GET_MODULE_INFORMATION:	Message( "SCE_KERNEL_ERROR_CANNOT_GET_MODULE_INFORMATION	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_LOADEXEC_BUFFER:			Message( "SCE_KERNEL_ERROR_ILLEGAL_LOADEXEC_BUFFER	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_LOADEXEC_FILENAME:		Message( "SCE_KERNEL_ERROR_ILLEGAL_LOADEXEC_FILENAME	" );	break;
	case SCE_KERNEL_ERROR_NO_EXIT_CALLBACK:					Message( "SCE_KERNEL_ERROR_NO_EXIT_CALLBACK	" );	break;
	case SCE_KERNEL_ERROR_NO_MEMORY:						Message( "SCE_KERNEL_ERROR_NO_MEMORY	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_ATTR:						Message( "SCE_KERNEL_ERROR_ILLEGAL_ATTR	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_ENTRY:					Message( "SCE_KERNEL_ERROR_ILLEGAL_ENTRY	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_PRIORITY:					Message( "SCE_KERNEL_ERROR_ILLEGAL_PRIORITY	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE:				Message( "SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_MODE:						Message( "SCE_KERNEL_ERROR_ILLEGAL_MODE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_MASK:						Message( "SCE_KERNEL_ERROR_ILLEGAL_MASK	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_THID:						Message( "SCE_KERNEL_ERROR_ILLEGAL_THID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_THID:						Message( "SCE_KERNEL_ERROR_UNKNOWN_THID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_SEMID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_SEMID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_EVFID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_EVFID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_MBXID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_MBXID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_VPLID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_VPLID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_FPLID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_FPLID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_MPPID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_MPPID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_ALMID:					Message( "SCE_KERNEL_ERROR_UNKNOWN_ALMID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_TEID:						Message( "SCE_KERNEL_ERROR_UNKNOWN_TEID	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_CBID:						Message( "SCE_KERNEL_ERROR_UNKNOWN_CBID	" );	break;
	case SCE_KERNEL_ERROR_DORMANT:							Message( "SCE_KERNEL_ERROR_DORMANT	" );	break;
	case SCE_KERNEL_ERROR_SUSPEND:							Message( "SCE_KERNEL_ERROR_SUSPEND	" );	break;
	case SCE_KERNEL_ERROR_NOT_DORMANT:						Message( "SCE_KERNEL_ERROR_NOT_DORMANT	" );	break;
	case SCE_KERNEL_ERROR_NOT_SUSPEND:						Message( "SCE_KERNEL_ERROR_NOT_SUSPEND	" );	break;
	case SCE_KERNEL_ERROR_NOT_WAIT:							Message( "SCE_KERNEL_ERROR_NOT_WAIT	" );	break;
	case SCE_KERNEL_ERROR_CAN_NOT_WAIT:						Message( "SCE_KERNEL_ERROR_CAN_NOT_WAIT	" );	break;
	case SCE_KERNEL_ERROR_WAIT_TIMEOUT:						Message( "SCE_KERNEL_ERROR_WAIT_TIMEOUT	" );	break;
	case SCE_KERNEL_ERROR_WAIT_CANCEL:						Message( "SCE_KERNEL_ERROR_WAIT_CANCEL	" );	break;
	case SCE_KERNEL_ERROR_RELEASE_WAIT:						Message( "SCE_KERNEL_ERROR_RELEASE_WAIT	" );	break;
	case SCE_KERNEL_ERROR_NOTIFY_CALLBACK:					Message( "SCE_KERNEL_ERROR_NOTIFY_CALLBACK	" );	break;
	case SCE_KERNEL_ERROR_THREAD_TERMINATED:				Message( "SCE_KERNEL_ERROR_THREAD_TERMINATED	" );	break;
	case SCE_KERNEL_ERROR_SEMA_ZERO:						Message( "SCE_KERNEL_ERROR_SEMA_ZERO	" );	break;
	case SCE_KERNEL_ERROR_SEMA_OVF:							Message( "SCE_KERNEL_ERROR_SEMA_OVF	" );	break;
	case SCE_KERNEL_ERROR_EVF_COND:							Message( "SCE_KERNEL_ERROR_EVF_COND	" );	break;
	case SCE_KERNEL_ERROR_EVF_MULTI:						Message( "SCE_KERNEL_ERROR_EVF_MULTI	" );	break;
	case SCE_KERNEL_ERROR_EVF_ILPAT:						Message( "SCE_KERNEL_ERROR_EVF_ILPAT	" );	break;
	case SCE_KERNEL_ERROR_MBOX_NOMSG:						Message( "SCE_KERNEL_ERROR_MBOX_NOMSG	" );	break;
	case SCE_KERNEL_ERROR_MPP_FULL:							Message( "SCE_KERNEL_ERROR_MPP_FULL	" );	break;
	case SCE_KERNEL_ERROR_MPP_EMPTY:						Message( "SCE_KERNEL_ERROR_MPP_EMPTY	" );	break;
	case SCE_KERNEL_ERROR_WAIT_DELETE:						Message( "SCE_KERNEL_ERROR_WAIT_DELETE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK:					Message( "SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_MEMSIZE:					Message( "SCE_KERNEL_ERROR_ILLEGAL_MEMSIZE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_SPADADDR:					Message( "SCE_KERNEL_ERROR_ILLEGAL_SPADADDR	" );	break;
	case SCE_KERNEL_ERROR_SPAD_INUSE:						Message( "SCE_KERNEL_ERROR_SPAD_INUSE	" );	break;
	case SCE_KERNEL_ERROR_SPAD_NOT_INUSE:					Message( "SCE_KERNEL_ERROR_SPAD_NOT_INUSE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_TYPE:						Message( "SCE_KERNEL_ERROR_ILLEGAL_TYPE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_SIZE:						Message( "SCE_KERNEL_ERROR_ILLEGAL_SIZE	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_COUNT:					Message( "SCE_KERNEL_ERROR_ILLEGAL_COUNT	" );	break;
	case SCE_KERNEL_ERROR_UNKNOWN_VTID:						Message( "SCE_KERNEL_ERROR_UNKNOWN_VTID	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_VTID:						Message( "SCE_KERNEL_ERROR_ILLEGAL_VTID	" );	break;
	case SCE_KERNEL_ERROR_ILLEGAL_KTLSID:					Message( "SCE_KERNEL_ERROR_ILLEGAL_KTLSID	" );	break;
	case SCE_KERNEL_ERROR_KTLS_FULL:						Message( "SCE_KERNEL_ERROR_KTLS_FULL	" );	break;
	case SCE_KERNEL_ERROR_KTLS_BUSY:						Message( "SCE_KERNEL_ERROR_KTLS_BUSY	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_PRIORITY:				Message( "SCE_KERNEL_ERROR_PM_INVALID_PRIORITY	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_DEVNAME:				Message( "SCE_KERNEL_ERROR_PM_INVALID_DEVNAME	" );	break;
	case SCE_KERNEL_ERROR_PM_UNKNOWN_DEVNAME:				Message( "SCE_KERNEL_ERROR_PM_UNKNOWN_DEVNAME	" );	break;
	case SCE_KERNEL_ERROR_PM_PMINFO_REGISTERED:				Message( "SCE_KERNEL_ERROR_PM_PMINFO_REGISTERED	" );	break;
	case SCE_KERNEL_ERROR_PM_PMINFO_UNREGISTERED:			Message( "SCE_KERNEL_ERROR_PM_PMINFO_UNREGISTERED	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_MAJOR_STATE:			Message( "SCE_KERNEL_ERROR_PM_INVALID_MAJOR_STATE	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_REQUEST:				Message( "SCE_KERNEL_ERROR_PM_INVALID_REQUEST	" );	break;
	case SCE_KERNEL_ERROR_PM_UNKNOWN_REQUEST:				Message( "SCE_KERNEL_ERROR_PM_UNKNOWN_REQUEST	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_UNIT:					Message( "SCE_KERNEL_ERROR_PM_INVALID_UNIT	" );	break;
	case SCE_KERNEL_ERROR_PM_CANNOT_CANCEL:					Message( "SCE_KERNEL_ERROR_PM_CANNOT_CANCEL	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_PMINFO:				Message( "SCE_KERNEL_ERROR_PM_INVALID_PMINFO	" );	break;
	case SCE_KERNEL_ERROR_PM_INVALID_ARGUMENT:				Message( "SCE_KERNEL_ERROR_PM_INVALID_ARGUMENT	" );	break;
	case SCE_KERNEL_ERROR_PM_ALREADY_TARGET_PWRSTATE:		Message( "SCE_KERNEL_ERROR_PM_ALREADY_TARGET_PWRSTATE	" );	break;
	case SCE_KERNEL_ERROR_PM_CHANGE_PWRSTATE_FAILED:		Message( "SCE_KERNEL_ERROR_PM_CHANGE_PWRSTATE_FAILED	" );	break;
	case SCE_KERNEL_ERROR_PM_CANNOT_CHANGE_DEVPWR_STATE:	Message( "SCE_KERNEL_ERROR_PM_CANNOT_CHANGE_DEVPWR_STATE	" );	break;
	case SCE_KERNEL_ERROR_PM_NO_SUPPORT_DEVPWR_STATE:		Message( "SCE_KERNEL_ERROR_PM_NO_SUPPORT_DEVPWR_STATE	" );	break;
	case SCE_KERNEL_ERROR_DMAC_REQUEST_FAILED:				Message( "SCE_KERNEL_ERROR_DMAC_REQUEST_FAILED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_REQUEST_DENIED:				Message( "SCE_KERNEL_ERROR_DMAC_REQUEST_DENIED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_QUEUED:					Message( "SCE_KERNEL_ERROR_DMAC_OP_QUEUED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_NOT_QUEUED:				Message( "SCE_KERNEL_ERROR_DMAC_OP_NOT_QUEUED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_RUNNING:					Message( "SCE_KERNEL_ERROR_DMAC_OP_RUNNING	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_NOT_ASSIGNED:				Message( "SCE_KERNEL_ERROR_DMAC_OP_NOT_ASSIGNED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_TIMEOUT:					Message( "SCE_KERNEL_ERROR_DMAC_OP_TIMEOUT	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_FREED:					Message( "SCE_KERNEL_ERROR_DMAC_OP_FREED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_USED:						Message( "SCE_KERNEL_ERROR_DMAC_OP_USED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_EMPTY:					Message( "SCE_KERNEL_ERROR_DMAC_OP_EMPTY	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_ABORTED:					Message( "SCE_KERNEL_ERROR_DMAC_OP_ABORTED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_OP_ERROR:					Message( "SCE_KERNEL_ERROR_DMAC_OP_ERROR	" );	break;
	case SCE_KERNEL_ERROR_DMAC_CHANNEL_RESERVED:			Message( "SCE_KERNEL_ERROR_DMAC_CHANNEL_RESERVED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_CHANNEL_EXCLUDED:			Message( "SCE_KERNEL_ERROR_DMAC_CHANNEL_EXCLUDED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_PRIVILEGE_ADDRESS:			Message( "SCE_KERNEL_ERROR_DMAC_PRIVILEGE_ADDRESS	" );	break;
	case SCE_KERNEL_ERROR_DMAC_NO_ENOUGHSPACE:				Message( "SCE_KERNEL_ERROR_DMAC_NO_ENOUGHSPACE	" );	break;
	case SCE_KERNEL_ERROR_DMAC_CHANNEL_NOT_ASSIGNED:		Message( "SCE_KERNEL_ERROR_DMAC_CHANNEL_NOT_ASSIGNED	" );	break;
	case SCE_KERNEL_ERROR_DMAC_CHILD_OPERATION:				Message( "SCE_KERNEL_ERROR_DMAC_CHILD_OPERATION	" );	break;
	case SCE_KERNEL_ERROR_DMAC_TOO_MUCH_SIZE:				Message( "SCE_KERNEL_ERROR_DMAC_TOO_MUCH_SIZE	" );	break;
	case SCE_KERNEL_ERROR_DMAC_INVALID_ARGUMENT:			Message( "SCE_KERNEL_ERROR_DMAC_INVALID_ARGUMENT	" );	break;
	case SCE_KERNEL_ERROR_MFILE:							Message( "SCE_KERNEL_ERROR_MFILE	" );	break;
	case SCE_KERNEL_ERROR_NODEV:							Message( "SCE_KERNEL_ERROR_NODEV	" );	break;
	case SCE_KERNEL_ERROR_XDEV:								Message( "SCE_KERNEL_ERROR_XDEV	" );	break;
	case SCE_KERNEL_ERROR_BADF:								Message( "SCE_KERNEL_ERROR_BADF	" );	break;
	case SCE_KERNEL_ERROR_INVAL:							Message( "SCE_KERNEL_ERROR_INVAL	" );	break;
	case SCE_KERNEL_ERROR_UNSUP:							Message( "SCE_KERNEL_ERROR_UNSUP	" );	break;
	case SCE_KERNEL_ERROR_ALIAS_USED:						Message( "SCE_KERNEL_ERROR_ALIAS_USED	" );	break;
	case SCE_KERNEL_ERROR_CANNOT_MOUNT:						Message( "SCE_KERNEL_ERROR_CANNOT_MOUNT	" );	break;
	case SCE_KERNEL_ERROR_DRIVER_DELETED:					Message( "SCE_KERNEL_ERROR_DRIVER_DELETED	" );	break;
	case SCE_KERNEL_ERROR_ASYNC_BUSY:						Message( "SCE_KERNEL_ERROR_ASYNC_BUSY	" );	break;
	case SCE_KERNEL_ERROR_NOASYNC:							Message( "SCE_KERNEL_ERROR_NOASYNC	" );	break;
	case SCE_KERNEL_ERROR_REGDEV:							Message( "SCE_KERNEL_ERROR_REGDEV	" );	break;
	case SCE_KERNEL_ERROR_NOCWD:							Message( "SCE_KERNEL_ERROR_NOCWD	" );	break;
	case SCE_KERNEL_ERROR_NAMETOOLONG:						Message( "SCE_KERNEL_ERROR_NAMETOOLONG	" );	break;
	case SCE_KERNEL_ERROR_NXIO:								Message( "SCE_KERNEL_ERROR_NXIO	" );	break;
	case SCE_KERNEL_ERROR_IO:								Message( "SCE_KERNEL_ERROR_IO	" );	break;
	case SCE_KERNEL_ERROR_NOMEM:							Message( "SCE_KERNEL_ERROR_NOMEM	" );	break;
	case SCE_KERNEL_ERROR_STDIO_NOT_OPENED:					Message( "SCE_KERNEL_ERROR_STDIO_NOT_OPENED	" );	break;
	case SCE_KERNEL_ERROR_CACHE_ALIGNMENT:					Message( "SCE_KERNEL_ERROR_CACHE_ALIGNMENT	" );	break;
	case SCE_KERNEL_ERROR_ERRORMAX:							Message( "SCE_KERNEL_ERROR_ERRORMAX	" );	break;
	default:												Message( "SCE_UNKNOWN_ERROR_CODE %X", code );	break;
	}
}

//*******************************  END OF FILE  ************************************
