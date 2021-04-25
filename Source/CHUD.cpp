/***********************************************************************************

  Module :	CHUD.cpp

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
#include "CHUD.h"
#include "CFrameWork.h"
#include "CRenderable.h"
#include "CGfx.h"
#include "CFont.h"
#include "CSkinManager.h"
#include "CTextureManager.h"
#include "CUSBManager.h"
#include "CInput.h"
#include "CRenderable.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************
static const CString	VERSION_STRING( "v1.5" );

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************

//**********************************************************************************
//   Global Variables
//**********************************************************************************

//**********************************************************************************
//   Static Variables
//**********************************************************************************
bool					CHUD::s_bVisible( true );
CHUD *					CHUD::s_pInstance( NULL );
CString					CHUD::s_szButtonText[ MAX_BUTTONS ] =	{ "", "", "", "", "", "" };
bool					CHUD::s_bShowProgressBar( false );
float					CHUD::s_fProgressBar( 0.f );
CString					CHUD::s_szProgressTitle( "" );

static const CString	s_szUSBConnected( "USB connected" );
static s32				s_BatteryFlash( 0 );


//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::Create()
{
	ASSERT( s_pInstance == NULL, "HUD has already been created!" );

	s_pInstance = new CHUD();
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::Destroy()
{
	ASSERT( s_pInstance != NULL, "HUD hasn't been created yet!" );

	SAFE_DELETE( s_pInstance );
}

//**********************************************************************************
//
//**********************************************************************************
CHUD::CHUD()
{
	CRenderable::Register( CRenderable::RO_HUD, Render );
}

//**********************************************************************************
//
//**********************************************************************************
CHUD::~CHUD()
{
	CRenderable::UnRegister( CRenderable::RO_HUD, Render );
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::Render()
{
	if ( s_pInstance != NULL )
	{
		if ( s_bVisible == true )
		{
			s_pInstance->RenderInternal();
		}
	}
}

//**********************************************************************************
//
//**********************************************************************************
static void	DrawButton( const V2 & in_pos, CSkinManager::eSkinComponent button_icon, const CString & text, bool left_justify )
{
	V2					pos( in_pos );
	V2					icon_size( 0.f, 0.f );
	CTexture * const	p_button_texture( CSkinManager::GetComponent( button_icon )->GetTexture() );
	const V2			text_size( CFont::GetDefaultFont()->GetStringSize( text ) );
	const ARGB			text_color( CSkinManager::GetColor( "hud", "text_color", 0xffffffff ) );

	if ( p_button_texture != NULL )
	{
		icon_size = V2( 16.f, 16.f );

		CGfx::DrawQuad( p_button_texture, pos, icon_size, 0xffffffff );
	}

	if ( left_justify == false )
	{
		CFont::GetDefaultFont()->Print( text, pos + V2( icon_size.x + 2, -0.5f * ( text_size.y - icon_size.y ) ), text_color, 1.f );
	}
	else
	{
		CFont::GetDefaultFont()->Print( text, pos - V2( text_size.x + 2, 0.5f * ( text_size.y - icon_size.y ) ), text_color, 1.f );
	}
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::RenderInternal()
{
	//
	//	Draw the pad buttons
	//
	const ARGB	text_color( CSkinManager::GetColor( "hud", "text_color", 0xffffffff ) );
	const V2	buttons_pos( CSkinManager::GetV2( "hud", "buttons_pos", V2( 400.f, 200.f ) ) );

//	DrawButton( V2( buttons_pos.x, buttons_pos.y + 0.f ), CSkinManager::SC_BUTTON_TRIANGLE, s_szButtonText[ BUTTON_TRIANGLE ], true );
//	DrawButton( V2( buttons_pos.x - 16.f, buttons_pos.y + 16.f ), CSkinManager::SC_BUTTON_SQUARE, s_szButtonText[ BUTTON_SQUARE ], true );
	DrawButton( V2( buttons_pos.x + 16.f, buttons_pos.y + 16.f ), CSkinManager::SC_BUTTON_CIRCLE, s_szButtonText[ BUTTON_CIRCLE ], false );
	DrawButton( V2( buttons_pos.x, buttons_pos.y + 32.f ), CSkinManager::SC_BUTTON_CROSS, s_szButtonText[ BUTTON_CROSS ], false );
//	DrawButton( V2( buttons_pos.x + 16.f, buttons_pos.y + 48.f ), CSkinManager::SC_BUTTON_START, s_szButtonText[ BUTTON_START ], false );
//	DrawButton( V2( buttons_pos.x, buttons_pos.y + 48.f ), CSkinManager::SC_BUTTON_SELECT, s_szButtonText[ BUTTON_SELECT ], true );

	//
	//	Draw the battery meter
	//
	if ( scePowerIsBatteryCharging() == 0 )
	{
		s_BatteryFlash = 0;
	}
	else
	{
		s_BatteryFlash = ( s_BatteryFlash + 1 ) & 63;
	}

	if ( ( s_BatteryFlash >> 5 ) == 0 )
	{
		CTexture *	p_battery;
		const s32	battery_power( scePowerGetBatteryLifePercent() );
		const V2	icon_pos( CSkinManager::GetV2( "hud", "battery_icon_pos", V2( CGfx::s_ScreenWidth - 48.f, 16.f ) ) );

		if ( battery_power <= 30 )			p_battery = CSkinManager::GetComponent( CSkinManager::SC_BATTERY_01 )->GetTexture();
		else if ( battery_power <= 60 )		p_battery = CSkinManager::GetComponent( CSkinManager::SC_BATTERY_02 )->GetTexture();
		else if ( battery_power <= 90 )		p_battery = CSkinManager::GetComponent( CSkinManager::SC_BATTERY_03 )->GetTexture();
		else								p_battery = CSkinManager::GetComponent( CSkinManager::SC_BATTERY_03 )->GetTexture();

		CGfx::DrawQuad( p_battery, icon_pos, V2( p_battery->m_nCanvasWidth, p_battery->m_nCanvasHeight ), 0x80ffffff );
	}

	//
	//	Draw the USB status
	//
	if ( CUSBManager::IsActive() == true )
	{
		const V2			icon_pos( CSkinManager::GetV2( "hud", "usb_icon_pos", V2( 16.f, 240.f ) ) );
		CTexture * const	p_usb( CSkinManager::GetComponent( CSkinManager::SC_ICON_USB )->GetTexture() );

		CGfx::DrawQuad( p_usb, icon_pos, V2( p_usb->m_nCanvasWidth, p_usb->m_nCanvasHeight ), 0xa0ffffff );

		if ( CUSBManager::CableConnected() == true && CUSBManager::ConnectionEstablished() == true )
		{
			const V2	text_size( CFont::GetDefaultFont()->GetStringSize( s_szUSBConnected ) );

			CFont::GetDefaultFont()->Print( s_szUSBConnected, icon_pos + V2( p_usb->m_nWidth + 2, -0.5f * ( text_size.y - p_usb->m_nHeight ) ), text_color, 1.f );
		}
	}

	//
	//	Draw the progress bar
	//
	if ( s_bShowProgressBar == true )
	{
		CTexture * const		p_progress_full( CSkinManager::GetComponent( CSkinManager::SC_PROGRESS_BAR_FULL )->GetTexture() );
		CTexture * const		p_progress_empty( CSkinManager::GetComponent( CSkinManager::SC_PROGRESS_BAR_EMPTY )->GetTexture() );
		const float				progress_u( p_progress_full->m_nWidth * s_fProgressBar );
		const V2				progress_pos( CSkinManager::GetV2( "hud", "progress_bar_pos", V2( 140.f, 220.f ) ) );
		sVertexTexturedColor *	p_full_verts;
		sVertexTexturedColor *	p_empty_verts;

		CGfx::GetPolyList( 2, &p_full_verts );
		CGfx::GetPolyList( 2, &p_empty_verts );

		p_full_verts[ 0 ].uv.x = 0.f;
		p_full_verts[ 0 ].uv.y = 0.f;
		p_full_verts[ 0 ].color = 0xffffffff;
		p_full_verts[ 0 ].pos.x = progress_pos.x;
		p_full_verts[ 0 ].pos.y = progress_pos.y;
		p_full_verts[ 0 ].pos.z = 0.f;

		p_full_verts[ 1 ].uv.x = progress_u;
		p_full_verts[ 1 ].uv.y = p_progress_full->m_nHeight;
		p_full_verts[ 1 ].color = 0xffffffff;
		p_full_verts[ 1 ].pos.x = progress_pos.x + progress_u;
		p_full_verts[ 1 ].pos.y = progress_pos.y + p_progress_full->m_nHeight;
		p_full_verts[ 1 ].pos.z = 0.f;

		p_empty_verts[ 0 ].uv.x = progress_u;
		p_empty_verts[ 0 ].uv.y = 0.f;
		p_empty_verts[ 0 ].color = 0xffffffff;
		p_empty_verts[ 0 ].pos.x = progress_pos.x + progress_u;
		p_empty_verts[ 0 ].pos.y = progress_pos.y;
		p_empty_verts[ 0 ].pos.z = 0.f;

		p_empty_verts[ 1 ].uv.x = p_progress_empty->m_nWidth;
		p_empty_verts[ 1 ].uv.y = p_progress_empty->m_nHeight;
		p_empty_verts[ 1 ].color = 0xffffffff;
		p_empty_verts[ 1 ].pos.x = progress_pos.x + p_progress_empty->m_nWidth;
		p_empty_verts[ 1 ].pos.y = progress_pos.y + p_progress_empty->m_nHeight;
		p_empty_verts[ 1 ].pos.z = 0.f;

		CGfx::DrawSprite( p_progress_full, p_full_verts, 2 );
		CGfx::DrawSprite( p_progress_empty, p_empty_verts, 2 );

		const V2	text_size( CFont::GetDefaultFont()->GetStringSize( s_szProgressTitle ) );

		CFont::GetDefaultFont()->Print( s_szProgressTitle, V2( progress_pos.x + ( 0.5f * ( p_progress_empty->m_nWidth - text_size.x ) ), progress_pos.y - text_size.y - 2.f ), text_color, 1.f );
	}

	//
	//	Draw version number
	//
	const V2	text_size( CFont::GetDefaultFont()->GetStringSize( VERSION_STRING ) );

	CFont::GetDefaultFont()->Print(	VERSION_STRING, V2( 0.f, CGfx::s_ScreenHeight - text_size.y ), 0x60ffffff, 1.f );

	//
	//	Display the current time and date
	//
	pspTime		time;
	CString		date;
	CString		half( "am" );
	const V2	date_time_pos( CSkinManager::GetV2( "hud", "date_time_pos", V2( 4.f, 4.f ) ) );

	sceRtcGetCurrentClockLocalTime( &time );

	if ( time.hour > 12 )
	{
		time.hour -= 12;

		half = "pm";
	}

//#if !defined( _DEBUG )
	date.Printf( "%.4d/%.2d/%.2d %.2d:%.2d%s", time.year, time.month, time.day, time.hour, time.minutes, half.GetPtr() );
//#else	// #if !defined( _DEBUG )
//	date.Printf( "%d/%d/%d %.2d:%.2d%s %d Kb free", time.year, time.month, time.day, time.hour, time.minutes, half.GetPtr(), sceKernelTotalFreeMemSize() / 1024 );
//#endif	// #if !defined( _DEBUG )

	CFont::GetDefaultFont()->Print(	date, date_time_pos, text_color, 1.f );
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::SetButton( eButton button, const CString & text )
{
	s_szButtonText[ button ] = text;
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::SetButtons( const CString & cross, const CString & circle, const CString & square, const CString & triangle )
{
	SetButton( BUTTON_CROSS, cross );
	SetButton( BUTTON_CIRCLE, circle );
	SetButton( BUTTON_SQUARE, square );
	SetButton( BUTTON_TRIANGLE, triangle );
}

//**********************************************************************************
//
//**********************************************************************************
float	CHUD::GetProgressBar()
{
	return s_fProgressBar;
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::SetProgressBar( float val )
{
	s_fProgressBar = val;

	CRenderable::Render();
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::ShowProgressBar( bool show, const CString & title )
{
	s_fProgressBar = 0.f;
	s_bShowProgressBar = show;
	s_szProgressTitle = title;

	CRenderable::Render();
}

//**********************************************************************************
//
//**********************************************************************************
void	CHUD::Show( bool show )
{
	s_bVisible = show;
}

//**********************************************************************************
//
//**********************************************************************************
bool	CHUD::IsVisible()
{
	return s_bVisible;
}

//*******************************  END OF FILE  ************************************
