// gameswf_font.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A font type for gameswf.


#include "gameswf_font.h"
#include "gameswf_stream.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_shape.h"
#include "base/tu_file.h"


extern s32	movie_version;

namespace gameswf
{
	font::font()
		:
		m_texture_glyph_nominal_size(96),	// Default is not important; gets overridden during glyph generation
		m_name(NULL),
		m_owning_movie(NULL),
		m_unicode_chars(false),
		m_shift_jis_chars(false),
		m_ansi_chars(true),
		m_is_italic(false),
		m_is_bold(false),
		m_wide_codes(false),
		m_ascent(0.0f),
		m_descent(0.0f),
		m_leading(0.0f)
	{
	}

	font::~font()
	{
		m_glyphs.resize(0);

		// Delete the name string.
		if (m_name)
		{
			delete [] m_name;
			m_name = NULL;
		}
	}

	shape_character_def*	font::get_glyph(int index) const
	{
		if (index >= 0 && index < m_glyphs.size())
		{
			return m_glyphs[index].get_ptr();
		}
		else
		{
			return NULL;
		}
	}


	const texture_glyph&	font::get_texture_glyph(int glyph_index) const
	// Return a pointer to a texture_glyph struct corresponding to
	// the given glyph_index, if we have one.  Otherwise return NULL.
	{
		if (glyph_index < 0 || glyph_index >= m_texture_glyphs.size())
		{
			static const texture_glyph	s_dummy_texture_glyph;
			return s_dummy_texture_glyph;
		}

		return m_texture_glyphs[glyph_index];
	}


	void	font::add_texture_glyph(int glyph_index, const texture_glyph& glyph)
	// Register some texture info for the glyph at the specified
	// index.  The texture_glyph can be used later to render the
	// glyph.
	{
		QASSERT(glyph_index >= 0 && glyph_index < m_glyphs.size());
		QASSERT(m_texture_glyphs.size() == m_glyphs.size());
		QASSERT(glyph.is_renderable());

		QASSERT(m_texture_glyphs[glyph_index].is_renderable() == false);

		m_texture_glyphs[glyph_index] = glyph;
	}


	void	font::wipe_texture_glyphs()
	// Delete all our texture glyph info.
	{
		QASSERT(m_texture_glyphs.size() == m_glyphs.size());

		// Replace with default (empty) glyph info.
		texture_glyph	default_tg;
		for (int i = 0, n = m_texture_glyphs.size(); i < n; i++)
		{
			m_texture_glyphs[i] = default_tg;
		}
	}


	void	font::read(stream* in, int tag_type, movie_definition_sub* m)
	{
		QASSERT(tag_type == 10 || tag_type == 48);

		// No add_ref() here, to avoid cycle.  m_owning_movie is our owner, so it has a ref to us.
		m_owning_movie = m;

		if (tag_type == 10)
		{
			IF_VERBOSE_PARSE(log_msg("reading DefineFont\n"));

			int	table_base = in->get_position();

			// Read the glyph offsets.  Offsets
			// are measured from the start of the
			// offset table.
			array<int>	offsets;
			offsets.push_back(in->read_u16());
			IF_VERBOSE_PARSE(log_msg("offset[0] = %d\n", offsets[0]));
			int	count = offsets[0] >> 1;
			for (int i = 1; i < count; i++)
			{
				offsets.push_back(in->read_u16());
				IF_VERBOSE_PARSE(log_msg("offset[%d] = %d\n", i, offsets[i]));
			}

			m_glyphs.resize(count);
			m_texture_glyphs.resize(m_glyphs.size());

			if (m->get_create_font_shapes() == DO_LOAD_FONT_SHAPES)
			{
				// Read the glyph shapes.
				{for (int i = 0; i < count; i++)
				{
					// Seek to the start of the shape data.
					int	new_pos = table_base + offsets[i];
					in->set_position(new_pos);

					// Create & read the shape.
					shape_character_def* s = new shape_character_def;
					s->read(in, 2, false, m);

					m_glyphs[i] = s;
				}}
			}
		}
		else if (tag_type == 48)
		{
			IF_VERBOSE_PARSE(log_msg("reading DefineFont2\n"));

			bool	has_layout = (in->read_uint(1) != 0);
			m_shift_jis_chars = (in->read_uint(1) != 0);
			m_unicode_chars = (in->read_uint(1) != 0);
			m_ansi_chars = (in->read_uint(1) != 0);
			bool	wide_offsets = (in->read_uint(1) != 0);
			m_wide_codes = (in->read_uint(1) != 0);
			m_is_italic = (in->read_uint(1) != 0);
			m_is_bold = (in->read_uint(1) != 0);
			Uint8	reserved = in->read_u8();

			// Inhibit warning.
			reserved = reserved;

			m_name = in->read_string_with_length();

			int	glyph_count = in->read_u16();
			
			int	table_base = in->get_position();

			// Read the glyph offsets.  Offsets
			// are measured from the start of the
			// offset table.
			array<int>	offsets;
			int	font_code_offset;
			if (wide_offsets)
			{
				// 32-bit offsets.
				for (int i = 0; i < glyph_count; i++)
				{
					offsets.push_back(in->read_u32());
				}
				font_code_offset = in->read_u32();
			}
			else
			{
				// 16-bit offsets.
				for (int i = 0; i < glyph_count; i++)
				{
					offsets.push_back(in->read_u16());
				}
				font_code_offset = in->read_u16();
			}

			m_glyphs.resize(glyph_count);
			m_texture_glyphs.resize(m_glyphs.size());

			if (m->get_create_font_shapes() == DO_LOAD_FONT_SHAPES)
			{
				// Read the glyph shapes.
				{for (int i = 0; i < glyph_count; i++)
				{
					// Seek to the start of the shape data.
					int	new_pos = table_base + offsets[i];
					// if we're seeking backwards, then that looks like a bug.
					QASSERT(new_pos >= in->get_position());
					in->set_position(new_pos);

					// Create & read the shape.
					shape_character_def* s = new shape_character_def;
					s->read(in, 22, false, m);

					m_glyphs[i] = s;
				}}

				int	current_position = in->get_position();
				if (font_code_offset + table_base != current_position)
				{
					// Bad offset!  Don't try to read any more.
					return;
				}
			}
			else
			{
				// Skip the shape data.
				int	new_pos = table_base + font_code_offset;
				if (new_pos >= in->get_tag_end_position())
				{
					// No layout data!
					return;
				}

				in->set_position(new_pos);
			}

			read_code_table(in);

			// Read layout info for the glyphs.
			if (has_layout)
			{
				m_ascent = (float) in->read_s16();
				m_descent = (float) in->read_s16();
				m_leading = (float) in->read_s16();
				
				// Advance table; i.e. how wide each character is.
				m_advance_table.resize(m_glyphs.size());
				for (int i = 0, n = m_advance_table.size(); i < n; i++)
				{
					m_advance_table[i] = (float) in->read_s16();
				}

				// Bounds table.
				//m_bounds_table.resize(m_glyphs.size());	// kill
				rect	dummy_rect;
				{for (int i = 0, n = m_glyphs.size(); i < n; i++)
				{
					//m_bounds_table[i].read(in);	// kill
					dummy_rect.read(in);
				}}

				// Kerning pairs.
				int	kerning_count = in->read_u16();
				{for (int i = 0; i < kerning_count; i++)
				{
					Uint16	char0, char1;
					if (m_wide_codes)
					{
						char0 = in->read_u16();
						char1 = in->read_u16();
					}
					else
					{
						char0 = in->read_u8();
						char1 = in->read_u8();
					}
					float	adjustment = (float) in->read_s16();

					kerning_pair	k;
					k.m_char0 = char0;
					k.m_char1 = char1;

					// Remember this adjustment; we can look it up quickly
					// later using the character pair as the key.
					m_kerning_pairs.add(k, adjustment);
				}}
			}
		}
	}


	void	font::read_font_info(stream* in)
	// Read additional information about this font, from a
	// DefineFontInfo tag.  The caller has already read the tag
	// type and font id.
	{
		if (m_name)
		{
			delete m_name;
			m_name = NULL;
		}

		m_name = in->read_string_with_length();

		unsigned char	flags = in->read_u8();
		m_unicode_chars = (flags & 0x20) != 0;
		m_shift_jis_chars = (flags & 0x10) != 0;
		m_ansi_chars = (flags & 0x08) != 0;
		m_is_italic = (flags & 0x04) != 0;
		m_is_bold = (flags & 0x02) != 0;
		m_wide_codes = (flags & 0x01) != 0;

		if ( movie_version >= 6 )
		{
			in->read_u8();
		}

		read_code_table(in);
	}


	void	font::read_code_table(stream* in)
	// Read the table that maps from glyph indices to character
	// codes.
	{
		IF_VERBOSE_PARSE(log_msg("reading code table at offset %d\n", in->get_position()));

		QASSERT(m_code_table.is_empty());

		if (m_wide_codes)
		{
			// Code table is made of Uint16's.
			for (int i = 0; i < m_glyphs.size(); i++)
			{
				m_code_table.add(in->read_u16(), i);
			}
		}
		else
		{
			// Code table is made of bytes.
			for (int i = 0; i < m_glyphs.size(); i++)
			{
				m_code_table.add(in->read_u8(), i);
			}
		}
	}

	int	font::get_glyph_index(Uint16 code) const
	{
		int glyph_index;
		if (m_code_table.get(code, &glyph_index))
		{
			return glyph_index;
		}
		return -1;
	}

	float	font::get_advance(int glyph_index) const
	{
		if (glyph_index == -1)
		{
			// Default advance.
			return 512.0f;
		}

		if (m_advance_table.size() == 0)
		{
			// No layout info for this font!!!
			static bool	s_logged = false;
			if (s_logged == false)
			{
				s_logged = true;
				log_error("error: empty advance table in font %s\n", get_name());
			}
			return 0;
		}

		if (glyph_index < m_advance_table.size())
		{
			QASSERT(glyph_index >= 0);
			return m_advance_table[glyph_index];
		}
		else
		{
			// Bad glyph index.  Due to bad data file?
			QASSERT(0);
			return 0;
		}
	}


	float	font::get_kerning_adjustment(int last_code, int code) const
	// Return the adjustment in advance between the given two
	// characters.  Normally this will be 0; i.e. the 
	{
		float	adjustment;
		kerning_pair	k;
		k.m_char0 = last_code;
		k.m_char1 = code;
		if (m_kerning_pairs.get(k, &adjustment))
		{
			return adjustment;
		}
		return 0;
	}


	void	font::output_cached_data(tu_file* out, const cache_options& options)
	// Dump our cached data into the given stream.
	{
// @@ Disabled.  Need to fix input_cached_data, so that it has a
// reliable and cheap way to skip over data for NULL glyphs.
#if 0
		// Dump cached shape data for glyphs (i.e. this will
		// be tesselations used to render larger glyph sizes).
		int	 n = m_glyphs.size();
		out->write_le32(n);
		for (int i = 0; i < n; i++)
		{
			shape_character_def*	s = m_glyphs[i].get_ptr();
			if (s)
			{
				s->output_cached_data(out, options);
			}
		}
#endif // 0
	}

	
	void	font::input_cached_data(tu_file* in)
	// Read our cached data from the given stream.
	{
// @@ Disable.  See comment in output_cached_data().
#if 0
		// Read cached shape data for glyphs.
		int	n = in->read_le32();
		if (n != m_glyphs.size())
		{
			log_error("error reading cache file in font::input_cached_data() "
				  "glyph count mismatch.\n");
			in->go_to_end();	// ensure that no more data will be read from this stream.
			return;
		}

		for (int i = 0; i < n; i++)
		{
			m_glyphs[i]->input_cached_data(in);
		}
#endif // 0
	}


};	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
