// gameswf_text.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code for the text tags.


#include "base/utf8.h"
#include "base/utility.h"
#include "gameswf_impl.h"
#include "gameswf_shape.h"
#include "gameswf_stream.h"
#include "gameswf_log.h"
#include "gameswf_font.h"
#include "gameswf_fontlib.h"
#include "gameswf_render.h"
#include "gameswf_textformat.h"

namespace gameswf
{
	//
	// text_character
	//


	// Helper struct.
	struct text_style
	{
		int	m_font_id;
		mutable font*	m_font;
		rgba	m_color;
		float	m_x_offset;
		float	m_y_offset;
		float	m_text_height;
		bool	m_has_x_offset;
		bool	m_has_y_offset;

		text_style()
			:
			m_font_id(-1),
			m_font(NULL),
			m_x_offset(0),
			m_y_offset(0),
			m_text_height(1.0f),
			m_has_x_offset(false),
			m_has_y_offset(false)
		{
			LOG_LINE;
		}

		void	resolve_font(movie_definition_sub* root_def) const
		{
			LOG_LINE;
			if (m_font == NULL)
			{
				QASSERT(m_font_id >= 0);

				LOG_LINE;
				m_font = root_def->get_font(m_font_id);
				if (m_font == NULL)
				{
					log_error("error: text style with undefined font; font_id = %d\n", m_font_id);
				}
			}
		}
	};


	// Helper struct.
	struct text_glyph_record
	{
		struct glyph_entry
		{
			int	m_glyph_index;
			float	m_glyph_advance;
		};
		text_style	m_style;
		array<glyph_entry>	m_glyphs;

		void	read(stream* in, int glyph_count, int glyph_bits, int advance_bits)
		{
			LOG_LINE;
			m_glyphs.resize(glyph_count);
			for (int i = 0; i < glyph_count; i++)
			{
				LOG_LINE;
				m_glyphs[i].m_glyph_index = in->read_uint(glyph_bits);
				LOG_LINE;
				m_glyphs[i].m_glyph_advance = (float) in->read_sint(advance_bits);
			}
		}
	};


	// Render the given glyph records.
	static void	display_glyph_records(
		const matrix& this_mat,
		character* inst,
		const array<text_glyph_record>& records,
		movie_definition_sub* root_def)
	{
		static array<fill_style>	s_dummy_style;	// used to pass a color on to shape_character::display()
		static array<line_style>	s_dummy_line_style;
		LOG_LINE;
		s_dummy_style.resize(1);

		LOG_LINE;
		matrix	mat = inst->get_world_matrix();
		LOG_LINE;
		mat.concatenate(this_mat);

		LOG_LINE;
		cxform	cx = inst->get_world_cxform();
		LOG_LINE;
		float	pixel_scale = inst->get_pixel_scale();

//		display_info	sub_di = di;
//		sub_di.m_matrix.concatenate(mat);

//		matrix	base_matrix = sub_di.m_matrix;
		LOG_LINE;
		matrix	base_matrix = mat;
		LOG_LINE;
		float	base_matrix_max_scale = base_matrix.get_max_scale();

		float	scale = 1.0f;
		float	x = 0.0f;
		float	y = 0.0f;

		LOG_LINE;
		for (int i = 0; i < records.size(); i++)
		{
			// Draw the characters within the current record; i.e. consecutive
			// chars that share a particular style.
			LOG_LINE;
			const text_glyph_record&	rec = records[i];

			LOG_LINE;
			rec.m_style.resolve_font(root_def);

			LOG_LINE;
			font*	fnt = rec.m_style.m_font;
			if (fnt == NULL)
			{
				continue;
			}

			LOG_LINE;
			scale = rec.m_style.m_text_height / 1024.0f;	// the EM square is 1024 x 1024
			float	text_screen_height = base_matrix_max_scale
				* scale
				* 1024.0f
				/ 20.0f
				* pixel_scale;

			LOG_LINE;
			int	nominal_glyph_height = fnt->get_texture_glyph_nominal_size();
			LOG_LINE;
			float	max_glyph_height = fontlib::get_texture_glyph_max_height(fnt);
#ifdef GAMESWF_ALWAYS_USE_TEXTURES_FOR_TEXT_WHEN_POSSIBLE
			const bool	use_glyph_textures = true;
#else
			bool	use_glyph_textures =
				text_screen_height <= max_glyph_height * 1.0f;
#endif

			if (rec.m_style.m_has_x_offset)
			{
				x = rec.m_style.m_x_offset;
			}
			if (rec.m_style.m_has_y_offset)
			{
				y = rec.m_style.m_y_offset;
			}

			LOG_LINE;
			s_dummy_style[0].set_color(rec.m_style.m_color);

			LOG_LINE;
			rgba	transformed_color = cx.transform(rec.m_style.m_color);

			LOG_LINE;
			for (int j = 0; j < rec.m_glyphs.size(); j++)
			{
				LOG_LINE;
				int	index = rec.m_glyphs[j].m_glyph_index;
					
				LOG_LINE;
				mat = base_matrix;
				mat.concatenate_translation(x, y);
				mat.concatenate_scale(scale);

				if (index == -1)
				{
					LOG_LINE;
					// Invalid glyph; render it as an empty box.
					render::set_matrix(mat);
					render::line_style_color(transformed_color);

					// The EM square is 1024x1024, but usually isn't filled up.
					// We'll use about half the width, and around 3/4 the height.
					// Values adjusted by eye.
					// The Y baseline is at 0; negative Y is up.
					static const Sint16	s_empty_char_box[5 * 2] =
					{
						 32,   32,
						480,   32,
						480, -656,
						 32, -656,
						 32,   32
					};
					render::draw_line_strip(s_empty_char_box, 5);
				}
				else
				{
					LOG_LINE;
					const texture_glyph&	tg = fnt->get_texture_glyph(index);
					shape_character_def*	glyph = fnt->get_glyph(index);

					if (tg.is_renderable()
					    && (use_glyph_textures || glyph == NULL))
					{
						fontlib::draw_glyph(mat, tg, transformed_color, nominal_glyph_height);
					}
					else
					{

						// Draw the character using the filled outline.
						if (glyph)
						{
							glyph->display(mat, cx, pixel_scale, s_dummy_style, s_dummy_line_style);
						}
					}
				}
				x += rec.m_glyphs[j].m_glyph_advance;
			}
		}
	}


	struct text_character_def : public character_def
	{
		movie_definition_sub*	m_root_def;
		rect	m_rect;
		matrix	m_matrix;
		array<text_glyph_record>	m_text_glyph_records;

		text_character_def(movie_definition_sub* root_def)
			:
			m_root_def(root_def)
		{
			LOG_LINE;
			QASSERT(m_root_def);
		}

		void	read(stream* in, int tag_type, movie_definition_sub* m)
		{
			QASSERT(m != NULL);
			QASSERT(tag_type == 11 || tag_type == 33);

			LOG_LINE;
			m_rect.read(in);
			m_matrix.read(in);

			int	glyph_bits = in->read_u8();
			int	advance_bits = in->read_u8();

			IF_VERBOSE_PARSE(log_msg("begin text records\n"));

			bool	last_record_was_style_change = false;

			text_style	style;
			for (;;)
			{
				int	first_byte = in->read_u8();
				
				if (first_byte == 0)
				{
					// This is the end of the text records.
					IF_VERBOSE_PARSE(log_msg("end text records\n"));
					break;
				}

				// Style changes and glyph records just alternate.
				// (Contrary to what most SWF references say!)
				if (last_record_was_style_change == false)
				{
					// This is a style change.

					last_record_was_style_change = true;

					bool	has_font = (first_byte >> 3) & 1;
					bool	has_color = (first_byte >> 2) & 1;
					bool	has_y_offset = (first_byte >> 1) & 1;
					bool	has_x_offset = (first_byte >> 0) & 1;

					IF_VERBOSE_PARSE(log_msg("  text style change\n"));

					if (has_font)
					{
						Uint16	font_id = in->read_u16();
						style.m_font_id = font_id;
						IF_VERBOSE_PARSE(log_msg("  has_font: font id = %d\n", font_id));
					}
					if (has_color)
					{
						if (tag_type == 11)
						{
							style.m_color.read_rgb(in);
						}
						else
						{
							QASSERT(tag_type == 33);
							style.m_color.read_rgba(in);
						}
						IF_VERBOSE_PARSE(log_msg("  has_color\n"));
					}
					if (has_x_offset)
					{
						style.m_has_x_offset = true;
						style.m_x_offset = in->read_s16();
						IF_VERBOSE_PARSE(log_msg("  has_x_offset = %g\n", style.m_x_offset));
					}
					else
					{
						style.m_has_x_offset = false;
						style.m_x_offset = 0.0f;
					}
					if (has_y_offset)
					{
						style.m_has_y_offset = true;
						style.m_y_offset = in->read_s16();
						IF_VERBOSE_PARSE(log_msg("  has_y_offset = %g\n", style.m_y_offset));
					}
					else
					{
						style.m_has_y_offset = false;
						style.m_y_offset = 0.0f;
					}
					if (has_font)
					{
						style.m_text_height = in->read_u16();
						IF_VERBOSE_PARSE(log_msg("  text_height = %g\n", style.m_text_height));
					}
				}
				else
				{
					// Read the glyph record.

 					last_record_was_style_change = false;

					int	glyph_count = first_byte;

// 					if (! last_record_was_style_change)
// 					{
// 						glyph_count &= 0x7F;
// 					}
// 					// else { Don't mask the top bit; the first record is allowed to have > 127 glyphs. }

					m_text_glyph_records.resize(m_text_glyph_records.size() + 1);
					m_text_glyph_records.back().m_style = style;
					m_text_glyph_records.back().read(in, glyph_count, glyph_bits, advance_bits);

					IF_VERBOSE_PARSE(log_msg("  glyph_records: count = %d\n", glyph_count));
				}
			}
		}


		void	display(character* inst)
		// Draw the string.
		{
			LOG_LINE;
			display_glyph_records(m_matrix, inst, m_text_glyph_records, m_root_def);
		}
	};


	void	define_text_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Read a DefineText tag.
	{
		LOG_LINE;
		QASSERT(tag_type == 11 || tag_type == 33);

		Uint16	character_id = in->read_u16();
		
		LOG_LINE;
		text_character_def*	ch = new text_character_def(m);
		IF_VERBOSE_PARSE(log_msg("text_character, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		// IF_VERBOSE_PARSE(print some stuff);

		LOG_LINE;
		m->add_character(character_id, ch);
	}


	//
	// edit_text_character_def
	//


	struct edit_text_character_def : public character_def
	// A definition for a text display character, whose text can
	// be changed at runtime (by script or host).
	{
		movie_definition_sub*	m_root_def;
		rect			m_rect;
		tu_string		m_default_name;
		text_format		m_format;
		bool			m_word_wrap;
		bool			m_multiline;
		bool			m_password;	// show asterisks instead of actual characters
		bool			m_readonly;
		bool			m_auto_size;	// resize our bound to fit the text
		bool			m_no_select;
		bool			m_border;	// forces white background and black border -- silly, but sometimes used
		bool			m_html;

		// Allowed HTML (from Alexi's SWF Reference):
		//
		// <a href=url target=targ>...</a> -- hyperlink
		// <b>...</b> -- bold
		// <br> -- line break
		// <font face=name size=[+|-][0-9]+ color=#RRGGBB>...</font>  -- font change; size in TWIPS
		// <i>...</i> -- italic
		// <li>...</li> -- list item
		// <p>...</p> -- paragraph
		// <tab> -- insert tab
		// <TEXTFORMAT>  </TEXTFORMAT>
		//   [ BLOCKINDENT=[0-9]+ ]
		//   [ INDENT=[0-9]+ ]
		//   [ LEADING=[0-9]+ ]
		//   [ LEFTMARGIN=[0-9]+ ]
		//   [ RIGHTMARGIN=[0-9]+ ]
		//   [ TABSTOPS=[0-9]+{,[0-9]+} ]
		//
		// Change the different parameters as indicated. The
		// sizes are all in TWIPs. There can be multiple
		// positions for the tab stops. These are seperated by
		// commas.
		// <U>...</U> -- underline


		bool	m_use_outlines;	// when true, use specified SWF internal font.  Otherwise, renderer picks a default font

		int	m_font_id;
		font*	m_font;
		float	m_text_height;

		rgba	m_color;
		int	m_max_length;

		enum alignment
		{
			ALIGN_LEFT = 0,
			ALIGN_RIGHT,
			ALIGN_CENTER,
			ALIGN_JUSTIFY	// probably don't need to implement...
		};
		alignment	m_alignment;
		
		float	m_left_margin;	// extra space between box border and text
		float	m_right_margin;
		float	m_indent;	// how much to indent the first line of multiline text
		float	m_leading;	// extra space between lines (in addition to default font line spacing)
		tu_string	m_default_text;

		edit_text_character_def(movie_definition_sub* root_def)
			:
			m_root_def(root_def),
			m_word_wrap(false),
			m_multiline(false),
			m_password(false),
			m_readonly(false),
			m_auto_size(false),
			m_no_select(false),
			m_border(false),
			m_html(false),
			m_use_outlines(false),
			m_font_id(-1),
			m_font(NULL),
			m_text_height(1.0f),
			m_max_length(0),
			m_alignment(ALIGN_LEFT),
			m_left_margin(0.0f),
			m_right_margin(0.0f),
			m_indent(0.0f),
			m_leading(0.0f)
		{
			QASSERT(m_root_def);

			LOG_LINE;
			m_color.set(0, 0, 0, 255);
		}

		// Set the format of the text
		void	set_format(text_format &format)
		{
			m_format = format;
		}
		
		~edit_text_character_def()
		{
		}


		character*	create_character_instance(movie* parent, int id);


		void	read(stream* in, int tag_type, movie_definition_sub* m)
		{
			QASSERT(m != NULL);
			QASSERT(tag_type == 37);

			LOG_LINE;
			m_rect.read(in);

			in->align();
			bool	has_text = in->read_uint(1) ? true : false;
			m_word_wrap = in->read_uint(1) ? true : false;
			m_multiline = in->read_uint(1) ? true : false;
			m_password = in->read_uint(1) ? true : false;
			m_readonly = in->read_uint(1) ? true : false;
			bool	has_color = in->read_uint(1) ? true : false;
			bool	has_max_length = in->read_uint(1) ? true : false;
			bool	has_font = in->read_uint(1) ? true : false;

			in->read_uint(1);	// reserved
			m_auto_size = in->read_uint(1) ? true : false;
			bool	has_layout = in->read_uint(1) ? true : false;
			m_no_select = in->read_uint(1) ? true : false;
			m_border = in->read_uint(1) ? true : false;
			in->read_uint(1);	// reserved
			m_html = in->read_uint(1) ? true : false;
			m_use_outlines = in->read_uint(1) ? true : false;

			if (has_font)
			{
				m_font_id = in->read_u16();
				m_text_height = (float) in->read_u16();
			}

			if (has_color)
			{
				m_color.read_rgba(in);
			}

			if (has_max_length)
			{
				m_max_length = in->read_u16();
			}

			if (has_layout)
			{
				m_alignment = (alignment) in->read_u8();
				m_left_margin = (float) in->read_u16();
				m_right_margin = (float) in->read_u16();
				m_indent = (float) in->read_s16();
				m_leading = (float) in->read_s16();
			}

			char*	name = in->read_string();
			m_default_name = name;
			delete [] name;

			if (has_text)
			{
				char*	str = in->read_string();
				m_default_text = str;
				delete [] str;
			}

			IF_VERBOSE_PARSE(log_msg("edit_text_char, varname = %s, text = %s\n",
						 m_default_name.c_str(), m_default_text.c_str()));
		}
	};


	//
	// edit_text_character
	//


	struct edit_text_character : public character
	{
		edit_text_character_def*	m_def;
		array<text_glyph_record>	m_text_glyph_records;
		array<fill_style>	m_dummy_style;	// used to pass a color on to shape_character::display()
		array<line_style>	m_dummy_line_style;
		rect	m_text_bounding_box;	// bounds of dynamic text, as laid out

		tu_string	m_text;

		edit_text_character(movie* parent, edit_text_character_def* def, int id)
			:
			character(parent, id),
			m_def(def)
		{
			LOG_LINE;
			QASSERT(parent);
			QASSERT(m_def);

			set_text_value(m_def->m_default_text.c_str());

			m_dummy_style.push_back(fill_style());

			reset_bounding_box(0, 0);
		}

		~edit_text_character()
		{
		}

		virtual const char*	get_text_name() const { return m_def->m_default_name.c_str(); }


		void	reset_bounding_box(float x, float y)
		// Reset our text bounding box to the given point.
		{
			LOG_LINE;
			m_text_bounding_box.m_x_min = x;
			m_text_bounding_box.m_x_max = x;
			m_text_bounding_box.m_y_min = y;
			m_text_bounding_box.m_y_max = y;
		}


		virtual void	set_text_value(const char* new_text)
		// Set our text to the given string.
		{
			LOG_LINE;
			if (m_text == new_text)
			{
				return;
			}

			m_text = new_text;
			if (m_def->m_max_length > 0
			    && m_text.length() > m_def->m_max_length)
			{
				m_text.resize(m_def->m_max_length);
			}

			format_text();
		}

		virtual const char*	get_text_value() const
		{
			return m_text.c_str();
		}


		void	set_member(const tu_stringi& name, const as_value& val)
		// We have a "text" member.
		{
			// @@ TODO need to inherit basic stuff like _x, _y, _xscale, _yscale etc

			LOG_LINE;
			as_standard_member	std_member = get_standard_member(name);
			switch (std_member)
			{
			default:
			case M_INVALID_MEMBER:
				break;
			case M_TEXT:
				//if (name == "text")
			{
				LOG_LINE;
				int version = get_parent()->get_movie_definition()->get_version();
				set_text_value(val.to_tu_string_versioned(version));
				return;
			}
			case M_X:
				//else if (name == "_x")
			{
				LOG_LINE;
				matrix	m = get_matrix();
				m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.to_number());
				set_matrix(m);

				// m_accept_anim_moves = false;
				
				return;
			}
			case M_Y:
				//else if (name == "_y")
			{
				LOG_LINE;
				matrix	m = get_matrix();
				m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.to_number());
				set_matrix(m);

				// m_accept_anim_moves = false;
				
				return;
			}
			case M_VISIBLE:
				//else if (name == "_visible")
			{
				LOG_LINE;
				set_visible(val.to_bool());
				return;
			}
			case M_ALPHA:
				//else if (name == "_alpha")
			{
				// @@ TODO this should be generic to struct character!
				// Arg is in percent.
				LOG_LINE;
				cxform	cx = get_cxform();
				cx.m_[3][0] = fclamp(float(val.to_number()) / 100.f, 0, 1);
				set_cxform(cx);
				return;
			}
			case M_TEXTCOLOR:
				//else if (name == "textColor")
			{	
				// The arg is 0xRRGGBB format.
				LOG_LINE;
				Uint32	rgb = (Uint32) val.to_number();

				cxform	cx = get_cxform();
				cx.m_[0][0] = fclamp(((rgb >> 16) & 255) / 255.0f, 0, 1);
				cx.m_[1][0] = fclamp(((rgb >>  8) & 255) / 255.0f, 0, 1);
				cx.m_[2][0] = fclamp(((rgb      ) & 255) / 255.0f, 0, 1);
				set_cxform(cx);

				return;
			}
			// @@ TODO see TextField members in Flash MX docs
			}	// end switch
		}


		bool	get_member(const tu_stringi& name, as_value* val)
		{
			LOG_LINE;
			as_standard_member	std_member = get_standard_member(name);
			switch (std_member)
			{
			default:
			case M_INVALID_MEMBER:
				break;
			case M_TEXT:
				//if (name == "text")
			{
				LOG_LINE;
				val->set_tu_string(m_text);
				return true;
			}
			case M_VISIBLE:
				//else if (name == "_visible")
			{
				LOG_LINE;
				val->set_bool(get_visible());
				return true;
			}
			case M_ALPHA:
				//else if (name == "_alpha")
			{
				// @@ TODO this should be generic to struct character!
				LOG_LINE;
				const cxform&	cx = get_cxform();
				val->set_double(cx.m_[3][0] * 100.f);
				return true;
			}
			case M_TEXTCOLOR:
				//else if (name == "textColor")
			{
				// Return color in 0xRRGGBB format
				LOG_LINE;
				const cxform&	cx = get_cxform();
				int	r = iclamp(int(cx.m_[0][0] * 255), 0, 255);
				int	g = iclamp(int(cx.m_[0][0] * 255), 0, 255);
				int	b = iclamp(int(cx.m_[0][0] * 255), 0, 255);
				val->set_int((r << 16) + (g << 8) + b);
				return true;
			}
			case M_X:
				//else if (name == "_x")
			{
				LOG_LINE;
				matrix	m = get_matrix();	// @@ get_world_matrix()???
				val->set_double(TWIPS_TO_PIXELS(m.m_[0][2]));
				return true;
			}
			case M_Y:
				//else if (name == "_y")
			{
				LOG_LINE;
				matrix	m = get_matrix();	// @@ get_world_matrix()???
				val->set_double(TWIPS_TO_PIXELS(m.m_[1][2]));
				return true;
			}
			case M_WIDTH:
				//else if (name == "_width")
			{
				// @@ TODO should implement this in
				// character and inherit into both here and sprite_instance
				LOG_LINE;
				rect	transformed_rect;
				transformed_rect.enclose_transformed_rect(get_world_matrix(), m_def->m_rect);
				val->set_double(TWIPS_TO_PIXELS(transformed_rect.width()));
				return true;
			}
			case M_HEIGHT:
				//else if (name == "_height")
			{
				// @@ TODO should implement this in
				// character and inherit into both here and sprite_instance
				LOG_LINE;
				rect	transformed_rect;
				transformed_rect.enclose_transformed_rect(get_world_matrix(), m_def->m_rect);
				val->set_double(TWIPS_TO_PIXELS(transformed_rect.height()));
				return true;
			}
			case M_TEXTWIDTH:
				//else if (name == "textWidth")
			{
				// Return the width, in pixels, of the text as laid out.
				// (I.e. the actual text content, not our defined
				// bounding box.)
				//
				// In local coords.  Verified against Macromedia Flash.
				LOG_LINE;
				val->set_double(TWIPS_TO_PIXELS(m_text_bounding_box.width()));

				return true;
			}
			}	// end switch

			return false;
		}

		
		// @@ WIDTH_FUDGE is a total fudge to make it match the Flash player!  Maybe
		// we have a bug?
		#define WIDTH_FUDGE 80.0f


		void	align_line(edit_text_character_def::alignment align, int last_line_start_record, float x)
		// Does LEFT/CENTER/RIGHT alignment on the records in
		// m_text_glyph_records[], starting with
		// last_line_start_record and going through the end of
		// m_text_glyph_records.
		{
			LOG_LINE;
			float	extra_space = (m_def->m_rect.width() - m_def->m_right_margin) - x - WIDTH_FUDGE;
			QASSERT(extra_space >= 0.0f);

			float	shift_right = 0.0f;

			if (align == edit_text_character_def::ALIGN_LEFT)
			{
				// Nothing to do; already aligned left.
				return;
			}
			else if (align == edit_text_character_def::ALIGN_CENTER)
			{
				// Distribute the space evenly on both sides.
				shift_right = extra_space / 2;
			}
			else if (align == edit_text_character_def::ALIGN_RIGHT)
			{
				// Shift all the way to the right.
				shift_right = extra_space;
			}

			// Shift the beginnings of the records on this line.
			for (int i = last_line_start_record; i < m_text_glyph_records.size(); i++)
			{
				text_glyph_record&	rec = m_text_glyph_records[i];

				if (rec.m_style.m_has_x_offset)
				{
					rec.m_style.m_x_offset += shift_right;
				}
			}
		}

		// Convert the characters in m_text into a series of
		// text_glyph_records to be rendered.
		void	format_text()
		{
			LOG_LINE;
			m_text_glyph_records.resize(0);

			if (m_def->m_font == NULL)
			{
				return;
			}

			// @@ mostly for debugging
			// Font substitution -- if the font has no
			// glyphs, try some other defined font!
			if (m_def->m_font->get_glyph_count() == 0)
			{
				// Find a better font.
				font*	newfont = m_def->m_font;
				for (int i = 0, n = fontlib::get_font_count(); i < n; i++)
				{
					font*	f = fontlib::get_font(i);
					QASSERT(f);

					if (f->get_glyph_count() > 0)
					{
						// This one looks good.
						newfont = f;
						break;
					}
				}

				if (m_def->m_font != newfont)
				{
					log_error("error: substituting font!  font '%s' has no glyphs, using font '%s'\n",
						  fontlib::get_font_name(m_def->m_font),
						  fontlib::get_font_name(newfont));

					m_def->m_font = newfont;
				}
			}


			float	scale = m_def->m_text_height / 1024.0f;	// the EM square is 1024 x 1024

			text_glyph_record	rec;	// one to work on
			rec.m_style.m_font = m_def->m_font;
			rec.m_style.m_color = m_def->m_color;
			rec.m_style.m_x_offset = fmax(0, m_def->m_left_margin + m_def->m_indent);
			rec.m_style.m_y_offset = m_def->m_text_height
				+ (m_def->m_font->get_leading() - m_def->m_font->get_descent()) * scale;
			rec.m_style.m_text_height = m_def->m_text_height;
			rec.m_style.m_has_x_offset = true;
			rec.m_style.m_has_y_offset = true;

			float	x = rec.m_style.m_x_offset;
			float	y = rec.m_style.m_y_offset;

			// Start the bbox at the upper-left corner of the first glyph.
 			reset_bounding_box(x, y - m_def->m_font->get_descent() * scale + m_def->m_text_height);

			float	leading = m_def->m_leading;
			leading += m_def->m_font->get_leading() * scale;

			int	last_code = -1;
			int	last_space_glyph = -1;
			int	last_line_start_record = 0;

			const char*	text = &m_text[0];
			while (Uint32 code = utf8::decode_next_unicode_character(&text))
			{
// @@ try to truncate overflow text??
#if 0
				if (y + m_def->m_font->get_descent() * scale > m_def->m_rect.height())
				{
					// Text goes below the bottom of our bounding box.
					rec.m_glyphs.resize(0);
					break;
				}
#endif // 0

				//Uint16	code = m_text[j];

				x += m_def->m_font->get_kerning_adjustment(last_code, (int) code) * scale;
				last_code = (int) code;

				// Expand the bounding-box to the lower-right corner of each glyph as
				// we generate it.
				m_text_bounding_box.expand_to_point(x, y + m_def->m_font->get_descent() * scale);

				if (code == 13 || code == 10)
				{
					// newline.

					// Frigging Flash seems to use '\r' (13) as its
					// default newline character.  If we get DOS-style \r\n
					// sequences, it'll show up as double newlines, so maybe we
					// need to detect \r\n and treat it as one newline.

					// Close out this stretch of glyphs.
					m_text_glyph_records.push_back(rec);
					align_line(m_def->m_alignment, last_line_start_record, x);

					x = fmax(0, m_def->m_left_margin + m_def->m_indent);	// new paragraphs get the indent.
					y += m_def->m_text_height + leading;

					// Start a new record on the next line.
					rec.m_glyphs.resize(0);
					rec.m_style.m_font = m_def->m_font;
					rec.m_style.m_color = m_def->m_color;
					rec.m_style.m_x_offset = x;
					rec.m_style.m_y_offset = y;
					rec.m_style.m_text_height = m_def->m_text_height;
					rec.m_style.m_has_x_offset = true;
					rec.m_style.m_has_y_offset = true;

					last_space_glyph = -1;
					last_line_start_record = m_text_glyph_records.size();

					continue;
				}

				if (code == 8)
				{
					// backspace (ASCII BS).

					// This is a limited hack to enable overstrike effects.
					// It backs the cursor up by one character and then continues
					// the layout.  E.g. you can use this to display an underline
					// cursor inside a simulated text-entry box.
					//
					// ActionScript understands the '\b' escape sequence
					// for inserting a BS character.
					//
					// ONLY WORKS FOR BACKSPACING OVER ONE CHARACTER, WON'T BS
					// OVER NEWLINES, ETC.

					if (rec.m_glyphs.size() > 0)
					{
						// Peek at the previous glyph, and zero out its advance
						// value, so the next char overwrites it.
						float	advance = rec.m_glyphs.back().m_glyph_advance;
						x -= advance;	// maintain formatting
						rec.m_glyphs.back().m_glyph_advance = 0;	// do the BS effect
					}
					continue;
				}

				// Remember where word breaks occur.
				if (code == 32)
				{
					last_space_glyph = rec.m_glyphs.size();
				}

				int	index = m_def->m_font->get_glyph_index((Uint16) code);
				if (index == -1)
				{
					// error -- missing glyph!
					
					// Log an error, but don't log too many times.
					static int	s_log_count = 0;
					if (s_log_count < 10)
					{
						s_log_count++;
						log_error("edit_text_character::display() -- missing glyph for char %d "
							  "-- make sure character shapes for font %s are being exported "
							  "into your SWF file!\n",
							  code,
							  m_def->m_font->get_name());
					}

					// Drop through and use index == -1; this will display
					// using the empty-box glyph
				}
				text_glyph_record::glyph_entry	ge;
				ge.m_glyph_index = index;
				ge.m_glyph_advance = scale * m_def->m_font->get_advance(index);

				rec.m_glyphs.push_back(ge);

				x += ge.m_glyph_advance;

				
				if (x >= m_def->m_rect.width() - m_def->m_right_margin - WIDTH_FUDGE)
				{
					// Whoops, we just exceeded the box width.  Do word-wrap.

					// Insert newline.

					// Close out this stretch of glyphs.
					m_text_glyph_records.push_back(rec);
					float	previous_x = x;

					x = m_def->m_left_margin;
					y += m_def->m_text_height + leading;

					// Start a new record on the next line.
					rec.m_glyphs.resize(0);
					rec.m_style.m_font = m_def->m_font;
					rec.m_style.m_color = m_def->m_color;
					rec.m_style.m_x_offset = x;
					rec.m_style.m_y_offset = y;
					rec.m_style.m_text_height = m_def->m_text_height;
					rec.m_style.m_has_x_offset = true;
					rec.m_style.m_has_y_offset = true;
					
					text_glyph_record&	last_line = m_text_glyph_records.back();
					if (last_space_glyph == -1)
					{
						// Pull the previous glyph down onto the
						// new line.
						if (last_line.m_glyphs.size() > 0)
						{
							rec.m_glyphs.push_back(last_line.m_glyphs.back());
							x += last_line.m_glyphs.back().m_glyph_advance;
							previous_x -= last_line.m_glyphs.back().m_glyph_advance;
							last_line.m_glyphs.resize(last_line.m_glyphs.size() - 1);
						}
					}
					else
					{
						// Move the previous word down onto the next line.

						previous_x -= last_line.m_glyphs[last_space_glyph].m_glyph_advance;

						for (int i = last_space_glyph + 1; i < last_line.m_glyphs.size(); i++)
						{
							rec.m_glyphs.push_back(last_line.m_glyphs[i]);
							x += last_line.m_glyphs[i].m_glyph_advance;
							previous_x -= last_line.m_glyphs[i].m_glyph_advance;
						}
						last_line.m_glyphs.resize(last_space_glyph);
					}

					align_line(m_def->m_alignment, last_line_start_record, previous_x);

					last_space_glyph = -1;
					last_line_start_record = m_text_glyph_records.size();
				}

				// TODO: HTML markup
			}

			// Add this line to our output.
			m_text_glyph_records.push_back(rec);
			align_line(m_def->m_alignment, last_line_start_record, x);
		}


		void	display()
		// Draw the dynamic string.
		{
			LOG_LINE;
			if (m_def->m_border)
			{
				matrix	mat = get_world_matrix();
				
				// @@ hm, should we apply the color xform?  It seems logical; need to test.
				// cxform	cx = get_world_cxform();

				// Show white background + black bounding box.
				render::set_matrix(mat);

				point	coords[4];
				coords[0] = m_def->m_rect.get_corner(0);
				coords[1] = m_def->m_rect.get_corner(1);
				coords[2] = m_def->m_rect.get_corner(3);
				coords[3] = m_def->m_rect.get_corner(2);

				Sint16	icoords[18] = 
				{
					// strip (fill in)
					(Sint16) coords[0].m_x, (Sint16) coords[0].m_y,
					(Sint16) coords[1].m_x, (Sint16) coords[1].m_y,
					(Sint16) coords[2].m_x, (Sint16) coords[2].m_y,
					(Sint16) coords[3].m_x, (Sint16) coords[3].m_y,

					// outline
					(Sint16) coords[0].m_x, (Sint16) coords[0].m_y,
					(Sint16) coords[1].m_x, (Sint16) coords[1].m_y,
					(Sint16) coords[3].m_x, (Sint16) coords[3].m_y,
					(Sint16) coords[2].m_x, (Sint16) coords[2].m_y,
					(Sint16) coords[0].m_x, (Sint16) coords[0].m_y,
				};
				
				render::fill_style_color(0, rgba(255, 255, 255, 255));
				render::draw_mesh_strip(&icoords[0], 4);

				render::line_style_color(rgba(0,0,0,255));
				render::draw_line_strip(&icoords[8], 5);
			}

			// Draw our actual text.
			display_glyph_records(matrix::identity, this, m_text_glyph_records, m_def->m_root_def);

			do_display_callback();
		}
	};


	character*	edit_text_character_def::create_character_instance(movie* parent, int id)
	{
		LOG_LINE;
		if (m_font == NULL)
		{
			// Resolve the font, if possible.
			m_font = m_root_def->get_font(m_font_id);
			if (m_font == NULL)
			{
				log_error("error: text style with undefined font; font_id = %d\n", m_font_id);
			}
		}

		edit_text_character*	ch = new edit_text_character(parent, this, id);
		ch->set_name(m_default_name.c_str());
		return ch;
	}


	void	define_edit_text_loader(stream* in, int tag_type, movie_definition_sub* m)
	// Read a DefineText tag.
	{
		LOG_LINE;
		QASSERT(tag_type == 37);

		Uint16	character_id = in->read_u16();

		edit_text_character_def*	ch = new edit_text_character_def(m);
		IF_VERBOSE_PARSE(log_msg("edit_text_char, id = %d\n", character_id));
		ch->read(in, tag_type, m);

		LOG_LINE;
		m->add_character(character_id, ch);
	}

}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:

