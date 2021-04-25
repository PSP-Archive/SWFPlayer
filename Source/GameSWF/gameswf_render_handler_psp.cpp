// gameswf_render_handler_psp.cpp	-- Willem Kokke <willem@mindparity.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A gameswf::render_handler that uses SDL & OpenGL

#include "CTypes.h"
#include "gameswf.h"
#include "gameswf_types.h"
#include "base/image.h"
#include "CGfx.h"
#include "CTextureManager.h"
#include "base/container.h"
#include "CFrameWork.h"
#include "CInput.h"
#include "CSWFFileHandler.h"
#include "CConfigFile.h"

//#define SHOW_IMAGES_ON_LOAD

extern float	swf_x_offset;

static CVarInt	CVAR_MAX_IMAGE_SIZE( "max_image_size", 256 );


// bitmap_info_psp declaration
struct bitmap_info_psp : public gameswf::bitmap_info
{
	bitmap_info_psp();
	bitmap_info_psp(int width, int height, u8* data);
	bitmap_info_psp(image::rgb* im, bool resize);
	bitmap_info_psp(image::rgba* im);
	virtual void	fill_rgb( const unsigned char * p_rgb_data, int width, int height );
};

static array<CTexture*>	s_psp_textures;

static ARGB						m_Color;
static gameswf::matrix			m_ModelMtx;
static array<gameswf::matrix>	m_MtxStack;

struct render_handler_psp : public gameswf::render_handler
{
	// Some renderer state.

	// Enable/disable antialiasing.
	bool	m_enable_antialias;

	// Output size.
	float	m_display_width;
	float	m_display_height;

	gameswf::matrix	m_current_matrix;
	gameswf::cxform	m_current_cxform;

	struct fill_style;
	static const fill_style *	m_pFillStyle;

	void set_antialiased(bool enable)
	{
		m_enable_antialias = enable;
	}

	static void make_next_miplevel(int* width, int* height, u8* data)
	// Utility.  Mutates *width, *height and *data to create the
	// next mip level.
	{
		QASSERT(width);
		QASSERT(height);
		QASSERT(data);

		int	new_w = *width >> 1;
		int	new_h = *height >> 1;
		if (new_w < 1) new_w = 1;
		if (new_h < 1) new_h = 1;
		
		if (new_w * 2 != *width	 || new_h * 2 != *height)
		{
			// Image can't be shrunk along (at least) one
			// of its dimensions, so don't bother
			// resampling.	Technically we should, but
			// it's pretty useless at this point.  Just
			// change the image dimensions and leave the
			// existing pixels.
		}
		else
		{
			// Resample.  Simple average 2x2 --> 1, in-place.
			for (int j = 0; j < new_h; j++) {
				u8*	out = ((u8*) data) + j * new_w;
				u8*	in = ((u8*) data) + (j << 1) * *width;
				for (int i = 0; i < new_w; i++) {
					int	a;
					a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
					*(out) = a >> 2;
					out++;
					in += 2;
				}
			}
		}

		// Munge parameters to reflect the shrunken image.
		*width = new_w;
		*height = new_h;
	}

	struct fill_style
	{
		enum mode
		{
			INVALID,
			COLOR,
			BITMAP_WRAP,
			BITMAP_CLAMP,
			LINEAR_GRADIENT,
			RADIAL_GRADIENT,
		};
		mode	m_mode;
		gameswf::rgba	m_color;
		const gameswf::bitmap_info*	m_bitmap_info;
		gameswf::matrix	m_bitmap_matrix;
		gameswf::cxform	m_bitmap_color_transform;
		bool	m_has_nonzero_bitmap_additive_color;

		fill_style()
			:
			m_mode(INVALID),
			m_has_nonzero_bitmap_additive_color(false)
		{
		}

		void	apply(/*const matrix& current_matrix*/) const
		// Push our style into OpenGL.
		{
//			LOG_LINE;

			QASSERT(m_mode != INVALID);

			if (m_mode == COLOR)
			{
				apply_color(m_color);
				sceGuDisable( GU_TEXTURE_2D );
				m_pFillStyle = NULL;
			}
			else if (m_mode == BITMAP_WRAP
				 || m_mode == BITMAP_CLAMP)
			{
				QASSERT(m_bitmap_info != NULL);

				apply_color(m_color);

				if (m_bitmap_info == NULL)
				{
					m_pFillStyle = NULL;
					sceGuDisable( GU_TEXTURE_2D );
				}
				else
				{
					// Set up the texture for rendering.

					{
						// Do the modulate part of the color
						// transform in the first pass.  The
						// additive part, if any, needs to
						// happen in a second pass.
//						glColor4f(m_bitmap_color_transform.m_[0][0],
//							  m_bitmap_color_transform.m_[1][0],
//							  m_bitmap_color_transform.m_[2][0],
//							  m_bitmap_color_transform.m_[3][0]
//							  );
					}

					if ( s_psp_textures[ m_bitmap_info->m_texture_id ] == NULL )
					{
						m_pFillStyle = NULL;
						sceGuDisable( GU_TEXTURE_2D );
					}
					else
					{
						m_pFillStyle = this;

						CGfx::SetTexture( s_psp_textures[ m_bitmap_info->m_texture_id ] );
					}

					if (m_mode == BITMAP_CLAMP)
					{
						sceGuTexWrap( GU_CLAMP, GU_CLAMP );
					}
					else
					{
						QASSERT(m_mode == BITMAP_WRAP);

						sceGuTexWrap( GU_REPEAT, GU_REPEAT );
					}
				}
			}
		}


		bool	needs_second_pass() const
		// Return true if we need to do a second pass to make
		// a valid color.  This is for cxforms with additive
		// parts; this is the simplest way (that we know of)
		// to implement an additive color with stock OpenGL.
		{
			if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
			{
				return m_has_nonzero_bitmap_additive_color;
			}
			else
			{
				return false;
			}
		}

		void	apply_second_pass() const
		// Set OpenGL state for a necessary second pass.
		{
			QASSERT(needs_second_pass());

			// The additive color also seems to be modulated by the texture. So,
			// maybe we can fake this in one pass using using the mean value of 
			// the colors: c0*t+c1*t = ((c0+c1)/2) * t*2
			// I don't know what the alpha component of the color is for.
			//glDisable(GL_TEXTURE_2D);

//			set_color( gameswf::rgba( m_bitmap_color_transform.m_[0][1], m_bitmap_color_transform.m_[1][1], m_bitmap_color_transform.m_[2][1], m_bitmap_color_transform.m_[3][1] ) );

//			sceGuBlendFunc( GU_ADD, GU_SRC_COLOR, GU_SRC_COLOR, 0, 0 );
		}

		void	cleanup_second_pass() const
		{
//			sceGuBlendFunc( GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 );
		}


		void	disable() { m_mode = INVALID; }
		void	set_color(gameswf::rgba color) { m_mode = COLOR; m_color = color; }
		void	set_bitmap(const gameswf::bitmap_info* bi, const gameswf::matrix& m, bitmap_wrap_mode wm, const gameswf::cxform& color_transform)
		{
			m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
			m_color = gameswf::rgba();
			m_bitmap_info = bi;
			m_bitmap_matrix = m;
			m_bitmap_color_transform = color_transform;

			if (m_bitmap_color_transform.m_[0][1] > 1.0f
			    || m_bitmap_color_transform.m_[1][1] > 1.0f
			    || m_bitmap_color_transform.m_[2][1] > 1.0f
			    || m_bitmap_color_transform.m_[3][1] > 1.0f)
			{
				m_has_nonzero_bitmap_additive_color = true;
			}
			else
			{
				m_has_nonzero_bitmap_additive_color = false;
			}
		}
		bool	is_valid() const { return m_mode != INVALID; }
	};


	// Style state.
	enum style_index
	{
		LEFT_STYLE = 0,
		RIGHT_STYLE,
		LINE_STYLE,

		STYLE_COUNT
	};
	fill_style	m_current_styles[STYLE_COUNT];


	gameswf::bitmap_info*	create_bitmap_info_rgb(image::rgb* im, bool resize)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
	// bitmap fill style.
	{
		return new bitmap_info_psp(im, resize);
	}


	gameswf::bitmap_info*	create_bitmap_info_rgba(image::rgba* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_style_bitmap(), to set a
	// bitmap fill style.
	//
	// This version takes an image with an alpha channel.
	{
		return new bitmap_info_psp(im);
	}


	gameswf::bitmap_info*	create_bitmap_info_empty()
	// Create a placeholder bitmap_info.  Used when
	// DO_NOT_LOAD_BITMAPS is set; then later on the host program
	// can use movie_definition::get_bitmap_info_count() and
	// movie_definition::get_bitmap_info() to stuff precomputed
	// textures into these bitmap infos.
	{
		return new bitmap_info_psp;
	}

	gameswf::bitmap_info*	create_bitmap_info_alpha(int w, int h, u8* data)
	// Create a bitmap_info so that it contains an alpha texture
	// with the given data (1 byte per texel).
	//
	// Munges *data (in order to make mipmaps)!!
	{
		return new bitmap_info_psp(w, h, data);
	}


	void	delete_bitmap_info(gameswf::bitmap_info* bi)
	// Delete the given bitmap info struct.
	{
		delete bi;
	}

	~render_handler_psp()
	{
		for ( int i = 0; i < s_psp_textures.size(); ++i )
		{
			delete s_psp_textures[ i ];
		}

		s_psp_textures.clear();
	}


	void	begin_display(
		gameswf::rgba background_color,
		int viewport_x0, int viewport_y0,
		int viewport_width, int viewport_height,
		float x0, float x1, float y0, float y1)
	// Set up to render a full frame from a movie and fills the
	// background.	Sets up necessary transforms, to scale the
	// movie to fit within the given dimensions.  Call
	// end_display() when you're done.
	//
	// The rectangle (viewport_x0, viewport_y0, viewport_x0 +
	// viewport_width, viewport_y0 + viewport_height) defines the
	// window coordinates taken up by the movie.
	//
	// The rectangle (x0, y0, x1, y1) defines the pixel
	// coordinates of the movie that correspond to the viewport
	// bounds.
	{
		m_ModelMtx.set_identity();
		m_display_width = fabsf(x1 - x0);
		m_display_height = fabsf(y1 - y0);

		CGfx::BeginRender();

		apply_color( background_color );
		CGfx::ClearScreen( get_color().color );

/*		glViewport(viewport_x0, viewport_y0, viewport_width, viewport_height);

		glOrtho(x0, x1, y0, y1, -1, 1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// GL_MODULATE

		glDisable(GL_TEXTURE_2D);

		// Clear the background, if background color has alpha > 0.
		if (background_color.m_a > 0)
		{
			// Draw a big quad.
			apply_color(background_color);
			glBegin(GL_QUADS);
			glVertex2f(x0, y0);
			glVertex2f(x1, y0);
			glVertex2f(x1, y1);
			glVertex2f(x0, y1);
			glEnd();
		}*/
	}


	void	end_display()
	// Clean up after rendering a frame.  Client program is still
	// responsible for calling glSwapBuffers() or whatever.
	{
	}


	void	set_matrix(const gameswf::matrix& m)
	// Set the current transform for mesh & line-strip rendering.
	{
		m_current_matrix = m;
	}


	void	set_cxform(const gameswf::cxform& cx)
	// Set the current color transform for mesh & line-strip rendering.
	{
		m_current_cxform = cx;
	}

	static void	push_matrix()
	{
		m_MtxStack.push_back( m_ModelMtx );
	}

	static void	pop_matrix()
	{
		m_ModelMtx = m_MtxStack.back();
		m_MtxStack.pop_back();
	}

	static void	apply_matrix(const gameswf::matrix& m)
	// multiply current matrix with opengl matrix
	{
//		LOG_LINE;

		m_ModelMtx.concatenate( m );
	}

	static ARGB	get_color()
	{
		return m_Color;
	}

	static void	apply_color(const gameswf::rgba& c)
	// Set the given color.
	{
		m_Color.r = c.m_r;
		m_Color.g = c.m_g;
		m_Color.b = c.m_b;
		m_Color.a = c.m_a;
	}

	void	fill_style_disable(int fill_side)
	// Don't fill on the {0 == left, 1 == right} side of a path.
	{
		QASSERT(fill_side >= 0 && fill_side < 2);

		m_current_styles[fill_side].disable();
	}


	void	line_style_disable()
	// Don't draw a line on this path.
	{
		m_current_styles[LINE_STYLE].disable();
	}


	void	fill_style_color(int fill_side, gameswf::rgba color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		QASSERT(fill_side >= 0 && fill_side < 2);

		m_current_styles[fill_side].set_color(m_current_cxform.transform(color));
	}


	void	line_style_color(gameswf::rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		m_current_styles[LINE_STYLE].set_color(m_current_cxform.transform(color));
	}


	void	fill_style_bitmap(int fill_side, const gameswf::bitmap_info* bi, const gameswf::matrix& m, bitmap_wrap_mode wm)
	{
		QASSERT(fill_side >= 0 && fill_side < 2);
		m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform);
	}
	
	void	line_style_width(float width)
	{
		// WK: what to do here???
	}


	bool	is_on_screen(const void* coords, int vertex_count)
	{
		const s16 *	p_coords( static_cast< const s16 * >( coords ) );
		const float	scaler( CSWFFileHandler::GetScale() );

		push_matrix();
		apply_matrix( m_current_matrix );

		for ( s32 i = 0; i < vertex_count; ++i )
		{
			gameswf::point	a;
			gameswf::point	p( p_coords[ ( i * 2 ) + 0 ], p_coords[ ( i * 2 ) + 1 ] );

			m_ModelMtx.transform( &a, p );

			const float	x = ( a.m_x * scaler ) + swf_x_offset;
			const float	y = a.m_y * scaler;

			if ( x >= 0 && x <= CGfx::s_ScreenWidth && y >= 0 && y <= CGfx::s_ScreenHeight )
			{
				pop_matrix();

				return true;
			}
		}

		pop_matrix();

		return false;
	}


	void	draw_mesh_strip(const void* coords, int vertex_count)
	{
//		if ( is_on_screen( coords, vertex_count ) == false )
//		{
//			return;
//		}

//		LOG_LINE;
		const s16 *		p_coords( static_cast< const s16 * >( coords ) );
		const float		scaler( CSWFFileHandler::GetScale() );

		// Set up current style.
		LOG_LINE;
		m_current_styles[LEFT_STYLE].apply();

		LOG_LINE;
		push_matrix();
		LOG_LINE;
		apply_matrix(m_current_matrix);

		if ( m_pFillStyle == NULL )
		{
			sVertexColor *	p_poly_list;

			CGfx::GetPolyList( vertex_count, &p_poly_list );

			for ( s32 i = 0; i < vertex_count; ++i )
			{
				gameswf::point	a;

				m_ModelMtx.transform(&a, gameswf::point(p_coords[ ( i * 2 ) + 0 ], p_coords[ ( i * 2 ) + 1 ]));

				p_poly_list[ i ].color = get_color();
				p_poly_list[ i ].pos.x = ( a.m_x * scaler ) + swf_x_offset;
				p_poly_list[ i ].pos.y = a.m_y * scaler;
				p_poly_list[ i ].pos.z = 0.f;
			}

			CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, p_poly_list, vertex_count );
		}
		else
		{
			const float				uv_scale( GetCurrentUVScale() );
			sVertexTexturedColor *	p_poly_list;

			CGfx::GetPolyList( vertex_count, &p_poly_list );

			const gameswf::matrix&	tex_mtx( m_pFillStyle->m_bitmap_matrix );

			for ( s32 i = 0; i < vertex_count; ++i )
			{
				gameswf::point	a;
				gameswf::point	p( p_coords[ ( i * 2 ) + 0 ], p_coords[ ( i * 2 ) + 1 ] );

				m_ModelMtx.transform( &a, p );

				p_poly_list[ i ].color = get_color();
				p_poly_list[ i ].pos.x = ( a.m_x * scaler ) + swf_x_offset;
				p_poly_list[ i ].pos.y = a.m_y * scaler;
				p_poly_list[ i ].pos.z = 0.f;

				tex_mtx.transform( &a, p );

				p_poly_list[ i ].uv.x = a.m_x * uv_scale;
				p_poly_list[ i ].uv.y = a.m_y * uv_scale;
			}

			CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, s_psp_textures[ m_pFillStyle->m_bitmap_info->m_texture_id ], p_poly_list, vertex_count );
		}

//		if (m_current_styles[LEFT_STYLE].needs_second_pass())
//		{
//			m_current_styles[LEFT_STYLE].apply_second_pass();
//			CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, s_psp_textures[ m_pFillStyle->m_bitmap_info->m_texture_id ], p_poly_list, vertex_count );
//			m_current_styles[LEFT_STYLE].cleanup_second_pass();
//		}

		pop_matrix();
	}


	void	draw_mesh_strip(const void* coords, int vertex_count, const gameswf::bitmap_info * const p_bitmap )
	{
//		if ( is_on_screen( coords, vertex_count ) == false )
//		{
//			return;
//		}
		const s16 *				p_coords( static_cast< const s16 * >( coords ) );
		const float				scaler( CSWFFileHandler::GetScale() );
		CTexture * const		p_tex( s_psp_textures[ p_bitmap->m_texture_id ] );
		sVertexTexturedColor *	p_poly_list;

		push_matrix();
		apply_matrix( m_current_matrix );

		CGfx::GetPolyList( vertex_count, &p_poly_list );

		for ( s32 i = 0; i < vertex_count; ++i )
		{
			gameswf::point	a;
			gameswf::point	p( p_coords[ ( i * 2 ) + 0 ], p_coords[ ( i * 2 ) + 1 ] );

			m_ModelMtx.transform( &a, p );

			p_poly_list[ i ].color = 0xffffffff;
			p_poly_list[ i ].pos.x = ( a.m_x * scaler ) + swf_x_offset;
			p_poly_list[ i ].pos.y = a.m_y * scaler;
			p_poly_list[ i ].pos.z = 0.f;
		}

		p_poly_list[ 0 ].uv.x = 0.f;
		p_poly_list[ 0 ].uv.y = 0.f;
		p_poly_list[ 1 ].uv.x = p_tex->m_nWidth;
		p_poly_list[ 1 ].uv.y = 0.f;
		p_poly_list[ 2 ].uv.x = 0.f;
		p_poly_list[ 2 ].uv.y = p_tex->m_nHeight;
		p_poly_list[ 3 ].uv.x = p_tex->m_nWidth;
		p_poly_list[ 3 ].uv.y = p_tex->m_nHeight;

		CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, p_tex, p_poly_list, vertex_count );

		pop_matrix();
	}


	void	draw_line_strip(const void* coords, int vertex_count)
	// Draw the line strip formed by the sequence of points.
	{
//		LOG_LINE;

		sVertexColor *	p_line_list;
		const s16 *		p_coords( static_cast< const s16 * >( coords ) );
		const float		scaler( CSWFFileHandler::GetScale() );

		// Set up current style.
		m_current_styles[LINE_STYLE].apply();

		push_matrix();
		apply_matrix(m_current_matrix);

		// Send the line-strip to OpenGL
		CGfx::GetPolyList( vertex_count, &p_line_list );

		for ( s32 i = 0; i < vertex_count; ++i )
		{
			gameswf::point	a;

			m_ModelMtx.transform(&a, gameswf::point(p_coords[ ( i * 2 ) + 0 ], p_coords[ ( i * 2 ) + 1 ]));

			p_line_list[ i ].color = get_color();
			p_line_list[ i ].pos.x = ( a.m_x * scaler ) + swf_x_offset;
			p_line_list[ i ].pos.y = a.m_y * scaler;
			p_line_list[ i ].pos.z = 0.f;
		}

		sceGuDrawArray( GU_LINE_STRIP, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, vertex_count, 0, p_line_list );

		pop_matrix();
	}

	float	GetCurrentUVScale() const
	{
		if ( m_pFillStyle != NULL )
		{
			if ( m_pFillStyle->m_bitmap_info != NULL )
			{
				CTexture * const	p_texture( s_psp_textures[ m_pFillStyle->m_bitmap_info->m_texture_id ] );

				if ( p_texture != NULL )
				{
					if ( p_texture->m_bResized == true )
					{
						return 0.5f;
					}
				}
			}
		}

		return 1.f;
	}

	void	draw_bitmap(
		const gameswf::matrix& m,
		const gameswf::bitmap_info* bi,
		const gameswf::rect& coords,
		const gameswf::rect& uv_coords,
		gameswf::rgba color)
	// Draw a rectangle textured with the given bitmap, with the
	// given color.	 Apply given transform; ignore any currently
	// set transforms.
	//
	// Intended for textured glyph rendering.
	{
//		LOG_LINE;

		QASSERT(bi);

		const float			scaler( CSWFFileHandler::GetScale() );
		CTexture * const	p_texture( s_psp_textures[ bi->m_texture_id ] );

		apply_color(color);

		gameswf::point a, b, c, d;
		m.transform(&a, gameswf::point(coords.m_x_min, coords.m_y_min));
		m.transform(&b, gameswf::point(coords.m_x_max, coords.m_y_min));
		m.transform(&c, gameswf::point(coords.m_x_min, coords.m_y_max));
		d.m_x = b.m_x + c.m_x - a.m_x;
		d.m_y = b.m_y + c.m_y - a.m_y;

		if ( p_texture == NULL )
		{
			sVertexColor *	p_poly_list;

			CGfx::GetPolyList( 4, &p_poly_list );

			p_poly_list[ 0 ].color = get_color();
			p_poly_list[ 0 ].pos.x = ( a.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 0 ].pos.y = a.m_y * scaler;
			p_poly_list[ 0 ].pos.z = 0.f;

			p_poly_list[ 1 ].color = get_color();
			p_poly_list[ 1 ].pos.x = ( b.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 1 ].pos.y = b.m_y * scaler;
			p_poly_list[ 1 ].pos.z = 0.f;

			p_poly_list[ 2 ].color = get_color();
			p_poly_list[ 2 ].pos.x = ( c.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 2 ].pos.y = c.m_y * scaler;
			p_poly_list[ 2 ].pos.z = 0.f;

			p_poly_list[ 3 ].color = get_color();
			p_poly_list[ 3 ].pos.x = ( d.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 3 ].pos.y = d.m_y * scaler;
			p_poly_list[ 3 ].pos.z = 0.f;

			CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, p_poly_list, 4 );
		}
		else
		{
			const float				uv_scale( GetCurrentUVScale() );
			sVertexTexturedColor *	p_poly_list;

			CGfx::GetPolyList( 4, &p_poly_list );

			p_poly_list[ 0 ].color = get_color();
			p_poly_list[ 0 ].pos.x = ( a.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 0 ].pos.y = a.m_y * scaler;
			p_poly_list[ 0 ].pos.z = 0.f;
			p_poly_list[ 0 ].uv.x = uv_coords.m_x_min * p_texture->m_nCanvasWidth * uv_scale;
			p_poly_list[ 0 ].uv.y = uv_coords.m_y_min * p_texture->m_nCanvasHeight * uv_scale;

			p_poly_list[ 1 ].color = get_color();
			p_poly_list[ 1 ].pos.x = ( b.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 1 ].pos.y = b.m_y * scaler;
			p_poly_list[ 1 ].pos.z = 0.f;
			p_poly_list[ 1 ].uv.x = uv_coords.m_x_max * p_texture->m_nCanvasWidth * uv_scale;
			p_poly_list[ 1 ].uv.y = uv_coords.m_y_min * p_texture->m_nCanvasHeight * uv_scale;

			p_poly_list[ 2 ].color = get_color();
			p_poly_list[ 2 ].pos.x = ( c.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 2 ].pos.y = c.m_y * scaler;
			p_poly_list[ 2 ].pos.z = 0.f;
			p_poly_list[ 2 ].uv.x = uv_coords.m_x_min * p_texture->m_nCanvasWidth * uv_scale;
			p_poly_list[ 2 ].uv.y = uv_coords.m_y_max * p_texture->m_nCanvasHeight * uv_scale;

			p_poly_list[ 3 ].color = get_color();
			p_poly_list[ 3 ].pos.x = ( d.m_x * scaler ) + swf_x_offset;
			p_poly_list[ 3 ].pos.y = d.m_y * scaler;
			p_poly_list[ 3 ].pos.z = 0.f;
			p_poly_list[ 3 ].uv.x = uv_coords.m_x_max * p_texture->m_nCanvasWidth * uv_scale;
			p_poly_list[ 3 ].uv.y = uv_coords.m_y_max * p_texture->m_nCanvasHeight * uv_scale;

			CGfx::DrawPoly2D( GU_TRIANGLE_STRIP, p_texture, p_poly_list, 4 );
		}
	}
	
	void begin_submit_mask()
	{
/*	    glEnable(GL_STENCIL_TEST); 
	    glClearStencil(0);
	    glClear(GL_STENCIL_BUFFER_BIT);
	    glColorMask(0,0,0,0);	// disable framebuffer writes
	    glEnable(GL_STENCIL_TEST);	// enable stencil buffer for "marking" the mask
	    glStencilFunc(GL_ALWAYS, 1, 1);	// always passes, 1 bit plane, 1 as mask
	    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	// we set the stencil buffer to 1 where we draw any polygon
							// keep if test fails, keep if test passes but buffer test fails
							// replace if test passes */
	}
	
	void end_submit_mask()
	{	     
/*	    glColorMask(1,1,1,1);	// enable framebuffer writes
	    glStencilFunc(GL_EQUAL, 1, 1);	// we draw only where the stencil is 1 (where the mask was drawn)
	    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	// don't change the stencil buffer    */
	}
	
	void disable_mask()
	{
/*	    glDisable(GL_STENCIL_TEST); */
	}
	
};	// end struct render_handler_psp

const render_handler_psp::fill_style *	render_handler_psp::m_pFillStyle;

// bitmap_info_psp implementation


bitmap_info_psp::bitmap_info_psp()
// Make a placeholder bitmap_info.  Must be filled in later before
// using.
{
	m_texture_id = 0;
	m_original_width = 0;
	m_original_height = 0;
}


bitmap_info_psp::bitmap_info_psp(int width, int height, u8* data)
// Initialize this bitmap_info to an alpha image
// containing the specified data (1 byte per texel).
//
// !! Munges *data in order to create mipmaps !!
{
//	BREAK_POINT( "bitmap_info_psp::bitmap_info_psp(int width, int height, u8* data)" );

	// Create the texture.
	s_psp_textures.push_back( NULL );
	m_texture_id = s_psp_textures.size() - 1;

	CTexture *	p_tex( new CTexture() );

	if ( p_tex != NULL )
	{
		if ( p_tex->Init( GU_PSM_8888, width, height ) == true )
		{
			m_original_width = width;
			m_original_height = height;

			u32 *	p_dst( ( u32 * )p_tex->m_pBuffer );

			for ( s32 y = 0; y < height; ++y )
			{
				for ( s32 x = 0; x < width; ++x )
				{
					u8 * p_pixel( data + ( x + ( y * width ) ) );

					p_dst[ x + ( y * p_tex->m_nCanvasWidth ) ] = MAKE_ARGB( p_pixel[ 0 ], 0x00, 0x00, 0x00 );
				}
			}

			if ( m_original_width > CVAR_MAX_IMAGE_SIZE.Get() || m_original_height > CVAR_MAX_IMAGE_SIZE.Get() )
			{
				p_tex->Resize();
			}

#ifdef SHOW_IMAGES_ON_LOAD
			CGfx::BeginRender();
			CGfx::ClearScreen( 0xffffffff );

			CInput::Process();
			CGfx::DrawQuad( p_tex, V2( 0.f, 0.f ), V2( p_tex->m_nWidth, p_tex->m_nHeight ), 0xffffffff );

			CGfx::EndRender();
			CGfx::SwapBuffers();
#endif	// #ifdef SHOW_IMAGES_ON_LOAD
		}
	}

	s_psp_textures.back() = p_tex;
}


bitmap_info_psp::bitmap_info_psp(image::rgb* im, bool resize)
{
	QASSERT(im);

//	BREAK_POINT( "bitmap_info_psp::bitmap_info_psp(image::rgb* im)" );

	// Create the texture.
	s_psp_textures.push_back( NULL );

	m_texture_id = s_psp_textures.size() - 1;
	m_original_width = im->m_width;
	m_original_height = im->m_height;

	CTexture *	p_tex( new CTexture() );

	if ( p_tex != NULL )
	{
		if ( p_tex->Init( GU_PSM_5551, im->m_width, im->m_height ) == true )
		{
			u8 *	p_src( im->m_data );
			u16 *	p_dst( ( u16 * )p_tex->m_pBuffer );

			for ( s32 y = 0; y < im->m_height; ++y )
			{
				for ( s32 x = 0; x < im->m_width; ++x )
				{
					u8 * p_pixel( p_src + ( ( x * 3 ) + ( y * im->m_pitch ) ) );

					p_dst[ x + ( y * p_tex->m_nCanvasWidth ) ] = MAKE_RGB( p_pixel[ 0 ], p_pixel[ 1 ], p_pixel[ 2 ] );
				}
			}

			if ( m_original_width > CVAR_MAX_IMAGE_SIZE.Get() || m_original_height > CVAR_MAX_IMAGE_SIZE.Get() || resize == true )
			{
				p_tex->Resize();
			}

#ifdef SHOW_IMAGES_ON_LOAD
			CGfx::BeginRender();
			CGfx::ClearScreen( 0xffffffff );

			CInput::Process();
			CGfx::DrawQuad( p_tex, V2( 0.f, 0.f ), V2( p_tex->m_nWidth, p_tex->m_nHeight ), 0xffffffff );

			CGfx::EndRender();
			CGfx::SwapBuffers();
#endif	// #ifdef SHOW_IMAGES_ON_LOAD
		}
	}

	s_psp_textures.back() = p_tex;
}


bitmap_info_psp::bitmap_info_psp(image::rgba* im)
// Version of the constructor that takes an image with alpha.
{
	QASSERT(im);

//	BREAK_POINT( "bitmap_info_psp::bitmap_info_psp(image::rgba* im)" );

	// Create the texture.
	s_psp_textures.push_back( NULL );

	m_texture_id = s_psp_textures.size() - 1;
	m_original_width = im->m_width;
	m_original_height = im->m_height;

	CTexture *	p_tex( new CTexture() );

	if ( p_tex != NULL )
	{
		if ( p_tex->Init( GU_PSM_8888, im->m_width, im->m_height ) == true )
		{
			u8 *	p_src( im->m_data );
			u32 *	p_dst( ( u32 * )p_tex->m_pBuffer );

			for ( s32 y = 0; y < im->m_height; ++y )
			{
				for ( s32 x = 0; x < im->m_width; ++x )
				{
					u8 * p_pixel( p_src + ( ( x * 4 ) + ( y * im->m_pitch ) ) );

					p_dst[ x + ( y * p_tex->m_nCanvasWidth ) ] = MAKE_ARGB( p_pixel[ 3 ], p_pixel[ 0 ], p_pixel[ 1 ], p_pixel[ 2 ] );
				}
			}

			if ( m_original_width > CVAR_MAX_IMAGE_SIZE.Get() || m_original_height > CVAR_MAX_IMAGE_SIZE.Get() )
			{
				p_tex->Resize();
			}

#ifdef SHOW_IMAGES_ON_LOAD
			CGfx::BeginRender();
			CGfx::ClearScreen( 0xffffffff );

			CInput::Process();
			CGfx::DrawQuad( p_tex, V2( 0.f, 0.f ), V2( p_tex->m_nWidth, p_tex->m_nHeight ), 0xffffffff );

			CGfx::EndRender();
			CGfx::SwapBuffers();
#endif	// #ifdef SHOW_IMAGES_ON_LOAD
		}
	}

	s_psp_textures.back() = p_tex;
}

void	bitmap_info_psp::fill_rgb( const unsigned char * p_src, int width, int height )
{
	CTexture * const	p_tex( s_psp_textures[ m_texture_id ] );
	u16 *				p_dst( ( u16 * )p_tex->m_pBuffer );

	if ( p_tex->m_bResized == true )
	{
		ASSERT( p_tex->m_nCanvasWidth >= ( width >> 1 ), "Destination is too small" );
		ASSERT( p_tex->m_nCanvasHeight >= ( height >> 1 ), "Destination is too small" );

		for ( int y = 0; y < height >> 1; ++y )
		{
			for ( int x = 0; x < width >> 1; ++x )
			{
				*p_dst++ = MAKE_RGB( p_src[ 0 ], p_src[ 1 ], p_src[ 2 ] );

				p_src += 6;
			}

			p_src += width * 3;
			p_dst += ( p_tex->m_nCanvasWidth - ( width >> 1 ) );
		}
	}
	else
	{
		ASSERT( p_tex->m_nCanvasWidth >= width, "Destination is too small" );
		ASSERT( p_tex->m_nCanvasHeight >= height, "Destination is too small" );

		for ( int y = 0; y < height; ++y )
		{
			for ( int x = 0; x < width; ++x )
			{
				*p_dst++ = MAKE_RGB( p_src[ 0 ], p_src[ 1 ], p_src[ 2 ] );

				p_src += 3;
			}

			p_dst += ( p_tex->m_nCanvasWidth - width );
		}
	}
}

gameswf::render_handler*	gameswf::create_render_handler_psp()
// Factory.
{
	return new render_handler_psp;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
