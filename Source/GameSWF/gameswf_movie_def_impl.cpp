// gameswf_movie_def_impl.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "gameswf_font.h"
#include "gameswf_movie_def_impl.h"
#include "base/jpeg.h"
#include "base/tu_file.h"
#include "base/zlib_adapter.h"
#include "gameswf_stream.h"
#include "gameswf_fontlib.h"
#include "gameswf_movie_root.h"
#include "gameswf_sprite.h"

namespace gameswf
{
	extern hash<int, loader_function>	s_tag_loaders;


	//
	// progress callback stuff (from Vitaly)
	//

	static progress_callback	s_progress_function = NULL;

	void	register_progress_callback(progress_callback progress_handle)
		// Host calls this to register a function for progress bar handling
		// during loading movies.
	{
		LOG_LINE;
		s_progress_function = progress_handle;
	}


	static void	dump_tag_bytes(stream* in)
	// Log the contents of the current tag, in hex.
	{
		LOG_LINE;
		static const int	ROW_BYTES = 16;
		LOG_LINE;
		char	row_buf[ROW_BYTES];
		LOG_LINE;
		int	row_count = 0;

		LOG_LINE;
		while(in->get_position() < in->get_tag_end_position())
		{
			LOG_LINE;
			int	c = in->read_u8();
			LOG_LINE;
			log_msg("%02X", c);

			LOG_LINE;
			if (c < 32) c = '.';
			LOG_LINE;
			if (c > 127) c = '.';
			LOG_LINE;
			row_buf[row_count] = c;
			
			LOG_LINE;
			row_count++;
			LOG_LINE;
			if (row_count >= ROW_BYTES)
			{
				LOG_LINE;
				log_msg("    ");
				LOG_LINE;
				for (int i = 0; i < ROW_BYTES; i++)
				{
					LOG_LINE;
					log_msg("%c", row_buf[i]);
				}

				LOG_LINE;
				log_msg("\n");
				LOG_LINE;
				row_count = 0;
			}
			else
			{
				LOG_LINE;
				log_msg(" ");
			}
		}

		LOG_LINE;
		if (row_count > 0)
		{
			LOG_LINE;
			log_msg("\n");
		}
	}





	movie_def_impl::movie_def_impl(create_bitmaps_flag cbf, create_font_shapes_flag cfs)
		:
	m_create_bitmaps(cbf),
		m_create_font_shapes(cfs),
		m_frame_rate(30.0f),
		m_frame_count(0),
		m_version(0),
		m_loading_frame(0),
		m_jpeg_in(0)
	{
		LOG_LINE;
	}

	movie_def_impl::~movie_def_impl()
	{
		// Release our playlist data.
		{
			LOG_LINE;
			for (int i = 0, n = m_playlist.size(); i < n; i++)
			{
				LOG_LINE;
				for (int j = 0, m = m_playlist[i].size(); j < m; j++)
				{
					LOG_LINE;
					delete m_playlist[i][j];
				}
			}
		}

		// Release init action data.
		LOG_LINE;
		{for (int i = 0, n = m_init_action_list.size(); i < n; i++)
		{
			LOG_LINE;
			for (int j = 0, m = m_init_action_list[i].size(); j < m; j++)
			{
				LOG_LINE;
				delete m_init_action_list[i][j];
			}
		}}

		QASSERT(m_jpeg_in == NULL);	// It's supposed to be cleaned up in read()
	}


	movie_interface*	movie_def_impl::create_instance()
		// Create a playable movie instance from a def.
	{
		LOG_LINE;
		movie_root*	m = new movie_root(this);
		QASSERT(m);

		LOG_LINE;
		sprite_instance*	root_movie = new sprite_instance(this, m, NULL, -1);
		QASSERT(root_movie);

		LOG_LINE;
		root_movie->set_name("_root");
		LOG_LINE;
		m->set_root_movie(root_movie);

		LOG_LINE;
		m->add_ref();
		return m;
	}


	// ...
	int	movie_def_impl::get_frame_count() const { return m_frame_count; }
	float	movie_def_impl::get_frame_rate() const { return m_frame_rate; }
	float	movie_def_impl::get_width_pixels() const { return ceilf(TWIPS_TO_PIXELS(m_frame_size.width())); }
	float	movie_def_impl::get_height_pixels() const { return ceilf(TWIPS_TO_PIXELS(m_frame_size.height())); }

	int	movie_def_impl::get_version() const { return m_version; }

	int	movie_def_impl::get_loading_frame() const { return m_loading_frame; }

	uint32	movie_def_impl::get_file_bytes() const { return m_file_length; }

	/* movie_def_impl */
	create_bitmaps_flag	movie_def_impl::get_create_bitmaps() const
		// Returns DO_CREATE_BITMAPS if we're supposed to
		// initialize our bitmap infos, or DO_NOT_INIT_BITMAPS
		// if we're supposed to create blank placeholder
		// bitmaps (to be init'd later explicitly by the host
		// program).
	{
		LOG_LINE;
		return m_create_bitmaps;
	}

	/* movie_def_impl */
	create_font_shapes_flag	movie_def_impl::get_create_font_shapes() const
		// Returns DO_LOAD_FONT_SHAPES if we're supposed to
		// initialize our font shape info, or
		// DO_NOT_LOAD_FONT_SHAPES if we're supposed to not
		// create any (vector) font glyph shapes, and instead
		// rely on precached textured fonts glyphs.
	{
		LOG_LINE;
		return m_create_font_shapes;
	}

	void	movie_def_impl::add_bitmap_info(bitmap_info* bi)
		// All bitmap_info's used by this movie should be
		// registered with this API.
	{
		LOG_LINE;
		m_bitmap_list.push_back(bi);
	}


	int	movie_def_impl::get_bitmap_info_count() const { return m_bitmap_list.size(); }
	bitmap_info*	movie_def_impl::get_bitmap_info(int i) const
	{
		LOG_LINE;
		return m_bitmap_list[i].get_ptr();
	}

	void	movie_def_impl::export_resource(const tu_string& symbol, resource* res)
		// Expose one of our resources under the given symbol,
		// for export.  Other movies can import it.
	{
		// SWF sometimes exports the same thing more than once!
		LOG_LINE;
		m_exports.set(symbol, res);
	}

	smart_ptr<resource>	movie_def_impl::get_exported_resource(const tu_string& symbol)
		// Get the named exported resource, if we expose it.
		// Otherwise return NULL.
	{
		LOG_LINE;
		smart_ptr<resource>	res;
		LOG_LINE;
		m_exports.get(symbol, &res);
		LOG_LINE;
		return res;
	}

	void	movie_def_impl::add_import(const char* source_url, int id, const char* symbol)
		// Adds an entry to a table of resources that need to
		// be imported from other movies.  Client code must
		// call resolve_import() later, when the source movie
		// has been loaded, so that the actual resource can be
		// used.
	{
		LOG_LINE;
		QASSERT(in_import_table(id) == false);

		LOG_LINE;
		m_imports.push_back(import_info(source_url, id, symbol));
	}

	bool	movie_def_impl::in_import_table(int character_id)
		// Debug helper; returns true if the given
		// character_id is listed in the import table.
	{
		LOG_LINE;
		for (int i = 0, n = m_imports.size(); i < n; i++)
		{
			LOG_LINE;
			if (m_imports[i].m_character_id == character_id)
			{
				return true;
			}
		}
		return false;
	}

	void	movie_def_impl::visit_imported_movies(import_visitor* visitor)
		// Calls back the visitor for each movie that we
		// import symbols from.
	{
		LOG_LINE;
		stringi_hash<bool>	visited;	// ugh!

		LOG_LINE;
		for (int i = 0, n = m_imports.size(); i < n; i++)
		{
			LOG_LINE;
			import_info&	inf = m_imports[i];
			LOG_LINE;
			if (visited.find(inf.m_source_url) == visited.end())
			{
				// Call back the visitor.
				LOG_LINE;
				visitor->visit(inf.m_source_url.c_str());
				LOG_LINE;
				visited.set(inf.m_source_url, true);
			}
		}
	}

	void	movie_def_impl::resolve_import(const char* source_url, movie_definition* source_movie)
		// Grabs the stuff we want from the source movie.
	{
		// @@ should be safe, but how can we verify
		// it?  Compare a member function pointer, or
		// something?
		LOG_LINE;
		movie_def_impl*	def_impl = static_cast<movie_def_impl*>(source_movie);
		LOG_LINE;
		movie_definition_sub*	def = static_cast<movie_definition_sub*>(def_impl);

		// Iterate in reverse, since we remove stuff along the way.
		LOG_LINE;
		for (int i = m_imports.size() - 1; i >= 0; i--)
		{
			LOG_LINE;
			const import_info&	inf = m_imports[i];
			LOG_LINE;
			if (inf.m_source_url == source_url)
			{
				// Do the import.
				LOG_LINE;
				smart_ptr<resource> res = def->get_exported_resource(inf.m_symbol);
				bool	 imported = true;

				LOG_LINE;
				if (res == NULL)
				{
					LOG_LINE;
					log_error("import error: resource '%s' is not exported from movie '%s'\n",
						inf.m_symbol.c_str(), source_url);
				}
				else if (font* f = res->cast_to_font())
				{
					// Add this shared font to our fonts.
					LOG_LINE;
					add_font(inf.m_character_id, f);
					LOG_LINE;
					imported = true;
				}
				else if (character_def* ch = res->cast_to_character_def())
				{
					// Add this character to our characters.
					LOG_LINE;
					add_character(inf.m_character_id, ch);
					imported = true;
				}
				else
				{
					LOG_LINE;
					log_error("import error: resource '%s' from movie '%s' has unknown type\n",
						inf.m_symbol.c_str(), source_url);
				}

				LOG_LINE;
				if (imported)
				{
					LOG_LINE;
					m_imports.remove(i);

					// Hold a ref, to keep this source movie_definition alive.
					LOG_LINE;
					m_import_source_movies.push_back(source_movie);
				}
			}
		}
	}

	void	movie_def_impl::add_character(int character_id, character_def* c)
	{
		QASSERT(c);
		LOG_LINE;
		//			BREAK_POINT( "add_character( %d, %X )", character_id, c );
		m_characters.add(character_id, c);
	}

	character_def*	movie_def_impl::get_character_def(int character_id)
	{
		//#ifndef NDEBUG
		// make sure character_id is resolved
		LOG_LINE;
		if (in_import_table(character_id))
		{
			LOG_LINE;
			log_error("get_character_def(): character_id %d is still waiting to be imported\n",
				character_id);
		}
		//#endif // not NDEBUG

		LOG_LINE;
		smart_ptr<character_def>	ch;
		LOG_LINE;
		m_characters.get(character_id, &ch);
		LOG_LINE;
		QASSERT(ch == NULL || ch->get_ref_count() > 1);
		LOG_LINE;
		return ch.get_ptr();
	}

	bool	movie_def_impl::get_labeled_frame(const char* label, int* frame_number)
		// Returns 0-based frame #
	{
		LOG_LINE;
		return m_named_frames.get(label, frame_number);
	}

	void	movie_def_impl::add_font(int font_id, font* f)
	{
		QASSERT(f);
		LOG_LINE;
		m_fonts.add(font_id, f);
	}

	font*	movie_def_impl::get_font(int font_id)
	{
#ifndef NDEBUG
		// make sure font_id is resolved
		LOG_LINE;
		if (in_import_table(font_id))
		{
			LOG_LINE;
			log_error("get_font(): font_id %d is still waiting to be imported\n",
				font_id);
		}
#endif // not NDEBUG

		LOG_LINE;
		smart_ptr<font>	f;
		LOG_LINE;
		m_fonts.get(font_id, &f);
		LOG_LINE;
		QASSERT(f == NULL || f->get_ref_count() > 1);
		LOG_LINE;
		return f.get_ptr();
	}

	bitmap_character_def*	movie_def_impl::get_bitmap_character(int character_id)
	{
		LOG_LINE;
		smart_ptr<bitmap_character_def>	ch;
		LOG_LINE;
		m_bitmap_characters.get(character_id, &ch);
		LOG_LINE;
		QASSERT(ch == NULL || ch->get_ref_count() > 1);
		LOG_LINE;
		return ch.get_ptr();
	}

	void	movie_def_impl::add_bitmap_character(int character_id, bitmap_character_def* ch)
	{
		LOG_LINE;
		QASSERT(ch);
		LOG_LINE;
		m_bitmap_characters.add(character_id, ch);

		LOG_LINE;
		add_bitmap_info(ch->get_bitmap_info());
	}

	sound_sample*	movie_def_impl::get_sound_sample(int character_id)
	{
		LOG_LINE;
		smart_ptr<sound_sample>	ch;
		LOG_LINE;
		m_sound_samples.get(character_id, &ch);
		LOG_LINE;
		QASSERT(ch == NULL || ch->get_ref_count() > 1);
		LOG_LINE;
		return ch.get_ptr();
	}

	void	movie_def_impl::add_sound_sample(int character_id, sound_sample* sam)
	{
		LOG_LINE;
		QASSERT(sam);
		LOG_LINE;
		m_sound_samples.add(character_id, sam);
	}

	void	movie_def_impl::add_execute_tag(execute_tag* e)
	{
		LOG_LINE;
		QASSERT(e);
		LOG_LINE;
		m_playlist[m_loading_frame].push_back(e);
	}

	void	movie_def_impl::add_init_action(int sprite_id, execute_tag* e)
		// Need to execute the given tag before entering the
		// currently-loading frame for the first time.
		//
		// @@ AFAIK, the sprite_id is totally pointless -- correct?
	{
		LOG_LINE;
		QASSERT(e);
		LOG_LINE;
		m_init_action_list[m_loading_frame].push_back(e);
	}

	void	movie_def_impl::add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.  A copy of the name string is made and
		// kept in this object.
	{
		LOG_LINE;
		QASSERT(m_loading_frame >= 0 && m_loading_frame < m_frame_count);

		LOG_LINE;
		tu_string	n = name;
		LOG_LINE;
		QASSERT(m_named_frames.get(n, NULL) == false);	// frame should not already have a name (?)
		LOG_LINE;
		m_named_frames.add(n, m_loading_frame);	// stores 0-based frame #
	}

	void	movie_def_impl::set_jpeg_loader(jpeg::input* j_in)
		// Set an input object for later loading DefineBits
		// images (JPEG images without the table info).
	{
		LOG_LINE;
		QASSERT(m_jpeg_in == NULL);
		LOG_LINE;
		m_jpeg_in = j_in;
	}

	jpeg::input*	movie_def_impl::get_jpeg_loader()
		// Get the jpeg input loader, to load a DefineBits
		// image (one without table info).
	{
		LOG_LINE;
		return m_jpeg_in;
	}


	const array<execute_tag*>&	movie_def_impl::get_playlist(int frame_number)
	{
		LOG_LINE;
		return m_playlist[frame_number];
	}

	/* movie_def_impl */
	const array<execute_tag*>*	movie_def_impl::get_init_actions(int frame_number)
	{
		LOG_LINE;
		return &m_init_action_list[frame_number];
	}

	/* movie_def_impl */
	void	movie_def_impl::read(tu_file* in)
		// Read a .SWF movie.
	{
		LOG_LINE;
		Uint32	file_start_pos = in->get_position();
		LOG_LINE;
		Uint32	header = in->read_le32();
		LOG_LINE;
		m_file_length = in->read_le32();
		LOG_LINE;
		Uint32	file_end_pos = file_start_pos + m_file_length;

		LOG_LINE;
		m_version = (header >> 24) & 255;
		LOG_LINE;
		if ((header & 0x0FFFFFF) != 0x00535746
			&& (header & 0x0FFFFFF) != 0x00535743)
		{
			// ERROR
			LOG_LINE;
			log_error("gameswf::movie_def_impl::read() -- file does not start with a SWF header!\n");
			return;
		}
		LOG_LINE;
		bool	compressed = (header & 255) == 'C';

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("version = %d, file_length = %d\n", m_version, m_file_length));

		tu_file*	original_in = NULL;
		LOG_LINE;
		if (compressed)
		{
#if TU_CONFIG_LINK_TO_ZLIB == 0
			LOG_LINE;
			log_error("movie_def_impl::read(): unable to read zipped SWF data; TU_CONFIG_LINK_TO_ZLIB is 0\n");
			return;
#endif

			LOG_LINE;
			IF_VERBOSE_PARSE(log_msg("file is compressed.\n"));
			original_in = in;

			// Uncompress the input as we read it.
			LOG_LINE;
			in = zlib_adapter::make_inflater(original_in);

			// Subtract the size of the 8-byte header, since
			// it's not included in the compressed
			// stream length.
			LOG_LINE;
			file_end_pos = m_file_length - 8;
		}

		LOG_LINE;
		stream	str(in);

		LOG_LINE;
		m_frame_size.read(&str);
		LOG_LINE;
		m_frame_rate = str.read_u16() / 256.0f;
		LOG_LINE;
		m_frame_count = str.read_u16();

		LOG_LINE;
		m_playlist.resize(m_frame_count);
		LOG_LINE;
		m_init_action_list.resize(m_frame_count);

		IF_VERBOSE_PARSE(m_frame_size.print());
		IF_VERBOSE_PARSE(log_msg("frame rate = %f, frames = %d\n", m_frame_rate, m_frame_count));

		LOG_LINE;
		while ((Uint32) str.get_position() < file_end_pos)
		{
			LOG_LINE;
			int	tag_type = str.open_tag();

			LOG_LINE;
			if (s_progress_function != NULL)
			{
				LOG_LINE;
				s_progress_function((Uint32) str.get_position(), file_end_pos);
			}

			LOG_LINE;
			loader_function	lf = NULL;
			//IF_VERBOSE_PARSE(log_error("tag_type = %d\n", tag_type));
			LOG_LINE;
			if (tag_type == 1)
			{
				// show frame tag -- advance to the next frame.
				LOG_LINE;
				IF_VERBOSE_PARSE(log_msg("  show_frame\n"));
				m_loading_frame++;
			}
			else if (s_tag_loaders.get(tag_type, &lf))
			{
//				BREAK_POINT( "tag_type = %d", tag_type );
				// call the tag loader.  The tag loader should add
				// characters or tags to the movie data structure.
				LOG_LINE;
				(*lf)(&str, tag_type, this);

			}
			else
			{
				LOG_LINE;
				// no tag loader for this tag type.
				IF_VERBOSE_PARSE(log_error("*** no tag loader for type %d\n", tag_type));
				IF_VERBOSE_PARSE(dump_tag_bytes(&str));
			}

			LOG_LINE;
			str.close_tag();

			LOG_LINE;
			if (tag_type == 0)
			{
				LOG_LINE;
				if ((unsigned int) str.get_position() != file_end_pos)
				{
					LOG_LINE;
					// Safety break, so we don't read past the end of the
					// movie.
					log_error("warning: hit stream-end tag, but not at the "
						"end of the file yet; stopping for safety\n");
					break;
				}
			}
		}

		LOG_LINE;
		if (m_jpeg_in)
		{
			LOG_LINE;
			delete m_jpeg_in;
			m_jpeg_in = NULL;
		}

		LOG_LINE;
		if (original_in)
		{
			// Done with the zlib_adapter.
			LOG_LINE;
			delete in;
		}
	}


	/* movie_def_impl */
	void	movie_def_impl::get_owned_fonts(array<font*>* fonts)
		// Fill up *fonts with fonts that we own.
	{
		LOG_LINE;
		QASSERT(fonts);
		LOG_LINE;
		fonts->resize(0);

		LOG_LINE;
		array<int>	font_ids;

		LOG_LINE;
		for (hash<int, smart_ptr<font> >::iterator it = m_fonts.begin();
			it != m_fonts.end();
			++it)
		{
			LOG_LINE;
			font*	f = it->second.get_ptr();
			LOG_LINE;
			if (f->get_owning_movie() == this)
			{
				// Sort by character id, so the ordering is
				// consistent for cache read/write.
				LOG_LINE;
				int	id = it->first;

				// Insert in correct place.
				LOG_LINE;
				int	insert;
				for (insert = 0; insert < font_ids.size(); insert++)
				{
					LOG_LINE;
					if (font_ids[insert] > id)
					{
						// We want to insert here.
						LOG_LINE;
						break;
					}
				}
				LOG_LINE;
				fonts->insert(insert, f);
				LOG_LINE;
				font_ids.insert(insert, id);
			}
		}
	}


	/* movie_def_impl */
	void	movie_def_impl::generate_font_bitmaps()
		// Generate bitmaps for our fonts, if necessary.
	{
		// Collect list of fonts.
		array<font*>	fonts;
		LOG_LINE;
		get_owned_fonts(&fonts);
		LOG_LINE;
		fontlib::generate_font_bitmaps(fonts, this);
	}


	// Increment this when the cache data format changes.
#define CACHE_FILE_VERSION 4


	/* movie_def_impl */
	void	movie_def_impl::output_cached_data(tu_file* out, const cache_options& options)
		// Dump our cached data into the given stream.
	{
		// Write a little header.
		char	header[5];
		LOG_LINE;
		strcpy(header, "gscX");
		LOG_LINE;
		header[3] = CACHE_FILE_VERSION;
		LOG_LINE;
		compiler_assert(CACHE_FILE_VERSION < 256);

		LOG_LINE;
		out->write_bytes(header, 4);

		// Write font data.
		LOG_LINE;
		array<font*>	fonts;
		LOG_LINE;
		get_owned_fonts(&fonts);
		LOG_LINE;
		fontlib::output_cached_data(out, fonts, this, options);

		// Write character data.
		LOG_LINE;
		{for (hash<int, smart_ptr<character_def> >::iterator it = m_characters.begin();
		it != m_characters.end();
		++it)
		{
			LOG_LINE;
			out->write_le16(it->first);
			LOG_LINE;
			it->second->output_cached_data(out, options);
		}}

		LOG_LINE;
		out->write_le16((Sint16) -1);	// end of characters marker
	}


	/* movie_def_impl */
	void	movie_def_impl::input_cached_data(tu_file* in)
		// Read in cached data and use it to prime our loaded characters.
	{
		// Read the header & check version.
		LOG_LINE;
		unsigned char	header[4];
		LOG_LINE;
		in->read_bytes(header, 4);
		LOG_LINE;
		if (header[0] != 'g' || header[1] != 's' || header[2] != 'c')
		{
			LOG_LINE;
			log_error("cache file does not have the correct format; skipping\n");
			return;
		}
		else if (header[3] != CACHE_FILE_VERSION)
		{
			LOG_LINE;
			log_error(
				"cached data is version %d, but we require version %d; skipping\n",
				int(header[3]), CACHE_FILE_VERSION);
			return;
		}

		// Read the cached font data.
		LOG_LINE;
		array<font*>	fonts;
		LOG_LINE;
		get_owned_fonts(&fonts);
		LOG_LINE;
		fontlib::input_cached_data(in, fonts, this);

		// Read the cached character data.
		LOG_LINE;
		for (;;)
		{
			LOG_LINE;
			if (in->get_error() != TU_FILE_NO_ERROR)
			{
				LOG_LINE;
				log_error("error reading cache file (characters); skipping\n");
				return;
			}
			LOG_LINE;
			if (in->get_eof())
			{
				LOG_LINE;
				log_error("unexpected eof reading cache file (characters); skipping\n");
				return;
			}

			LOG_LINE;
			Sint16	id = in->read_le16();
			LOG_LINE;
			if (id == (Sint16) -1) { break; }	// done

			LOG_LINE;
			smart_ptr<character_def> ch;
			LOG_LINE;
			m_characters.get(id, &ch);
			LOG_LINE;
			if (ch != NULL)
			{
				LOG_LINE;
				ch->input_cached_data(in);
			}
			else
			{
				LOG_LINE;
				log_error("sync error in cache file (reading characters)!  "
					"Skipping rest of cache data.\n");
				return;
			}
		}
	}

}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
