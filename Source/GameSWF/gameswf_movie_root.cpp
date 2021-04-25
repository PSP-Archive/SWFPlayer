// gameswf_movie_root.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf_movie_root.h"
#include "gameswf_render.h"
#include "gameswf_timers.h"
#include "gameswf_movie_def_impl.h"

namespace gameswf
{
	movie_root::movie_root(movie_def_impl* def)
		:
		m_def(def),
		m_movie(NULL),
		m_viewport_x0(0),
		m_viewport_y0(0),
		m_viewport_width(1),
		m_viewport_height(1),
		m_pixel_scale(1.0f),
		m_background_color(0, 0, 0, 255),
		m_timer(0.0f),
		m_mouse_x(0),
		m_mouse_y(0),
		m_mouse_buttons(0),
		m_userdata(NULL),
		m_on_event_load_called(false),
		m_on_event_xmlsocket_ondata_called(false),
		m_on_event_xmlsocket_onxml_called(false),
		m_on_event_load_progress_called(false)
	{
		LOG_LINE;
		QASSERT(m_def != NULL);

		LOG_LINE;
		set_display_viewport(0, 0, (int) m_def->get_width_pixels(), (int) m_def->get_height_pixels());
	}

	movie_root::~movie_root()
	{
		LOG_LINE;
		QASSERT(m_def != NULL);
		LOG_LINE;
		m_movie = NULL;
		LOG_LINE;
		m_def = NULL;
	}

	// @@ should these delegate to m_movie?  Probably...
	void	movie_root::set_member(const tu_stringi& name, const as_value& val) {}
	bool	movie_root::get_member(const tu_stringi& name, as_value* val) { return false; }
	movie*	movie_root::to_movie() { QASSERT(0); return 0; } // @@ should this return m_movie.get_ptr()?
	

	void	movie_root::set_root_movie(movie* root_movie)
	{
		LOG_LINE;
		m_movie = root_movie;
		QASSERT(m_movie != NULL);
	}

	void	movie_root::set_display_viewport(int x0, int y0, int w, int h)
	{
		LOG_LINE;
		m_viewport_x0 = x0;
		LOG_LINE;
		m_viewport_y0 = y0;
		LOG_LINE;
		m_viewport_width = w;
		LOG_LINE;
		m_viewport_height = h;

		// Recompute pixel scale.
		LOG_LINE;
		float	scale_x = m_viewport_width / TWIPS_TO_PIXELS(m_def->m_frame_size.width());
		LOG_LINE;
		float	scale_y = m_viewport_height / TWIPS_TO_PIXELS(m_def->m_frame_size.height());
		LOG_LINE;
		m_pixel_scale = fmax(scale_x, scale_y);
	}


	void	movie_root::notify_mouse_state(int x, int y, int buttons)
	// The host app uses this to tell the movie where the
	// user's mouse pointer is.
	{
		LOG_LINE;
		m_mouse_x = x;
		LOG_LINE;
		m_mouse_y = y;
		LOG_LINE;
		m_mouse_buttons = buttons;
	}

	void	movie_root::get_mouse_state(int* x, int* y, int* buttons)
	// Use this to retrieve the last state of the mouse, as set via
	// notify_mouse_state().  Coordinates are in PIXELS, NOT TWIPS.
	{
		LOG_LINE;
		QASSERT(x);
		LOG_LINE;
		QASSERT(y);
		LOG_LINE;
		QASSERT(buttons);

		LOG_LINE;
		*x = m_mouse_x;
		LOG_LINE;
		*y = m_mouse_y;
		LOG_LINE;
		*buttons = m_mouse_buttons;
	}

	movie*	movie_root::get_root_movie()
	{
		LOG_LINE;
		return m_movie.get_ptr();
	}


	void	movie_root::stop_drag()
	{
		LOG_LINE;
		m_drag_state.m_character = NULL;
	}


	movie_definition*	movie_root::get_movie_definition()
	{
		LOG_LINE;
		return m_movie->get_movie_definition();
	}

	uint32	movie_root::get_file_bytes() const
	{
		LOG_LINE;
		return m_def->get_file_bytes();
	}

	int    movie_root::add_interval_timer(void *timer)
	{
		LOG_LINE;
		Timer *ptr = static_cast<Timer *>(timer);
		
		LOG_LINE;
		m_interval_timers.push_back(ptr);
		LOG_LINE;
		return m_interval_timers.size();
	}

	void    movie_root::clear_interval_timer(int x)
	{
		LOG_LINE;
		m_interval_timers.remove(x-1);
		//m_interval_timers[x]->clearInterval();
	}

	void    movie_root::do_something(void *timer)
	{
		LOG_LINE;
		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
	}
	
	// 0-based!!
	int	movie_root::get_current_frame() const
	{
		LOG_LINE;
		return m_movie->get_current_frame();
	}
	float	movie_root::get_frame_rate() const
	{
		LOG_LINE;
		return m_def->get_frame_rate();
	}

	float	movie_root::get_pixel_scale() const
	// Return the size of a logical movie pixel as
	// displayed on-screen, with the current device
	// coordinates.
	{
		LOG_LINE;
		return m_pixel_scale;
	}

	// @@ Is this one necessary?
	character*	movie_root::get_character(int character_id)
	{
		LOG_LINE;
		return m_movie->get_character(character_id);
	}

	void	movie_root::set_background_color(const rgba& color)
	{
		LOG_LINE;
		m_background_color = color;
	}

	void	movie_root::set_background_alpha(float alpha)
	{
		LOG_LINE;
		m_background_color.m_a = iclamp(frnd(alpha * 255.0f), 0, 255);
	}

	float	movie_root::get_background_alpha() const
	{
		LOG_LINE;
		return m_background_color.m_a / 255.0f;
	}

	float	movie_root::get_timer() const
	{
		LOG_LINE;
		return m_timer;
	}

	void	movie_root::restart()
	{
		LOG_LINE;
		m_movie->restart();
	}
	
	void	movie_root::advance(float delta_time)
	{
		int i;
		LOG_LINE;
		if (m_on_event_load_called == false)
		{
			// Must do loading events.  For child sprites this is
			// done by the dlist, but root movies don't get added
			// to a dlist, so we do it here.
			LOG_LINE;
			m_on_event_load_called = true;
			LOG_LINE;
			m_movie->on_event_load();
		}

		LOG_LINE;
		if (m_interval_timers.size() > 0) {
			LOG_LINE;
			for (i=0; i<m_interval_timers.size(); i++) {
				LOG_LINE;
				if (m_interval_timers[i]->expired()) {
					// printf("FIXME: Interval Timer Expired!\n");
					//m_movie->on_event_interval_timer();
					LOG_LINE;
					m_movie->do_something(m_interval_timers[i]);
					// clear_interval_timer(m_interval_timers[i]->getIntervalID()); // FIXME: we shouldn't really disable the timer here
				}
			}
		}
		
		
		LOG_LINE;
		m_timer += delta_time;
		// @@ TODO handle multi-frame catch-up stuff
		// here, and make it optional.  Make
		// movie::advance() a fixed framerate w/ no
		// dt.

		// Handle the mouse.
		LOG_LINE;
		m_mouse_button_state.m_topmost_entity =
			m_movie->get_topmost_mouse_entity(PIXELS_TO_TWIPS(m_mouse_x), PIXELS_TO_TWIPS(m_mouse_y));
		LOG_LINE;
		m_mouse_button_state.m_mouse_button_state_current = (m_mouse_buttons & 1);
		LOG_LINE;
		generate_mouse_button_events(&m_mouse_button_state);

		LOG_LINE;
		m_movie->advance(delta_time);
	}

	// 0-based!!
	void	movie_root::goto_frame(int target_frame_number)
	{
		LOG_LINE;
		m_movie->goto_frame(target_frame_number);
	}

	bool	movie_root::has_looped() const
	{
		LOG_LINE;
		return m_movie->has_looped();
	}

	void	movie_root::display()
	{
		LOG_LINE;
		if (m_movie->get_visible() == false)
		{
			// Don't display.
			return;
		}

		LOG_LINE;
		gameswf::render::begin_display(
			m_background_color,
			m_viewport_x0, m_viewport_y0,
			m_viewport_width, m_viewport_height,
			m_def->m_frame_size.m_x_min, m_def->m_frame_size.m_x_max,
			m_def->m_frame_size.m_y_min, m_def->m_frame_size.m_y_max);

		LOG_LINE;
		m_movie->display();

		LOG_LINE;
		gameswf::render::end_display();
	}

	bool	movie_root::goto_labeled_frame(const char* label)
	{
		LOG_LINE;
		int	target_frame = -1;
		LOG_LINE;
		if (m_def->get_labeled_frame(label, &target_frame))
		{
			LOG_LINE;
			goto_frame(target_frame);
			return true;
		}
		else
		{
			LOG_LINE;
			IF_VERBOSE_ACTION(
				log_error("error: movie_impl::goto_labeled_frame('%s') unknown label\n", label));
			return false;
		}
	}

	void	movie_root::set_play_state(play_state s)
	{
		LOG_LINE;
		m_movie->set_play_state(s);
	}
	movie_interface::play_state	movie_root::get_play_state() const
	{
		LOG_LINE;
		return m_movie->get_play_state();
	}

	/* movie_root */
	void	movie_root::set_variable(const char* path_to_var, const char* new_value)
	{
		LOG_LINE;
		m_movie->set_variable(path_to_var, new_value);
	}

	/* movie_root */
	void	movie_root::set_variable(const char* path_to_var, const wchar_t* new_value)
	{
		LOG_LINE;
		m_movie->set_variable(path_to_var, new_value);
	}

	const char*	movie_root::get_variable(const char* path_to_var) const
	{
		LOG_LINE;
		return m_movie->get_variable(path_to_var);
	}

	/*movie_root*/
	// For ActionScript interfacing convenience.
	const char*	movie_root::call_method(const char* method_name, const char* method_arg_fmt, ...)
	{
		QASSERT(m_movie != NULL);

		LOG_LINE;
		va_list	args;
		LOG_LINE;
		va_start(args, method_arg_fmt);
		LOG_LINE;
		const char*	result = m_movie->call_method_args(method_name, method_arg_fmt, args);
		LOG_LINE;
		va_end(args);

		LOG_LINE;
		return result;
	}

	/*movie_root*/
	const char*	movie_root::call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
	{
		QASSERT(m_movie != NULL);
		LOG_LINE;
		return m_movie->call_method_args(method_name, method_arg_fmt, args);
	}

	void	movie_root::set_visible(bool visible)
	{
		LOG_LINE;
		m_movie->set_visible(visible);
	}
	bool	movie_root::get_visible() const
	{
		LOG_LINE;
		return m_movie->get_visible();
	}

	void * movie_root::get_userdata()
	{
		LOG_LINE;
		return m_userdata;
	}
	void movie_root::set_userdata(void * ud )
	{
		LOG_LINE;
		m_userdata = ud;
	}

	void	movie_root::attach_display_callback(const char* path_to_object, void (*callback)(void* user_ptr), void* user_ptr)
	{
		LOG_LINE;
		m_movie->attach_display_callback(path_to_object, callback, user_ptr);
	}


}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
