/***********************************************************************************

  Module :	CSWFFileHandler.cpp

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
#include "CSWFFileHandler.h"
#include "CRenderable.h"
#include "CHUD.h"
#include "CInput.h"
#include "CFileSystem.h"
#include "CProcess.h"
#include "CFrameWork.h"
#include "CMessageBox.h"
#include "gameswf.h"
#include "gameswf_impl.h"
#include "base/tu_file.h"
#include "CGfx.h"
#include "CFont.h"
#include "CTextureManager.h"
#include "TinyXML/tinyxml.h"
#include "CMessageBox.h"

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************
struct sKeyMap
{
	gameswf::key::code	key;
	CString				name;
};

struct sKey
{
	CString				button_name;
	CInput::eButton		button;
	gameswf::key::code	key;
};

static const sKeyMap	KEYMAPPINGS[ gameswf::key::KEYCOUNT ] =
{
	{	gameswf::key::INVALID,			"INVALID",			},
	{	gameswf::key::A,				"A",				},
	{	gameswf::key::B,				"B",				},
	{	gameswf::key::C,				"C",				},
	{	gameswf::key::D,				"D",				},
	{	gameswf::key::E,				"E",				},
	{	gameswf::key::F,				"F",				},
	{	gameswf::key::G,				"G",				},
	{	gameswf::key::H,				"H",				},
	{	gameswf::key::I,				"I",				},
	{	gameswf::key::J,				"J",				},
	{	gameswf::key::K,				"K",				},
	{	gameswf::key::L,				"L",				},
	{	gameswf::key::M,				"M",				},
	{	gameswf::key::N,				"N",				},
	{	gameswf::key::O,				"O",				},
	{	gameswf::key::P,				"P",				},
	{	gameswf::key::Q,				"Q",				},
	{	gameswf::key::R,				"R",				},
	{	gameswf::key::S,				"S",				},
	{	gameswf::key::T,				"T",				},
	{	gameswf::key::U,				"U",				},
	{	gameswf::key::V,				"V",				},
	{	gameswf::key::W,				"W",				},
	{	gameswf::key::X,				"X",				},
	{	gameswf::key::Y,				"Y",				},
	{	gameswf::key::Z,				"Z",				},
	{	gameswf::key::_0,				"_0",				},
	{	gameswf::key::_1,				"_1",				},
	{	gameswf::key::_2,				"_2",				},
	{	gameswf::key::_3,				"_3",				},
	{	gameswf::key::_4,				"_4",				},
	{	gameswf::key::_5,				"_5",				},
	{	gameswf::key::_6,				"_6",				},
	{	gameswf::key::_7,				"_7",				},
	{	gameswf::key::_8,				"_8",				},
	{	gameswf::key::_9,				"_9",				},
	{	gameswf::key::KP_0,				"KP_0",				},
	{	gameswf::key::KP_1,				"KP_1",				},
	{	gameswf::key::KP_2,				"KP_2",				},
	{	gameswf::key::KP_3,				"KP_3",				},
	{	gameswf::key::KP_4,				"KP_4",				},
	{	gameswf::key::KP_5,				"KP_5",				},
	{	gameswf::key::KP_6,				"KP_6",				},
	{	gameswf::key::KP_7,				"KP_7",				},
	{	gameswf::key::KP_8,				"KP_8",				},
	{	gameswf::key::KP_9,				"KP_9",				},
	{	gameswf::key::KP_MULTIPLY,		"KP_MULTIPLY",		},
	{	gameswf::key::KP_ADD,			"KP_ADD",			},
	{	gameswf::key::KP_ENTER,			"KP_ENTER",			},
	{	gameswf::key::KP_SUBTRACT,		"KP_SUBTRACT",		},
	{	gameswf::key::KP_DECIMAL,		"KP_DECIMAL",		},
	{	gameswf::key::KP_DIVIDE,		"KP_DIVIDE",		},
	{	gameswf::key::F1,				"F1",				},
	{	gameswf::key::F2,				"F2",				},
	{	gameswf::key::F3,				"F3",				},
	{	gameswf::key::F4,				"F4",				},
	{	gameswf::key::F5,				"F5",				},
	{	gameswf::key::F6,				"F6",				},
	{	gameswf::key::F7,				"F7",				},
	{	gameswf::key::F8,				"F8",				},
	{	gameswf::key::F9,				"F9",				},
	{	gameswf::key::F10,				"F10",				},
	{	gameswf::key::F11,				"F11",				},
	{	gameswf::key::F12,				"F12",				},
	{	gameswf::key::F13,				"F13",				},
	{	gameswf::key::F14,				"F14",				},
	{	gameswf::key::F15,				"F15",				},
	{	gameswf::key::BACKSPACE,		"BACKSPACE",		},
	{	gameswf::key::TAB,				"TAB",				},
	{	gameswf::key::CLEAR,			"CLEAR",			},
	{	gameswf::key::ENTER,			"ENTER",			},
	{	gameswf::key::SHIFT,			"SHIFT",			},
	{	gameswf::key::CONTROL,			"CONTROL",			},
	{	gameswf::key::ALT,				"ALT",				},
	{	gameswf::key::CAPSLOCK,			"CAPSLOCK",			},
	{	gameswf::key::ESCAPE,			"ESCAPE",			},
	{	gameswf::key::SPACE,			"SPACE",			},
	{	gameswf::key::PGDN,				"PGDN",				},
	{	gameswf::key::PGUP,				"PGUP",				},
	{	gameswf::key::END,				"END",				},
	{	gameswf::key::HOME,				"HOME",				},
	{	gameswf::key::LEFT,				"LEFT",				},
	{	gameswf::key::UP,				"UP",				},
	{	gameswf::key::RIGHT,			"RIGHT",			},
	{	gameswf::key::DOWN,				"DOWN",				},
	{	gameswf::key::INSERT,			"INSERT",			},
	{	gameswf::key::DELETEKEY,		"DELETEKEY",		},
	{	gameswf::key::HELP,				"HELP",				},
	{	gameswf::key::NUM_LOCK,			"NUM_LOCK",			},
	{	gameswf::key::SEMICOLON,		"SEMICOLON",		},
	{	gameswf::key::EQUALS,			"EQUALS",			},
	{	gameswf::key::MINUS,			"MINUS",			},
	{	gameswf::key::SLASH,			"SLASH",			},
	{	gameswf::key::BACKTICK,			"BACKTICK",			},
	{	gameswf::key::LEFT_BRACKET,		"LEFT_BRACKET",		},
	{	gameswf::key::BACKSLASH,		"BACKSLASH",		},
	{	gameswf::key::RIGHT_BRACKET,	"RIGHT_BRACKET",	},
	{	gameswf::key::QUOTE,			"QUOTE",			},
};

//**********************************************************************************
//   Static Prototypes
//**********************************************************************************
static float	s_SWFScale = 1.0f;
static bool		s_verbose = true;
static bool		s_background = false;//true;
static float	MOUSE_SPEED = 200.f;
static bool		s_bKeyDown[ CInput::MAX_BUTTONS ];
static sKey		s_Keys[ CInput::MAX_BUTTONS ] =
{
	{	"SELECT",	CInput::SELECT,		gameswf::key::INVALID	},
	{	"START",	CInput::START,		gameswf::key::INVALID	},
	{	"UP",		CInput::UP,			gameswf::key::INVALID	},
	{	"RIGHT",	CInput::RIGHT,		gameswf::key::INVALID	},
	{	"DOWN",		CInput::DOWN,		gameswf::key::INVALID	},
	{	"LEFT",		CInput::LEFT,		gameswf::key::INVALID	},
	{	"LTRIGGER",	CInput::LTRIGGER,	gameswf::key::INVALID	},
	{	"RTRIGGER",	CInput::RTRIGGER,	gameswf::key::INVALID	},
	{	"TRIANGLE",	CInput::TRIANGLE,	gameswf::key::INVALID	},
	{	"CIRCLE",	CInput::CIRCLE,		gameswf::key::INVALID	},
	{	"CROSS",	CInput::CROSS,		gameswf::key::INVALID	},
	{	"SQUARE",	CInput::SQUARE,		gameswf::key::INVALID	},
	{	"HOME",		CInput::HOME,		gameswf::key::INVALID	},
	{	"HOLD",		CInput::HOLD,		gameswf::key::INVALID	},
	{	"NOTE",		CInput::NOTE,		gameswf::key::INVALID	},
};

//**********************************************************************************
//   Global Variables
//**********************************************************************************
float	swf_x_offset = 0;
float	mouse_x = 0;
float	mouse_y = 0;
int		mouse_buttons = 0;
s32		movie_version = 0;

//**********************************************************************************
//   Static Variables
//**********************************************************************************
static const sFileExtensionInfo	s_FileExtensionInfo =
{
	"View",
	CSkinManager::SC_ICON_MOVIE
};

//**********************************************************************************
//   Class Definition
//**********************************************************************************

//**********************************************************************************
//	Process a log message
//**********************************************************************************
static void	MessageLog( const char * const p_message )
{
	if ( s_verbose == true )
	{
		TRACE( p_message );
	}
}

//**********************************************************************************
//	Error callback for handling gameswf messages.
//**********************************************************************************
static void	LogCallback( bool error, const char * const p_message )
{
	if ( s_verbose == true )
	{
		if ( error == false )
		{
			TRACE( p_message );
		}
		else
		{
			BREAK_POINT( p_message );

//			CString	message( p_message );
//			CErrorMessage	error( message );
		}
	}
}

//**********************************************************************************
//	Callback function.  This opens files for the gameswf library.
//**********************************************************************************
static tu_file *	FileOpener( const char * const p_url )
{
	return new tu_file( p_url, "rb" );
}

//**********************************************************************************
//	For handling notification callbacks from ActionScript.
//**********************************************************************************
static void	FSCallback( gameswf::movie_interface * movie, const char * command, const char * args )
{
	MessageLog( "fs_callback: '" );
	MessageLog( command );
	MessageLog( "' '" );
	MessageLog( args );
	MessageLog( "'\n" );
}

//**********************************************************************************
//	
//**********************************************************************************
void	ProgressCallback( unsigned int loaded_bytes, unsigned int total_bytes )
{
	const float	progress( static_cast< float >( loaded_bytes ) / static_cast< float >( total_bytes ) );

	CHUD::SetProgressBar( progress );

	scePowerTick( 0 );
}

//**********************************************************************************
//	
//**********************************************************************************
static void	ParseKeyConfig()
{
	TiXmlDocument *	p_config( new TiXmlDocument() );

	ASSERT( p_config != NULL, "Failed to create the XML document" );

	if ( p_config != NULL )
	{
		if ( p_config->LoadFile( "Data/Keys.xml" ) == true )
		{
			for ( u32 i = 0; i < CInput::MAX_BUTTONS; ++i )
			{
				TiXmlHandle		doc_handle( p_config );
				TiXmlElement *	p_key( doc_handle.FirstChild( "keys" ).FirstChild( s_Keys[ i ].button_name ).Element() );

				if ( p_key != NULL )
				{
					if ( p_key->Attribute( "val" ) != NULL )
					{
						const CString	key_name( p_key->Attribute( "val" ) );

						for ( u32 k = 0; k < gameswf::key::KEYCOUNT; ++k )
						{
							if ( KEYMAPPINGS[ k ].name == key_name )
							{
								s_Keys[ i ].key = KEYMAPPINGS[ k ].key;
							}
						}
					}
				}
			}
		}

		SAFE_DELETE( p_config );
	}

	for ( u32 i = 0; i < CInput::MAX_BUTTONS; ++i )
	{
		s_bKeyDown[ i ] = false;
	}
}

//**********************************************************************************
//
//**********************************************************************************
bool	CSWFFileHandler::Execute( const CString & file, bool kiosk_mode )
{
	ParseKeyConfig();

	CHUD::ShowProgressBar( true, "Loading..." );

	gameswf::register_file_opener_callback( FileOpener );
	gameswf::register_fscommand_callback( FSCallback );
	gameswf::register_log_callback( LogCallback );

	gameswf::sound_handler *	p_sound( gameswf::create_sound_handler_sdl() );
	gameswf::render_handler *	p_render( gameswf::create_render_handler_psp() );

	gameswf::set_sound_handler( p_sound );
	gameswf::set_render_handler( p_render );

	gameswf::register_progress_callback( ProgressCallback );


	// Get info about the width & height of the movie.
	float	movie_fps( 30.f );
	int		movie_width( 0 ), movie_height( 0 );

	gameswf::get_movie_info( file, &movie_version, &movie_width, &movie_height, &movie_fps, NULL, NULL);

//	BREAK_POINT( "movie_version = %d", movie_version );

	if ( movie_version == 0 )
	{
		CErrorMessage( CString().Printf( "Can't get movie info from '%s'", file.GetPtr() ) );

		CHUD::ShowProgressBar( false, "" );

		return false;
	}

	const float	div_width( static_cast< float >( movie_width ) / CGfx::s_ScreenWidth );
	const float	div_height( static_cast< float >( movie_height ) / CGfx::s_ScreenHeight );

	if ( div_width > div_height )
	{
		s_SWFScale = 1.f / div_width;
	}
	else
	{
		s_SWFScale = 1.f / div_height;
	}

	const int	width = static_cast< int >( static_cast< float >( movie_width ) * s_SWFScale );
	const int	height = static_cast< int >( static_cast< float >( movie_height ) * s_SWFScale );

	// Load the actual movie.
	gameswf::movie_definition * const	p_movie_definition( gameswf::create_library_movie( file ) );

	if ( p_movie_definition == NULL )
	{
		CErrorMessage( CString().Printf( "Can't create a movie from '%s'", file.GetPtr() ) );

		CHUD::ShowProgressBar( false, "" );

		return false;
	}

	gameswf::movie_interface *	p_movie_instance( create_library_movie_inst( p_movie_definition ) );

	if ( p_movie_instance == NULL )
	{
		CErrorMessage( "Can't create movie instance" );

		CHUD::ShowProgressBar( false, "" );

		return false;
	}

	// Mouse state.
	bool	exit = false;
	bool	paused = false;
	float	speed_scale = 1.0f;
	u32		exit_count( 0 );
	u64		last_ticks;
	bool	first_pass( true );

	sceRtcGetCurrentTick( &last_ticks );

	gameswf::set_current_root( p_movie_instance );

	swf_x_offset = ( CGfx::s_ScreenWidth - width ) * 0.5f;

	mouse_x = 0.f;
	mouse_y = 0.f;

	BREAK_POINT( "**** UPDATE STATE ****" );

	CHUD::ShowProgressBar( false, "" );

	while ( exit == false )
	{
		u64	ticks;

		scePowerTick( 0 );
		sceRtcGetCurrentTick( &ticks );

		const int	delta_ticks( ticks - last_ticks );
		const float	delta_t( paused == true ? 0.f : ( ((float)delta_ticks) / 1000.f ) / 1000.f );

		last_ticks = ticks;

		//
		//	Update the mouse input
		//
		if ( CInput::IsButtonDown( CInput::CROSS ) == true || first_pass == true )
		{
			first_pass = false;
			mouse_buttons = -1;
		}
		else
		{
			mouse_buttons = 0;
		}

		if ( CInput::GetAnalogStick().LengthSq() > SQUARE( 0.3f ) )
		{
			mouse_x += MOUSE_SPEED * delta_t * CInput::GetAnalogStick().x;
			mouse_y += MOUSE_SPEED * delta_t * CInput::GetAnalogStick().y;

			if ( mouse_x < 0.f )	mouse_x = 0.f;
			if ( mouse_x > width )	mouse_x = width;
			if ( mouse_y < 0.f )	mouse_y = 0.f;
			if ( mouse_y > height )	mouse_y = height;
		}

		//
		//	Update the keyboard input
		//
		for ( u32 k = 0; k < CInput::MAX_BUTTONS; ++k )
		{
			if ( s_Keys[ k ].key != gameswf::key::INVALID )
			{
				if ( CInput::IsButtonDown( s_Keys[ k ].button ) == true )
				{
					s_bKeyDown[ k ] = true;

					gameswf::notify_key_event( s_Keys[ k ].key, true );
				}
				else
				{
					if ( s_bKeyDown[ k ] == true )
					{
						s_bKeyDown[ k ] = false;

						gameswf::notify_key_event( s_Keys[ k ].key, false );
					}
				}
			}
		}


		p_movie_instance = gameswf::get_current_root();
		gameswf::delete_unused_root();
		p_movie_instance->set_display_viewport( 0, 0, width, height );
		p_movie_instance->set_background_alpha( s_background ? 1.0f : 0.05f );
		p_movie_instance->notify_mouse_state( (int)( mouse_x / s_SWFScale ), (int)( mouse_y / s_SWFScale ), mouse_buttons );
		LOG_LINE;
		p_movie_instance->advance( delta_t * speed_scale );
		p_movie_instance->display();

		//
		//	Draw mouse
		//
		CTexture * const	p_mouse( CSkinManager::GetComponent( CSkinManager::SC_CURSOR )->GetTexture() );

		if ( p_mouse != NULL )
		{
			CGfx::DrawQuad( p_mouse, V2( swf_x_offset + static_cast< s32 >( mouse_x ), static_cast< s32 >( mouse_y ) ), V2( p_mouse->m_nWidth, p_mouse->m_nHeight ), mouse_buttons != 0 ? ARGB( 128, 80, 80 ) : ARGB( 255, 255, 255 ) );
		}

		//
		//	Draw borders around animation
		//
		CGfx::DrawQuad( V2( 0.f, 0.f ), V2( swf_x_offset, CGfx::s_ScreenHeight ), ARGB( 255, 0, 0, 0 ) );
		CGfx::DrawQuad( V2( CGfx::s_ScreenWidth - swf_x_offset, 0.f ), V2( swf_x_offset, CGfx::s_ScreenHeight ), ARGB( 255, 0, 0, 0 ) );

//		CString	fps;
//		fps.Printf( "FPS: %f", delta_t * 1000.f );
//		CFont::GetDefaultFont()->Print( fps, V2( 0.f, 0.f ), ARGB( 255, 255, 255 ) );

		CInput::Process();

		if ( kiosk_mode == false )
		{
			if ( CInput::IsButtonClicked( CInput::START ) == true )
			{
				exit = true;
			}
		}
		else
		{
			if ( CInput::IsButtonDown( CInput::RTRIGGER ) == true && CInput::IsButtonDown( CInput::LTRIGGER ) == true )
			{
				if ( CInput::IsButtonClicked( CInput::START ) == true )
				{
					++exit_count;

					if ( exit_count > 32 )
					{
						exit = true;
					}
				}
			}
		}

		CGfx::EndRender();
		CGfx::SwapBuffers();
	}

	if ( p_movie_definition != NULL )
	{
		p_movie_definition->drop_ref();
	}

	if ( p_movie_instance != NULL )
	{
		p_movie_instance->drop_ref();
	}

	delete p_sound;
	delete p_render;

	// Clean up gameswf as much as possible, so valgrind will help find actual leaks.
	gameswf::clear();

	return true;
}

//**********************************************************************************
//
//**********************************************************************************
const sFileExtensionInfo &	CSWFFileHandler::Information( const CString & file )
{
	return s_FileExtensionInfo;
}

//**********************************************************************************
//
//**********************************************************************************
float	CSWFFileHandler::GetScale()
{
	static float	SCALE( 0.05f );

	return s_SWFScale * SCALE;
}

//*******************************  END OF FILE  ************************************
