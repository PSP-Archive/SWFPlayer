// gameswf_sound_handler_sdl.cpp	-- Thatcher Ulrich http://tulrich.com 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A gameswf::sound_handler that uses SDL_mixer for output


#include "gameswf/gameswf.h"
#include "base/container.h"
#include "SDL_mixer.h"
#include "gameswf/gameswf_log.h"
#include "gameswf/gameswf_types.h"	// for IF_VERBOSE_* macros


// Use SDL_mixer to handle gameswf sounds.
struct SDL_sound_handler : gameswf::sound_handler
{
	bool					m_opened;
	bool					m_stereo;
	int						m_sample_rate;
	Uint16					m_format;
	array< Mix_Chunk * >	m_samples;
	int						m_nTotalMemUsed;

	#define	SAMPLE_RATE 11025
//	#define	SAMPLE_RATE 22050
//	#define	SAMPLE_RATE 44100
	#define MIX_CHANNELS 8
	#define CHANNELS 2		//stereo - 2, mono - 1
	#define BUFSIZE 4096		// for 44100 bufsize 1024 is small

	SDL_sound_handler()
		:
		m_opened(false),
		m_stereo(false),
		m_sample_rate(0),
		m_format(0),
		m_nTotalMemUsed(0)
	{
		// !!! some drivers on Linux always open audio with channels=2
		if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16SYS, CHANNELS, BUFSIZE) != 0)
		{
			gameswf::log_error("can't open SDL_mixer: %s\n", Mix_GetError());
		}
		else
		{
			m_opened = true;     
			Mix_AllocateChannels(MIX_CHANNELS);
			Mix_Volume(-1, MIX_MAX_VOLUME);

			// get and print the audio format in use
			int channels;
			int num_times_opened = Mix_QuerySpec(&m_sample_rate, &m_format, &channels);
			UNUSED(num_times_opened);
			m_stereo = channels == 2 ? true : false;
		}
	}

	~SDL_sound_handler()
	{
		if (m_opened)
		{
			Mix_CloseAudio();
			for (int i = 0, n = m_samples.size(); i < n; i++)
			{
				if (m_samples[i])
				{
					Mix_FreeChunk(m_samples[i]);
				}
			}
		}
		else
		{
			QASSERT(m_samples.size() == 0);
		}
	}


	virtual int	create_sound(
		void* data,
		int data_bytes,
		int sample_count,
		format_type format,
		int sample_rate,
		bool stereo)
	// Called by gameswf to create a sample.  We'll return a sample ID that gameswf
	// can use for playing it.
	{
		if (m_opened == false)
		{
			return -1;
		}

		if ( m_nTotalMemUsed > ( 3 * 1024 * 1024 ) )
		{
			return -1;
		}

		Sint16*	adjusted_data = 0;
		int	adjusted_size = 0;
		Mix_Chunk*	sample = 0;

		TRACE( "create_sound %d\n", format );

		switch (format)
		{
		case FORMAT_RAW:
			convert_raw_data(&adjusted_data, &adjusted_size, data, sample_count, 1, sample_rate, stereo);
			break;

		case FORMAT_NATIVE16:
			convert_raw_data(&adjusted_data, &adjusted_size, data, sample_count, 2, sample_rate, stereo);
			break;

		case FORMAT_MP3:
#ifdef GAMESWF_MP3_SUPPORT
			extern void convert_mp3_data(Sint16 **adjusted_data, int *adjusted_size, void *data, const int sample_count, const int sample_size, const int sample_rate, const bool stereo);
			if (1) {
				Sint16*	x_adjusted_data = 0;
				int	x_adjusted_size = 0;
				convert_mp3_data(&x_adjusted_data, &x_adjusted_size, data, sample_count, 0, sample_rate, stereo);
				// convert_mp3_data doesn't ACTUALLY convert samplerate, so...
				convert_raw_data(&adjusted_data, &adjusted_size, x_adjusted_data, sample_count, 0, sample_rate, stereo);
				if (x_adjusted_data) {
					delete x_adjusted_data;
				}
			} else {
				convert_mp3_data(&adjusted_data, &adjusted_size, data, sample_count, 0, sample_rate, stereo);
			}
#else
			IF_VERBOSE_DEBUG(gameswf::log_error("mp3 format sound requested; this demo does not handle mp3\n"));
#endif
			break;

		default:
			// Unhandled format.
			IF_VERBOSE_DEBUG(gameswf::log_error("unknown format sound requested; this demo does not handle it\n"));
			return -1;
		}

		m_nTotalMemUsed += adjusted_size;

		if (adjusted_data)
		{
			sample = Mix_QuickLoad_RAW((unsigned char*) adjusted_data, adjusted_size);
			Mix_VolumeChunk(sample, MIX_MAX_VOLUME);	// full volume by default
		}

		m_samples.push_back(sample);
		return m_samples.size() - 1;
	}


	virtual void	play_sound(int sound_handle, int loop_count /* other params */)
	// Play the index'd sample.
	{
		if (sound_handle >= 0 && sound_handle < m_samples.size())
		{
			if (m_samples[sound_handle])
			{
				// Play this sample on the first available channel.
				Mix_PlayChannel(-1, m_samples[sound_handle], loop_count);
			}
		}
	}

	
	virtual void	stop_sound(int sound_handle)
	{
		if (sound_handle < 0 || sound_handle >= m_samples.size())
		{
			// Invalid handle.
			return;
		}

		for (int i = 0; i < MIX_CHANNELS; i++)
		{
			Mix_Chunk*	playing_chunk = Mix_GetChunk(i);
			if (Mix_Playing(i)
			    && playing_chunk == m_samples[sound_handle])
			{
				// Stop this channel.
				Mix_HaltChannel(i);
			}
		}
	}


	virtual void	delete_sound(int sound_handle)
	// gameswf calls this when it's done with a sample.
	{
		if (sound_handle >= 0 && sound_handle < m_samples.size())
		{
			Mix_Chunk*	chunk = m_samples[sound_handle];
			if (chunk)
			{
				delete [] (chunk->abuf);
				Mix_FreeChunk(chunk);
				m_samples[sound_handle] = 0;
			}
		}
	}

	virtual void convert_raw_data(
		Sint16** adjusted_data,
		int* adjusted_size,
		void* data,
		int sample_count,
		int sample_size,
		int sample_rate,
		bool stereo)
	// VERY crude sample-rate & sample-size conversion.  Converts
	// input data to the SDL_mixer output format (SAMPLE_RATE,
	// stereo, 16-bit native endianness)
	{
		// simple hack to handle dup'ing mono to stereo
		if ( !stereo && m_stereo)
		{
			sample_rate >>= 1;
		}

		 // simple hack to lose half the samples to get mono from stereo
		if ( stereo && !m_stereo)
		{
			sample_rate <<= 1; 
		}

		// Brain-dead sample-rate conversion: duplicate or
		// skip input samples an integral number of times.
		int	inc = 1;	// increment
		int	dup = 1;	// duplicate
		if (sample_rate > m_sample_rate)
		{
			inc = sample_rate / m_sample_rate;
		}
		else if (sample_rate < m_sample_rate)
		{
			dup = m_sample_rate / sample_rate;
		}

//		BREAK_POINT( "inc = %d, dup = %d", inc, dup );

		int	output_sample_count = (sample_count * dup) / inc;
//		BREAK_POINT( "output_sample_count = %d", output_sample_count );
		Sint16*	out_data = new Sint16[output_sample_count];
		ASSERT( out_data != NULL, "Out of memory: convert_raw_data" );

		if ( out_data != NULL )
		{
			*adjusted_data = out_data;
			*adjusted_size = output_sample_count * 2;	// 2 bytes per sample

			if (sample_size == 1)
			{
				// Expand from 8 bit to 16 bit.
				Uint8*	in = (Uint8*) data;
				for (int i = 0; i < output_sample_count; i++)
				{
					Uint8	val = *in;
					for (int j = 0; j < dup; j++)
					{
						*out_data++ = (int(val) - 128);
					}
					in += inc;
				}
			}
			else
			{
				// 16-bit to 16-bit conversion.
				Sint16*	in = (Sint16*) data;
				for (int i = 0; i < output_sample_count; i += dup)
				{
					Sint16	val = *in;
					for (int j = 0; j < dup; j++)
					{
						*out_data++ = val;
					}
					in += inc;
				}
			}
		}
	}

};


gameswf::sound_handler*	gameswf::create_sound_handler_sdl()
// Factory.
{
	return new SDL_sound_handler;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
