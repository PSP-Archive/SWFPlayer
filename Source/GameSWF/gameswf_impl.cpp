// gameswf_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation for SWF player.

// Useful links:
//
// http://sswf.sourceforge.net/SWFalexref.html
// http://www.openswf.org

// @@ Need to break this file into pieces


#include "base/tu_file.h"
#include "base/utility.h"
#include "gameswf_action.h"
#include "gameswf_button.h"
#include "gameswf_impl.h"
#include "gameswf_font.h"
#include "gameswf_fontlib.h"
#include "gameswf_log.h"
#include "gameswf_morph2.h"
#include "gameswf_render.h"
#include "gameswf_shape.h"
#include "gameswf_stream.h"
#include "gameswf_styles.h"
#include "gameswf_dlist.h"
#include "gameswf_timers.h"
#include "gameswf_sprite.h"
#include "gameswf_movie_root.h"
#include "gameswf_movie_def_impl.h"
#include "base/image.h"
#include "base/jpeg.h"
#include "base/zlib_adapter.h"
#include "gameswf_videostream.h"
#include <string.h>	// for memset
#include <typeinfo>
#include <float.h>


#if TU_CONFIG_LINK_TO_ZLIB
#include <zlib.h>
#endif // TU_CONFIG_LINK_TO_ZLIB


namespace gameswf
{
	bool	s_verbose_action = true;
	bool	s_verbose_parse = true;

#ifdef _DEBUG
	bool	s_verbose_debug = true;
#else
	bool	s_verbose_debug = false;
#endif


	void	set_verbose_action(bool verbose)
	// Enable/disable log messages re: actions.
	{
		LOG_LINE;
		s_verbose_action = verbose;
	}


	void	set_verbose_parse(bool verbose)
	// Enable/disable log messages re: parsing the movie.
	{
		LOG_LINE;
		s_verbose_parse = verbose;
	}


	static bool	s_use_cache_files = false;

	void	set_use_cache_files(bool use_cache)
	// Enable/disable attempts to read cache files when loading
	// movies.
	{
		LOG_LINE;
		s_use_cache_files = use_cache;
	}


	// Keep a table of loader functions for the different tag types.
	hash<int, loader_function>	s_tag_loaders;

	void	register_tag_loader(int tag_type, loader_function lf)
	// Associate the specified tag type with the given tag loader
	// function.
	{
		LOG_LINE;
		QASSERT(s_tag_loaders.get(tag_type, NULL) == false);
		QASSERT(lf != NULL);

		LOG_LINE;
		s_tag_loaders.add(tag_type, lf);
	}


	//
	// file_opener callback stuff
	//

	static file_opener_callback	s_opener_function = NULL;

	void	register_file_opener_callback(file_opener_callback opener)
	// Host calls this to register a function for opening files,
	// for loading movies.
	{
		LOG_LINE;
		s_opener_function = opener;
	}

	//
	// some utility stuff
	//


	character*	character_def::create_character_instance(movie* parent, int id)
	// Default.  Make a generic_character.
	{
		LOG_LINE;
		return new generic_character(this, parent, id);
	}


	//
	// ref_counted
	//


	ref_counted::ref_counted()
		:
		m_ref_count(0),
		m_weak_proxy(0)
	{
		LOG_LINE;
	}

	ref_counted::~ref_counted()
	{
		LOG_LINE;
		QASSERT(m_ref_count == 0);

		if (m_weak_proxy)
		{
			LOG_LINE;
			m_weak_proxy->notify_object_died();
			LOG_LINE;
			m_weak_proxy->drop_ref();
		}
	}

	void	ref_counted::add_ref() const
	{
		LOG_LINE;
		QASSERT(m_ref_count >= 0);
		LOG_LINE;
		m_ref_count++;
	}

	void	ref_counted::drop_ref() const
	{
		LOG_LINE;
		QASSERT(m_ref_count > 0);
		LOG_LINE;
		m_ref_count--;
		LOG_LINE;
		if (m_ref_count <= 0)
		{
			// Delete me!
			LOG_LINE;
			delete this;
		}
	}

	weak_proxy* ref_counted::get_weak_proxy() const
	{
		LOG_LINE;
		QASSERT(m_ref_count > 0);	// By rights, somebody should be holding a ref to us.

		LOG_LINE;
		if (m_weak_proxy == NULL)
		{
			LOG_LINE;
			m_weak_proxy = new weak_proxy;
			LOG_LINE;
			m_weak_proxy->add_ref();
		}

		LOG_LINE;
		return m_weak_proxy;
	}


	//
	// character
	//


	void	character::do_mouse_drag()
	// Implement mouse-dragging for this movie.
	{
		LOG_LINE;
		drag_state	st;
		LOG_LINE;
		get_drag_state(&st);
		LOG_LINE;
		if (this == st.m_character)
		{
			// We're being dragged!
			LOG_LINE;
			int	x, y, buttons;
			LOG_LINE;
			get_root_movie()->get_mouse_state(&x, &y, &buttons);

			LOG_LINE;
			point	world_mouse(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
			LOG_LINE;
			if (st.m_bound)
			{
				// Clamp mouse coords within a defined rect.
				LOG_LINE;
				world_mouse.m_x =
					fclamp(world_mouse.m_x, st.m_bound_x0, st.m_bound_x1);
				LOG_LINE;
				world_mouse.m_y =
					fclamp(world_mouse.m_y, st.m_bound_y0, st.m_bound_y1);
			}

			LOG_LINE;
			if (st.m_lock_center)
			{
				LOG_LINE;
				matrix	world_mat = get_world_matrix();
				LOG_LINE;
				point	local_mouse;
				LOG_LINE;
				world_mat.transform_by_inverse(&local_mouse, world_mouse);

				LOG_LINE;
				matrix	parent_world_mat;
				LOG_LINE;
				if (m_parent)
				{
					LOG_LINE;
					parent_world_mat = m_parent->get_world_matrix();
				}

				LOG_LINE;
				point	parent_mouse;
				LOG_LINE;
				parent_world_mat.transform_by_inverse(&parent_mouse, world_mouse);
					
				// Place our origin so that it coincides with the mouse coords
				// in our parent frame.
				LOG_LINE;
				matrix	local = get_matrix();
				LOG_LINE;
				local.m_[0][2] = parent_mouse.m_x;
				LOG_LINE;
				local.m_[1][2] = parent_mouse.m_y;
				LOG_LINE;
				set_matrix(local);
			}
			else
			{
				// Implement relative drag...
			}
		}
	}


  

	static void	ensure_loaders_registered()
	{
		static bool	s_registered = false;
	
		LOG_LINE;
		if (s_registered == false)
		{
			// Register the standard loaders.
			s_registered = true;
			LOG_LINE;
			register_tag_loader(0, end_loader);
			LOG_LINE;
			register_tag_loader(2, define_shape_loader);
			LOG_LINE;
			register_tag_loader(4, place_object_2_loader);
			LOG_LINE;
			register_tag_loader(5, remove_object_2_loader);
			LOG_LINE;
			register_tag_loader(6, define_bits_jpeg_loader);
			LOG_LINE;
			register_tag_loader(7, button_character_loader);
			LOG_LINE;
			register_tag_loader(8, jpeg_tables_loader);
			LOG_LINE;
			register_tag_loader(9, set_background_color_loader);
			LOG_LINE;
			register_tag_loader(10, define_font_loader);
			LOG_LINE;
			register_tag_loader(11, define_text_loader);
			LOG_LINE;
			register_tag_loader(12, do_action_loader);
			LOG_LINE;
			register_tag_loader(13, define_font_info_loader);
			LOG_LINE;
			register_tag_loader(14, define_sound_loader);
			LOG_LINE;
			register_tag_loader(15, start_sound_loader);
			LOG_LINE;
			register_tag_loader(17, button_sound_loader);
			LOG_LINE;
			register_tag_loader(18, null_loader);	// NOT SUPPORTED YET
			LOG_LINE;
			register_tag_loader(19, null_loader);	// NOT SUPPORTED YET
			LOG_LINE;
			register_tag_loader(20, define_bits_lossless_2_loader);
			LOG_LINE;
			register_tag_loader(21, define_bits_jpeg2_loader);
			LOG_LINE;
			register_tag_loader(22, define_shape_loader);
			LOG_LINE;
			register_tag_loader(24, null_loader);	// "protect" tag; we're not an authoring tool so we don't care.
			LOG_LINE;
			register_tag_loader(26, place_object_2_loader);
			LOG_LINE;
			register_tag_loader(28, remove_object_2_loader);
			LOG_LINE;
			register_tag_loader(32, define_shape_loader);
			LOG_LINE;
			register_tag_loader(33, define_text_loader);
			LOG_LINE;
			register_tag_loader(37, define_edit_text_loader);
			LOG_LINE;
			register_tag_loader(34, button_character_loader);
			LOG_LINE;
			register_tag_loader(35, define_bits_jpeg3_loader);
			LOG_LINE;
			register_tag_loader(36, define_bits_lossless_2_loader);
			LOG_LINE;
			register_tag_loader(39, sprite_loader);
			LOG_LINE;
			register_tag_loader(43, frame_label_loader);
			LOG_LINE;
			register_tag_loader(46, define_shape_morph_loader);
			LOG_LINE;
			register_tag_loader(48, define_font_loader);
			LOG_LINE;
			register_tag_loader(56, export_loader);
			LOG_LINE;
			register_tag_loader(57, import_loader);
			LOG_LINE;
			register_tag_loader(59, do_init_action_loader);   
			LOG_LINE;
			register_tag_loader(60, define_video_stream);
			LOG_LINE;
			register_tag_loader(61, read_video_frame);
			LOG_LINE;
			register_tag_loader(62, define_font_info_loader);
			LOG_LINE;
			register_tag_loader(70, place_object_2_loader);
		}
	}



	void	get_movie_info(
		const char* filename,
		int* version,
		int* width,
		int* height,
		float* frames_per_second,
		int* frame_count,
		int* tag_count
		)
	// Attempt to read the header of the given .swf movie file.
	// Put extracted info in the given vars.
	// Sets *version to 0 if info can't be extracted.
	{
		LOG_LINE;
		if (s_opener_function == NULL)
		{
			LOG_LINE;
			log_error("error: get_movie_info(): no file opener function registered\n");
			if (version) *version = 0;
			return;
		}

		LOG_LINE;
		tu_file*	in = s_opener_function(filename);
		LOG_LINE;
		if (in == NULL || in->get_error() != TU_FILE_NO_ERROR)
		{
			LOG_LINE;
			log_error("error: get_movie_info(): can't open '%s'\n", filename);
			LOG_LINE;
			if (version) *version = 0;
			delete in;
			return;
		}

		LOG_LINE;
		Uint32	file_start_pos = in->get_position();
		LOG_LINE;
		Uint32	header = in->read_le32();
		LOG_LINE;
		Uint32	file_length = in->read_le32();
		LOG_LINE;
		Uint32	file_end_pos = file_start_pos + file_length;

		LOG_LINE;
		int	local_version = (header >> 24) & 255;
		LOG_LINE;
		if ((header & 0x0FFFFFF) != 0x00535746
		    && (header & 0x0FFFFFF) != 0x00535743)
		{
			// ERROR
			LOG_LINE;
			log_error("error: get_movie_info(): file '%s' does not start with a SWF header!\n", filename);
			LOG_LINE;
			if (version) *version = 0;
			delete in;
			return;
		}
		LOG_LINE;
		bool	compressed = (header & 255) == 'C';

		LOG_LINE;
		tu_file*	original_in = NULL;
		if (compressed)
		{
#if TU_CONFIG_LINK_TO_ZLIB == 0
			LOG_LINE;
			log_error("get_movie_info(): can't read zipped SWF data; TU_CONFIG_LINK_TO_ZLIB is 0!\n");
			return;
#endif
			LOG_LINE;
			original_in = in;

			// Uncompress the input as we read it.
			LOG_LINE;
			in = zlib_adapter::make_inflater(original_in);

			// Subtract the size of the 8-byte header, since
			// it's not included in the compressed
			// stream length.
			LOG_LINE;
			file_length -= 8;
		}

		LOG_LINE;
		stream	str(in);

		LOG_LINE;
		rect	frame_size;
		LOG_LINE;
		frame_size.read(&str);

		LOG_LINE;
		float	local_frame_rate = str.read_u16() / 256.0f;
		LOG_LINE;
		int	local_frame_count = str.read_u16();

		LOG_LINE;
		if (version) *version = local_version;
		LOG_LINE;
		if (width) *width = int(frame_size.width() / 20.0f + 0.5f);
		LOG_LINE;
		if (height) *height = int(frame_size.height() / 20.0f + 0.5f);
		LOG_LINE;
		if (frames_per_second) *frames_per_second = local_frame_rate;
		LOG_LINE;
		if (frame_count) *frame_count = local_frame_count;

		LOG_LINE;
		if (tag_count)
		{
			// Count tags.
			LOG_LINE;
			int local_tag_count = 0;
			LOG_LINE;
			while ((Uint32) str.get_position() < file_end_pos)
			{
				LOG_LINE;
				str.open_tag();
				LOG_LINE;
				str.close_tag();
				LOG_LINE;
				local_tag_count++;
			}
			LOG_LINE;
			*tag_count = local_tag_count;
		}

		LOG_LINE;
		delete in;
		LOG_LINE;
		delete original_in;
	}



	movie_definition*	create_movie(const char* filename)
	// Create the movie definition from the specified .swf file.
	{
		LOG_LINE;
		return create_movie_sub(filename);
	}


	movie_definition_sub*	create_movie_sub(const char* filename)
	{
		LOG_LINE;
		if (s_opener_function == NULL)
		{
			// Don't even have a way to open the file.
			LOG_LINE;
			log_error("error: no file opener function; can't create movie.  "
				  "See gameswf::register_file_opener_callback\n");
			return NULL;
		}

		LOG_LINE;
		tu_file* in = s_opener_function(filename);
		if (in == NULL)
		{
			LOG_LINE;
			log_error("failed to open '%s'; can't create movie.\n", filename);
			return NULL;
		}
		else if (in->get_error())
		{
			LOG_LINE;
			log_error("error: file opener can't open '%s' - %d\n", filename,in->get_error());
			return NULL;
		}

		LOG_LINE;
		ensure_loaders_registered();

		LOG_LINE;
		movie_def_impl*	m = new movie_def_impl(DO_LOAD_BITMAPS, DO_LOAD_FONT_SHAPES);
		LOG_LINE;
		m->read(in);

		LOG_LINE;
		delete in;

		LOG_LINE;
		if (m && s_use_cache_files)
		{
			// Try to load a .gsc file.
			LOG_LINE;
			tu_string	cache_filename(filename);
			LOG_LINE;
			cache_filename += ".gsc";
			LOG_LINE;
			tu_file*	cache_in = s_opener_function(cache_filename.c_str());
			LOG_LINE;
			if (cache_in == NULL
			    || cache_in->get_error() != TU_FILE_NO_ERROR)
			{
				// Can't open cache file; don't sweat it.
				LOG_LINE;
				IF_VERBOSE_PARSE(log_error("note: couldn't open cache file '%s'\n", cache_filename.c_str()));

				LOG_LINE;
				m->generate_font_bitmaps();	// can't read cache, so generate font texture data.
			}
			else
			{
				// Load the cached data.
				LOG_LINE;
				m->input_cached_data(cache_in);
			}

			LOG_LINE;
			delete cache_in;
		}

		LOG_LINE;
		m->add_ref();
		LOG_LINE;
		return m;
	}


	static bool	s_no_recurse_while_loading = false;	// @@ TODO get rid of this; make it the normal mode.


	movie_definition*	create_movie_no_recurse(
		tu_file* in,
		create_bitmaps_flag cbf,
		create_font_shapes_flag cfs)
	{
		LOG_LINE;
		ensure_loaders_registered();

		// @@ TODO make no_recurse the standard way to load.
		// In create_movie(), use the visit_ API to keep
		// visiting dependent movies, until everything is
		// loaded.  That way we only have one code path and
		// the resource_proxy stuff gets tested.
		LOG_LINE;
		s_no_recurse_while_loading = true;

		LOG_LINE;
		movie_def_impl*	m = new movie_def_impl(cbf, cfs);
		LOG_LINE;
		m->read(in);

		LOG_LINE;
		s_no_recurse_while_loading = false;

		LOG_LINE;
		m->add_ref();
		return m;
	}


	//
	// global gameswf management
	//
	void	sprite_builtins_clear();

	void	clear()
	// Maximum release of resources.
	{
		LOG_LINE;
		clear_library();
		LOG_LINE;
		sprite_builtins_clear();
		LOG_LINE;
		fontlib::clear();
		LOG_LINE;
		action_clear();
	}


	//
	// library stuff, for sharing resources among different movies.
	//


	static stringi_hash< smart_ptr<movie_definition_sub> >	s_movie_library;
	static hash< movie_definition_sub*, smart_ptr<movie_interface> >	s_movie_library_inst;
	static array<movie_interface*> s_extern_sprites;
	static movie_interface* s_current_root;

	static tu_string s_workdir;

	void save_extern_movie(movie_interface* m)
	{
		LOG_LINE;
		s_extern_sprites.push_back(m);
	}

//#if 0
	void delete_unused_root()
	{
		LOG_LINE;
		for (int i = 0; i < s_extern_sprites.size(); i++)
		{
			LOG_LINE;
			movie_interface* root_m = s_extern_sprites[i];
			LOG_LINE;
			movie* m = root_m->get_root_movie();
      
			LOG_LINE;
			if (m->get_ref_count() < 2)
			{
				LOG_LINE;
				IF_VERBOSE_ACTION(log_error("extern movie deleted\n"));
				s_extern_sprites.remove(i);
				LOG_LINE;
				i--;
				LOG_LINE;
				root_m->drop_ref();
			}
		}
	}
//#endif // 0

	movie_interface* get_current_root()
	{
		LOG_LINE;
		QASSERT(s_current_root != NULL);
		return s_current_root;
	}

	void set_current_root(movie_interface* m)
	{
		LOG_LINE;
		QASSERT(m != NULL);
		s_current_root = m;
	}

	const char* get_workdir()
	{
		LOG_LINE;
		return s_workdir.c_str();
	}

	void set_workdir(const char* dir)
	{
		LOG_LINE;
		QASSERT(dir != NULL);
		s_workdir = dir;
	}

	void	clear_library()
	// Drop all library references to movie_definitions, so they
	// can be cleaned up.
	{
		LOG_LINE;
		s_movie_library.clear();
		LOG_LINE;
		s_movie_library_inst.clear();
	}

// tulrich: Vitaly sent this code.  I don't trust it though!  It looks
// like it may be dropping refs in order to remove cyclic references.
// I would rather fix the source of any cyclic references directly.
//
// 	void clear_library()
// 	// Drop all library references to movie_definitions, so they
// 	// can be cleaned up.
// 	{
// 		{for (hash< movie_definition_sub*, smart_ptr<movie_interface> >::iterator it =
// 			      s_movie_library_inst.begin();
// 		      it != s_movie_library_inst.end();
// 		      ++it)
// 		{
// 			smart_ptr<movie_interface> obj = it->second;
// 			while (obj->get_ref_count() > 2)            
// 			{                               
// 				obj->drop_ref();
// 			}
// 		}}
// 		s_movie_library_inst.clear();
//   
// 		{for (stringi_hash< smart_ptr<movie_definition_sub> >::iterator it = s_movie_library.begin();
// 		      it != s_movie_library.end();                                                            
// 		      ++it)                        
// 		{
// 			smart_ptr<movie_definition_sub> obj = it->second;
// 			while (obj->get_ref_count() > 2)                 
// 			{                               
// 				obj->drop_ref();
// 			}
// 		}}
// 		s_movie_library.clear();
// 	}


	movie_definition*	create_library_movie(const char* filename)
	// Try to load a movie from the given url, if we haven't
	// loaded it already.  Add it to our library on success, and
	// return a pointer to it.
	{
		LOG_LINE;
		return create_library_movie_sub(filename);
	}


	movie_definition_sub*	create_library_movie_sub(const char* filename)
	{
		LOG_LINE;
		tu_string	fn(filename);

		// Is the movie already in the library?
		{
			LOG_LINE;
			smart_ptr<movie_definition_sub>	m;
			LOG_LINE;
			s_movie_library.get(fn, &m);
			LOG_LINE;
			if (m != NULL)
			{
				// Return cached movie.
				LOG_LINE;
				m->add_ref();
				LOG_LINE;
				return m.get_ptr();
			}
		}

		// Try to open a file under the filename.
		LOG_LINE;
		movie_definition_sub*	mov = create_movie_sub(filename);

		LOG_LINE;
		if (mov == NULL)
		{
			LOG_LINE;
			log_error("error: couldn't load library movie '%s'\n", filename);
			return NULL;
		}
		else
		{
			LOG_LINE;
			s_movie_library.add(fn, mov);
		}

		LOG_LINE;
		mov->add_ref();
		return mov;
	}
	
	movie_interface*	create_library_movie_inst(movie_definition* md)
	{
		LOG_LINE;
		return create_library_movie_inst_sub((movie_definition_sub*)md);
	}


	movie_interface*	create_library_movie_inst_sub(movie_definition_sub* md)
	{
		// Is the movie instance already in the library?
		{
			LOG_LINE;
			smart_ptr<movie_interface>	m;
			LOG_LINE;
			s_movie_library_inst.get(md, &m);
			LOG_LINE;
			if (m != NULL)
			{
				// Return cached movie instance.
				LOG_LINE;
				m->add_ref();
				LOG_LINE;
				return m.get_ptr();
			}
		}

		// Try to create movie interface
		LOG_LINE;
		movie_interface* mov = md->create_instance();

		if (mov == NULL)
		{
			LOG_LINE;
			log_error("error: couldn't create instance\n");

			return NULL;
		}
		else
		{
			LOG_LINE;
			s_movie_library_inst.add(md, mov);
		}

		LOG_LINE;
		mov->add_ref();
		return mov;
	}


	void	precompute_cached_data(movie_definition* movie_def)
	// Fill in cached data in movie_def.
	// @@@@ NEEDS TESTING -- MIGHT BE BROKEN!!!
	{
		QASSERT(movie_def != NULL);

		// Temporarily install null render and sound handlers,
		// so we don't get output during preprocessing.
		//
		// Use automatic struct var to make sure we restore
		// when exiting the function.
		struct save_stuff
		{
			render_handler*	m_original_rh;
			sound_handler*	m_original_sh;

			save_stuff()
			{
				// Save.
				LOG_LINE;
				m_original_rh = get_render_handler();
				LOG_LINE;
				m_original_sh = get_sound_handler();
				LOG_LINE;
				set_render_handler(NULL);
				LOG_LINE;
				set_sound_handler(NULL);
			}

			~save_stuff()
			{
				// Restore.
				LOG_LINE;
				set_render_handler(m_original_rh);
				LOG_LINE;
				set_sound_handler(m_original_sh);
			}
		} save_stuff_instance;

		// Need an instance.
		LOG_LINE;
		gameswf::movie_interface*	m = movie_def->create_instance();
		LOG_LINE;
		if (m == NULL)
		{
			log_error("error: precompute_cached_data can't create instance of movie\n");
			return;
		}
		
		// Run through the movie's frames.
		//
		// @@ there might be cleaner ways to do this; e.g. do
		// execute_frame_tags(i, true) on movie and all child
		// sprites.
		LOG_LINE;
		int	kick_count = 0;
		for (;;)
		{
			// @@ do we also have to run through all sprite frames
			// as well?
			//
			// @@ also, ActionScript can rescale things
			// dynamically -- we can't really do much about that I
			// guess?
			//
			// @@ Maybe we should allow the user to specify some
			// safety margin on scaled shapes.

			LOG_LINE;
			int	last_frame = m->get_current_frame();
			LOG_LINE;
			m->advance(0.010f);
			LOG_LINE;
			m->display();

			LOG_LINE;
			if (m->get_current_frame() == movie_def->get_frame_count() - 1)
			{
				// Done.
				break;
			}

			LOG_LINE;
			if (m->get_play_state() == gameswf::movie_interface::STOP)
			{
				// Kick the movie.
				//printf("kicking movie, kick ct = %d\n", kick_count);
				LOG_LINE;
				m->goto_frame(last_frame + 1);
				LOG_LINE;
				m->set_play_state(gameswf::movie_interface::PLAY);
				LOG_LINE;
				kick_count++;

				LOG_LINE;
				if (kick_count > 10)
				{
					//printf("movie is stalled; giving up on playing it through.\n");
					break;
				}
			}
			else if (m->get_current_frame() < last_frame)
			{
				LOG_LINE;
				// Hm, apparently we looped back.  Skip ahead...
				log_error("loop back; jumping to frame %d\n", last_frame);
				LOG_LINE;
				m->goto_frame(last_frame + 1);
			}
			else
			{
				kick_count = 0;
			}
		}

		LOG_LINE;
		m->drop_ref();
	}


	//
	// Some tag implementations
	//


	void	null_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Silently ignore the contents of this tag.
	{
		LOG_LINE;
		static const u32	tag( 0x37314D21 );

		if ( tag_type == tag )
		{
			u32	length = in->get_tag_length();

			length -= 2;
		}
	}

	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Label the current frame of m with the name from the stream.
	{
		LOG_LINE;
		char*	n = in->read_string();
		LOG_LINE;
		m->add_frame_name(n);
		LOG_LINE;
		delete [] n;
	}

	struct set_background_color : public execute_tag
	{
		rgba	m_color;

		void	execute(movie* m)
		{
			LOG_LINE;
			float	current_alpha = m->get_background_alpha();
			LOG_LINE;
			m_color.m_a = frnd(current_alpha * 255.0f);
			LOG_LINE;
			m->set_background_color(m_color);
		}

		void	execute_state(movie* m)
		{
			LOG_LINE;
			execute(m);
		}

		void	read(stream* in)
		{
			LOG_LINE;
			m_color.read_rgb(in);

			LOG_LINE;
			IF_VERBOSE_PARSE(log_msg("  set_background_color: (%d %d %d)\n",
						 m_color.m_r, m_color.m_g, m_color.m_b));
		}
	};


	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 9);
		LOG_LINE;
		QASSERT(m);

		LOG_LINE;
		set_background_color*	t = new set_background_color;
		LOG_LINE;
		t->read(in);

		LOG_LINE;
		m->add_execute_tag(t);
	}

#if 0
	// Bitmap character
	struct bitmap_character : public bitmap_character_def
	{
		bitmap_character(bitmap_info* bi)
			:
			m_bitmap_info(bi)
		{
		}

// 		bitmap_character(image::rgb* image)
// 		{
// 			QASSERT(image != 0);

// 			// Create our bitmap info, from our image.
// 			m_bitmap_info = gameswf::render::create_bitmap_info_rgb(image);
// 		}

// 		bitmap_character(image::rgba* image)
// 		{
// 			QASSERT(image != 0);

// 			// Create our bitmap info, from our image.
// 			m_bitmap_info = gameswf::render::create_bitmap_info_rgba(image);
// 		}

		gameswf::bitmap_info*	get_bitmap_info()
		{
			return m_bitmap_info.get_ptr();
		}

	private:
		smart_ptr<gameswf::bitmap_info>	m_bitmap_info;
	};
#endif

	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load JPEG compression tables that can be used to load
	// images further along in the stream.
	{
		LOG_LINE;
		QASSERT(tag_type == 8);

#if TU_CONFIG_LINK_TO_JPEGLIB
		LOG_LINE;
		jpeg::input*	j_in = jpeg::input::create_swf_jpeg2_header_only(in->get_underlying_stream());
		LOG_LINE;
		QASSERT(j_in);

		LOG_LINE;
		m->set_jpeg_loader(j_in);
#endif // TU_CONFIG_LINK_TO_JPEGLIB
	}


	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m)
	// A JPEG image without included tables; those should be in an
	// existing jpeg::input object stored in the movie.
	{
		QASSERT(tag_type == 6);

		LOG_LINE;
		Uint16	character_id = in->read_u16();

		//
		// Read the image data.
		//
		LOG_LINE;
		bitmap_info*	bi = NULL;

		LOG_LINE;
		if (m->get_create_bitmaps() == DO_LOAD_BITMAPS)
		{
#if TU_CONFIG_LINK_TO_JPEGLIB
			LOG_LINE;
			jpeg::input*	j_in = m->get_jpeg_loader();
			LOG_LINE;
			QASSERT(j_in);
			LOG_LINE;
			j_in->discard_partial_buffer();

			LOG_LINE;
			image::rgb*	im = image::read_swf_jpeg2_with_tables(j_in);
			LOG_LINE;
			bi = render::create_bitmap_info_rgb(im,false);
			LOG_LINE;
			delete im;
#else
			LOG_LINE;
			log_error("gameswf is not linked to jpeglib -- can't load jpeg image data!\n");
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
#endif
		}
		else
		{
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
		}

		LOG_LINE;
		QASSERT(bi->get_ref_count() == 0);

		LOG_LINE;
		bitmap_character*	ch = new bitmap_character(bi);

		LOG_LINE;
		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 21);
		
		LOG_LINE;
		Uint16	character_id = in->read_u16();

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  define_bits_jpeg2_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		//
		// Read the image data.
		//
		
		LOG_LINE;
		bitmap_info*	bi = NULL;

		LOG_LINE;
		if (m->get_create_bitmaps() == DO_LOAD_BITMAPS)
		{
#if TU_CONFIG_LINK_TO_JPEGLIB
			LOG_LINE;
			image::rgb* im = image::read_swf_jpeg2(in->get_underlying_stream());
			LOG_LINE;
			bi = render::create_bitmap_info_rgb(im,false);
			LOG_LINE;
			delete im;
#else
			LOG_LINE;
			log_error("gameswf is not linked to jpeglib -- can't load jpeg image data!\n");
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
#endif
		}
		else
		{
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
		}

		LOG_LINE;
		QASSERT(bi->get_ref_count() == 0);

		LOG_LINE;
		bitmap_character*	ch = new bitmap_character(bi);

		LOG_LINE;
		m->add_bitmap_character(character_id, ch);
	}


#if TU_CONFIG_LINK_TO_ZLIB
	void	inflate_wrapper(tu_file* in, void* buffer, int buffer_bytes)
	// Wrapper function -- uses Zlib to uncompress in_bytes worth
	// of data from the input file into buffer_bytes worth of data
	// into *buffer.
	{
		QASSERT(in);
		QASSERT(buffer);
		QASSERT(buffer_bytes > 0);

		LOG_LINE;
		int err;
		LOG_LINE;
		z_stream d_stream; /* decompression stream */

		LOG_LINE;
		d_stream.zalloc = (alloc_func)0;
		LOG_LINE;
		d_stream.zfree = (free_func)0;
		LOG_LINE;
		d_stream.opaque = (voidpf)0;

		LOG_LINE;
		d_stream.next_in  = 0;
		LOG_LINE;
		d_stream.avail_in = 0;

		LOG_LINE;
		d_stream.next_out = (Byte*) buffer;
		LOG_LINE;
		d_stream.avail_out = (uInt) buffer_bytes;

		LOG_LINE;
		err = inflateInit(&d_stream);
		LOG_LINE;
		if (err != Z_OK) {
			log_error("error: inflate_wrapper() inflateInit() returned %d\n", err);
			return;
		}

		Uint8	buf[1];

		LOG_LINE;
		for (;;) {
			// Fill a one-byte (!) buffer.
			buf[0] = in->read_byte();
			d_stream.next_in = &buf[0];
			d_stream.avail_in = 1;

			err = inflate(&d_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END) break;
			if (err != Z_OK)
			{
				log_error("error: inflate_wrapper() inflate() returned %d\n", err);
			}
		}

		LOG_LINE;
		err = inflateEnd(&d_stream);
		if (err != Z_OK)
		{
			log_error("error: inflate_wrapper() inflateEnd() return %d\n", err);
		}
	}
#endif // TU_CONFIG_LINK_TO_ZLIB


	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m)
	// loads a define_bits_jpeg3 tag. This is a jpeg file with an alpha
	// channel using zlib compression.
	{
		QASSERT(tag_type == 35);

		Uint16	character_id = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("  define_bits_jpeg3_loader: charid = %d pos = 0x%x\n", character_id, in->get_position()));

		LOG_LINE;
		Uint32	jpeg_size = in->read_u32();
		LOG_LINE;
		Uint32	alpha_position = in->get_position() + jpeg_size;

		bitmap_info*	bi = NULL;

		LOG_LINE;
		if (m->get_create_bitmaps() == DO_LOAD_BITMAPS)
		{
#if TU_CONFIG_LINK_TO_JPEGLIB == 0 || TU_CONFIG_LINK_TO_ZLIB == 0
			LOG_LINE;
			log_error("gameswf is not linked to jpeglib/zlib -- can't load jpeg/zipped image data!\n");
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
#else
			//
			// Read the image data.
			//
		
			// Read rgb data.
			LOG_LINE;
			image::rgba*	im = image::read_swf_jpeg3(in->get_underlying_stream());

			// Read alpha channel.
			LOG_LINE;
			in->set_position(alpha_position);

			LOG_LINE;
			int	buffer_bytes = im->m_width * im->m_height;
			LOG_LINE;
			Uint8*	buffer = new Uint8[buffer_bytes];

			LOG_LINE;
			inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);

			LOG_LINE;
			for (int i = 0; i < buffer_bytes; i++)
			{
				im->m_data[4*i+3] = buffer[i];
			}

			LOG_LINE;
			delete [] buffer;

			LOG_LINE;
			bi = render::create_bitmap_info_rgba(im);

			LOG_LINE;
			delete im;
#endif

		}
		else
		{
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
		}

		// Create bitmap character.
		LOG_LINE;
		bitmap_character*	ch = new bitmap_character(bi);

		LOG_LINE;
		m->add_bitmap_character(character_id, ch);
	}


	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 20 || tag_type == 36);

		LOG_LINE;
		Uint16	character_id = in->read_u16();
		LOG_LINE;
		Uint8	bitmap_format = in->read_u8();	// 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
		LOG_LINE;
		Uint16	width = in->read_u16();
		LOG_LINE;
		Uint16	height = in->read_u16();

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  defbitslossless2: tag_type = %d, id = %d, fmt = %d, w = %d, h = %d\n",
				tag_type,
				character_id,
				bitmap_format,
				width,
				height));

		LOG_LINE;
		bitmap_info*	bi = NULL;
		LOG_LINE;
		if (m->get_create_bitmaps() == DO_LOAD_BITMAPS)
		{
#if TU_CONFIG_LINK_TO_ZLIB == 0
			LOG_LINE;
			log_error("gameswf is not linked to zlib -- can't load zipped image data!\n");
			return;
#else
			LOG_LINE;
			if (tag_type == 20)
			{
				// RGB image data.
				LOG_LINE;
				image::rgb*	image = image::create_rgb(width, height);

				LOG_LINE;
				if (bitmap_format == 3)
				{
					// 8-bit data, preceded by a palette.

					LOG_LINE;
					const int	bytes_per_pixel = 1;
					LOG_LINE;
					int	color_table_size = in->read_u8();
					LOG_LINE;
					color_table_size += 1;	// !! SWF stores one less than the actual size

					LOG_LINE;
					int	pitch = (width * bytes_per_pixel + 3) & ~3;

					LOG_LINE;
					int	buffer_bytes = color_table_size * 3 + pitch * height;
					LOG_LINE;
					Uint8*	buffer = new Uint8[buffer_bytes];

					LOG_LINE;
					inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
					LOG_LINE;
					QASSERT(in->get_position() <= in->get_tag_end_position());

					LOG_LINE;
					Uint8*	color_table = buffer;

					LOG_LINE;
					for (int j = 0; j < height; j++)
					{
						Uint8*	image_in_row = buffer + color_table_size * 3 + j * pitch;
						Uint8*	image_out_row = image::scanline(image, j);
						for (int i = 0; i < width; i++)
						{
							Uint8	pixel = image_in_row[i * bytes_per_pixel];
							image_out_row[i * 3 + 0] = color_table[pixel * 3 + 0];
							image_out_row[i * 3 + 1] = color_table[pixel * 3 + 1];
							image_out_row[i * 3 + 2] = color_table[pixel * 3 + 2];
						}
					}

					LOG_LINE;
					delete [] buffer;
				}
				else if (bitmap_format == 4)
				{
					// 16 bits / pixel
					LOG_LINE;
					const int	bytes_per_pixel = 2;
					LOG_LINE;
					int	pitch = (width * bytes_per_pixel + 3) & ~3;

					LOG_LINE;
					int	buffer_bytes = pitch * height;
					LOG_LINE;
					Uint8*	buffer = new Uint8[buffer_bytes];

					LOG_LINE;
					inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
					LOG_LINE;
					QASSERT(in->get_position() <= in->get_tag_end_position());
			
					LOG_LINE;
					for (int j = 0; j < height; j++)
					{
						Uint8*	image_in_row = buffer + j * pitch;
						Uint8*	image_out_row = image::scanline(image, j);
						for (int i = 0; i < width; i++)
						{
							Uint16	pixel = image_in_row[i * 2] | (image_in_row[i * 2 + 1] << 8);
					
							// @@ How is the data packed???  I'm just guessing here that it's 565!
							image_out_row[i * 3 + 0] = (pixel >> 8) & 0xF8;	// red
							image_out_row[i * 3 + 1] = (pixel >> 3) & 0xFC;	// green
							image_out_row[i * 3 + 2] = (pixel << 3) & 0xF8;	// blue
						}
					}
			
					LOG_LINE;
					delete [] buffer;
				}
				else if (bitmap_format == 5)
				{
					// 32 bits / pixel, input is ARGB format (???)
					LOG_LINE;
					const int	bytes_per_pixel = 4;
					LOG_LINE;
					int	pitch = width * bytes_per_pixel;

					LOG_LINE;
					int	buffer_bytes = pitch * height;
					LOG_LINE;
					Uint8*	buffer = new Uint8[buffer_bytes];

					LOG_LINE;
					inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
					LOG_LINE;
					QASSERT(in->get_position() <= in->get_tag_end_position());
			
					// Need to re-arrange ARGB into RGB.
					LOG_LINE;
					for (int j = 0; j < height; j++)
					{
						Uint8*	image_in_row = buffer + j * pitch;
						Uint8*	image_out_row = image::scanline(image, j);
						for (int i = 0; i < width; i++)
						{
							Uint8	a = image_in_row[i * 4 + 0];
							Uint8	r = image_in_row[i * 4 + 1];
							Uint8	g = image_in_row[i * 4 + 2];
							Uint8	b = image_in_row[i * 4 + 3];
							image_out_row[i * 3 + 0] = r;
							image_out_row[i * 3 + 1] = g;
							image_out_row[i * 3 + 2] = b;
							a = a;	// Inhibit warning.
						}
					}

					LOG_LINE;
					delete [] buffer;
				}

//				bitmap_character*	ch = new bitmap_character(image);
				LOG_LINE;
				bi = render::create_bitmap_info_rgb(image,false);
				LOG_LINE;
				delete image;

// 				// add image to movie, under character id.
// 				m->add_bitmap_character(character_id, ch);
			}
			else
			{
				// RGBA image data.
				QASSERT(tag_type == 36);

				LOG_LINE;
				image::rgba*	image = image::create_rgba(width, height);

				LOG_LINE;
				if (bitmap_format == 3)
				{
					// 8-bit data, preceded by a palette.

					LOG_LINE;
					const int	bytes_per_pixel = 1;
					LOG_LINE;
					int	color_table_size = in->read_u8();
					LOG_LINE;
					color_table_size += 1;	// !! SWF stores one less than the actual size

					LOG_LINE;
					int	pitch = (width * bytes_per_pixel + 3) & ~3;

					LOG_LINE;
					int	buffer_bytes = color_table_size * 4 + pitch * height;
					LOG_LINE;
					Uint8*	buffer = new Uint8[buffer_bytes];

					LOG_LINE;
					inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
					QASSERT(in->get_position() <= in->get_tag_end_position());

					LOG_LINE;
					Uint8*	color_table = buffer;

					LOG_LINE;
					for (int j = 0; j < height; j++)
					{
						Uint8*	image_in_row = buffer + color_table_size * 4 + j * pitch;
						Uint8*	image_out_row = image::scanline(image, j);
						for (int i = 0; i < width; i++)
						{
							Uint8	pixel = image_in_row[i * bytes_per_pixel];
							image_out_row[i * 4 + 0] = color_table[pixel * 4 + 0];
							image_out_row[i * 4 + 1] = color_table[pixel * 4 + 1];
							image_out_row[i * 4 + 2] = color_table[pixel * 4 + 2];
							image_out_row[i * 4 + 3] = color_table[pixel * 4 + 3];
						}
					}

					LOG_LINE;
					delete [] buffer;
				}
				else if (bitmap_format == 4)
				{
					// 16 bits / pixel
					const int	bytes_per_pixel = 2;
					LOG_LINE;
					int	pitch = (width * bytes_per_pixel + 3) & ~3;

					LOG_LINE;
					int	buffer_bytes = pitch * height;
					LOG_LINE;
					Uint8*	buffer = new Uint8[buffer_bytes];

					LOG_LINE;
					inflate_wrapper(in->get_underlying_stream(), buffer, buffer_bytes);
					LOG_LINE;
					QASSERT(in->get_position() <= in->get_tag_end_position());
			
					LOG_LINE;
					for (int j = 0; j < height; j++)
					{
						Uint8*	image_in_row = buffer + j * pitch;
						Uint8*	image_out_row = image::scanline(image, j);
						for (int i = 0; i < width; i++)
						{
							Uint16	pixel = image_in_row[i * 2] | (image_in_row[i * 2 + 1] << 8);
					
							// @@ How is the data packed???  I'm just guessing here that it's 565!
							image_out_row[i * 4 + 0] = 255;			// alpha
							image_out_row[i * 4 + 1] = (pixel >> 8) & 0xF8;	// red
							image_out_row[i * 4 + 2] = (pixel >> 3) & 0xFC;	// green
							image_out_row[i * 4 + 3] = (pixel << 3) & 0xF8;	// blue
						}
					}
			
					LOG_LINE;
					delete [] buffer;
				}
				else if (bitmap_format == 5)
				{
					// 32 bits / pixel, input is ARGB format

					LOG_LINE;
					inflate_wrapper(in->get_underlying_stream(), image->m_data, width * height * 4);
					LOG_LINE;
					QASSERT(in->get_position() <= in->get_tag_end_position());
			
					// Need to re-arrange ARGB into RGBA.
					LOG_LINE;
					for (int j = 0; j < height; j++)
					{
						Uint8*	image_row = image::scanline(image, j);
						for (int i = 0; i < width; i++)
						{
							Uint8	a = image_row[i * 4 + 0];
							Uint8	r = image_row[i * 4 + 1];
							Uint8	g = image_row[i * 4 + 2];
							Uint8	b = image_row[i * 4 + 3];
							image_row[i * 4 + 0] = r;
							image_row[i * 4 + 1] = g;
							image_row[i * 4 + 2] = b;
							image_row[i * 4 + 3] = a;
						}
					}
				}

				LOG_LINE;
				bi = render::create_bitmap_info_rgba(image);
//				bitmap_character*	ch = new bitmap_character(image);
				LOG_LINE;
				delete image;

//	 			// add image to movie, under character id.
//	 			m->add_bitmap_character(character_id, ch);
			}
#endif // TU_CONFIG_LINK_TO_ZLIB
		}
		else
		{
			LOG_LINE;
			bi = render::create_bitmap_info_empty();
		}
		QASSERT(bi->get_ref_count() == 0);

		LOG_LINE;
		bitmap_character*	ch = new bitmap_character(bi);

		// add image to movie, under character id.
		LOG_LINE;
		m->add_bitmap_character(character_id, ch);
	}


	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 2
		       || tag_type == 22
		       || tag_type == 32);

		LOG_LINE;
		Uint16	character_id = in->read_u16();
		IF_VERBOSE_PARSE(log_msg("  shape_loader: id = %d\n", character_id));

		LOG_LINE;
		shape_character_def*	ch = new shape_character_def;
		LOG_LINE;
		ch->read(in, tag_type, true, m);

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  bound rect:"); ch->get_bound().print());

		LOG_LINE;
		m->add_character(character_id, ch);
	}

	void define_shape_morph_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 46);
		LOG_LINE;
		Uint16 character_id = in->read_u16();
		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  shape_morph_loader: id = %d\n", character_id));
		LOG_LINE;
		morph2_character_def* morph = new morph2_character_def;
		LOG_LINE;
		morph->read(in, tag_type, true, m);
		LOG_LINE;
		m->add_character(character_id, morph);
	}

	//
	// font loaders
	//


	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load a DefineFont or DefineFont2 tag.
	{
		LOG_LINE;
		QASSERT(tag_type == 10 || tag_type == 48);

		LOG_LINE;
		Uint16	font_id = in->read_u16();
		
		LOG_LINE;
		font*	f = new font;
		LOG_LINE;
		f->read(in, tag_type, m);

		LOG_LINE;
		m->add_font(font_id, f);

		// Automatically keeping fonts in fontlib is
		// problematic.  The app should be responsible for
		// optionally adding fonts to fontlib.
		// //fontlib::add_font(f);
	}


	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load a DefineFontInfo tag.  This adds information to an
	// existing font.
	{
		LOG_LINE;
		QASSERT(tag_type == 13 || tag_type == 62);

		LOG_LINE;
		Uint16	font_id = in->read_u16();
		
		LOG_LINE;
		font*	f = m->get_font(font_id);
		LOG_LINE;
		if (f)
		{
			LOG_LINE;
			f->read_font_info(in);
		}
		else
		{
			LOG_LINE;
			log_error("define_font_info_loader: can't find font w/ id %d\n", font_id);
		}
	}


	//
	// swf_event
	//
	// For embedding event handlers in place_object_2

	swf_event::swf_event()
	{
		LOG_LINE;
	}

	void	swf_event::attach_to(character* ch) const
	{
		LOG_LINE;
		ch->set_event_handler(m_event, m_method);
	}

	void	swf_event::read(stream* in, Uint32 flags)
	{
		LOG_LINE;
		QASSERT(flags != 0);

		// Scream if more than one bit is set, since we're not set up to handle
		// that, and it doesn't seem possible to express in ActionScript source,
		// so it's important to know if this ever occurs in the wild.
		LOG_LINE;
		if (flags & (flags - 1))
		{
			LOG_LINE;
			log_error("error: swf_event::read() -- more than one event type encoded!  "
					"unexpected! flags = 0x%x\n", flags);
		}

		// 14 bits reserved, 18 bits used

		static const event_id	s_code_bits[18] =
		{
			event_id::LOAD,
			event_id::ENTER_FRAME,
			event_id::UNLOAD,
			event_id::MOUSE_MOVE,
			event_id::MOUSE_DOWN,
			event_id::MOUSE_UP,
			event_id::KEY_DOWN,
			event_id::KEY_UP,
			event_id::DATA,
			event_id::INITIALIZE,
			event_id::PRESS,
			event_id::RELEASE,
			event_id::RELEASE_OUTSIDE,
			event_id::ROLL_OVER,
			event_id::ROLL_OUT,
			event_id::DRAG_OVER,
			event_id::DRAG_OUT,
		};

		LOG_LINE;
		for (int i = 0, mask = 1; i < int(sizeof(s_code_bits)/sizeof(s_code_bits[0])); i++, mask <<= 1)
		{
			LOG_LINE;
			if (flags & mask)
			{
				LOG_LINE;
				m_event = s_code_bits[i];
				break;
			}
		}

		// what to do w/ key_press???  Is the data in the reserved parts of the flags???
		LOG_LINE;
		if (flags & (1 << 17))
		{
			LOG_LINE;
			log_error("swf_event::read -- KEY_PRESS found, not handled yet, flags = 0x%x\n", flags);
		}

		LOG_LINE;
		Uint32	event_length = in->read_u32();
		LOG_LINE;
		UNUSED(event_length);

		// Read the actions.
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("---- actions for event %s\n", m_event.get_function_name().c_str()));
		LOG_LINE;
		m_action_buffer.read(in);

		LOG_LINE;
		if (m_action_buffer.get_length() != (int) event_length)
		{
			LOG_LINE;
			log_error("error -- swf_event::read(), event_length = %d, but read %d\n",
					event_length,
					m_action_buffer.get_length());
			// @@ discard this event handler??
		}

		// Create a function to execute the actions.
		LOG_LINE;
		array<with_stack_entry>	empty_with_stack;
		LOG_LINE;
		as_as_function*	func = new as_as_function(&m_action_buffer, NULL, 0, empty_with_stack);
		LOG_LINE;
		func->set_length(m_action_buffer.get_length());

		LOG_LINE;
		m_method.set_as_as_function(func);
	}



	//
	// place_object_2
	//
	
	struct place_object_2 : public execute_tag
	{
		int	m_tag_type;
		char*	m_name;
		float	m_ratio;
		cxform	m_color_transform;
		matrix	m_matrix;
		bool	m_has_matrix;
		bool	m_has_cxform;
		Uint16	m_depth;
		Uint16	m_character_id;
                Uint16 	m_clip_depth;
		enum place_type {
			PLACE,
			MOVE,
			REPLACE,
		} m_place_type;
		array<swf_event*>	m_event_handlers;


		place_object_2()
			:
			m_tag_type(0),
			m_name(NULL),
			m_ratio(0),
			m_has_matrix(false),
			m_has_cxform(false),
			m_depth(0),
			m_character_id(0),
                        m_clip_depth(0),
			m_place_type(PLACE)
		{
			LOG_LINE;
		}

		~place_object_2()
		{
			LOG_LINE;
			delete [] m_name;
			LOG_LINE;
			m_name = NULL;

			LOG_LINE;
			for (int i = 0, n = m_event_handlers.size(); i < n; i++)
			{
				LOG_LINE;
				delete m_event_handlers[i];
			}
			LOG_LINE;
			m_event_handlers.resize(0);
		}

		void	read(stream* in, int tag_type, int movie_version)
		{
			LOG_LINE;
			QASSERT(tag_type == 4 || tag_type == 26);

			LOG_LINE;
			m_tag_type = tag_type;

			LOG_LINE;
			if (tag_type == 4)
			{
				// Original place_object tag; very simple.
				LOG_LINE;
				m_character_id = in->read_u16();
				LOG_LINE;
				m_depth = in->read_u16();
				LOG_LINE;
				m_matrix.read(in);

				LOG_LINE;
				IF_VERBOSE_PARSE(
					log_msg("  char_id = %d\n"
						"  depth = %d\n"
						"  mat = \n",
						m_character_id,
						m_depth);
					m_matrix.print());

					LOG_LINE;
				if (in->get_position() < in->get_tag_end_position())
				{
					LOG_LINE;
					m_color_transform.read_rgb(in);
					IF_VERBOSE_PARSE(log_msg("  cxform:\n"); m_color_transform.print());
				}
			}
			else if (tag_type == 26 || tag_type == 70 )
			{
				in->align();

				bool	has_actions = in->read_uint(1) ? true : false;
				bool	has_clip_bracket = in->read_uint(1) ? true : false;
				bool	has_name = in->read_uint(1) ? true : false;
				bool	has_ratio = in->read_uint(1) ? true : false;
				bool	has_cxform = in->read_uint(1) ? true : false;
				bool	has_matrix = in->read_uint(1) ? true : false;
				bool	has_char = in->read_uint(1) ? true : false;
				bool	flag_move = in->read_uint(1) ? true : false;

				m_depth = in->read_u16();
				IF_VERBOSE_PARSE(log_msg("  depth = %d\n", m_depth));

				if (has_char) {
					m_character_id = in->read_u16();
					IF_VERBOSE_PARSE(log_msg("  char id = %d\n", m_character_id));
				}

				if (has_matrix) {
					m_has_matrix = true;
					m_matrix.read(in);
					IF_VERBOSE_PARSE(log_msg("  mat:\n"); m_matrix.print());
				}
				if (has_cxform) {
					m_has_cxform = true;
					m_color_transform.read_rgba(in);
					IF_VERBOSE_PARSE(log_msg("  cxform:\n"); m_color_transform.print());
				}

				if (has_ratio) {
					m_ratio = (float)in->read_u16() / (float)65535;
					IF_VERBOSE_PARSE(log_msg("  ratio: %f\n", m_ratio));
				}

				if (has_name) {
					m_name = in->read_string();
					IF_VERBOSE_PARSE(log_msg("  name = %s\n", m_name ? m_name : "<null>"));
				}
				if (has_clip_bracket) {
					m_clip_depth = in->read_u16(); 
					IF_VERBOSE_PARSE(log_msg("  clip_depth = %d\n", m_clip_depth));
				}
				if (has_actions)
				{
					Uint16	reserved = in->read_u16();
					UNUSED(reserved);

					// The logical 'or' of all the following handlers.
					// I don't think we care about this...
					Uint32	all_flags = 0;
					if (movie_version >= 6)
					{
						all_flags = in->read_u32();
					}
					else
					{
						all_flags = in->read_u16();
					}
					UNUSED(all_flags);

					IF_VERBOSE_PARSE(log_msg("  actions: flags = 0x%X\n", all_flags));

					// Read swf_events.
					for (;;)
					{
						// Read event.
						in->align();

						Uint32	this_flags = 0;
						if (movie_version >= 6)
						{
							this_flags = in->read_u32();
						}
						else
						{
							this_flags = in->read_u16();
						}

						if (this_flags == 0)
						{
							// Done with events.
							break;
						}

						swf_event*	ev = new swf_event;
						ev->read(in, this_flags);

						m_event_handlers.push_back(ev);
					}
				}


				if (has_char == true && flag_move == true)
				{
					// Remove whatever's at m_depth, and put m_character there.
					m_place_type = REPLACE;
				}
				else if (has_char == false && flag_move == true)
				{
					// Moves the object at m_depth to the new location.
					m_place_type = MOVE;
				}
				else if (has_char == true && flag_move == false)
				{
					// Put m_character at m_depth.
					m_place_type = PLACE;
				}

				//log_msg("place object at depth %i\n", m_depth);


/*				LOG_LINE;
				bool	has_actions = false;

				if(movie_version >= 8)
				{
					in->read_uint(8);
				}

				if(movie_version >= 5)
				{
					has_actions = in->read_uint(1) ? true : false;
				}
				else
				{
					in->read_uint(1);
				}

				LOG_LINE;
				bool	has_clip_bracket = in->read_uint(1) ? true : false;
				LOG_LINE;
				bool	has_name = in->read_uint(1) ? true : false;
				LOG_LINE;
				bool	has_ratio = in->read_uint(1) ? true : false;
				LOG_LINE;
				bool	has_cxform = in->read_uint(1) ? true : false;
				LOG_LINE;
				bool	has_matrix = in->read_uint(1) ? true : false;
				LOG_LINE;
				bool	has_char = in->read_uint(1) ? true : false;
				LOG_LINE;
				bool	flag_move = in->read_uint(1) ? true : false;

				LOG_LINE;
				m_depth = in->read_u16();
				LOG_LINE;
				IF_VERBOSE_PARSE(log_msg("  depth = %d\n", m_depth));

				LOG_LINE;
				if (has_char) {
					LOG_LINE;
					m_character_id = in->read_u16();
					IF_VERBOSE_PARSE(log_msg("  char id = %d\n", m_character_id));
				}

				LOG_LINE;
				if (has_matrix) {
					LOG_LINE;
					m_has_matrix = true;
					LOG_LINE;
					m_matrix.read(in);
					IF_VERBOSE_PARSE(log_msg("  mat:\n"); m_matrix.print());
				}
				LOG_LINE;
				if (has_cxform) {
					LOG_LINE;
					m_has_cxform = true;
					LOG_LINE;
					m_color_transform.read_rgba(in);
					IF_VERBOSE_PARSE(log_msg("  cxform:\n"); m_color_transform.print());
				}
				
				LOG_LINE;
				if (has_ratio) {
					LOG_LINE;
					m_ratio = (float)in->read_u16() / (float)65535;
					IF_VERBOSE_PARSE(log_msg("  ratio: %f\n", m_ratio));
				}
				
				LOG_LINE;
				if (has_name) {
					LOG_LINE;
					m_name = in->read_string();
					IF_VERBOSE_PARSE(log_msg("  name = %s\n", m_name ? m_name : "<null>"));
				}
				LOG_LINE;
				if (has_clip_bracket) {
					LOG_LINE;
					m_clip_depth = in->read_u16(); 
					IF_VERBOSE_PARSE(log_msg("  clip_depth = %d\n", m_clip_depth));
				}
				LOG_LINE;
				if (has_actions)
				{
					LOG_LINE;
					Uint16	reserved = in->read_u16();
					LOG_LINE;
					UNUSED(reserved);

					// The logical 'or' of all the following handlers.
					// I don't think we care about this...
					LOG_LINE;
					Uint32	all_flags = 0;
					LOG_LINE;
					if (movie_version >= 6)
					{
						LOG_LINE;
						all_flags = in->read_u32();
					}
					else
					{
						LOG_LINE;
						all_flags = in->read_u16();
					}
					UNUSED(all_flags);

					LOG_LINE;
					IF_VERBOSE_PARSE(log_msg("  actions: flags = 0x%X\n", all_flags));

					// Read swf_events.
					LOG_LINE;
					for (;;)
					{
						// Read event.
						LOG_LINE;
						in->align();

						LOG_LINE;
						Uint32	this_flags = 0;
						LOG_LINE;
						if (movie_version >= 6)
						{
							LOG_LINE;
							this_flags = in->read_u32();
						}
						else
						{
							LOG_LINE;
							this_flags = in->read_u16();
						}

						LOG_LINE;
						if (this_flags == 0)
						{
							// Done with events.
							LOG_LINE;
							break;
						}

						LOG_LINE;
						swf_event*	ev = new swf_event;
						LOG_LINE;
						ev->read(in, this_flags);

						LOG_LINE;
						m_event_handlers.push_back(ev);
					}
				}


				LOG_LINE;
				if (has_char == true && flag_move == true)
				{
					// Remove whatever's at m_depth, and put m_character there.
					LOG_LINE;
					m_place_type = REPLACE;
				}
				else if (has_char == false && flag_move == true)
				{
					// Moves the object at m_depth to the new location.
					LOG_LINE;
					m_place_type = MOVE;
				}
				else if (has_char == true && flag_move == false)
				{
					// Put m_character at m_depth.
					LOG_LINE;
					m_place_type = PLACE;
				}
                                
				//log_msg("place object at depth %i\n", m_depth);*/
			}
		}

		
		void	execute(movie* m)
		// Place/move/whatever our object in the given movie.
		{
			LOG_LINE;
			switch (m_place_type)
			{
			case PLACE:
				LOG_LINE;
				m->add_display_object(
					m_character_id,
					m_name,
					m_event_handlers,
					m_depth,
					m_tag_type != 4,	// original place_object doesn't do replacement
					m_color_transform,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;

			case MOVE:
				LOG_LINE;
				m->move_display_object(
					m_depth,
					m_has_cxform,
					m_color_transform,
					m_has_matrix,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;

			case REPLACE:
				LOG_LINE;
				m->replace_display_object(
					m_character_id,
					m_name,
					m_depth,
					m_has_cxform,
					m_color_transform,
					m_has_matrix,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;
			}
		}

		void	execute_state(movie* m)
		{
			LOG_LINE;
			execute(m);
		}

		void	execute_state_reverse(movie* m, int frame)
		{
			LOG_LINE;
			switch (m_place_type)
			{
			case PLACE:
				// reverse of add is remove
				LOG_LINE;
				m->remove_display_object(m_depth, m_tag_type == 4 ? m_character_id : -1);
				break;

			case MOVE:
				// reverse of move is move
				LOG_LINE;
				m->move_display_object(
					m_depth,
					m_has_cxform,
					m_color_transform,
					m_has_matrix,
					m_matrix,
					m_ratio,
					m_clip_depth);
				break;

			case REPLACE:
			{
				// reverse of replace is to re-add the previous object.
				LOG_LINE;
				execute_tag*	last_add = m->find_previous_replace_or_add_tag(frame, m_depth, -1);
				LOG_LINE;
				if (last_add)
				{
					LOG_LINE;
					last_add->execute_state(m);
				}
				else
				{
					LOG_LINE;
					log_error("reverse REPLACE can't find previous replace or add tag(%d, %d)\n",
						  frame, m_depth);
					
				}
				break;
			}
			}
		}

		virtual uint32	get_depth_id_of_replace_or_add_tag() const
		// "depth_id" is the 16-bit depth & id packed into one 32-bit int.
		{
			LOG_LINE;
			if (m_place_type == PLACE || m_place_type == REPLACE)
			{
				int	id = -1;
				LOG_LINE;
				if (m_tag_type == 4)
				{
					// Old-style PlaceObject; the corresponding Remove
					// is specific to the character_id.
					LOG_LINE;
					id = m_character_id;
				}
				LOG_LINE;
				return ((m_depth & 0x0FFFF) << 16) | (id & 0x0FFFF);
			}
			else
			{
				LOG_LINE;
				return (uint32) -1;
			}
		}
	};


	
	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 4 || tag_type == 26);

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  place_object_2\n"));

		LOG_LINE;
		place_object_2*	ch = new place_object_2;
		LOG_LINE;
		ch->read(in, tag_type, m->get_version());

		LOG_LINE;
		m->add_execute_tag(ch);
	}



  
	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Create and initialize a sprite, and add it to the movie.
	{
		QASSERT(tag_type == 39);
                
		LOG_LINE;
		int	character_id = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("  sprite\n  char id = %d\n", character_id));

		LOG_LINE;
		sprite_definition*	ch = new sprite_definition(m);	// @@ combine sprite_definition with movie_def_impl
		LOG_LINE;
		ch->read(in);

		LOG_LINE;
		m->add_character(character_id, ch);
	}

	void	define_video_stream(stream* in, int tag_type, movie_definition_sub* m)
	{
		QASSERT(tag_type == 60);

		LOG_LINE;
		int	character_id = in->read_u16();

		IF_VERBOSE_PARSE(log_msg("  video\n  char id = %d\n", character_id));

		LOG_LINE;
		video_definition *	vd = new video_definition(m);
		LOG_LINE;
		vd->read( in );

		LOG_LINE;
		m->add_character( character_id, vd );
	}

	void	read_video_frame(stream *in, int tag_type, movie_definition_sub *m)
	{
		QASSERT(tag_type == 61);

		LOG_LINE;
		int	character_id = in->read_u16();

		LOG_LINE;
		video_definition *	vd = ( video_definition * )( m->get_character_def(character_id) );
		ASSERT( vd != NULL, "Couldn't find video stream for frame" );

		LOG_LINE;
		if ( vd != NULL )
		{
			LOG_LINE;
			vd->add_frame( in );
		}
	}

	//
	// end_tag
	//

	// end_tag doesn't actually need to exist.

	void	end_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 0);
		QASSERT(in->get_position() == in->get_tag_end_position());
	}


	//
	// remove_object_2
	//

	
	struct remove_object_2 : public execute_tag
	{
		int	m_depth, m_id;

		remove_object_2() : m_depth(-1), m_id(-1) {}

		void	read(stream* in, int tag_type)
		{
			LOG_LINE;
			QASSERT(tag_type == 5 || tag_type == 28);

			if (tag_type == 5)
			{
				// Older SWF's allow multiple objects at the same depth;
				// this m_id disambiguates.  Later SWF's just use one
				// object per depth.
				LOG_LINE;
				m_id = in->read_u16();
			}
			LOG_LINE;
			m_depth = in->read_u16();
		}

		virtual void	execute(movie* m)
		{
			LOG_LINE;
			m->remove_display_object(m_depth, m_id);
		}

		virtual void	execute_state(movie* m)
		{
			LOG_LINE;
			execute(m);
		}

		virtual void	execute_state_reverse(movie* m, int frame)
		{
			// reverse of remove is to re-add the previous object.
			LOG_LINE;
			execute_tag*	last_add = m->find_previous_replace_or_add_tag(frame, m_depth, m_id);
			if (last_add)
			{
				LOG_LINE;
				last_add->execute_state(m);
			}
			else
			{
				LOG_LINE;
				log_error("reverse REMOVE can't find previous replace or add tag(%d, %d)\n",
					  frame, m_depth);
					
			}
		}

		virtual bool	is_remove_tag() const { return true; }
	};


	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		QASSERT(tag_type == 5 || tag_type == 28);

		LOG_LINE;
		remove_object_2*	t = new remove_object_2;
		LOG_LINE;
		t->read(in, tag_type);

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  remove_object_2(%d)\n", t->m_depth));

		LOG_LINE;
		m->add_execute_tag(t);
	}


	void	button_sound_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		QASSERT(tag_type == 17);

		LOG_LINE;
		int	button_character_id = in->read_u16();
		LOG_LINE;
		button_character_definition* ch = (button_character_definition*) m->get_character_def(button_character_id);
		QASSERT(ch != NULL);

		LOG_LINE;
		ch->read(in, tag_type, m);
	}


	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		QASSERT(tag_type == 7 || tag_type == 34);

		LOG_LINE;
		int	character_id = in->read_u16();

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  button character loader: char_id = %d\n", character_id));

		LOG_LINE;
		button_character_definition*	ch = new button_character_definition;
		LOG_LINE;
		ch->read(in, tag_type, m);

		LOG_LINE;
		m->add_character(character_id, ch);
	}


	//
	// export
	//


	void	export_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load an export tag (for exposing internal resources of m)
	{
		QASSERT(tag_type == 56);

		LOG_LINE;
		int	count = in->read_u16();

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  export: count = %d\n", count));

		// Read the exports.
		LOG_LINE;
		for (int i = 0; i < count; i++)
		{
			LOG_LINE;
			Uint16	id = in->read_u16();
			LOG_LINE;
			char*	symbol_name = in->read_string();
			LOG_LINE;
			IF_VERBOSE_PARSE(log_msg("  export: id = %d, name = %s\n", id, symbol_name));

			LOG_LINE;
			if (font* f = m->get_font(id))
			{
				// Expose this font for export.
				LOG_LINE;
				m->export_resource(tu_string(symbol_name), f);
			}
			else if (character_def* ch = m->get_character_def(id))
			{
				// Expose this movie/button/whatever for export.
				LOG_LINE;
				m->export_resource(tu_string(symbol_name), ch);
			}
			else if (sound_sample* ch = m->get_sound_sample(id))
			{
				LOG_LINE;
				m->export_resource(tu_string(symbol_name), ch);
			}
			else
			{
				LOG_LINE;
				log_error("export error: don't know how to export resource '%s'\n",
					  symbol_name);
			}

			LOG_LINE;
			delete [] symbol_name;
		}
	}


	//
	// import
	//


	void	import_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Load an import tag (for pulling in external resources)
	{
		QASSERT(tag_type == 57);

		LOG_LINE;
		char*	source_url = in->read_string();
		LOG_LINE;
		int	count = in->read_u16();

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  import: source_url = %s, count = %d\n", source_url, count));

		// Try to load the source movie into the movie library.
		LOG_LINE;
		movie_definition_sub*	source_movie = NULL;

		LOG_LINE;
		if (s_no_recurse_while_loading == false)
		{
			LOG_LINE;
			source_movie = create_library_movie_sub(source_url);
			LOG_LINE;
			if (source_movie == NULL)
			{
				// Give up on imports.
				LOG_LINE;
				log_error("can't import movie from url %s\n", source_url);
				return;
			}
		}

		// Get the imports.
		LOG_LINE;
		for (int i = 0; i < count; i++)
		{
			LOG_LINE;
			Uint16	id = in->read_u16();
			LOG_LINE;
			char*	symbol_name = in->read_string();
			IF_VERBOSE_PARSE(log_msg("  import: id = %d, name = %s\n", id, symbol_name));

			LOG_LINE;
			if (s_no_recurse_while_loading)
			{
				LOG_LINE;
				m->add_import(source_url, id, symbol_name);
			}
			else
			{
				// @@ TODO get rid of this, always use
				// s_no_recurse_while_loading, change
				// create_movie_sub().

				LOG_LINE;
				smart_ptr<resource> res = source_movie->get_exported_resource(symbol_name);
				if (res == NULL)
				{
					LOG_LINE;
					log_error("import error: resource '%s' is not exported from movie '%s'\n",
						  symbol_name, source_url);
				}
				else if (font* f = res->cast_to_font())
				{
					// Add this shared font to the currently-loading movie.
					LOG_LINE;
					m->add_font(id, f);
				}
				else if (character_def* ch = res->cast_to_character_def())
				{
					// Add this character to the loading movie.
					LOG_LINE;
					m->add_character(id, ch);
				}
				else
				{
					LOG_LINE;
					log_error("import error: resource '%s' from movie '%s' has unknown type\n",
						  symbol_name, source_url);
				}
			}

			LOG_LINE;
			delete [] symbol_name;
		}

		LOG_LINE;
		delete [] source_url;
	}
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
