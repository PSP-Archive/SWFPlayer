/***********************************************************************************

  Module :	gameswf_sound_handler_psp.cpp

  Description :	

  Last Modified $Date: $

  $Revision: $

  Copyright (C) 12 March 2006 71M

***********************************************************************************/

//**********************************************************************************
//   Include Files
//**********************************************************************************
#include "CTypes.h"
#include "gameswf.h"
#include <mad.h>

//**********************************************************************************
//   Local Macros
//**********************************************************************************

//**********************************************************************************
//   Local Constants
//**********************************************************************************
#define	SAMPLE_RATE				44100
#define MIX_CHANNELS			PSP_AUDIO_CHANNEL_MAX

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
class CPSPSoundHandler : public gameswf::sound_handler
{
	public:

		CPSPSoundHandler();
		~CPSPSoundHandler();

		virtual int		create_sound( void * data, int data_bytes, int sample_count, format_type format, int sample_rate, bool stereo );
		virtual void	play_sound( int sound_handle, int loop_count );
		virtual void	stop_sound( int sound_handle );
		virtual void	delete_sound( int sound_handle );

	private:

		struct Sample 
		{ 
			short	left; 
			short	right; 
		}; 

	private:

		u32				m_nBufferSize;
		u32				m_nBufferPos;
		mad_stream		m_Stream;
		mad_frame		m_Frame;
		mad_synth		m_Synth;
		u8				m_SampleBuffer[ 2048 ];
		u32				m_SamplesRead;
};

//**********************************************************************************
//	
//**********************************************************************************
gameswf::sound_handler *	gameswf::create_sound_handler_psp()
{
	return create_sound_handler_sdl();
//	return new CPSPSoundHandler();
}

//**********************************************************************************
//	
//**********************************************************************************
CPSPSoundHandler::CPSPSoundHandler()
{
	TRACE( "Setting up MAD... " );
	mad_stream_init( &m_Stream );
	mad_frame_init( &m_Frame );
	mad_synth_init( &m_Synth );
	TRACE( "MAD OK.\n" );

	pspAudioInit(); 

	TRACE( "Decoding...\n" );
//	pspAudioSetChannelCallback( 0, fillOutputBuffer, 0 );
}

//**********************************************************************************
//	
//**********************************************************************************
CPSPSoundHandler::~CPSPSoundHandler()
{
	pspAudioEnd();

	mad_synth_finish( &m_Synth );
	mad_frame_finish( &m_Frame );
	mad_stream_finish( &m_Stream );
}

//**********************************************************************************
//	
//**********************************************************************************
int	CPSPSoundHandler::create_sound( void * data, int data_bytes, int sample_count, format_type format, int sample_rate, bool stereo )
{
	return 0;
}

//**********************************************************************************
//	
//**********************************************************************************
void	CPSPSoundHandler::play_sound( int sound_handle, int loop_count )
{
}

//**********************************************************************************
//	
//**********************************************************************************
void	CPSPSoundHandler::stop_sound( int sound_handle )
{
}

//**********************************************************************************
//	
//**********************************************************************************
void	CPSPSoundHandler::delete_sound( int sound_handle )
{
}

//*******************************  END OF FILE  ************************************
