// gameswf_test_ogl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003 -*- coding: utf-8;-*-

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A minimal test player app for the gameswf library.


#include "SDL.h"
#include "SDL_thread.h"

#include "gameswf.h"
#include <stdlib.h>
#include <stdio.h>
#include "base/ogl.h"
#include "base/utility.h"
#include "base/container.h"
#include "base/tu_file.h"
#include "base/tu_types.h"
#include "gameswf_xmlsocket.h"


#ifdef HAVE_LIBXML
extern int xml_fd;		// FIXME: this is the file descriptor
				// from XMLSocket::connect(). This
				// needs to be propogated up through
				// the layers properly, but first I
				// want to make sure it all works.
#endif // HAVE_LIBXML


void	print_usage()
// Brief instructions.
{
	printf(
		"gameswf_test_ogl -- a test player for the gameswf library.\n"
		"\n"
		"This program has been donated to the Public Domain.\n"
		"See http://tulrich.com/geekstuff/gameswf.html for more info.\n"
		"\n"
		"usage: gameswf_test_ogl [options] movie_file.swf\n"
		"\n"
		"Plays a SWF (Shockwave Flash) movie, using OpenGL and the\n"
		"gameswf library.\n"
		"\n"
		"options:\n"
		"\n"
		"  -h          Print this info.\n"
		"  -s <factor> Scale the movie up/down by the specified factor\n"
		"  -c          Produce a core file instead of letting SDL trap it\n"
		"  -d num      Number of milli-seconds to delay in main loop\n"
		"  -a          Turn antialiasing on/off.  (obsolete)\n"
		"  -v          Be verbose; i.e. print log messages to stdout\n"
		"  -va         Be verbose about movie Actions\n"
		"  -vp         Be verbose about parsing the movie\n"
		"  -ml <bias>  Specify the texture LOD bias (float, default is -1)\n"
		"  -p          Run full speed (no sleep) and log frame rate\n"
		"  -e          Use SDL Event thread\n"
		"  -1          Play once; exit when/if movie reaches the last frame\n"
                "  -r <0|1|2>  0 disables renderering & sound (good for batch tests)\n"
                "              1 enables rendering & sound (default setting)\n"
                "              2 enables rendering & disables sound\n"
		"  -t <sec>    Timeout and exit after the specified number of seconds\n"
		"  -b <bits>   Bit depth of output window (16 or 32, default is 16)\n"
		"\n"
		"keys:\n"
		"  CTRL-Q          Quit/Exit\n"
		"  CTRL-W          Quit/Exit\n"
		"  ESC             Quit/Exit\n"
		"  CTRL-P          Toggle Pause\n"
		"  CTRL-R          Restart the movie\n"
		"  CTRL-[ or kp-   Step back one frame\n"
		"  CTRL-] or kp+   Step forward one frame\n"
		"  CTRL-A          Toggle antialiasing (doesn't work)\n"
		"  CTRL-T          Debug.  Test the set_variable() function\n"
		"  CTRL-G          Debug.  Test the get_variable() function\n"
		"  CTRL-M          Debug.  Test the call_method() function\n"
		"  CTRL-B          Toggle background color\n"
		);
}


#define OVERSIZE	1.0f

static int runThread(void *nothing);
static int doneYet = 0;

static float	s_scale = 1.0f;
static bool	s_antialiased = false;
static int	s_bit_depth = 16;
static bool	s_verbose = false;
static bool	s_background = true;
static bool	s_measure_performance = false;
static bool	s_event_thread = false;

static void	message_log(const char* message)
// Process a log message.
{
	if (s_verbose)
	{
		fputs(message, stdout);
                fflush(stdout); // needed on osx for some reason
	}
}


static void	log_callback(bool error, const char* message)
// Error callback for handling gameswf messages.
{
	if (error)
	{
		// Log, and also print to stderr.
		message_log(message);
		fputs(message, stderr);
	}
	else
	{
		message_log(message);
	}
}


static tu_file*	file_opener(const char* url)
// Callback function.  This opens files for the gameswf library.
{
	return new tu_file(url, "rb");
}


static void	fs_callback(gameswf::movie_interface* movie, const char* command, const char* args)
// For handling notification callbacks from ActionScript.
{
	message_log("fs_callback: '");
	message_log(command);
	message_log("' '");
	message_log(args);
	message_log("'\n");
}


static void	key_event(SDLKey key, bool down)
// For forwarding SDL key events to gameswf.
{
	gameswf::key::code	c(gameswf::key::INVALID);

	if (key >= SDLK_a && key <= SDLK_z)
	{
		c = (gameswf::key::code) ((key - SDLK_a) + gameswf::key::A);
	}
	else if (key >= SDLK_F1 && key <= SDLK_F15)
	{
		c = (gameswf::key::code) ((key - SDLK_F1) + gameswf::key::F1);
	}
	else if (key >= SDLK_KP0 && key <= SDLK_KP9)
	{
		c = (gameswf::key::code) ((key - SDLK_KP0) + gameswf::key::KP_0);
	}
	else
	{
		// many keys don't correlate, so just use a look-up table.
		struct
		{
			SDLKey	sdlk;
			gameswf::key::code	gs;
		} table[] =
			{
				{ SDLK_RETURN, gameswf::key::ENTER },
				{ SDLK_ESCAPE, gameswf::key::ESCAPE },
				{ SDLK_LEFT, gameswf::key::LEFT },
				{ SDLK_UP, gameswf::key::UP },
				{ SDLK_RIGHT, gameswf::key::RIGHT },
				{ SDLK_DOWN, gameswf::key::DOWN },
				// @@ TODO fill this out some more
				{ SDLK_UNKNOWN, gameswf::key::INVALID }
			};

		for (int i = 0; table[i].sdlk != SDLK_UNKNOWN; i++)
		{
			if (key == table[i].sdlk)
			{
				c = table[i].gs;
				break;
			}
		}
	}

	if (c != gameswf::key::INVALID)
	{
		gameswf::notify_key_event(c, down);
	}
}




int	main(int argc, char *argv[])
{
	QASSERT(tu_types_validate());

	const char* infile = NULL;

	float	exit_timeout = 0;
	bool	do_render = true;
        bool    do_sound = true;
	bool	do_loop = true;
	bool	sdl_abort = true;
	int     delay = 31;
	float	tex_lod_bias;

	// -1.0 tends to look good.
	tex_lod_bias = -1.2f;
	
	for (int arg = 1; arg < argc; arg++)
	{
		if (argv[arg][0] == '-')
		{
			// Looks like an option.

			if (argv[arg][1] == 'h')
			{
				// Help.
				print_usage();
				exit(1);
			}
			if (argv[arg][1] == 'c')
			{
				sdl_abort = false;
			}
			else if (argv[arg][1] == 's')
			{
				// Scale.
				arg++;
				if (arg < argc)
				{
					s_scale = fclamp((float) atof(argv[arg]), 0.01f, 100.f);
				}
				else
				{
					fprintf(stderr, "-s arg must be followed by a scale value\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'a')
			{
				// Set antialiasing on or off.
				arg++;
				if (arg < argc)
				{
					s_antialiased = atoi(argv[arg]) ? true : false;
				}
				else
				{
					fprintf(stderr, "-a arg must be followed by 0 or 1 to disable/enable antialiasing\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'b')
			{
				// Set default bit depth.
				arg++;
				if (arg < argc)
				{
					s_bit_depth = atoi(argv[arg]);
					if (s_bit_depth != 16 && s_bit_depth != 32)
					{
						fprintf(stderr, "Command-line supplied bit depth %d, but it must be 16 or 32", s_bit_depth);
						print_usage();
						exit(1);
					}
				}
				else
				{
					fprintf(stderr, "-b arg must be followed by 16 or 32 to set bit depth\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'd')
			{
				// Set a delay
				arg++;
				if (arg < argc)
				{
					delay = atoi(argv[arg]);
				}
				else
				{
					fprintf(stderr, "-d arg must be followed by number of milli-seconds to del in the main loop\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'p')
			{
				// Enable frame-rate/performance logging.
				s_measure_performance = true;
			}
			else if (argv[arg][1] == 'e')
			{
				// Use an SDL thread to handle events
				s_event_thread = true;
			}
			else if (argv[arg][1] == '1')
			{
				// Play once; don't loop.
				do_loop = false;
			}
			else if (argv[arg][1] == 'r')
			{
				// Set rendering on/off.
				arg++;
				if (arg < argc)
				{
					const int render_arg = atoi(argv[arg]);
					switch (render_arg) {
					case 0:
						// Disable both
						do_render = false;
						do_sound = false;
						break;
					case 1:
						// Enable both
						do_render = true;
						do_sound = true;
						break;
					case 2:
						// Disable just sound
						do_render = true;
						do_sound = false;
						break;
					default:
						fprintf(stderr, "-r must be followed by 0, 1 or 2 (%d is invalid)\n",
							render_arg);
						print_usage();
						exit(1);
						break;
					}
				} else {
					fprintf(stderr, "-r must be followed by 0 an argument to disable/enable rendering\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 't')
			{
				// Set timeout.
				arg++;
				if (arg < argc)
				{
					exit_timeout = (float) atof(argv[arg]);
				}
				else
				{
					fprintf(stderr, "-t must be followed by an exit timeout, in seconds\n");
					print_usage();
					exit(1);
				}
			}
			else if (argv[arg][1] == 'v')
			{
				// Be verbose; i.e. print log messages to stdout.
				s_verbose = true;

				if (argv[arg][2] == 'a')
				{
					// Enable spew re: action.
					gameswf::set_verbose_action(true);
				}
				else if (argv[arg][2] == 'p')
				{
					// Enable parse spew.
					gameswf::set_verbose_parse(true);
				}
				// ...
			}
			else if (argv[arg][1] == 'm')
			{
				if (argv[arg][2] == 'l') {
					arg++;
					tex_lod_bias = (float) atof(argv[arg]);
					//printf("Texture LOD Bais is no %f\n", tex_lod_bias);
				}
				else
				{
					fprintf(stderr, "unknown variant of -m arg\n");
					print_usage();
					exit(1);
				}
			}
		}
		else
		{
			infile = argv[arg];
		}
	}

	if (infile == NULL)
	{
		printf("no input file\n");
		print_usage();
		exit(1);
	}

	gameswf::register_file_opener_callback(file_opener);
	gameswf::register_fscommand_callback(fs_callback);
	if (s_verbose == true) {
		gameswf::register_log_callback(log_callback);
	}
	//gameswf::set_antialiased(s_antialiased);

	gameswf::sound_handler*	sound = NULL;
	gameswf::render_handler*	render = NULL;
	if (do_render)
	{
		if (do_sound) {
			sound = gameswf::create_sound_handler_sdl();
			gameswf::set_sound_handler(sound);
		}
		render = gameswf::create_render_handler_ogl();
		gameswf::set_render_handler(render);
	}

	// Get info about the width & height of the movie.
	int	movie_version = 0;
	int	movie_width = 0;
	int	movie_height = 0;
	float	movie_fps = 30.0f;
	gameswf::get_movie_info(infile, &movie_version, &movie_width, &movie_height, &movie_fps, NULL, NULL);
	if (movie_version == 0)
	{
		fprintf(stderr, "error: can't get info about %s\n", infile);
		exit(1);
	}

	int	width = int(movie_width * s_scale);
	int	height = int(movie_height * s_scale);

	if (do_render)
	{
		// Initialize the SDL subsystems we're using. Linux
		// and Darwin use Pthreads for SDL threads, Win32
		// doesn't. Otherwise the SDL event loop just polls.
		if (sdl_abort) {
			//  Other flags are SDL_INIT_JOYSTICK | SDL_INIT_CDROM
#ifdef _WIN32
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
#else
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTTHREAD ))
#endif
				{
				fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
					exit(1);
			}
		} else {
			fprintf(stderr, "warning: SDL won't trap core dumps \n");
#ifdef _WIN32
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE  | SDL_INIT_EVENTTHREAD))
#else
			if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE))
#endif
			{
				fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
					exit(1);
			}
		}

		atexit(SDL_Quit);

		SDL_EnableKeyRepeat(250, 33);

		
		if (s_bit_depth == 16)
		{
			// 16-bit color, surface creation is likely to succeed.
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 15);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
		}
		else
		{
			QASSERT(s_bit_depth == 32);

			// 32-bit color etc, for getting dest alpha,
			// for MULTIPASS_ANTIALIASING (see
			// gameswf_render_handler_ogl.cpp).
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
			SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
		}

		// Change the LOD BIAS values to tweak blurriness.
		if (tex_lod_bias != 0.0f) {
#ifdef FIX_I810_LOD_BIAS	
			// If 2D textures weren't previously enabled, enable
			// them now and force the driver to notice the update,
			// then disable them again.
			if (!glIsEnabled(GL_TEXTURE_2D)) {
				// Clearing a mask of zero *should* have no
				// side effects, but coupled with enbling
				// GL_TEXTURE_2D it works around a segmentation
				// fault in the driver for the Intel 810 chip.
				glEnable(GL_TEXTURE_2D);
				glClear(0);
				glDisable(GL_TEXTURE_2D);
			}
#endif // FIX_I810_LOD_BIAS
			glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, tex_lod_bias);
		}
		
		// Set the video mode.
		if (SDL_SetVideoMode(width, height, s_bit_depth, SDL_OPENGL) == 0)
		{
			fprintf(stderr, "SDL_SetVideoMode() failed.");
			exit(1);
		}

		ogl::open();

		// Turn on alpha blending.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Turn on line smoothing.  Antialiased lines can be used to
		// smooth the outsides of shapes.
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);	// GL_NICEST, GL_FASTEST, GL_DONT_CARE

		glMatrixMode(GL_PROJECTION);
		glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// We don't need lighting effects
		glDisable(GL_LIGHTING);
		// glColorPointer(4, GL_UNSIGNED_BYTE, 0, *);
		// glInterleavedArrays(GL_T2F_N3F_V3F, 0, *)
		glPushAttrib (GL_ALL_ATTRIB_BITS);		
	}

	// Load the actual movie.
	gameswf::movie_definition*	md = gameswf::create_library_movie(infile);
	if (md == NULL)
	{
		fprintf(stderr, "error: can't create a movie from '%s'\n", infile);
		exit(1);
	}
	gameswf::movie_interface*	m = create_library_movie_inst(md);
	if (m == NULL)
	{
		fprintf(stderr, "error: can't create movie instance\n");
		exit(1);
	}
	gameswf::set_current_root(m);

	// Mouse state.
	int	mouse_x = 0;
	int	mouse_y = 0;
	int	mouse_buttons = 0;

	float	speed_scale = 1.0f;
	Uint32	start_ticks = 0;
	if (do_render)
	{
		start_ticks = SDL_GetTicks();
		
	}
	Uint32	last_ticks = start_ticks;
	int	frame_counter = 0;
	int	last_logged_fps = last_ticks;


	SDL_Thread *thread = NULL;
	if (s_event_thread) {
		thread = SDL_CreateThread(runThread, NULL);
		if ( thread == NULL) {
			fprintf(stderr, "Unable to create thread: %s\n", SDL_GetError());
		}
	}
	
	
	for (;;)
	{
		Uint32	ticks;
		if (do_render)
		{
			ticks = SDL_GetTicks();
		}
		else
		{
			// Simulate time.
			ticks = last_ticks + (Uint32) (1000.0f / movie_fps);
		}
		int	delta_ticks = ticks - last_ticks;
		float	delta_t = delta_ticks / 1000.f;
		last_ticks = ticks;

		// Check auto timeout counter.
		if (exit_timeout > 0
		    && ticks - start_ticks > (Uint32) (exit_timeout * 1000))
		{
			// Auto exit now.
			break;
		}
		
		bool ret = true;
		if (do_render)
		{
			SDL_Event	event;
			// Handle input.
			while (ret)
			{
#ifdef HAVE_LIBXML
				if (s_event_thread && xml_fd > 0) {
					ret = SDL_WaitEvent(&event) ? true : false;
				} else {
					ret = SDL_PollEvent(&event) ? true : false;
				}
#else
				ret = SDL_PollEvent(&event) ? true : false;
#endif

				//printf("EVENT Type is %d\n", event.type);
				switch (event.type)
				{
				case SDL_USEREVENT:
					//printf("SDL_USER_EVENT at %s, code %d%d\n", __FUNCTION__, __LINE__, event.user.code);
					ret = false;
					break;
				case SDL_KEYDOWN:
				{
					SDLKey	key = event.key.keysym.sym;
					bool	ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;

					if (key == SDLK_ESCAPE
					    || (ctrl && key == SDLK_q)
					    || (ctrl && key == SDLK_w))
					{
						goto done;
					}
					else if (ctrl && key == SDLK_p)
					{
						// Toggle paused state.
						if (m->get_play_state() == gameswf::movie_interface::STOP)
						{
							m->set_play_state(gameswf::movie_interface::PLAY);
						}
						else
						{
							m->set_play_state(gameswf::movie_interface::STOP);
						}
					}
					else if (ctrl && key == SDLK_r)
					{
						// Restart the movie.
						m->restart();
					}
					else if (ctrl && (key == SDLK_LEFTBRACKET || key == SDLK_KP_MINUS))
					{
						m->goto_frame(m->get_current_frame()-1);
					}
					else if (ctrl && (key == SDLK_RIGHTBRACKET || key == SDLK_KP_PLUS))
					{
						m->goto_frame(m->get_current_frame()+1);
					}
					else if (ctrl && key == SDLK_a)
					{
						// Toggle antialiasing.
						s_antialiased = !s_antialiased;
						//gameswf::set_antialiased(s_antialiased);
					}
					else if (ctrl && key == SDLK_t)
					{
						// test text replacement / variable setting:
						m->set_variable("test.text", "set_edit_text was here...\nanother line of text for you to see in the text box\nSome UTF-8: ñö£ç°ÄÀÔ¿");
					}
					else if (ctrl && key == SDLK_g)
					{
						// test get_variable.
						message_log("testing get_variable: '");
						message_log(m->get_variable("test.text"));
						message_log("'\n");
					}
					else if (ctrl && key == SDLK_m)
					{
						// Test call_method.
						const char* result = m->call_method(
							"test_call",
							"%d, %f, %s, %ls",
							200,
							1.0f,
							"Test string",
							L"Test long string");

						if (result)
						{
							message_log("call_method: result = ");
							message_log(result);
							message_log("\n");
						}
						else
						{
							message_log("call_method: null result\n");
						}
					}
					else if (ctrl && key == SDLK_b)
					{
						// toggle background color.
						s_background = !s_background;
					}
					else if (ctrl && key == SDLK_f)	//xxxxxx
					{
						extern bool gameswf_debug_show_paths;
						gameswf_debug_show_paths = !gameswf_debug_show_paths;
					}
					else if (ctrl && key == SDLK_EQUALS)
					{
						float	f = gameswf::get_curve_max_pixel_error();
						f *= 1.1f;
						gameswf::set_curve_max_pixel_error(f);
						printf("curve error tolerance = %f\n", f);
					}
					else if (ctrl && key == SDLK_MINUS)
					{
						float	f = gameswf::get_curve_max_pixel_error();
						f *= 0.9f;
						gameswf::set_curve_max_pixel_error(f);
						printf("curve error tolerance = %f\n", f);
					}

					key_event(key, true);

					break;
				}

				case SDL_KEYUP:
				{
					SDLKey	key = event.key.keysym.sym;
					key_event(key, false);
					break;
				}

				case SDL_MOUSEMOTION:
					mouse_x = (int) (event.motion.x / s_scale);
					mouse_y = (int) (event.motion.y / s_scale);
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				{
					int	mask = 1 << (event.button.button - 1);
					if (event.button.state == SDL_PRESSED)
					{
						mouse_buttons |= mask;
					}
					else
					{
						mouse_buttons &= ~mask;
					}
					break;
				}

				case SDL_QUIT:
					goto done;
					break;

				default:
					break;
				}
			}
		}

		m = gameswf::get_current_root();
		gameswf::delete_unused_root();

		m->set_display_viewport(0, 0, width, height);
		m->set_background_alpha(s_background ? 1.0f : 0.05f);

		m->notify_mouse_state(mouse_x, mouse_y, mouse_buttons);

		m->advance(delta_t * speed_scale);

		if (do_render)
		{
			glDisable(GL_DEPTH_TEST);	// Disable depth testing.
			glDrawBuffer(GL_BACK);
		}
		m->display();
		frame_counter++;
		
		if (do_render)
		{
			SDL_GL_SwapBuffers();
			//glPopAttrib ();

			if (s_measure_performance == false)
			{
				// Don't hog the CPU.
				SDL_Delay(delay);
			}
			else
			{
				// Log the frame rate every second or so.
				if (last_ticks - last_logged_fps > 1000)
				{
					float	delta = (last_ticks - last_logged_fps) / 1000.f;

					if (delta > 0)
					{
						printf("fps = %3.1f\n", frame_counter / delta);
					}
					else
					{
						printf("fps = *inf*\n");
					}

					last_logged_fps = last_ticks;
					frame_counter = 0;
				}
			}
		}

		// See if we should exit.
		if (do_loop == false
		    && m->get_current_frame() + 1 == md->get_frame_count())
		{
			// We're reached the end of the movie; exit.
			break;
		}
	}

done:
	doneYet = 1;
	SDL_KillThread(thread);	// kill the network read thread
	//SDL_Quit();
	
	if (md) md->drop_ref();
	if (m) m->drop_ref();
	delete sound;
	delete render;

	// For testing purposes, throw some keypresses into gameswf,
	// to make sure the key handler is properly using weak
	// references to listeners.
	gameswf::notify_key_event(gameswf::key::A, true);
	gameswf::notify_key_event(gameswf::key::B, true);
	gameswf::notify_key_event(gameswf::key::C, true);

	// Clean up gameswf as much as possible, so valgrind will help find actual leaks.
	gameswf::clear();

	return 0;
}

static int
runThread(void *nothing)
{
#ifdef HAVE_LIBXML

	//int i = 123;
	int val;
	int count = 0;
	SDL_Event *ptr;
#if 1
	SDL_Event ev;
	ev.type = SDL_USEREVENT;
	ev.user.code  = 0;
	ev.user.data1 = 0;
	ev.user.data2 = 0;
	ptr = &ev;
#else
	ptr = (SDL_Event *)ev_ptr;
	ptr->type = SDL_USEREVENT;
	ptr->user.code  = 0;
	ptr->user.data1 = 0;
	ptr->user.data2 = 0;
#endif

	printf("Initializing event thread...\n");
	
	while (!doneYet) {
		//ptr->user.data1 = (void *)i;
		// FIXME: this file descriptor is hardcoded so this
		// doesn't work under GDB right now.
		if ((val = gameswf::check_sockets(xml_fd)) == -1) {
			sleep(100);
			continue;
		}
		s_event_thread = true;
		// Don't push an event if there is already one in the
		// queue. XMLSocket::onData() will come around and get
		// the data anyway.
		count = SDL_PeepEvents(ptr, 1, SDL_PEEKEVENT, SDL_USEREVENT);
		// printf("%d User Events in queue\n", count);
		if ((count == 0) && (val >= 0)) {
			//printf("Pushing User Event on queue\n");
			SDL_PushEvent(ptr);
			SDL_Delay(300);
		}
	}
#endif // HAVE_LIBXML
	
	return 0;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
