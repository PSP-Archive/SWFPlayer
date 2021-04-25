// gameswf_render_handler_ogl.cpp	-- Willem Kokke <willem@mindparity.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A gameswf::render_handler that uses SDL & OpenGL


#include "gameswf.h"
#include "gameswf_types.h"
#include "base/image.h"
#include "base/ogl.h"
#include "base/utility.h"

#include <string.h>	// for memset()


// choose the resampling method:
// 1 = hardware (experimental, should be fast, somewhat buggy)
// 2 = fast software bilinear (default)
// 3 = use image::resample(), slow software resampling
#define RESAMPLE_METHOD 2


// Determines whether to generate mipmaps for smoother rendering of
// minified textures.  It actually tends to look OK to not use
// mipmaps, though it is potentially a performance hit if you use big
// textures.
//
// TODO: need to code mipmap LOD bias adjustment, to keep mipmaps from
// looking too blurry.  (Also applies to text rendering.)
#define GENERATE_MIPMAPS 0

// bitmap_info_ogl declaration
struct bitmap_info_ogl : public gameswf::bitmap_info
{
	bitmap_info_ogl();
	bitmap_info_ogl(int width, int height, Uint8* data);
	bitmap_info_ogl(image::rgb* im);
	bitmap_info_ogl(image::rgba* im);

	~bitmap_info_ogl()
	{
		if (m_texture_id > 0)
		{
			glDeleteTextures(1, (GLuint*) &m_texture_id);
		}
	}
};

struct render_handler_ogl : public gameswf::render_handler
{
	// Some renderer state.

	// Enable/disable antialiasing.
	bool	m_enable_antialias;
	
	// Output size.
	float	m_display_width;
	float	m_display_height;
	
	gameswf::matrix	m_current_matrix;
	gameswf::cxform	m_current_cxform;
	void set_antialiased(bool enable)
	{
		m_enable_antialias = enable;
	}

	static void make_next_miplevel(int* width, int* height, Uint8* data)
	// Utility.  Mutates *width, *height and *data to create the
	// next mip level.
	{
		assert(width);
		assert(height);
		assert(data);

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
				Uint8*	out = ((Uint8*) data) + j * new_w;
				Uint8*	in = ((Uint8*) data) + (j << 1) * *width;
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
			assert(m_mode != INVALID);
			
			if (m_mode == COLOR)
			{
				apply_color(m_color);
				glDisable(GL_TEXTURE_2D);
			}
			else if (m_mode == BITMAP_WRAP
				 || m_mode == BITMAP_CLAMP)
			{
				assert(m_bitmap_info != NULL);

				apply_color(m_color);

				if (m_bitmap_info == NULL)
				{
					glDisable(GL_TEXTURE_2D);
				}
				else
				{
					// Set up the texture for rendering.
					{
						// Do the modulate part of the color
						// transform in the first pass.  The
						// additive part, if any, needs to
						// happen in a second pass.
						glColor4f(m_bitmap_color_transform.m_[0][0],
							  m_bitmap_color_transform.m_[1][0],
							  m_bitmap_color_transform.m_[2][0],
							  m_bitmap_color_transform.m_[3][0]
							  );
					}

					glBindTexture(GL_TEXTURE_2D, m_bitmap_info->m_texture_id);
					glEnable(GL_TEXTURE_2D);
					glEnable(GL_TEXTURE_GEN_S);
					glEnable(GL_TEXTURE_GEN_T);
				
					if (m_mode == BITMAP_CLAMP)
					{	
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					}
					else
					{
						assert(m_mode == BITMAP_WRAP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}

					// Set up the bitmap matrix for texgen.

					float	inv_width = 1.0f / m_bitmap_info->m_original_width;
					float	inv_height = 1.0f / m_bitmap_info->m_original_height;

					const gameswf::matrix&	m = m_bitmap_matrix;
					glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					float	p[4] = { 0, 0, 0, 0 };
					p[0] = m.m_[0][0] * inv_width;
					p[1] = m.m_[0][1] * inv_width;
					p[3] = m.m_[0][2] * inv_width;
					glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

					glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
					p[0] = m.m_[1][0] * inv_height;
					p[1] = m.m_[1][1] * inv_height;
					p[3] = m.m_[1][2] * inv_height;
					glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
				}
			}
		}


		bool	needs_second_pass() const
		// Return true if we need to do a second pass to make
		// a valid color.  This is for cxforms with additive
		// parts; this is the simplest way (that we know of)
		// to implement an additive color with stock OpenGL.
		{
			if (m_mode == BITMAP_WRAP
			    || m_mode == BITMAP_CLAMP)
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
			assert(needs_second_pass());

			// The additive color also seems to be modulated by the texture. So,
			// maybe we can fake this in one pass using using the mean value of 
			// the colors: c0*t+c1*t = ((c0+c1)/2) * t*2
			// I don't know what the alpha component of the color is for.
			//glDisable(GL_TEXTURE_2D);

			glColor4f(
				m_bitmap_color_transform.m_[0][1] / 255.0f,
				m_bitmap_color_transform.m_[1][1] / 255.0f,
				m_bitmap_color_transform.m_[2][1] / 255.0f,
				m_bitmap_color_transform.m_[3][1] / 255.0f
				);

			glBlendFunc(GL_ONE, GL_ONE);
		}

		void	cleanup_second_pass() const
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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


	gameswf::bitmap_info*	create_bitmap_info_rgb(image::rgb* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
	// bitmap fill style.
	{
		return new bitmap_info_ogl(im);
	}


	gameswf::bitmap_info*	create_bitmap_info_rgba(image::rgba* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_style_bitmap(), to set a
	// bitmap fill style.
	//
	// This version takes an image with an alpha channel.
	{
		return new bitmap_info_ogl(im);
	}


	gameswf::bitmap_info*	create_bitmap_info_empty()
	// Create a placeholder bitmap_info.  Used when
	// DO_NOT_LOAD_BITMAPS is set; then later on the host program
	// can use movie_definition::get_bitmap_info_count() and
	// movie_definition::get_bitmap_info() to stuff precomputed
	// textures into these bitmap infos.
	{
		return new bitmap_info_ogl;
	}

	gameswf::bitmap_info*	create_bitmap_info_alpha(int w, int h, Uint8* data)
	// Create a bitmap_info so that it contains an alpha texture
	// with the given data (1 byte per texel).
	//
	// Munges *data (in order to make mipmaps)!!
	{
		return new bitmap_info_ogl(w, h, data);
	}


	void	delete_bitmap_info(gameswf::bitmap_info* bi)
	// Delete the given bitmap info struct.
	{
		delete bi;
	}


	~render_handler_ogl()
	{
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
		m_display_width = fabsf(x1 - x0);
		m_display_height = fabsf(y1 - y0);

		glViewport(viewport_x0, viewport_y0, viewport_width, viewport_height);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
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
		}

		// Old unused code.  Might get revived someday.
		#if 0
				// See if we want to, and can, use multitexture
				// antialiasing.
				s_multitexture_antialias = false;
				if (m_enable_antialias)
				{
					int	tex_units = 0;
					glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &tex_units);
					if (tex_units >= 2)
					{
						s_multitexture_antialias = true;
					}

					// Make sure we have an edge texture available.
					if (s_multitexture_antialias == true
						&& s_edge_texture_id == 0)
					{
						// Very simple texture: 2 texels wide, 1 texel high.
						// Both texels are white; left texel is all clear, right texel is all opaque.
						unsigned char	edge_data[8] = { 255, 255, 255, 0, 255, 255, 255, 255 };

						ogl::active_texture(GL_TEXTURE1_ARB);
						glEnable(GL_TEXTURE_2D);
						glGenTextures(1, &s_edge_texture_id);
						glBindTexture(GL_TEXTURE_2D, s_edge_texture_id);

						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, edge_data);

						glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	// @@ should we use a 1D texture???

						glDisable(GL_TEXTURE_2D);
						ogl::active_texture(GL_TEXTURE0_ARB);
						glDisable(GL_TEXTURE_2D);
					}
				}
		#endif // 0
	}


	void	end_display()
	// Clean up after rendering a frame.  Client program is still
	// responsible for calling glSwapBuffers() or whatever.
	{
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
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
	
	static void	apply_matrix(const gameswf::matrix& m)
	// multiply current matrix with opengl matrix
	{
		float	mat[16];
		memset(&mat[0], 0, sizeof(mat));
		mat[0] = m.m_[0][0];
		mat[1] = m.m_[1][0];
		mat[4] = m.m_[0][1];
		mat[5] = m.m_[1][1];
		mat[10] = 1;
		mat[12] = m.m_[0][2];
		mat[13] = m.m_[1][2];
		mat[15] = 1;
		glMultMatrixf(mat);
	}

	static void	apply_color(const gameswf::rgba& c)
	// Set the given color.
	{
		glColor4ub(c.m_r, c.m_g, c.m_b, c.m_a);
	}

	void	fill_style_disable(int fill_side)
	// Don't fill on the {0 == left, 1 == right} side of a path.
	{
		assert(fill_side >= 0 && fill_side < 2);

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
		assert(fill_side >= 0 && fill_side < 2);

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
		assert(fill_side >= 0 && fill_side < 2);
		m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform);
	}
	
	void	line_style_width(float width)
	{
		// WK: what to do here???
	}


	void	draw_mesh_strip(const void* coords, int vertex_count)
	{
#define NORMAL_RENDERING
//#define MULTIPASS_ANTIALIASING

#ifdef NORMAL_RENDERING
		// Set up current style.
		m_current_styles[LEFT_STYLE].apply();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		apply_matrix(m_current_matrix);

		// Send the tris to OpenGL
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_SHORT, sizeof(Sint16) * 2, coords);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);

		if (m_current_styles[LEFT_STYLE].needs_second_pass())
		{
			m_current_styles[LEFT_STYLE].apply_second_pass();
			glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);
			m_current_styles[LEFT_STYLE].cleanup_second_pass();
		}

		glDisableClientState(GL_VERTEX_ARRAY);

		glPopMatrix();
#endif // NORMAL_RENDERING

#ifdef MULTIPASS_ANTIALIASING
		// So this approach basically works.  This
		// implementation is not totally finished; two pass
		// materials (i.e. w/ additive color) aren't correct,
		// and there are some texture etc issues because I'm
		// just hosing state uncarefully here.  It needs the
		// optimization of only filling the bounding box of
		// the shape.  You must have destination alpha.
		//
		// It doesn't look quite perfect on my GF4.  For one
		// thing, you kinda want to crank down the max curve
		// subdivision error, because suddenly you can see
		// sub-pixel shape much better.  For another thing,
		// the antialiasing isn't quite perfect, to my eye.
		// It could be limited alpha precision, imperfections
		// GL_POLYGON_SMOOTH, and/or my imagination.

		glDisable(GL_TEXTURE_2D);

		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);	// GL_NICEST, GL_FASTEST, GL_DONT_CARE

		// Clear destination alpha.
		//
		// @@ TODO Instead of drawing this huge screen-filling
		// quad, we should take a bounding-box param from the
		// caller, and draw the box (after apply_matrix;
		// i.e. the box is in object space).  The point being,
		// to only fill the part of the screen that the shape
		// is in.
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		glColor4f(1, 1, 1, 0);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(100000, 0);
		glVertex2f(100000, 100000);
		glVertex2f(0, 100000);
		glEnd();

		// Set mode for drawing alpha mask.
		glBlendFunc(GL_ONE, GL_ONE);	// additive blending
		glColor4f(0, 0, 0, m_current_styles[LEFT_STYLE].m_color.m_a / 255.0f);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		apply_matrix(m_current_matrix);

		// Send the tris to OpenGL.  This produces an
		// antialiased alpha mask of the mesh shape, in the
		// destination alpha channel.
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_SHORT, sizeof(Sint16) * 2, coords);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);
		glDisableClientState(GL_VERTEX_ARRAY);

		glPopMatrix();
		
		// Set up desired fill style.
		m_current_styles[LEFT_STYLE].apply();

		// Apply fill, modulated with alpha mask.
		//
		// @@ TODO see note above about filling bounding box only.
		glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(100000, 0);
		glVertex2f(100000, 100000);
		glVertex2f(0, 100000);
		glEnd();

// xxxxx ??? Hm, is our mask still intact, or did we just erase it?
// 		if (m_current_styles[LEFT_STYLE].needs_second_pass())
// 		{
// 			m_current_styles[LEFT_STYLE].apply_second_pass();
// 			glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);
// 			m_current_styles[LEFT_STYLE].cleanup_second_pass();
// 		}

		// @@ hm, there is perhaps more state that needs
		// fixing here, or setting elsewhere.
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif // MULTIPASS_ANTIALIASING
	}


	void	draw_line_strip(const void* coords, int vertex_count)
	// Draw the line strip formed by the sequence of points.
	{
		// Set up current style.
		m_current_styles[LINE_STYLE].apply();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		apply_matrix(m_current_matrix);

		// Send the line-strip to OpenGL
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_SHORT, sizeof(Sint16) * 2, coords);
		glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
		glDisableClientState(GL_VERTEX_ARRAY);

		glPopMatrix();
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
		assert(bi);

		apply_color(color);

		gameswf::point a, b, c, d;
		m.transform(&a, gameswf::point(coords.m_x_min, coords.m_y_min));
		m.transform(&b, gameswf::point(coords.m_x_max, coords.m_y_min));
		m.transform(&c, gameswf::point(coords.m_x_min, coords.m_y_max));
		d.m_x = b.m_x + c.m_x - a.m_x;
		d.m_y = b.m_y + c.m_y - a.m_y;

		glBindTexture(GL_TEXTURE_2D, bi->m_texture_id);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glBegin(GL_TRIANGLE_STRIP);

		glTexCoord2f(uv_coords.m_x_min, uv_coords.m_y_min);
		glVertex2f(a.m_x, a.m_y);

		glTexCoord2f(uv_coords.m_x_max, uv_coords.m_y_min);
		glVertex2f(b.m_x, b.m_y);

		glTexCoord2f(uv_coords.m_x_min, uv_coords.m_y_max);
		glVertex2f(c.m_x, c.m_y);

		glTexCoord2f(uv_coords.m_x_max, uv_coords.m_y_max);
		glVertex2f(d.m_x, d.m_y);

		glEnd();
	}
	
	void begin_submit_mask()
	{
	    glEnable(GL_STENCIL_TEST); 
	    glClearStencil(0);
	    glClear(GL_STENCIL_BUFFER_BIT);
	    glColorMask(0,0,0,0);	// disable framebuffer writes
	    glEnable(GL_STENCIL_TEST);	// enable stencil buffer for "marking" the mask
	    glStencilFunc(GL_ALWAYS, 1, 1);	// always passes, 1 bit plane, 1 as mask
	    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);	// we set the stencil buffer to 1 where we draw any polygon
							// keep if test fails, keep if test passes but buffer test fails
							// replace if test passes 
	}
	
	void end_submit_mask()
	{	     
	    glColorMask(1,1,1,1);	// enable framebuffer writes
	    glStencilFunc(GL_EQUAL, 1, 1);	// we draw only where the stencil is 1 (where the mask was drawn)
	    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	// don't change the stencil buffer    
	}
	
	void disable_mask()
	{	       
	    glDisable(GL_STENCIL_TEST); 
	}
	
};	// end struct render_handler_ogl


// bitmap_info_ogl implementation



void	hardware_resample(int bytes_per_pixel, int src_width, int src_height, uint8* src_data, int dst_width, int dst_height)
// Code from Alex Streit
//
// Sets the current texture to a resampled/expanded version of the
// given image data.
{
	assert(bytes_per_pixel == 3 || bytes_per_pixel == 4);

	unsigned int	in_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;
	unsigned int	out_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;

	// alex: use the hardware to resample the image
	// issue: does not work when image > allocated window size!
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	{
		char* temp = new char[dst_width * dst_height * bytes_per_pixel];
		//memset(temp,255,w*h*3);
		glTexImage2D(GL_TEXTURE_2D, 0, in_format, dst_width, dst_height, 0, out_format, GL_UNSIGNED_BYTE, temp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src_width, src_height, out_format, GL_UNSIGNED_BYTE, src_data);

		glLoadIdentity();
		glViewport(0, 0, dst_width, dst_height);
		glOrtho(0, dst_width, 0, dst_height, 0.9, 1.1);
		glColor3f(1, 1, 1);
		glNormal3f(0, 0, 1);
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0, (float) src_height / dst_height);
			glVertex3f(0, 0, -1);
			glTexCoord2f( (float) src_width / dst_width, (float) src_height / dst_height);
			glVertex3f((float) dst_width, 0, -1);
			glTexCoord2f( (float) src_width / dst_width, 0);
			glVertex3f((float) dst_width, (float) dst_height, -1);
			glTexCoord2f(0, 0);
			glVertex3f(0, (float) dst_height, -1);
		}
		glEnd();
		glCopyTexImage2D(GL_TEXTURE_2D, 0, out_format, 0,0, dst_width, dst_height, 0);
		delete temp;
	}
	glPopAttrib();
	glPopMatrix();
	glPopMatrix();
}


void	generate_mipmaps(unsigned int internal_format, unsigned int input_format, int bytes_per_pixel, image::image_base* im)
// DESTRUCTIVELY generate mipmaps of the given image.  The image data
// and width/height of im are munged in this process.
{
	int	level = 1;
	while (im->m_width > 1 || im->m_height > 1)
	{
		if (bytes_per_pixel == 3)
		{
			image::make_next_miplevel((image::rgb*) im);
		}
		else
		{
			image::make_next_miplevel((image::rgba*) im);
		}

		glTexImage2D(GL_TEXTURE_2D, level, internal_format, im->m_width, im->m_height, 0,
			     input_format, GL_UNSIGNED_BYTE, im->m_data);
		level++;
	}
}


void	software_resample(
	int bytes_per_pixel,
	int src_width,
	int src_height,
	int src_pitch,
	uint8* src_data,
	int dst_width,
	int dst_height)
// Code from Alex Streit
//
// Creates an OpenGL texture of the specified dst dimensions, from a
// resampled version of the given src image.  Does a bilinear
// resampling to create the dst image.
{
	assert(bytes_per_pixel == 3 || bytes_per_pixel == 4);

	assert(dst_width >= src_width);
	assert(dst_height >= src_height);

	unsigned int	internal_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;
	unsigned int	input_format = bytes_per_pixel == 3 ? GL_RGB : GL_RGBA;

	// FAST bi-linear filtering
	// the code here is designed to be fast, not readable
	Uint8* rescaled = new Uint8[dst_width * dst_height * bytes_per_pixel];
	float Uf, Vf;		// fractional parts
	float Ui, Vi;		// integral parts
	float w1, w2, w3, w4;	// weighting
	Uint8* psrc;
	Uint8* pdst = rescaled;
	// i1,i2,i3,i4 are the offsets of the surrounding 4 pixels
	const int i1 = 0;
	const int i2 = bytes_per_pixel;
	int i3 = src_pitch;
	int i4 = src_pitch + bytes_per_pixel;
	// change in source u and v
	float dv = (float)(src_height-2) / dst_height;
	float du = (float)(src_width-2) / dst_width;
	// source u and source v
	float U;
	float V=0;

#define BYTE_SAMPLE(offset)	\
	(Uint8) (w1 * psrc[i1 + (offset)] + w2 * psrc[i2 + (offset)] + w3 * psrc[i3 + (offset)] + w4 * psrc[i4 + (offset)])

	if (bytes_per_pixel == 3)
	{
		for (int v = 0; v < dst_height; ++v)
		{
			Vf = modff(V, &Vi);
			V+=dv;
			U=0;

			for (int u = 0; u < dst_width; ++u)
			{
				Uf = modff(U, &Ui);
				U+=du;

				w1 = (1 - Uf) * (1 - Vf);
				w2 = Uf * (1 - Vf);
				w3 = (1 - Uf) * Vf;
				w4 = Uf * Vf;
				psrc = &src_data[(int) (Vi * src_pitch) + (int) (Ui * bytes_per_pixel)];

				*pdst++ = BYTE_SAMPLE(0);	// red
				*pdst++ = BYTE_SAMPLE(1);	// green
				*pdst++ = BYTE_SAMPLE(2);	// blue

				psrc += 3;
			}
		}

#ifdef DEBUG_WRITE_TEXTURES_TO_PPM
		static int s_image_sequence = 0;
		char temp[256];
		sprintf(temp, "image%d.ppm", s_image_sequence++);
		FILE* f = fopen(temp, "wb");
		if (f)
		{
			fprintf(f, "P6\n# test code\n%d %d\n255\n", dst_width, dst_height);
			fwrite(rescaled, dst_width * dst_height * 3, 1, f);
			fclose(f);
		}
#endif
	}
	else
	{
		assert(bytes_per_pixel == 4);

		for (int v = 0; v < dst_height; ++v)
		{
			Vf = modff(V, &Vi);
			V+=dv;
			U=0;

			for (int u = 0; u < dst_width; ++u)
			{
				Uf = modff(U, &Ui);
				U+=du;

				w1 = (1 - Uf) * (1 - Vf);
				w2 = Uf * (1 - Vf);
				w3 = (1 - Uf) * Vf;
				w4 = Uf * Vf;
				psrc = &src_data[(int) (Vi * src_pitch) + (int) (Ui * bytes_per_pixel)];

				*pdst++ = BYTE_SAMPLE(0);	// red
				*pdst++ = BYTE_SAMPLE(1);	// green
				*pdst++ = BYTE_SAMPLE(2);	// blue
				*pdst++ = BYTE_SAMPLE(3);	// alpha

				psrc += 4;
			}
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, dst_width, dst_height, 0, input_format, GL_UNSIGNED_BYTE, rescaled);

#if GENERATE_MIPMAPS
	// Build mipmaps.
	image::image_base	im(rescaled, dst_width, dst_height, dst_width * bytes_per_pixel);
	generate_mipmaps(internal_format, input_format, bytes_per_pixel, &im);
#endif // GENERATE_MIPMAPS

	delete [] rescaled;
}


bitmap_info_ogl::bitmap_info_ogl()
// Make a placeholder bitmap_info.  Must be filled in later before
// using.
{
	m_texture_id = 0;
	m_original_width = 0;
	m_original_height = 0;
}


bitmap_info_ogl::bitmap_info_ogl(int width, int height, Uint8* data)
// Initialize this bitmap_info to an alpha image
// containing the specified data (1 byte per texel).
//
// !! Munges *data in order to create mipmaps !!
{
	assert(width > 0);
	assert(height > 0);
	assert(data);

	m_texture_id = 0;
	
	// Create the texture.

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, (GLuint*)&m_texture_id);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	m_original_width = width;
	m_original_height = height;

	#ifndef NDEBUG
	// You must use power-of-two dimensions!!
	int	w = 1; while (w < width) { w <<= 1; }
	int	h = 1; while (h < height) { h <<= 1; }
	assert(w == width);
	assert(h == height);
	#endif // not NDEBUG

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);

	// Build mips.
	int	level = 1;
	while (width > 1 || height > 1)
	{
		render_handler_ogl::make_next_miplevel(&width, &height, data);
		glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
		level++;
	}
}


bitmap_info_ogl::bitmap_info_ogl(image::rgb* im)
// NOTE: This function destroys im's data in the process of making mipmaps.
{
	assert(im);

	// Create the texture.

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, (GLuint*)&m_texture_id);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#if GENERATE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif

	m_original_width = im->m_width;
	m_original_height = im->m_height;

	int	w = 1; while (w < im->m_width) { w <<= 1; }
	int	h = 1; while (h < im->m_height) { h <<= 1; }

	if (w != im->m_width
	    || h != im->m_height)
	{
#if (RESAMPLE_METHOD == 1)
		int	viewport_dim[2] = { 0, 0 };
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &viewport_dim[0]);
		if (w > viewport_dim[0]
		    || h > viewport_dim[1]
		    || im->m_width * 3 != im->m_pitch)
		{
			// Can't use hardware resample.  Either frame
			// buffer isn't big enough to fit the source
			// texture, or the source data isn't padded
			// quite right.
			software_resample(3, im->m_width, im->m_height, im->m_pitch, im->m_data, w, h);
		}
		else
		{
			hardware_resample(3, im->m_width, im->m_height, im->m_data, w, h);
		}
#elif (RESAMPLE_METHOD == 2)
		{
			// Faster/simpler software bilinear rescale.
			software_resample(3, im->m_width, im->m_height, im->m_pitch, im->m_data, w, h);
		}
#else
		{
			// Fancy but slow software resampling.
			image::rgb*	rescaled = image::create_rgb(w, h);
			image::resample(rescaled, 0, 0, w - 1, h - 1,
					im, 0, 0, (float) im->m_width, (float) im->m_height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rescaled->m_data);
#if GENERATE_MIPMAPS
			generate_mipmaps(GL_RGB, GL_RGB, 3, rescaled);
#endif // GENERATE_MIPMAPS

			delete rescaled;
		}
#endif
	}
	else
	{
		// Use original image directly.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, im->m_data);
#if GENERATE_MIPMAPS
		generate_mipmaps(GL_RGB, GL_RGB, 3, im);
#endif // GENERATE_MIPMAPS
	}
}


bitmap_info_ogl::bitmap_info_ogl(image::rgba* im)
// Version of the constructor that takes an image with alpha.
// NOTE: This function destroys im's data in the process of making mipmaps.
{
	assert(im);

	// Create the texture.

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, (GLuint*)&m_texture_id);
	glBindTexture(GL_TEXTURE_2D, m_texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_NEAREST ?
#if GENERATE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif

	m_original_width = im->m_width;
	m_original_height = im->m_height;

	int	w = 1; while (w < im->m_width) { w <<= 1; }
	int	h = 1; while (h < im->m_height) { h <<= 1; }

	if (w != im->m_width
	    || h != im->m_height)
	{
#if (RESAMPLE_METHOD == 1)
		int	viewport_dim[2] = { 0, 0 };
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &viewport_dim[0]);
		if (w > viewport_dim[0]
		    || h > viewport_dim[1]
		    || im->m_width * 4 != im->m_pitch)
		{
			// Can't use hardware resample.  Either frame
			// buffer isn't big enough to fit the source
			// texture, or the source data isn't padded
			// quite right.
			software_resample(4, im->m_width, im->m_height, im->m_pitch, im->m_data, w, h);
		}
		else
		{
			hardware_resample(4, im->m_width, im->m_height, im->m_data, w, h);
		}
#elif (RESAMPLE_METHOD == 2)
		{
			// Faster/simpler software bilinear rescale.
			software_resample(4, im->m_width, im->m_height, im->m_pitch, im->m_data, w, h);
		}
#else
		{
			// Fancy but slow software resampling.
			image::rgba*	rescaled = image::create_rgba(w, h);
			image::resample(rescaled, 0, 0, w - 1, h - 1,
					im, 0, 0, (float) im->m_width, (float) im->m_height);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rescaled->m_data);
#if GENERATE_MIPMAPS
			generate_mipmaps(GL_RGBA, GL_RGBA, 4, rescaled);
#endif // GENERATE_MIPMAPS

			delete rescaled;
		}
#endif
	}
	else
	{
		// Use original image directly.
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, im->m_data);
#if GENERATE_MIPMAPS
		generate_mipmaps(GL_RGBA, GL_RGBA, 4, im);
#endif // GENERATE_MIPMAPS
	}
}


gameswf::render_handler*	gameswf::create_render_handler_ogl()
// Factory.
{
	return new render_handler_ogl;
}


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
