// gameswf_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some implementation code for the gameswf SWF player library.


#ifndef GAMESWF_IMPL_H
#define GAMESWF_IMPL_H


#include "gameswf.h"
#include "gameswf_action.h"
#include "gameswf_types.h"
#include "gameswf_log.h"
#include <assert.h>
#include "base/container.h"
#include "base/utility.h"
#include "base/smart_ptr.h"
#include <stdarg.h>


namespace jpeg { struct input; }


namespace gameswf
{
	struct action_buffer;
	struct bitmap_character_def;
        struct bitmap_info;
	struct character;
	struct character_def;
	struct display_info;
	struct execute_tag;
	struct font;
	struct movie_root;
	struct sound_sample : public resource //virtual public ref_counted
	{
		virtual sound_sample*	cast_to_sound_sample() { return this; }
	};
	struct stream;


	//
	// swf_event
	//
	// For embedding event handlers in place_object_2
	struct swf_event
	{
		// NOTE: DO NOT USE THESE AS VALUE TYPES IN AN
		// array<>!  They cannot be moved!  The private
		// operator=(const swf_event&) should help guard
		// against that.

		event_id	m_event;
		action_buffer	m_action_buffer;
		as_value	m_method;

		swf_event();
		void	attach_to(character* ch) const;
		void	read(stream* in, Uint32 flags);

	private:
		// DON'T USE THESE
		swf_event(const swf_event& s) { QASSERT(0); }
		void	operator=(const swf_event& s) { QASSERT(0); }
	};



	void save_extern_movie(movie_interface* m);

	// Extra internal interfaces added to movie_definition
	struct movie_definition_sub : public movie_definition
	{
		virtual const array<execute_tag*>&	get_playlist(int frame_number) = 0;
		virtual const array<execute_tag*>*	get_init_actions(int frame_number) = 0;
		virtual smart_ptr<resource>	get_exported_resource(const tu_string& symbol) = 0;
		virtual character_def*	get_character_def(int id) = 0;

		virtual bool	get_labeled_frame(const char* label, int* frame_number) = 0;

		// For use during creation.
		virtual int	get_loading_frame() const = 0;
		virtual void	add_character(int id, character_def* ch) = 0;
		virtual void	add_font(int id, font* ch) = 0;
		virtual font*	get_font(int id) = 0;
		virtual void	add_execute_tag(execute_tag* c) = 0;
		virtual void	add_init_action(int sprite_id, execute_tag* c) = 0;
		virtual void	add_frame_name(const char* name) = 0;
		virtual void	set_jpeg_loader(jpeg::input* j_in) = 0;
		virtual jpeg::input*	get_jpeg_loader() = 0;
		virtual bitmap_character_def*	get_bitmap_character(int character_id) = 0;
		virtual void	add_bitmap_character(int character_id, bitmap_character_def* ch) = 0;
		virtual sound_sample*	get_sound_sample(int character_id) = 0;
		virtual void	add_sound_sample(int character_id, sound_sample* sam) = 0;
		virtual void	export_resource(const tu_string& symbol, resource* res) = 0;
		virtual void	add_import(const char* source_url, int id, const char* symbol_name) = 0;
		virtual void	add_bitmap_info(bitmap_info* ch) = 0;

		virtual create_bitmaps_flag	get_create_bitmaps() const = 0;
		virtual create_font_shapes_flag	get_create_font_shapes() const = 0;
	};


	// For internal use.
	movie_definition_sub*	create_movie_sub(const char* filename);
	movie_definition_sub*	create_library_movie_sub(const char* filename);
	movie_interface*	create_library_movie_inst_sub(movie_definition_sub* md);

//v for extern movies
	movie_interface*	create_library_movie_inst(movie_definition* md);
	movie_interface*        get_current_root();
	void set_current_root(movie_interface* m);
	const char* get_workdir();
	void set_workdir(const char* dir);
	void delete_unused_root();

	struct movie : public movie_interface
	{
		virtual void set_extern_movie(movie_interface* m) { }
		virtual movie_interface*	get_extern_movie() { return NULL; }

		virtual movie_definition*	get_movie_definition() { return NULL; }
		virtual movie_root*	        get_root() { return NULL; }
		virtual movie_interface*	get_root_interface() { return NULL; }
		virtual movie*	                get_root_movie() { return NULL; }

		virtual float	                get_pixel_scale() const { return 1.0f; }
		virtual character*	        get_character(int id) { return NULL; }

		virtual matrix	                get_world_matrix() const { return matrix::identity; }
		virtual cxform	                get_world_cxform() const { return cxform::identity; }

		//
		// display-list management.
		//

		virtual execute_tag*	find_previous_replace_or_add_tag(int current_frame, int depth, int id)
		{
			return NULL;
		}

		virtual character*	add_display_object(
			Uint16 character_id,
			const char*		 name,
			const array<swf_event*>& event_handlers,
			Uint16			 depth,
			bool			 replace_if_depth_is_occupied,
			const cxform&		 color_transform,
			const matrix&		 mat,
			float			 ratio,
			Uint16			clip_depth)
		{
			return NULL;
		}

		virtual void	move_display_object(
			Uint16		depth,
			bool		use_cxform,
			const cxform&	color_transform,
			bool		use_matrix,
			const matrix&	mat,
			float		ratio,
			Uint16		clip_depth)
		{
		}

		virtual void	replace_display_object(
			Uint16		character_id,
			const char*	name,
			Uint16		depth,
			bool		use_cxform,
			const cxform&	color_transform,
			bool		use_matrix,
			const matrix&	mat,
			float		ratio,
			Uint16		clip_depth)
		{
		}

		virtual void	replace_display_object(
			character*	ch,
			const char*	name,
			Uint16		depth,
			bool		use_cxform,
			const cxform&	color_transform,
			bool		use_matrix,
			const matrix&	mat,
			float		ratio,
			Uint16		clip_depth)
		{
		}

		virtual void	remove_display_object(Uint16 depth, int id)	{}

		virtual void	set_background_color(const rgba& color) {}
		virtual void	set_background_alpha(float alpha) {}
		virtual float	get_background_alpha() const { return 1.0f; }
		virtual void	set_display_viewport(int x0, int y0, int width, int height) {}

		virtual void	add_action_buffer(action_buffer* a) { QASSERT(0); }

		virtual void	goto_frame(int target_frame_number) { QASSERT(0); }
		virtual bool	goto_labeled_frame(const char* label) { QASSERT(0); return false; }

		virtual void	set_play_state(play_state s) {}
		virtual play_state	get_play_state() const { QASSERT(0); return STOP; }

		virtual void	notify_mouse_state(int x, int y, int buttons)
		// The host app uses this to tell the movie where the
		// user's mouse pointer is.
		{
		}

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		// Use this to retrieve the last state of the mouse, as set via
		// notify_mouse_state().
		{
			QASSERT(0);
		}

		struct drag_state
		{
			movie*	m_character;
			bool	m_lock_center;
			bool	m_bound;
			float	m_bound_x0;
			float	m_bound_y0;
			float	m_bound_x1;
			float	m_bound_y1;

			drag_state()
				:
				m_character(0), m_lock_center(0), m_bound(0),
				m_bound_x0(0), m_bound_y0(0), m_bound_x1(1), m_bound_y1(1)
			{
				LOG_LINE;
			}
		};
		virtual void	get_drag_state(drag_state* st) { QASSERT(0); *st = drag_state(); }
		virtual void	set_drag_state(const drag_state& st) { QASSERT(0); }
		virtual void	stop_drag() { QASSERT(0); }


		// External
		virtual void	set_variable(const char* path_to_var, const char* new_value)
		{
			QASSERT(0);
		}

		// External
		virtual void	set_variable(const char* path_to_var, const wchar_t* new_value)
		{
			QASSERT(0);
		}

		// External
		virtual const char*	get_variable(const char* path_to_var) const
		{
			QASSERT(0);
			return "";
		}

		virtual void * get_userdata() { QASSERT(0); return NULL; }
		virtual void set_userdata(void *) { QASSERT(0); }

		// External
		virtual bool	has_looped() const { return true; }


		//
		// Mouse/Button interface.
		//

		virtual movie* get_topmost_mouse_entity(float x, float y) { return NULL; }
		virtual bool	get_track_as_menu() const { return false; }
		virtual void	on_button_event(event_id id) { on_event(id); }


		//
		// ActionScript.
		//


		virtual movie*	get_relative_target(const tu_string& name)
		{
			QASSERT(0);	
			return NULL;
		}

		// ActionScript event handler.  Returns true if a handler was called.
		virtual bool	on_event(event_id id) { return false; }

		int    add_interval_timer(void *timer)
		{
			log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
			return -1;	// ???
		}
		
		void    clear_interval_timer(int x)
		{
			log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		}
		
		virtual void    do_something(void *timer)
		{
			log_msg("FIXME: %s: unimplemented\n", __FUNCTION__);
		}
		
		// Special event handler; sprites also execute their frame1 actions on this event.
		virtual void	on_event_load() { on_event(event_id::LOAD); }

#if 0
		// tulrich: @@ is there a good reason these are in the
		// vtable?  I.e. can the caller just call
		// on_event(event_id::SOCK_DATA) instead of
		// on_event_xmlsocket_ondata()?
		virtual void	on_event_xmlsocket_ondata() { on_event(event_id::SOCK_DATA); }
		virtual void	on_event_xmlsocket_onxml() { on_event(event_id::SOCK_XML); }
		virtual void	on_event_interval_timer() { on_event(event_id::TIMER); }
		virtual void	on_event_load_progress() { on_event(event_id::LOAD_PROGRESS); }
#endif
		// as_object_interface stuff
		virtual void	set_member(const tu_stringi& name, const as_value& val) { QASSERT(0); }
		virtual bool	get_member(const tu_stringi& name, as_value* val) { QASSERT(0); return false; }

		virtual void	call_frame_actions(const as_value& frame_spec) { QASSERT(0); }

		virtual float	get_timer() const { return 0.0f; }
		virtual movie*	to_movie() { return this; }

		virtual void	clone_display_object(const tu_string& name, const tu_string& newname, Uint16 depth) { QASSERT(0); }
		virtual void	remove_display_object(const tu_string& name) { QASSERT(0); }

		// Forward vararg call to version taking va_list.
		virtual const char*	call_method(const char* method_name, const char* method_arg_fmt, ...)
		{
			va_list	args;
			va_start(args, method_arg_fmt);
			const char*	result = call_method_args(method_name, method_arg_fmt, args);
			va_end(args);

			return result;
		}

		virtual const char*	call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
		// Override this if you implement call_method.
		{
			QASSERT(0);
			return NULL;
		}

		virtual void	execute_frame_tags(int frame, bool state_only = false) {}

		// External.
		virtual void	attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
		{
			QASSERT(0);
		}

		virtual void	set_display_callback(void (*callback)(void*), void* user_ptr)
		// Override me to provide this functionality.
		{
		}
	};


	// Information about how to display a character.
	struct display_info
	{
		movie*	m_parent;
		int	m_depth;
		cxform	m_color_transform;
		matrix	m_matrix;
		float	m_ratio;
                Uint16 	m_clip_depth;

		display_info()
			:
			m_parent(NULL),
			m_depth(0),
			m_ratio(0.0f),
                        m_clip_depth(0)
		{
			LOG_LINE;
		}

		void	concatenate(const display_info& di)
		// Concatenate the transforms from di into our
		// transforms.
		{
			LOG_LINE;
			m_depth = di.m_depth;
			LOG_LINE;
			m_color_transform.concatenate(di.m_color_transform);
			LOG_LINE;
			m_matrix.concatenate(di.m_matrix);
			LOG_LINE;
			m_ratio = di.m_ratio;
                        m_clip_depth = di.m_clip_depth;
		}
	};


	// character is a live, stateful instance of a character_def.
	// It represents a single active element in a movie.
	struct character : public movie
	{
		int		m_id;
		movie*		m_parent;
		tu_string	m_name;
		int		m_depth;
		cxform		m_color_transform;
		matrix		m_matrix;
		float		m_ratio;
		Uint16		m_clip_depth;
		bool		m_visible;
		hash<event_id, as_value>	m_event_handlers;
		void		(*m_display_callback)(void*);
		void*		m_display_callback_user_ptr;

		character(movie* parent, int id)
			:
			m_id(id),
			m_parent(parent),
			m_depth(-1),
			m_ratio(0.0f),
			m_clip_depth(0),
			m_visible(true),
			m_display_callback(NULL),
			m_display_callback_user_ptr(NULL)
			
		{
			LOG_LINE;
			QASSERT((parent == NULL && m_id == -1)
			       || (parent != NULL && m_id >= 0));
		}

		// Accessors for basic display info.
		int	get_id() const { return m_id; }
		movie*	get_parent() const { return m_parent; }
		void set_parent(movie* parent) { m_parent = parent; }  // for extern movie
		int	get_depth() const { return m_depth; }
		void	set_depth(int d) { m_depth = d; }
		const matrix&	get_matrix() const { return m_matrix; }
		void	set_matrix(const matrix& m)
		{
			QASSERT(m.is_valid());
			m_matrix = m;
		}
		const cxform&	get_cxform() const { return m_color_transform; }
		void	set_cxform(const cxform& cx) { m_color_transform = cx; }
		void	concatenate_cxform(const cxform& cx) { m_color_transform.concatenate(cx); }
		void	concatenate_matrix(const matrix& m) { m_matrix.concatenate(m); }
		float	get_ratio() const { return m_ratio; }
		void	set_ratio(float f) { m_ratio = f; }
		Uint16	get_clip_depth() const { return m_clip_depth; }
		void	set_clip_depth(Uint16 d) { m_clip_depth = d; }

		void	set_name(const char* name) { m_name = name; }
		const tu_string&	get_name() const { return m_name; }

		// For edit_text support (Flash 5).  More correct way
		// is to do "text_character.text = whatever", via
		// set_member().
		virtual const char*	get_text_name() const { return ""; }
		virtual void	set_text_value(const char* new_text) { QASSERT(0); }

		virtual matrix	get_world_matrix() const
		// Get our concatenated matrix (all our ancestor transforms, times our matrix).  Maps
		// from our local space into "world" space (i.e. root movie space).
		{
			matrix	m;
			LOG_LINE;
			if (m_parent)
			{
				LOG_LINE;
				m = m_parent->get_world_matrix();
			}
			LOG_LINE;
			m.concatenate(get_matrix());

			return m;
		}

		virtual cxform	get_world_cxform() const
		// Get our concatenated color transform (all our ancestor transforms,
		// times our cxform).  Maps from our local space into normal color space.
		{
			cxform	m;
			LOG_LINE;
			if (m_parent)
			{
				LOG_LINE;
				m = m_parent->get_world_cxform();
			}
			LOG_LINE;
			m.concatenate(get_cxform());

			return m;
		}

		// Event handler accessors.
		bool	get_event_handler(event_id id, as_value* result)
		{
			LOG_LINE;
			return m_event_handlers.get(id, result);
		}
		void	set_event_handler(event_id id, const as_value& method)
		{
			LOG_LINE;
			m_event_handlers.set(id, method);
		}

		// Movie interfaces.  By default do nothing.  sprite_instance and some others override these.
		virtual void	display() {}
		virtual float	get_height() { return 0; }
		virtual float	get_width() { return 0; }

		virtual movie*	get_root_movie() { return m_parent->get_root_movie(); }
		virtual int	get_current_frame() const { QASSERT(0); return 0; }
		virtual bool	has_looped() const { QASSERT(0); return false; }
		virtual void	restart() { /*QASSERT(0);*/ }
		virtual void	advance(float delta_time) {}	// for buttons and sprites
		virtual void	goto_frame(int target_frame) {}
		virtual bool	get_accept_anim_moves() const { return true; }

		virtual void	get_drag_state(drag_state* st) { QASSERT(m_parent); m_parent->get_drag_state(st); }

		virtual void	set_visible(bool visible) { m_visible = visible; }
		virtual bool	get_visible() const { return m_visible; }

		virtual void	set_display_callback(void (*callback)(void*), void* user_ptr)
		{
			LOG_LINE;
			m_display_callback = callback;
			m_display_callback_user_ptr = user_ptr;
		}

		virtual void	do_display_callback()
		{
			LOG_LINE;
			if (m_display_callback)
			{
				LOG_LINE;
				(*m_display_callback)(m_display_callback_user_ptr);
			}
		}

		virtual void	get_mouse_state(int* x, int* y, int* buttons)
		{
			LOG_LINE;
			get_parent()->get_mouse_state(x, y, buttons);
		}

		// Utility.
		void	do_mouse_drag();
	};


	// For characters that don't store unusual state in their instances.
	struct generic_character : public character
	{
		character_def*	m_def;

		generic_character(character_def* def, movie* parent, int id)
			:
			character(parent, id),
			m_def(def)
		{
			QASSERT(m_def);
		}

		virtual void	display()
		{
			m_def->display(this);	// pass in transform info
			do_display_callback();
		}

		// @@ tulrich: these are used for finding bounds; TODO
		// need to do this using enclose_transformed_rect(),
		// not by scaling the local height/width!

		virtual float	get_height()
		{
			LOG_LINE;
			matrix	m = get_world_matrix();
			LOG_LINE;
			float	h = m_def->get_height_local() * m.m_[1][1];
			return h;
		}

		virtual float	get_width()
		{
			LOG_LINE;
			matrix	m = get_world_matrix();
			LOG_LINE;
			float	w = m_def->get_width_local() * m.m_[0][0];
			return w;
		}

		// new, from Vitaly.
		virtual movie*	get_topmost_mouse_entity(float x, float y)
		{
			QASSERT(get_visible());	// caller should check this.

			LOG_LINE;
			matrix	m = get_matrix();
			point	p;
			LOG_LINE;
			m.transform_by_inverse(&p, point(x, y));

			LOG_LINE;
			if (m_def->point_test_local(p.m_x, p.m_y))
			{
				// The mouse is inside the shape.
				return this;
			}
			return NULL;
		}
	};


	struct bitmap_character_def : public character_def
	{
		virtual gameswf::bitmap_info*	get_bitmap_info() = 0;
	};

#if 1
	// Bitmap character
	struct bitmap_character : public bitmap_character_def
	{
		bitmap_character(bitmap_info* bi)
			:
			m_bitmap_info(bi)
		{
			LOG_LINE;
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
			LOG_LINE;
			return m_bitmap_info.get_ptr();
		}

	private:
		smart_ptr<gameswf::bitmap_info>	m_bitmap_info;
	};

#endif

	// Execute tags include things that control the operation of
	// the movie.  Essentially, these are the events associated
	// with a frame.
	struct execute_tag
	{
		virtual ~execute_tag() {}
		virtual void	execute(movie* m) {}
		virtual void	execute_state(movie* m) {}
		virtual void	execute_state_reverse(movie* m, int frame) { execute_state(m); }
		virtual bool	is_remove_tag() const { return false; }
		virtual bool	is_action_tag() const { return false; }
		virtual uint32	get_depth_id_of_replace_or_add_tag() const { return static_cast<uint32>(-1); }
	};


	//
	// Loader callbacks.
	//
	
	// Register a loader function for a certain tag type.  Most
	// standard tags are handled within gameswf.  Host apps might want
	// to call this in order to handle special tag types.
	typedef void (*loader_function)(stream* input, int tag_type, movie_definition_sub* m);
	void	register_tag_loader(int tag_type, loader_function lf);
	
	
	// Tag loader functions.
	void	null_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	set_background_color_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	jpeg_tables_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_jpeg3_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_shape_morph_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_font_info_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_edit_text_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	place_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_bits_lossless_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	sprite_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_video_stream(stream* in, int tag_type, movie_definition_sub* m);
	void	read_video_frame(stream *in, int tag_type, movie_definition_sub *m);
	void	end_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	remove_object_2_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_character_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	frame_label_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	export_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	import_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	define_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	start_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	button_sound_loader(stream* in, int tag_type, movie_definition_sub* m);
	void	do_init_action_loader(stream* in, int tag_type, movie_definition_sub* m);
	// sound_stream_loader();	// head, head2, block


}	// end namespace gameswf


#endif // GAMESWF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
