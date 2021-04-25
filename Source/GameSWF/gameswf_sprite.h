// gameswf_sprite.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_SPRITE_H
#define GAMESWF_SPRITE_H


#include "gameswf.h"
#include "gameswf_styles.h"
#include "gameswf_dlist.h"

namespace gameswf
{
  	//
  	// sprite_instance
  	//

  	struct sprite_instance : public character
  	{
  		smart_ptr<movie_definition_sub>	m_def;
  		movie_root*	m_root;
  
  		display_list	m_display_list;
  		array<action_buffer*>	m_action_list;
  
  		play_state	m_play_state;
  		int		m_current_frame;
  		float		m_time_remainder;
  		bool		m_update_frame;
  		bool		m_has_looped;
  		bool	        m_accept_anim_moves;	// once we've been moved by ActionScript, don't accept moves from anim tags.
  		array<bool>	m_init_actions_executed;	// a bit-array class would be ideal for this
  
  		as_environment	m_as_environment;
  
  		enum mouse_state
  		{
  			UP = 0,
  			DOWN,
  			OVER
  		};
  		mouse_state m_mouse_state;
  
  		sprite_instance(movie_definition_sub* def, movie_root* r, movie* parent, int id);
  
  		// sprite instance of add_interval_handler()
  		virtual int    add_interval_timer(void *timer);
   		virtual void    clear_interval_timer(int x);
  		virtual void    do_something(void *timer);
  		virtual ~sprite_instance();
  		movie_interface*	get_root_interface();
  		movie_root*	get_root();
  		movie*	get_root_movie();
  		movie_definition*	get_movie_definition();
  		float	get_width();
  		float	get_height();
  		int	get_current_frame() const;
  		int	get_frame_count() const;
  		void	set_play_state(play_state s);
		play_state	get_play_state() const;
  		character*	get_character(int character_id);
  		float	get_background_alpha() const;
  		float	get_pixel_scale() const;
  		virtual void	get_mouse_state(int* x, int* y, int* buttons);
  		void	set_background_color(const rgba& color);
  		float	get_timer() const;
  		void	restart();
  		virtual bool	has_looped() const;
  		virtual bool	get_accept_anim_moves() const;
  		inline int	transition(int a, int b) const
  		// Combine the flags to avoid a conditional. It would be faster with a macro.
  		{
  			LOG_LINE;
  			return (a << 2) | b;
  		}
  		bool can_handle_mouse_event();
  		virtual movie*	get_topmost_mouse_entity(float x, float y);
  		void	increment_frame_and_check_for_loop();
  		virtual void	advance(float delta_time);
  		void	execute_frame_tags(int frame, bool state_only = false);
  		void	execute_frame_tags_reverse(int frame);
  		execute_tag*	find_previous_replace_or_add_tag(int frame, int depth, int id);
  		void	execute_remove_tags(int frame);
  		void	do_actions();
  		void	goto_frame(int target_frame_number);
  		bool	goto_labeled_frame(const char* label);
  		void	display();
  		character*	add_display_object(
  			Uint16 character_id,
  			const char* name,
  			const array<swf_event*>& event_handlers,
  			Uint16 depth,
  			bool replace_if_depth_is_occupied,
  			const cxform& color_transform,
  			const matrix& matrix,
  			float ratio,
  			Uint16 clip_depth);
  		void	move_display_object(
  			Uint16 depth,
  			bool use_cxform,
  			const cxform& color_xform,
  			bool use_matrix,
  			const matrix& mat,
  			float ratio,
  			Uint16 clip_depth);
  		void	replace_display_object(
  			Uint16 character_id,
  			const char* name,
  			Uint16 depth,
  			bool use_cxform,
  			const cxform& color_transform,
  			bool use_matrix,
  			const matrix& mat,
  			float ratio,
  			Uint16 clip_depth);
  		void	replace_display_object(
  			character* ch,
  			const char* name,
  			Uint16 depth,
  			bool use_cxform,
  			const cxform& color_transform,
  			bool use_matrix,
  			const matrix& mat,
  			float ratio,
  			Uint16 clip_depth);
  		void	remove_display_object(Uint16 depth, int id);
  		void	add_action_buffer(action_buffer* a);
  		int	get_id_at_depth(int depth);
  		virtual void	set_variable(const char* path_to_var, const char* new_value);
  		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value);
  		virtual const char*	get_variable(const char* path_to_var) const;
  		virtual void	set_member(const tu_stringi& name, const as_value& val);
  		virtual bool	get_member(const tu_stringi& name, as_value* val);
  		virtual movie*	get_relative_target(const tu_string& name);
  		virtual void	call_frame_actions(const as_value& frame_spec);
  		virtual void	set_drag_state(const drag_state& st);
  		virtual void	stop_drag();
  		virtual void	get_drag_state(drag_state* st);
  		void	clone_display_object(const tu_string& name, const tu_string& newname, Uint16 depth);
  		void	remove_display_object(const tu_string& name);
  		virtual bool	on_event(event_id id);
  		virtual void	on_event_load();
  		virtual void	on_event_xmlsocket_onxml();
  		virtual void	on_event_interval_timer();
  		virtual void	on_event_load_progress();
  		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args);
  		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr);
  	};



	//
	// sprite_definition
	//


	// A sprite is a mini movie-within-a-movie.  It doesn't define
	// its own characters; it uses the characters from the parent
	// movie, but it has its own frame counter, display list, etc.
	//
	// The sprite implementation is divided into a
	// sprite_definition and a sprite_instance.  The _definition
	// holds the immutable data for a sprite, while the _instance
	// contains the state for a specific instance being updated
	// and displayed in the parent movie's display list.

	struct sprite_definition : public movie_definition_sub
	{
		movie_definition_sub*	     m_movie_def;		// parent movie.
		array<array<execute_tag*> >  m_playlist;	// movie control events for each frame.
		stringi_hash<int>	     m_named_frames;	// stores 0-based frame #'s
		int	                     m_frame_count;
		int	                     m_loading_frame;

		sprite_definition(movie_definition_sub* m);

		~sprite_definition();

		// overloads from movie_definition
		virtual float	get_width_pixels() const;
		virtual float	get_height_pixels() const;
		virtual int	get_frame_count() const;
		virtual float	get_frame_rate() const;
		virtual int	get_loading_frame() const;
		virtual int	get_version() const;
		virtual void	add_character(int id, character_def* ch);
		virtual void	add_font(int id, font* ch);
		virtual font*	get_font(int id);
		virtual void	set_jpeg_loader(jpeg::input* j_in);
		virtual jpeg::input*	get_jpeg_loader();
		virtual bitmap_character_def*	get_bitmap_character(int id);
		virtual void	add_bitmap_character(int id, bitmap_character_def* ch);
		virtual sound_sample*	get_sound_sample(int id);
		virtual void	add_sound_sample(int id, sound_sample* sam);

		// @@ would be nicer to not inherit these...
		virtual create_bitmaps_flag	get_create_bitmaps() const;
		virtual create_font_shapes_flag	get_create_font_shapes() const;
		virtual int	get_bitmap_info_count() const;
		virtual bitmap_info*	get_bitmap_info(int i) const;
		virtual void	add_bitmap_info(bitmap_info* bi);

		virtual void	export_resource(const tu_string& symbol, resource* res);
		virtual smart_ptr<resource>	get_exported_resource(const tu_string& sym);
		virtual void	add_import(const char* source_url, int id, const char* symbol);
		virtual void	visit_imported_movies(import_visitor* v);
		virtual void	resolve_import(const char* source_url, movie_definition* d);
		virtual character_def*	get_character_def(int id);
		virtual void	generate_font_bitmaps();

		virtual void	output_cached_data(tu_file* out, const cache_options& options);
		virtual void	input_cached_data(tu_file* in);

		virtual movie_interface*	create_instance();

		// overloads from character_def
		virtual character*	create_character_instance(movie* parent, int id);

		virtual void	add_execute_tag(execute_tag* c);
		virtual void	add_init_action(int sprite_id, execute_tag* c);
		virtual void	add_frame_name(const char* name);
		bool	get_labeled_frame(const char* label, int* frame_number);
		const array<execute_tag*>&	get_playlist(int frame_number);
		virtual const array<execute_tag*>*	get_init_actions(int frame_number);
		void	read(stream* in);
	};

}	// end namespace gameswf


#endif // GAMESWF_SHAPE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
