// gameswf_styles.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Fill and line style types.


#include "gameswf_styles.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_render.h"
#include "gameswf_stream.h"


namespace gameswf
{
	//
	// gradient_record
	//

	gradient_record::gradient_record()
		:
		m_ratio(0)
	{
	}


	void	gradient_record::read(stream* in, int tag_type)
	{
		LOG_LINE;
		m_ratio = in->read_u8();
		LOG_LINE;
		m_color.read(in, tag_type);
	}


	//
	// fill_style
	//


	fill_style::fill_style()
		:
		m_type(0),
		m_gradient_bitmap_info(0),
		m_bitmap_character(0)
	{
		LOG_LINE;
		QASSERT(m_gradients.size() == 0);
	}


	fill_style::~fill_style()
	{
		LOG_LINE;
	}

	void	fill_style::read(stream* in, int tag_type, movie_definition_sub* md)
	{
		LOG_LINE;
		m_type = in->read_u8();

		IF_VERBOSE_PARSE(log_msg("  fill_style read type = 0x%X\n", m_type));

		LOG_LINE;
		if (m_type == 0x00)
		{
			// 0x00: solid fill
			LOG_LINE;
			if (tag_type <= 22) {
				LOG_LINE;
				m_color.read_rgb(in);
			} else {
				LOG_LINE;
				m_color.read_rgba(in);
			}

			LOG_LINE;
			IF_VERBOSE_PARSE(log_msg("  color: ");
					 m_color.print());
		}
		else if (m_type == 0x10 || m_type == 0x12)
		{
			// 0x10: linear gradient fill
			// 0x12: radial gradient fill

			LOG_LINE;
			matrix	input_matrix;
			LOG_LINE;
			input_matrix.read(in);

			LOG_LINE;
			if (m_type == 0x10)
			{
				LOG_LINE;
				m_gradient_matrix.set_identity();
				LOG_LINE;
				m_gradient_matrix.concatenate_translation(128.f, 0.f);
				LOG_LINE;
				m_gradient_matrix.concatenate_scale(1.0f / 128.0f);
			}
			else 
			{
				LOG_LINE;
				m_gradient_matrix.set_identity();
				LOG_LINE;
				m_gradient_matrix.concatenate_translation(32.f, 32.f);
				LOG_LINE;
				m_gradient_matrix.concatenate_scale(1.0f / 512.0f);
			}


			matrix	m;
			LOG_LINE;
			m.set_inverse(input_matrix);
			LOG_LINE;
			m_gradient_matrix.concatenate(m);
				
			// GRADIENT
			LOG_LINE;
			int	num_gradients = in->read_u8();
			LOG_LINE;
			QASSERT(num_gradients >= 1 && num_gradients <= 8);
			LOG_LINE;
			m_gradients.resize(num_gradients);
			LOG_LINE;
			for (int i = 0; i < num_gradients; i++)
			{
				LOG_LINE;
				m_gradients[i].read(in, tag_type);
			}

			IF_VERBOSE_PARSE(log_msg("  gradients: num_gradients = %d\n", num_gradients));

			// @@ hack.
			LOG_LINE;
			if (num_gradients > 0)
			{
				LOG_LINE;
				m_color = m_gradients[0].m_color;
			}

			LOG_LINE;
			if (md->get_create_bitmaps() == DO_LOAD_BITMAPS)
			{
				LOG_LINE;
				m_gradient_bitmap_info = create_gradient_bitmap();
			}
			else
			{
				LOG_LINE;
				m_gradient_bitmap_info = render::create_bitmap_info_empty();
			}

			// Make sure our movie_def_impl knows about this bitmap.
			LOG_LINE;
			md->add_bitmap_info(m_gradient_bitmap_info.get_ptr());
		}
		else if (m_type == 0x40 || m_type == 0x41)
		{
			// 0x40: tiled bitmap fill
			// 0x41: clipped bitmap fill

			LOG_LINE;
			int	bitmap_char_id = in->read_u16();
			LOG_LINE;
			IF_VERBOSE_PARSE(log_msg("  bitmap_char = %d\n", bitmap_char_id));

			// Look up the bitmap character.
			LOG_LINE;
			m_bitmap_character = md->get_bitmap_character(bitmap_char_id);

			LOG_LINE;
			matrix	m;
			LOG_LINE;
			m.read(in);

			// For some reason, it looks like they store the inverse of the
			// TWIPS-to-texcoords matrix.
			LOG_LINE;
			m_bitmap_matrix.set_inverse(m);
			LOG_LINE;
			IF_VERBOSE_PARSE(m_bitmap_matrix.print());
		}
	}


	rgba	fill_style::sample_gradient(int ratio) const
	// Return the color at the specified ratio into our gradient.
	// Ratio is in [0, 255].
	{
		QASSERT(ratio >= 0 && ratio <= 255);
		QASSERT(m_type == 0x10 || m_type == 0x12);
		QASSERT(m_gradients.size() > 0);

		if (ratio < m_gradients[0].m_ratio)
		{
			return m_gradients[0].m_color;		
		}
		
		for (int i = 1; i < m_gradients.size(); i++)
		{
			if (m_gradients[i].m_ratio >= ratio)
			{
				const gradient_record& gr0 = m_gradients[i - 1];
				const gradient_record& gr1 = m_gradients[i];
				float	f = 0.0f;
				if (gr0.m_ratio != gr1.m_ratio)
				{
					f = (ratio - gr0.m_ratio) / float(gr1.m_ratio - gr0.m_ratio);
				}

				rgba	result;
				result.set_lerp(m_gradients[i - 1].m_color, m_gradients[i].m_color, f);
				return result;
			}
		}
		return m_gradients.back().m_color;
	}

	gameswf::bitmap_info*	fill_style::create_gradient_bitmap() const
	// Make a bitmap_info* corresponding to our gradient.
	// We can use this to set the gradient fill style.
	{
		LOG_LINE;
		QASSERT(m_type == 0x10 || m_type == 0x12);

		LOG_LINE;
		image::rgba*	im = NULL;

		LOG_LINE;
		if (m_type == 0x10)
		{
			// Linear gradient.
			LOG_LINE;
			im = image::create_rgba(256, 1);

			LOG_LINE;
			for (int i = 0; i < im->m_width; i++)
			{
				rgba	sample = sample_gradient(i);
				im->set_pixel(i, 0, sample.m_r, sample.m_g, sample.m_b, sample.m_a);
			}
		}
		else if (m_type == 0x12)
		{
			// Radial gradient.
			LOG_LINE;
			im = image::create_rgba(64, 64);

			LOG_LINE;
			for (int j = 0; j < im->m_height; j++)
			{
				for (int i = 0; i < im->m_width; i++)
				{
					float	radius = (im->m_height - 1) / 2.0f;
					float	y = (j - radius) / radius;
					float	x = (i - radius) / radius;
					int	ratio = (int) floorf(255.5f * sqrt(x * x + y * y));
					if (ratio > 255)
					{
						ratio = 255;
					}
					rgba	sample = sample_gradient( ratio );
					im->set_pixel(i, j, sample.m_r, sample.m_g, sample.m_b, sample.m_a);
				}
			}
		}

		LOG_LINE;
		gameswf::bitmap_info*	bi = gameswf::render::create_bitmap_info_rgba(im);
		LOG_LINE;
		delete im;

		LOG_LINE;
		return bi;
	}


	void	fill_style::apply(int fill_side, float ratio) const
	// Push our style parameters into the renderer.
	{
		LOG_LINE;
		UNUSED(ratio);
		LOG_LINE;
		if (m_type == 0x00)
		{
			// 0x00: solid fill
			LOG_LINE;
			gameswf::render::fill_style_color(fill_side, m_color);
		}
		else if (m_type == 0x10 || m_type == 0x12)
		{
			// 0x10: linear gradient fill
			// 0x12: radial gradient fill

			LOG_LINE;
			if (m_gradient_bitmap_info == NULL)
			{
				// This can happen when morphing gradient styles.
				// QASSERT(morphing???);
				// log an error?
				LOG_LINE;
				fill_style*	this_non_const = const_cast<fill_style*>(this);
				LOG_LINE;
				this_non_const->m_gradient_bitmap_info = create_gradient_bitmap();
			}

			LOG_LINE;
			if (m_gradient_bitmap_info != NULL)
			{
				LOG_LINE;
				gameswf::render::fill_style_bitmap(
					fill_side,
					m_gradient_bitmap_info.get_ptr(),
					m_gradient_matrix,
					gameswf::render_handler::WRAP_CLAMP);
			}
		}
		else if (m_type == 0x40
				 || m_type == 0x41)
		{
			// bitmap fill (either tiled or clipped)
			LOG_LINE;
			gameswf::bitmap_info*	bi = NULL;
			LOG_LINE;
			if (m_bitmap_character != NULL)
			{
				LOG_LINE;
				bi = m_bitmap_character->get_bitmap_info();
				LOG_LINE;
				if (bi != NULL)
				{
					LOG_LINE;
					gameswf::render_handler::bitmap_wrap_mode	wmode = gameswf::render_handler::WRAP_REPEAT;
					LOG_LINE;
					if (m_type == 0x41)
					{
						LOG_LINE;
						wmode = gameswf::render_handler::WRAP_CLAMP;
					}
					LOG_LINE;
					gameswf::render::fill_style_bitmap(
						fill_side,
						bi,
						m_bitmap_matrix,
						wmode);
				}
			}
		}
	}


	void	fill_style::set_lerp(const fill_style& a, const fill_style& b, float t)
	// Sets this style to a blend of a and b.  t = [0,1]
	{
		LOG_LINE;
		QASSERT(t >= 0 && t <= 1);

		// fill style type
		LOG_LINE;
		m_type = a.get_type();
		LOG_LINE;
		QASSERT(m_type == b.get_type());

		// fill style color
		LOG_LINE;
		m_color.set_lerp(a.get_color(), b.get_color(), t);

		// fill style gradient matrix
		//
		// @@ TODO morphed gradients don't come out exactly
		// right; they shift around some.  Not sure where the
		// problem is.
		LOG_LINE;
		m_gradient_matrix.set_lerp(a.m_gradient_matrix, b.m_gradient_matrix, t);

		// fill style gradients
		LOG_LINE;
		QASSERT(m_gradients.size() == a.m_gradients.size());
		LOG_LINE;
		QASSERT(m_gradients.size() == b.m_gradients.size());
		LOG_LINE;
		for (int j=0; j < m_gradients.size(); j++)
		{
			LOG_LINE;
			m_gradients[j].m_ratio =
				(Uint8) frnd(
					flerp(a.m_gradients[j].m_ratio, b.m_gradients[j].m_ratio, t)
					);
			LOG_LINE;
			m_gradients[j].m_color.set_lerp(a.m_gradients[j].m_color, b.m_gradients[j].m_color, t);
		}
		LOG_LINE;
		m_gradient_bitmap_info = NULL;

		// fill style bitmap ID
		LOG_LINE;
		m_bitmap_character = a.m_bitmap_character;
		LOG_LINE;
		QASSERT(m_bitmap_character == b.m_bitmap_character);

		// fill style bitmap matrix
		LOG_LINE;
		m_bitmap_matrix.set_lerp(a.m_bitmap_matrix, b.m_bitmap_matrix, t);
	}


	//
	// line_style
	//

	
	line_style::line_style()
		:
		m_width(0)
	{
		LOG_LINE;
	}


	void	line_style::read(stream* in, int tag_type)
	{
		LOG_LINE;
		m_width = in->read_u16();
		LOG_LINE;
		m_color.read(in, tag_type);
	}


	void	line_style::apply(float ratio) const
	{
		LOG_LINE;
		UNUSED(ratio);
		LOG_LINE;
		gameswf::render::line_style_color(m_color);
		LOG_LINE;
		gameswf::render::line_style_width(m_width);
	}

}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
