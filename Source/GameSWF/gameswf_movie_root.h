// gameswf_movie_root.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_MOVIE_ROOT_H
#define GAMESWF_MOVIE_ROOT_H

#include "gameswf.h"
#include "gameswf_button.h"

namespace gameswf
{
	class Timer;
	class movie_def_impl;

	//
	// movie_root
	//
	// Global, shared root state for a movie and all its characters.
	//

	struct movie_root : public movie_interface
	{
		smart_ptr<movie_def_impl>	m_def;
		smart_ptr<movie>	m_movie;
		int			m_viewport_x0, m_viewport_y0, m_viewport_width, m_viewport_height;
		float			m_pixel_scale;

		rgba			m_background_color;
		float			m_timer;
		int			m_mouse_x, m_mouse_y, m_mouse_buttons;
		void *			m_userdata;
		movie::drag_state	m_drag_state;	// @@ fold this into m_mouse_button_state?
		mouse_button_state	m_mouse_button_state;
		bool			m_on_event_load_called;

		// Flags for event handlers
		bool			m_on_event_xmlsocket_ondata_called;
		bool			m_on_event_xmlsocket_onxml_called;
		bool			m_on_event_load_progress_called;
		array<Timer *>	m_interval_timers;

		movie_root(movie_def_impl* def);
		~movie_root();

		// @@ should these delegate to m_movie?  Probably...
		virtual void	set_member(const tu_stringi& name, const as_value& val);
		virtual bool	get_member(const tu_stringi& name, as_value* val);
		virtual movie*	to_movie();
		
		void	set_root_movie(movie* root_movie);

		void	set_display_viewport(int x0, int y0, int w, int h);

		void	notify_mouse_state(int x, int y, int buttons);
		virtual void	get_mouse_state(int* x, int* y, int* buttons);
		movie*	get_root_movie();
		void	stop_drag();

		movie_definition*	get_movie_definition();
		uint32	get_file_bytes() const;
		virtual int    add_interval_timer(void *timer);
	
		virtual void    clear_interval_timer(int x);
	
		virtual void    do_something(void *timer);
		int	get_current_frame() const;
		float	get_frame_rate() const;
		virtual float	get_pixel_scale() const;
		// @@ Is this one necessary?
		character*	get_character(int character_id);
		void	set_background_color(const rgba& color);
		void	set_background_alpha(float alpha);
		float	get_background_alpha() const;
		float	get_timer() const;
		void	restart();
		void	advance(float delta_time);
		// 0-based!!
		void	goto_frame(int target_frame_number);

		virtual bool	has_looped() const;

		void	display();

		virtual bool	goto_labeled_frame(const char* label);
		virtual void	set_play_state(play_state s);
		virtual play_state	get_play_state() const;

		virtual void	set_variable(const char* path_to_var, const char* new_value);
		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value);
		virtual const char*	get_variable(const char* path_to_var) const;
		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...);
		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args);
		virtual void	set_visible(bool visible);
		virtual bool	get_visible() const;
		virtual void * get_userdata();
		virtual void set_userdata(void * ud );
		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void* user_ptr), void* user_ptr);
	};

}	// end namespace gameswf


#endif // GAMESWF_MOVIE_ROOT_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
