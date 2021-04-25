/***********************************************************************************

  Module :	CRenderable.cpp

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
#include "CRenderable.h"
#include "CGfx.h"

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
CRenderCallbackList	CRenderable::s_RenderList[ MAX_RENDER_ORDER ];

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************

//**********************************************************************************
//
//**********************************************************************************
void	CRenderable::Open()
{
}

//**********************************************************************************
//
//**********************************************************************************
void	CRenderable::Close()
{
	for ( u32 i = 0; i < MAX_RENDER_ORDER; ++i )
	{
		s_RenderList[ i ].clear();
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	CRenderable::Render()
{
	CGfx::BeginRender();
	sceGuOffset( 2048 - ( CGfx::s_ScreenWidth / 2 ), 2048 - ( CGfx::s_ScreenHeight / 2 ) );
	sceGuViewport( 2048, 2048, CGfx::s_ScreenWidth, CGfx::s_ScreenHeight );
	sceGuScissor( 0, 0, CGfx::s_ScreenWidth, CGfx::s_ScreenHeight );
	CGfx::EndRender();

	CGfx::BeginRender();
	CGfx::ClearScreen( 0x00000000 );

	for ( u32 i = 0; i < MAX_RENDER_ORDER; ++i )
	{
		for ( CRenderCallbackList::iterator it = s_RenderList[ i ].begin(); it != s_RenderList[ i ].end(); ++it )
		{
			( *it )();
		}
	}

	CGfx::EndRender();
	CGfx::SwapBuffers();
}

//**********************************************************************************
//
//**********************************************************************************
void	CRenderable::Register( eRenderOrder order, RenderCallback callback )
{
	s_RenderList[ order ].remove( callback );
	s_RenderList[ order ].push_back( callback );
}

//**********************************************************************************
//
//**********************************************************************************
void	CRenderable::UnRegister( eRenderOrder order, RenderCallback callback )
{
	s_RenderList[ order ].remove( callback );
}

//*******************************  END OF FILE  ************************************
