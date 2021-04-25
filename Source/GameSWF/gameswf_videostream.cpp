// gameswf_videostream.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "CTypes.h"
#include "CGfx.h"
#include "CTextureManager.h"
#include "gameswf_videostream.h"
#include "gameswf_movie_root.h"
#include "gameswf_stream.h"
#include "gameswf.h"
#include "gameswf_impl.h"
#include "gameswf_timers.h"
#include "gameswf_render.h"
#include "CSWFFileHandler.h"
#include "CFont.h"

extern AVInputFormat	flv_iformat;			// From ffmpeg

namespace gameswf
{
	// For built-in videostream ActionScript methods.
	as_object*	s_video_builtins = 0;	// shared among all videos.

	//
	// video built-in ActionScript methods
	//
	void	video_play(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);
		video->set_play_state(movie_interface::PLAY);
	}

	void	video_stop(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);
		video->set_play_state(movie_interface::STOP);
	}

	void	video_goto_and_play(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);

		if (fn.nargs < 1)
		{
			log_error("error: video_goto_and_play needs one arg\n");
			return;
		}

		int	target_frame = int(fn.arg(0).to_number() - 1);	// Convert to 0-based

		video->goto_frame(target_frame);
		video->set_play_state(movie_interface::PLAY);
	}

	void	video_goto_and_stop(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);

		if (fn.nargs < 1)
		{
			log_error("error: video_goto_and_stop needs one arg\n");
			return;
		}

		int	target_frame = int(fn.arg(0).to_number() - 1);	// Convert to 0-based

		video->goto_frame(target_frame);
		video->set_play_state(movie_interface::STOP);
	}

	void	video_next_frame(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);

		int frame_count = video->get_frame_count();
		int current_frame = video->get_current_frame();
		if (current_frame < frame_count)
		{
			video->goto_frame(current_frame + 1);
		}
		video->set_play_state(movie_interface::STOP);
	}

	void	video_prev_frame(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);

		int current_frame = video->get_current_frame();
		if (current_frame > 0)
		{
			video->goto_frame(current_frame - 1);
		}
		video->set_play_state(movie_interface::STOP);
	}

	void	video_get_bytes_loaded(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);

		fn.result->set_int(video->get_root()->get_file_bytes());
	}

	void	video_get_bytes_total(const fn_call& fn)
	{
		video_instance* video = (video_instance*) fn.this_ptr;
		if (video == NULL)
		{
			video = (video_instance*) fn.env->get_target();
		}
		QASSERT(video);

		fn.result->set_int(video->get_root()->get_file_bytes());
	}


	void	video_builtins_init()
	{
		if (s_video_builtins)
		{
			return;
		}

		s_video_builtins = new as_object;
		s_video_builtins->set_member("play", &video_play);
		s_video_builtins->set_member("stop", &video_stop);
		s_video_builtins->set_member("gotoAndStop", &video_goto_and_stop);
		s_video_builtins->set_member("gotoAndPlay", &video_goto_and_play);
		s_video_builtins->set_member("nextFrame", &video_next_frame);
		s_video_builtins->set_member("prevFrame", &video_prev_frame);
		s_video_builtins->set_member("getBytesLoaded", &video_get_bytes_loaded);
		s_video_builtins->set_member("getBytesTotal", &video_get_bytes_loaded);
	}

	void	video_builtins_clear()
	{
		if (s_video_builtins)
		{
			delete s_video_builtins;
			s_video_builtins = 0;
		}
	}


	static void	execute_actions(as_environment* env, const array<action_buffer*>& action_list)
	// Execute the actions in the action list, in the given
	// environment.
	{
  		for (int i = 0; i < action_list.size(); i++)
  		{
  			action_list[i]->execute(env);
  		}
	}




	//
	// video_instance
	//
	video_instance::video_instance(movie_definition_sub* def, movie_root* r, movie* parent, int id)
  		:
  		character(parent, id),
  		m_def(def),
  		m_root(r),
  		m_play_state(PLAY),
  		m_current_frame(0),
  		m_time_remainder(0),
  		m_update_frame(true),
  		m_has_looped(false),
  		m_accept_anim_moves(true),
  		m_mouse_state(UP)
	{
  		QASSERT(m_def != NULL);
  		QASSERT(m_root != NULL);
	  	
  		//m_root->add_ref();	// @@ circular!
  		m_as_environment.set_target(this);

  		video_builtins_init();

  		// Initialize the flags for init action executed.
  		m_init_actions_executed.resize(m_def->get_frame_count());
  		memset(&m_init_actions_executed[0], 0,
  				sizeof(m_init_actions_executed[0]) * m_init_actions_executed.size());
	}

	// video instance of add_interval_handler()
	int    video_instance::add_interval_timer(void *timer)
	{
  		// log_error("FIXME: %s:\n", __FUNCTION__);
  		return m_root->add_interval_timer(timer);
	}

	void    video_instance::clear_interval_timer(int x)
	{
  		// log_error("FIXME: %s:\n", __FUNCTION__);
  		m_root->clear_interval_timer(x);
	}


	void    video_instance::do_something(void *timer)
	{
		as_value	val;
  		as_object      *obj, *this_ptr;
  		as_environment *as_env;

  		//printf("FIXME: %s:\n", __FUNCTION__);
  		Timer *ptr = (Timer *)timer;
  		//log_error("INTERVAL ID is %d\n", ptr->getIntervalID());

  		const as_value&	timer_method = ptr->getASFunction();
  		as_env = ptr->getASEnvironment();
  		this_ptr = ptr->getASObject();
  		obj = ptr->getObject();
  		//m_as_environment.push(obj);
	  	
  		as_c_function_ptr	cfunc = timer_method.to_c_function();
  		if (cfunc) {
  			// It's a C function. Call it.
  			//log_error("Calling C function for interval timer\n");
  			//(*cfunc)(&val, obj, as_env, 0, 0);
  			(*cfunc)(fn_call(&val, obj, &m_as_environment, 0, 0));
	  		
  		} else if (as_as_function* as_func = timer_method.to_as_function()) {
  			// It's an ActionScript function. Call it.
  			as_value method;
  			//log_error("Calling ActionScript function for interval timer\n");
  			(*as_func)(fn_call(&val, (as_object_interface *)this_ptr, as_env, 0, 0));
  			//(*as_func)(&val, (as_object_interface *)this_ptr, &m_as_environment, 1, 1);
  		} else {
  			log_error("error in call_method(): method is not a function\n");
  		}    
	}	

	video_instance::~video_instance()
	{
  		m_display_list.clear();
  		//m_root->drop_ref();
	}

	movie_interface*	video_instance::get_root_interface()
	{
		return m_root;
	}

	movie_root*	video_instance::get_root()
	{
		return m_root;
	}

	movie*	video_instance::get_root_movie()
	{
		return m_root->get_root_movie();
	}

	movie_definition*	video_instance::get_movie_definition()
	{
		return m_def.get_ptr();
	}

	float	video_instance::get_width()
	{
		return ( float )( ( ( video_definition * )m_def.get_ptr() )->m_width ) / CSWFFileHandler::GetScale();
	}

	float	video_instance::get_height()
	{
		return ( float )( ( ( video_definition * )m_def.get_ptr() )->m_height ) / CSWFFileHandler::GetScale();
	}

	int	video_instance::get_current_frame() const
	{
		return m_current_frame;
	}

	int	video_instance::get_frame_count() const
	{
  		return m_def->get_frame_count();
	}

	void	video_instance::set_play_state(play_state s)
	// Stop or play the video.
	{
  		if (m_play_state != s)
  		{
  			m_time_remainder = 0;
  		}

  		m_play_state = s;
	}

	video_instance::play_state	video_instance::get_play_state() const
	{
  		return m_play_state;
	}


	character*	video_instance::get_character(int character_id)
	{
	//			return m_def->get_character_def(character_id);
  		// @@ TODO -- look through our dlist for a match
  		return NULL;
	}

	float	video_instance::get_background_alpha() const
	{
  		// @@ this doesn't seem right...
  		return m_root->get_background_alpha();
	}

	float	video_instance::get_pixel_scale() const
	{
  		return m_root->get_pixel_scale();
	}

	void	video_instance::get_mouse_state(int* x, int* y, int* buttons)
	{
  		m_root->get_mouse_state(x, y, buttons);
	}

	void	video_instance::set_background_color(const rgba& color)
	{
  		m_root->set_background_color(color);
	}

	float	video_instance::get_timer() const
	{
  		return m_root->get_timer();
	}

	void	video_instance::restart()
	{
  		m_current_frame = 0;
  		m_time_remainder = 0;
  		m_update_frame = true;
  		m_has_looped = false;
  		m_play_state = PLAY;

  		execute_frame_tags(m_current_frame);
  		m_display_list.update();
	}


	bool	video_instance::has_looped() const
	{
		return m_has_looped;
	}

	bool	video_instance::get_accept_anim_moves() const
	{
		return m_accept_anim_moves;
	}

	bool video_instance::can_handle_mouse_event()
	// Return true if we have any mouse event handlers.
	{
  		// We should cache this!
  		as_value dummy;

  		// Functions that qualify as mouse event handlers.
  		const char* FN_NAMES[] = {
  			"onKeyPress",
  			"onRelease",
  			"onDragOver",
  			"onDragOut",
  			"onPress",
  			"onReleaseOutside",
  			"onRollout",
  			"onRollover",
  		};

		for (int i = 0; i < (int)ARRAYSIZE(FN_NAMES); i++)
		{
  			if (get_member(FN_NAMES[i], &dummy))
			{
  				return true;
  			}
  		}

  		// Event handlers that qualify as mouse event handlers.
  		const event_id::id_code EH_IDS[] = {
  			event_id::PRESS,
  			event_id::RELEASE,
  			event_id::RELEASE_OUTSIDE,
  			event_id::ROLL_OVER,
  			event_id::ROLL_OUT,
  			event_id::DRAG_OVER,
  			event_id::DRAG_OUT,
  		};
		for (int i = 0; i < (int)ARRAYSIZE(EH_IDS); i++)
		{
  			if (get_event_handler(EH_IDS[i], &dummy))
			{
  				return true;
  			}
  		}

  		return false;
	}


	movie*	video_instance::get_topmost_mouse_entity(float x, float y)
	// Return the topmost entity that the given point
	// covers that can receive mouse events.  NULL if
	// none.  Coords are in parent's frame.
	{
		if (get_visible() == false)
		{
  			return NULL;
		}

		matrix	m = get_matrix();
  		point	p;
  		m.transform_by_inverse(&p, point(x, y));

  		int i, n = m_display_list.get_character_count();
  		// Go backwards, to check higher objects first.
  		for (i = n - 1; i >= 0; i--)
  		{
  			character* ch = m_display_list.get_character(i);
	  		
  			if (ch != NULL && ch->get_visible())
  			{
  				movie*	te = ch->get_topmost_mouse_entity(p.m_x, p.m_y);
  				if (te)
  				{
  					// The containing entity that 1) is closest to root and 2) can
  					// handle mouse events takes precedence.
  					if (can_handle_mouse_event()) {
  						return this;
  					} else {
  						return te;
  					}
  				}
  			}
  		}

  		return NULL;
	}


	void	video_instance::increment_frame_and_check_for_loop()
	// Increment m_current_frame, and take care of looping.
	{
  		m_current_frame++;

  		int	frame_count = m_def->get_frame_count();
  		if (m_current_frame >= frame_count)
  		{
  			// Loop.
  			m_current_frame = 0;
  			m_has_looped = true;
  			if (frame_count > 1)
  			{
  				m_display_list.reset();
  			}
  		}
	}

	void	video_instance::advance(float delta_time)
	{
		// Keep this (particularly m_as_environment) alive during execution!
  		smart_ptr<as_object_interface>	this_ptr(this);

  		QASSERT(m_def != NULL && m_root != NULL);

  		// Advance everything in the display list.
  		m_display_list.advance(delta_time);

  		// mouse drag.
  		character::do_mouse_drag();

  		m_time_remainder += delta_time;

  		const float	frame_time = 1.0f / m_root->get_frame_rate();	// @@ cache this

  		// Check for the end of frame
  		if (m_time_remainder >= frame_time)
  		{
  			m_time_remainder -= frame_time;

  			// Update current and next frames.
  			if (m_play_state == PLAY)
  			{
  				int	current_frame0 = m_current_frame;
  				increment_frame_and_check_for_loop();

  				// Execute the current frame's tags.
  				if (m_current_frame != current_frame0)
  				{
  					execute_frame_tags(m_current_frame);
  				}
  			}

  			// Dispatch onEnterFrame event.
  			on_event(event_id::ENTER_FRAME);

  			do_actions();

  			// Clean up display list (remove dead objects).
  			m_display_list.update();
  		}

  		// Skip excess time.  TODO root caller should
  		// loop to prevent this happening; probably
  		// only root should keep m_time_remainder, and
  		// advance(dt) should be a discrete tick()
  		// with no dt.
  		m_time_remainder = fmod(m_time_remainder, frame_time);
	}


	void	video_instance::execute_frame_tags(int frame, bool state_only)
	// Execute the tags associated with the specified frame.
	// frame is 0-based
	{
  		// Keep this (particularly m_as_environment) alive during execution!
  		smart_ptr<as_object_interface>	this_ptr(this);

  		QASSERT(frame >= 0);
  		QASSERT(frame < m_def->get_frame_count());

  		// Execute this frame's init actions, if necessary.
  		if (m_init_actions_executed[frame] == false)
  		{
  			const array<execute_tag*>*	init_actions = m_def->get_init_actions(frame);
  			if (init_actions && init_actions->size() > 0)
  			{
  				// Need to execute these actions.
  				for (int i= 0; i < init_actions->size(); i++)
  				{
  					execute_tag*	e = (*init_actions)[i];
  					e->execute(this);
  				}

  				// Mark this frame done, so we never execute these init actions
  				// again.
  				m_init_actions_executed[frame] = true;
  			}
  		}

  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);

  		for (int i = 0; i < playlist.size(); i++)
  		{
  			execute_tag*	e = playlist[i];
  			if (state_only)
  			{
  				e->execute_state(this);
  			}
  			else
  			{
  				e->execute(this);
  			}
  		}
	}



	void	video_instance::execute_frame_tags_reverse(int frame)
	// Execute the tags associated with the specified frame, IN REVERSE.
	// I.e. if it's an "add" tag, then we do a "remove" instead.
	// Only relevant to the display-list manipulation tags: add, move, remove, replace.
	//
	// frame is 0-based
	{
  		// Keep this (particularly m_as_environment) alive during execution!
  		smart_ptr<as_object_interface>	this_ptr(this);

  		QASSERT(frame >= 0);
  		QASSERT(frame < m_def->get_frame_count());

  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			execute_tag*	e = playlist[i];
  			e->execute_state_reverse(this, frame);
  		}
	}



	execute_tag*	video_instance::find_previous_replace_or_add_tag(int frame, int depth, int id)
	{
  		uint32	depth_id = ((depth & 0x0FFFF) << 16) | (id & 0x0FFFF);

  		for (int f = frame - 1; f >= 0; f--)
  		{
  			const array<execute_tag*>&	playlist = m_def->get_playlist(f);
  			for (int i = playlist.size() - 1; i >= 0; i--)
  			{
  				execute_tag*	e = playlist[i];
  				if (e->get_depth_id_of_replace_or_add_tag() == depth_id)
  				{
  					return e;
  				}
  			}
  		}

  		return NULL;
	}


	void	video_instance::execute_remove_tags(int frame)
	// Execute any remove-object tags associated with the specified frame.
	// frame is 0-based
	{
  		QASSERT(frame >= 0);
  		QASSERT(frame < m_def->get_frame_count());

  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			execute_tag*	e = playlist[i];
  			if (e->is_remove_tag())
  			{
  				e->execute_state(this);
  			}
  		}
	}


	void	video_instance::do_actions()
	// Take care of this frame's actions.
	{
  		// Keep m_as_environment alive during any method calls!
  		smart_ptr<as_object_interface>	this_ptr(this);

  		execute_actions(&m_as_environment, m_action_list);
  		m_action_list.resize(0);
	}



	void	video_instance::goto_frame(int target_frame_number)
	// Set the video state at the specified frame number.
	// 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
	{
//		IF_VERBOSE_DEBUG(log_msg("video::goto_frame(%d)\n", target_frame_number));//xxxxx

  		target_frame_number = iclamp(target_frame_number, 0, m_def->get_frame_count() - 1);

  		if (target_frame_number < m_current_frame)
  		{
  			for (int f = m_current_frame; f > target_frame_number; f--)
  			{
  				execute_frame_tags_reverse(f);
  			}

  			execute_frame_tags(target_frame_number, false);
  			m_display_list.update();
  		}
  		else if (target_frame_number > m_current_frame)
  		{
  			for (int f = m_current_frame + 1; f < target_frame_number; f++)
  			{
  				execute_frame_tags(f, true);
  			}

  			execute_frame_tags(target_frame_number, false);
  			m_display_list.update();
  		}

  		m_current_frame = target_frame_number;      

  		// goto_frame stops by default.
  		m_play_state = STOP;
	}


	bool	video_instance::goto_labeled_frame(const char* label)
	// Look up the labeled frame, and jump to it.
	{
  		int	target_frame = -1;
  		if (m_def->get_labeled_frame(label, &target_frame))
  		{
  			goto_frame(target_frame);
  			return true;
  		}
  		else
  		{
  			IF_VERBOSE_ACTION(
  				log_error("error: movie_impl::goto_labeled_frame('%s') unknown label\n", label));
  			return false;
  		}
	}


	void	video_instance::display()
	{
		video_definition * const	p_vid_def( ( video_definition * )m_def.get_ptr() );

  		if ( p_vid_def == NULL || get_visible() == false )
  		{
  			return;
  		}

		const Sint16	icoords[8] = 
		{
			(Sint16) 0.f, (Sint16) 0.f,
			(Sint16) get_width(), (Sint16) 0.f,
			(Sint16) 0.f, (Sint16) get_height(),
			(Sint16) get_width(), (Sint16) get_height(),
		};

		m_display_list.display();

  		do_display_callback();


		//
		//	Make sure we're definitely on screen as decoding is very expensive!
		//
//		if ( render::is_on_screen( &icoords[ 0 ], 4 ) == true )
		{
			//
			//	Decode the current frame
			//
			if ( p_vid_def->decode_frame( m_current_frame ) == true )
			{
				matrix	mat = get_world_matrix();

				render::set_matrix(mat);
				render::fill_style_color( 0, rgba( 255, 255, 255, 255 ) );
				render::draw_mesh_strip( &icoords[ 0 ], 4, p_vid_def->m_pBitmap );
			}
		}
	}

	character*	video_instance::add_display_object(
  		Uint16 character_id,
  		const char* name,
  		const array<swf_event*>& event_handlers,
  		Uint16 depth,
  		bool replace_if_depth_is_occupied,
  		const cxform& color_transform,
  		const matrix& matrix,
  		float ratio,
  		Uint16 clip_depth)
	// Add an object to the display list.
	{
		QASSERT(m_def != NULL);

  		character_def*	cdef = m_def->get_character_def(character_id);
  		if (cdef == NULL)
  		{
  			log_error("video::add_display_object(): unknown cid = %d\n", character_id);
  			return NULL;
  		}

  		// If we already have this object on this
  		// plane, then move it instead of replacing
  		// it.
  		character*	existing_char = m_display_list.get_character_at_depth(depth);
  		if (existing_char
  			&& existing_char->get_id() == character_id
  			&& ((name == NULL && existing_char->get_name().length() == 0)
  			|| (name && existing_char->get_name() == name)))
  		{
	//				IF_VERBOSE_DEBUG(log_msg("add changed to move on depth %d\n", depth));//xxxxxx
  			move_display_object(depth, true, color_transform, true, matrix, ratio, clip_depth);
  			return NULL;
  		}

  		QASSERT(cdef);
  		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
  		QASSERT(ch != NULL);
  		if (name != NULL && name[0] != 0)
  		{
  			ch->set_name(name);
  		}

  		// Attach event handlers (if any).
  		{for (int i = 0, n = event_handlers.size(); i < n; i++)
  		{
  			event_handlers[i]->attach_to(ch.get_ptr());
  		}}

  		m_display_list.add_display_object(
  			ch.get_ptr(),
  			depth,
  			replace_if_depth_is_occupied,
  			color_transform,
  			matrix,
  			ratio,
  			clip_depth);

  		QASSERT(ch == NULL || ch->get_ref_count() > 1);
  		return ch.get_ptr();
	}


	void	video_instance::move_display_object(
  		Uint16 depth,
  		bool use_cxform,
  		const cxform& color_xform,
  		bool use_matrix,
  		const matrix& mat,
  		float ratio,
  		Uint16 clip_depth)
	// Updates the transform properties of the object at
	// the specified depth.
	{
  		m_display_list.move_display_object(depth, use_cxform, color_xform, use_matrix, mat, ratio, clip_depth);
	}


	void	video_instance::replace_display_object(
  		Uint16 character_id,
  		const char* name,
  		Uint16 depth,
  		bool use_cxform,
  		const cxform& color_transform,
  		bool use_matrix,
  		const matrix& mat,
  		float ratio,
  		Uint16 clip_depth)
	{
  		QASSERT(m_def != NULL);

  		character_def*	cdef = m_def->get_character_def(character_id);
  		if (cdef == NULL)
  		{
  			log_error("video::replace_display_object(): unknown cid = %d\n", character_id);
  			return;
  		}
  		QASSERT(cdef);

  		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
  		QASSERT(ch != NULL);

  		if (name != NULL && name[0] != 0)
  		{
  			ch->set_name(name);
  		}

  		m_display_list.replace_display_object(
  			ch.get_ptr(),
  			depth,
  			use_cxform,
  			color_transform,
  			use_matrix,
  			mat,
  			ratio,
  			clip_depth);
	}


	void	video_instance::replace_display_object(
  		character* ch,
  		const char* name,
  		Uint16 depth,
  		bool use_cxform,
  		const cxform& color_transform,
  		bool use_matrix,
  		const matrix& mat,
  		float ratio,
  		Uint16 clip_depth)
	{
  		QASSERT(ch != NULL);

  		if (name != NULL && name[0] != 0)
  		{
  			ch->set_name(name);
  		}

  		m_display_list.replace_display_object(
  			ch,
  			depth,
  			use_cxform,
  			color_transform,
  			use_matrix,
  			mat,
  			ratio,
  			clip_depth);
	}


	void	video_instance::remove_display_object(Uint16 depth, int id)
	// Remove the object at the specified depth.
	// If id != -1, then only remove the object at depth with matching id.
	{
  		m_display_list.remove_display_object(depth, id);
	}


	void	video_instance::add_action_buffer(action_buffer* a)
	// Add the given action buffer to the list of action
	// buffers to be processed at the end of the next
	// frame advance.
	{
  		m_action_list.push_back(a);
	}


	int	video_instance::get_id_at_depth(int depth)
	// For debugging -- return the id of the character at the specified depth.
	// Return -1 if nobody's home.
	{
  		int	index = m_display_list.get_display_index(depth);
  		if (index == -1)
  		{
  			return -1;
  		}

  		character*	ch = m_display_list.get_display_object(index).m_character.get_ptr();

  		return ch->get_id();
	}


	//
	// ActionScript support
	//


	void	video_instance::set_variable(const char* path_to_var, const char* new_value)
	{
  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		if (path_to_var == NULL)
  		{
  			log_error("error: NULL path_to_var passed to set_variable()\n");
  			return;
  		}
  		if (new_value == NULL)
  		{
  			log_error("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
  			return;
  		}

  		array<with_stack_entry>	empty_with_stack;
  		tu_string	path(path_to_var);
  		as_value	val(new_value);

  		m_as_environment.set_variable(path, val, empty_with_stack);
	}

	void	video_instance::set_variable(const char* path_to_var, const wchar_t* new_value)
	{
  		if (path_to_var == NULL)
  		{
  			log_error("error: NULL path_to_var passed to set_variable()\n");
  			return;
  		}
  		if (new_value == NULL)
  		{
  			log_error("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
  			return;
  		}

  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		array<with_stack_entry>	empty_with_stack;
  		tu_string	path(path_to_var);
  		as_value	val(new_value);

  		m_as_environment.set_variable(path, val, empty_with_stack);
	}

	const char*	video_instance::get_variable(const char* path_to_var) const
	{
  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		array<with_stack_entry>	empty_with_stack;
  		tu_string	path(path_to_var);

  		// NOTE: this is static so that the string
  		// value won't go away after we return!!!
  		// It'll go away during the next call to this
  		// function though!!!  NOT THREAD SAFE!
  		static as_value	val;

  		val = m_as_environment.get_variable(path, empty_with_stack);

  		return val.to_string();	// ack!
	}


	void	video_instance::set_member(const tu_stringi& name, const as_value& val)
	// Set the named member to the value.  Return true if we have
	// that member; false otherwise.
	{
  		as_standard_member	std_member = get_standard_member(name);
  		switch (std_member)
  		{
  		default:
  		case M_INVALID_MEMBER:
  			break;
  		case M_X:
  			//if (name == "_x")
  		{
  			matrix	m = get_matrix();
  			m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.to_number());
  			set_matrix(m);

  			m_accept_anim_moves = false;

  			return;
  		}
  		case M_Y:
  			//else if (name == "_y")
  		{
  			matrix	m = get_matrix();
  			m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.to_number());
  			set_matrix(m);

  			m_accept_anim_moves = false;

  			return;
  		}
  		case M_XSCALE:
  			//else if (name == "_xscale")
  		{
  			matrix	m = get_matrix();

  			// Decompose matrix and insert the desired value.
  			float	x_scale = (float) val.to_number() / 100.f;	// input is in percent
  			float	y_scale = m.get_y_scale();
  			float	rotation = m.get_rotation();
  			m.set_scale_rotation(x_scale, y_scale, rotation);

  			set_matrix(m);
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_YSCALE:
  			//else if (name == "_yscale")
  		{
  			matrix	m = get_matrix();

  			// Decompose matrix and insert the desired value.
  			float	x_scale = m.get_x_scale();
  			float	y_scale = (float) val.to_number() / 100.f;	// input is in percent
  			float	rotation = m.get_rotation();
  			m.set_scale_rotation(x_scale, y_scale, rotation);

  			set_matrix(m);
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_ALPHA:
  			//else if (name == "_alpha")
  		{
  			// Set alpha modulate, in percent.
  			cxform	cx = get_cxform();
  			cx.m_[3][0] = float(val.to_number()) / 100.f;
  			set_cxform(cx);
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_VISIBLE:
  			//else if (name == "_visible")
  		{
  			set_visible(val.to_bool());
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_WIDTH:
  			//else if (name == "_width")
  		{
  			// @@ tulrich: is parameter in world-coords or local-coords?
  			matrix	m = get_matrix();
  			m.m_[0][0] = float(PIXELS_TO_TWIPS(val.to_number()));
  			float w = get_width();
  			if (fabsf(w) > 1e-6f)
  			{
  					m.m_[0][0] /= w;
  			}
  			set_matrix(m);
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_HEIGHT:
  			//else if (name == "_height")
  		{
  			// @@ tulrich: is parameter in world-coords or local-coords?
  			matrix	m = get_matrix();
  			m.m_[1][1] = float(PIXELS_TO_TWIPS(val.to_number()));
  			float h = get_width();
  			if (fabsf(h) > 1e-6f)
  			{
  				m.m_[1][1] /= h;
  			}
  			set_matrix(m);
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_ROTATION:
  			//else if (name == "_rotation")
  		{
  			matrix	m = get_matrix();

  			// Decompose matrix and insert the desired value.
  			float	x_scale = m.get_x_scale();
  			float	y_scale = m.get_y_scale();
  			float	rotation = (float) val.to_number() * float(M_PI) / 180.f;	// input is in degrees
  			m.set_scale_rotation(x_scale, y_scale, rotation);

  			set_matrix(m);
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_HIGHQUALITY:
  			//else if (name == "_highquality")
  		{
  			// @@ global { 0, 1, 2 }
	//				// Whether we're in high quality mode or not.
	//				val->set(true);
  			return;
  		}
  		case M_FOCUSRECT:
  			//else if (name == "_focusrect")
  		{
	//				// Is a yellow rectangle visible around a focused movie clip (?)
	//				val->set(false);
  			return;
  		}
  		case M_SOUNDBUFTIME:
  			//else if (name == "_soundbuftime")
  		{
  			// @@ global
	//				// Number of seconds before sound starts to stream.
	//				val->set(0.0);
  			return;
  		}
  		}	// end switch

  		// Not a built-in property.  See if we have a
  		// matching edit_text character in our display
  		// list.
  		bool	text_val = val.get_type() == as_value::STRING
  			|| val.get_type() == as_value::NUMBER;
  		if (text_val)
  		{
  			bool	success = false;
  			for (int i = 0, n = m_display_list.get_character_count(); i < n; i++)
  			{
  				character*	ch = m_display_list.get_character(i);
  				// CASE INSENSITIVE compare.  In ActionScript 2.0, this
  				// changes to CASE SENSITIVE!!!
  				if (name == ch->get_text_name())
  				{
  					const char* text = val.to_string();
  					ch->set_text_value(text);
  					success = true;
  				}
  			}
  			if (success) return;
  		}

  		// If that didn't work, set a variable within this environment.
  		m_as_environment.set_member(name, val);
	}


	bool	video_instance::get_member(const tu_stringi& name, as_value* val)
	// Set *val to the value of the named member and
	// return true, if we have the named member.
	// Otherwise leave *val alone and return false.
	{
  		as_standard_member	std_member = get_standard_member(name);
  		switch (std_member)
  		{
  		default:
  		case M_INVALID_MEMBER:
  			break;
  		case M_X:
  			//if (name == "_x")
  		{
  			matrix	m = get_matrix();
  			val->set_double(TWIPS_TO_PIXELS(m.m_[0][2]));
  			return true;
  		}
  		case M_Y:
  			//else if (name == "_y")
  		{
  			matrix	m = get_matrix();
  			val->set_double(TWIPS_TO_PIXELS(m.m_[1][2]));
  			return true;
  		}
  		case M_XSCALE:
  			//else if (name == "_xscale")
  		{
  			matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
  			float xscale = m.get_x_scale();
  			val->set_double(xscale * 100);		// result in percent
  			return true;
  		}
  		case M_YSCALE:
  			//else if (name == "_yscale")
  		{
  			matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
  			float yscale = m.get_y_scale();
  			val->set_double(yscale * 100);		// result in percent
  			return true;
  		}
  		case M_CURRENTFRAME:
  			//else if (name == "_currentframe")
  		{
  			val->set_int(m_current_frame + 1);
  			return true;
  		}
  		case M_TOTALFRAMES:
  			//else if (name == "_totalframes")
  		{
  			// number of frames.  Read only.
  			val->set_int(m_def->get_frame_count());
  			return true;
  		}
  		case M_ALPHA:
  			//else if (name == "_alpha")
  		{
  			// Alpha units are in percent.
  			val->set_double(get_cxform().m_[3][0] * 100.f);
  			return true;
  		}
  		case M_VISIBLE:
  			//else if (name == "_visible")
  		{
  			val->set_bool(get_visible());
  			return true;
  		}
  		case M_WIDTH:
  			//else if (name == "_width")
  		{
  			matrix	m = get_world_matrix();
  			rect	transformed_rect;

  			// @@ not sure about this...
  			rect	source_rect;
  			source_rect.m_x_min = 0;
  			source_rect.m_y_min = 0;
  			source_rect.m_x_max = (float) get_width();
  			source_rect.m_y_max = (float) get_height();

  			transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
  			val->set_double(TWIPS_TO_PIXELS(transformed_rect.width()));
  			return true;
  		}
  		case M_HEIGHT:
  			//else if (name == "_height")
  		{
  			rect	transformed_rect;

  			// @@ not sure about this...
  			rect	source_rect;
  			source_rect.m_x_min = 0;
  			source_rect.m_y_min = 0;
  			source_rect.m_x_max = (float) get_width();
  			source_rect.m_y_max = (float) get_height();

  			transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
  			val->set_double(TWIPS_TO_PIXELS(transformed_rect.height()));
  			return true;
  		}
  		case M_ROTATION:
  			//else if (name == "_rotation")
  		{
  			// Verified against Macromedia player using samples/test_rotation.swf
  			float	angle = get_matrix().get_rotation();

  			// Result is CLOCKWISE DEGREES, [-180,180]
  			angle *= 180.0f / float(M_PI);

  			val->set_double(angle);
  			return true;
  		}
  		case M_TARGET:
  			//else if (name == "_target")
  		{
  			// Full path to this object; e.g. "/_level0/video1/video2/ourvideo"
  			val->set_string("/_root");
  			return true;
  		}
  		case M_FRAMESLOADED:
  			//else if (name == "_framesloaded")
  		{
  			val->set_int(m_def->get_frame_count());
  			return true;
  		}
  		case M_NAME:
  			//else if (name == "_name")
  		{
  			val->set_tu_string(get_name());
  			return true;
  		}
  		case M_DROPTARGET:
  			//else if (name == "_droptarget")
  		{
  			// Absolute path in slash syntax where we were last dropped (?)
  			// @@ TODO
  			val->set_string("/_root");
  			return true;
  		}
  		case M_URL:
  			//else if (name == "_url")
  		{
  			// our URL.
  			val->set_string("gameswf");
  			return true;
  		}
  		case M_HIGHQUALITY:
  			//else if (name == "_highquality")
  		{
  			// Whether we're in high quality mode or not.
  			val->set_bool(true);
  			return true;
  		}
  		case M_FOCUSRECT:
  			//else if (name == "_focusrect")
  		{
  			// Is a yellow rectangle visible around a focused movie clip (?)
  			val->set_bool(false);
  			return true;
  		}
  		case M_SOUNDBUFTIME:
  			//else if (name == "_soundbuftime")
  		{
  			// Number of seconds before sound starts to stream.
  			val->set_double(0.0);
  			return true;
  		}
  		case M_XMOUSE:
  			//else if (name == "_xmouse")
  		{
  			// Local coord of mouse IN PIXELS.
  			int	x, y, buttons;
  			QASSERT(m_root);
  			m_root->get_mouse_state(&x, &y, &buttons);

  			matrix	m = get_world_matrix();

  			point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
  			point	b;
	  		
  			m.transform_by_inverse(&b, a);

  			val->set_double(TWIPS_TO_PIXELS(b.m_x));
  			return true;
  		}
  		case M_YMOUSE:
  			//else if (name == "_ymouse")
  		{
  			// Local coord of mouse IN PIXELS.
  			int	x, y, buttons;
  			QASSERT(m_root);
  			m_root->get_mouse_state(&x, &y, &buttons);

  			matrix	m = get_world_matrix();

  			point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
  			point	b;
	  		
  			m.transform_by_inverse(&b, a);

  			val->set_double(TWIPS_TO_PIXELS(b.m_y));
  			return true;
  		}
  		case M_PARENT:
  		{
  			val->set_as_object_interface(static_cast<as_object_interface*>(m_parent));
  			return true;
  		}
  		case M_ONLOAD:
  		{
  			if (m_as_environment.get_member(name, val))
  			{
  				return true;
  			}
  			// Optimization: if no hit, don't bother looking in the display list, etc.
  			return false;
  		}
  		}	// end switch

  		// Try variables.
  		if (m_as_environment.get_member(name, val))
  		{
  			return true;
  		}

  		// Not a built-in property.  Check items on our
  		// display list.
  		character*	ch = m_display_list.get_character_by_name_i(name);
  		if (ch)
  		{
  			// Found object.
  			val->set_as_object_interface(static_cast<as_object_interface*>(ch));
  			return true;
  		}

  		// Try static builtin functions.
  		QASSERT(s_video_builtins);
  		if (s_video_builtins->get_member(name, val))
  		{
  			return true;
  		}

  		return false;
	}


	movie*	video_instance::get_relative_target(const tu_string& name)
	// Find the movie which is one degree removed from us,
	// given the relative pathname.
	//
	// If the pathname is "..", then return our parent.
	// If the pathname is ".", then return ourself.  If
	// the pathname is "_level0" or "_root", then return
	// the root movie.
	//
	// Otherwise, the name should refer to one our our
	// named characters, so we return it.
	//
	// NOTE: In ActionScript 2.0, top level names (like
	// "_root" and "_level0") are CASE SENSITIVE.
	// Character names in a display list are CASE
	// SENSITIVE. Member names are CASE INSENSITIVE.  Gah.
	//
	// In ActionScript 1.0, everything seems to be CASE
	// INSENSITIVE.
	{
  		if (name == "." || name == "this")
  		{
  			return this;
  		}
  		else if (name == "..")
  		{
  			return get_parent();
  		}
  		else if (name == "_level0"
  				|| name == "_root")
  		{
  			return m_root->m_movie.get_ptr();
  		}

  		// See if we have a match on the display list.
  		return m_display_list.get_character_by_name(name);
	}


	void	video_instance::call_frame_actions(const as_value& frame_spec)
	// Execute the actions for the specified frame.  The
	// frame_spec could be an integer or a string.
	{
  		int	frame_number = -1;

  		// Figure out what frame to call.
  		if (frame_spec.get_type() == as_value::STRING)
  		{
  			if (m_def->get_labeled_frame(frame_spec.to_string(), &frame_number) == false)
  			{
  				// Try converting to integer.
  				frame_number = (int) frame_spec.to_number();
  			}
  		}
  		else
  		{
  			// convert from 1-based to 0-based
  			frame_number = (int) frame_spec.to_number() - 1;
  		}

  		if (frame_number < 0 || frame_number >= m_def->get_frame_count())
  		{
  			// No dice.
  			log_error("error: call_frame('%s') -- unknown frame\n", frame_spec.to_string());
  			return;
  		}

  		int	top_action = m_action_list.size();

  		// Execute the actions.
  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame_number);
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			execute_tag*	e = playlist[i];
  			if (e->is_action_tag())
  			{
  				e->execute(this);
  			}
  		}

  		// Execute any new actions triggered by the tag,
  		// leaving existing actions to be executed.
  		while (m_action_list.size() > top_action)
  		{
  			m_action_list[top_action]->execute(&m_as_environment);
  			m_action_list.remove(top_action);
  		}
  		QASSERT(m_action_list.size() == top_action);
	}


	void	video_instance::set_drag_state(const drag_state& st)
	{
  		m_root->m_drag_state = st;
	}

	void	video_instance::stop_drag()
	{
  		QASSERT(m_parent == NULL);	// we must be the root movie!!!
	  	
  		m_root->stop_drag();
	}


	void	video_instance::get_drag_state(drag_state* st)
	{
  		*st = m_root->m_drag_state;
	}


	void	video_instance::clone_display_object(const tu_string& name, const tu_string& newname, Uint16 depth)
	// Duplicate the object with the specified name and add it with a new name 
	// at a new depth.
	{
  		character* ch = m_display_list.get_character_by_name(name);
  		if (ch)
  		{
  			array<swf_event*>	dummy_event_handlers;

  			add_display_object(
  				ch->get_id(),
  				newname.c_str(),
  				dummy_event_handlers,
  				depth,
  				true,	// replace if depth is occupied
  				ch->get_cxform(),
  				ch->get_matrix(),
  				ch->get_ratio(),
  				ch->get_clip_depth());
  			// @@ TODO need to duplicate ch's event handlers, and presumably other members?
  			// Probably should make a character::clone() function to handle this.
  		}
	}


	void	video_instance::remove_display_object(const tu_string& name)
	// Remove the object with the specified name.
	{
  		character* ch = m_display_list.get_character_by_name(name);
  		if (ch)
  		{
  			// @@ TODO: should only remove movies that were created via clone_display_object --
  			// apparently original movies, placed by anim events, are immune to this.
  			remove_display_object(ch->get_depth(), ch->get_id());
  		}
	}


	bool	video_instance::on_event(event_id id)
	// Dispatch event handler(s), if any.
	{
		// Keep m_as_environment alive during any method calls!
  		smart_ptr<as_object_interface>	this_ptr(this);

  		bool called = false;
	  	
  		// First, check for built-in event handler.
  		{
  			as_value	method;
  			if (get_event_handler(id, &method))
  			{
  				// Dispatch.
  				call_method0(method, &m_as_environment, this);

  				called = true;
  				// Fall through and call the function also, if it's defined!
  				// (@@ Seems to be the behavior for mouse events; not tested & verified for
  				// every event type.)
  			}
  		}

		// Check for member function.
  		{
  			// In ActionScript 2.0, event method names are CASE SENSITIVE.
  			// In ActionScript 1.0, event method names are CASE INSENSITIVE.
  			const tu_stringi&	method_name = id.get_function_name().to_tu_stringi();
  			if (method_name.length() > 0)
  			{
  				as_value	method;
  				if (get_member(method_name, &method))
  				{
  					call_method0(method, &m_as_environment, this);
  					called = true;
  				}
  			}
  		}

  		return called;
	}


	void	video_instance::on_event_load()
	// Do the events that (appear to) happen as the movie
	// loads.  frame1 tags and actions are executed (even
	// before advance() is called).  Then the onLoad event
	// is triggered.
	{
  		execute_frame_tags(0);
  		do_actions();
  		on_event(event_id::LOAD);
	}

	// Do the events that happen when there is XML data waiting
	// on the XML socket connection.
	void	video_instance::on_event_xmlsocket_onxml()
	{
  		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
  		on_event(event_id::SOCK_XML);
	}

	// Do the events that (appear to) happen on a specified interval.
	void	video_instance::on_event_interval_timer()
	{
  		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
  		on_event(event_id::TIMER);
	}

	// Do the events that happen as a MovieClip (swf 7 only) loads.
	void	video_instance::on_event_load_progress()
	{
  		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
  		on_event(event_id::LOAD_PROGRESS);
	}

	const char*	video_instance::call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
	{
  		// Keep m_as_environment alive during any method calls!
  		smart_ptr<as_object_interface>	this_ptr(this);

  		return call_method_parsed(&m_as_environment, this, method_name, method_arg_fmt, args);
	}

	void	video_instance::attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
	{
  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		array<with_stack_entry>	dummy;
  		as_value	obj = m_as_environment.get_variable(tu_string(path_to_object), dummy);
  		as_object_interface*	as_obj = obj.to_object();
  		if (as_obj)
  		{
  			movie*	m = as_obj->to_movie();
  			if (m)
  			{
  				m->set_display_callback(callback, user_ptr);
  			}
  		}
	}


	//
	// video_definition
	//


	// A video is a mini movie-within-a-movie.  It doesn't define
	// its own characters; it uses the characters from the parent
	// movie, but it has its own frame counter, display list, etc.
	//
	// The video implementation is divided into a
	// video_definition and a video_instance.  The _definition
	// holds the immutable data for a video, while the _instance
	// contains the state for a specific instance being updated
	// and displayed in the parent movie's display list.

	video_definition::video_definition(movie_definition_sub* m)
	:	m_movie_def( m )
	,	m_frame_count( 0 )
	,	m_loading_frame( 0 )
	,	m_pBitmap( NULL )
	,	m_pBuffer( NULL )
	,	m_pCodec( NULL )
	,	m_pCodecCtx( NULL )
	,	m_pFormatCtx( NULL )
	,	m_pFrame( NULL )
	,	m_pFrameRGB( NULL )
	{
		QASSERT( m_movie_def );
	}

	video_definition::~video_definition()
	{
		// Release our playlist data.
		for (int i = 0, n = m_playlist.size(); i < n; i++)
		{
			for (int j = 0, m = m_playlist[i].size(); j < m; j++)
			{
				delete m_playlist[i][j];
			}
		}

		BREAK_POINT( "Codec delete" );
		// Delete the video frames
		for ( s32 i = 0; i < m_Frames.size(); ++i )
		{
			delete [] m_Frames[ i ].m_pData;
		}

		av_free( m_pFrameRGB );
		av_free( m_pFrame );
		avcodec_close( m_pCodecCtx );

		BREAK_POINT( "m_pBitmap" );
		SAFE_DELETE( m_pBitmap );
		BREAK_POINT( "m_pBuffer" );
		SAFE_RDELETE( m_pBuffer );
	}

	// overloads from movie_definition
	float	video_definition::get_width_pixels() const { return 1; }
	float	video_definition::get_height_pixels() const { return 1; }
	int	video_definition::get_frame_count() const { return m_frame_count; }
	float	video_definition::get_frame_rate() const
	{
		return m_movie_def->get_frame_rate();
	}
	int	video_definition::get_loading_frame() const
	{
		return m_loading_frame;
	}
	int	video_definition::get_version() const { return m_movie_def->get_version(); }
	void	video_definition::add_character(int id, character_def* ch) { log_error("add_character tag appears in video tags!\n"); }
	void	video_definition::add_font(int id, font* ch) { log_error("add_font tag appears in video tags!\n"); }
	font*	video_definition::get_font(int id) { return m_movie_def->get_font(id); }
	void	video_definition::set_jpeg_loader(jpeg::input* j_in) { QASSERT(0); }
	jpeg::input*	video_definition::get_jpeg_loader() { return NULL; }
	bitmap_character_def*	video_definition::get_bitmap_character(int id) { return m_movie_def->get_bitmap_character(id); }
	void	video_definition::add_bitmap_character(int id, bitmap_character_def* ch) { log_error("add_bc appears in video tags!\n"); }
	sound_sample*	video_definition::get_sound_sample(int id) { return m_movie_def->get_sound_sample(id); }
	void	video_definition::add_sound_sample(int id, sound_sample* sam) { log_error("add sam appears in video tags!\n"); }

	// @@ would be nicer to not inherit these...
	create_bitmaps_flag	video_definition::get_create_bitmaps() const { QASSERT(0); return DO_LOAD_BITMAPS; }
	create_font_shapes_flag	video_definition::get_create_font_shapes() const { QASSERT(0); return DO_LOAD_FONT_SHAPES; }
	int	video_definition::get_bitmap_info_count() const { QASSERT(0); return 0; }
	bitmap_info*	video_definition::get_bitmap_info(int i) const { QASSERT(0); return NULL; }
	void	video_definition::add_bitmap_info(bitmap_info* bi) { QASSERT(0); }

	void	video_definition::export_resource(const tu_string& symbol, resource* res) { log_error("can't export from video\n"); }
	smart_ptr<resource>	video_definition::get_exported_resource(const tu_string& sym) { return m_movie_def->get_exported_resource(sym); }
	void	video_definition::add_import(const char* source_url, int id, const char* symbol) { QASSERT(0); }
	void	video_definition::visit_imported_movies(import_visitor* v) { QASSERT(0); }
	void	video_definition::resolve_import(const char* source_url, movie_definition* d) { QASSERT(0); }
	character_def*	video_definition::get_character_def(int id)
	{
		return m_movie_def->get_character_def(id);
	}
	void	video_definition::generate_font_bitmaps() { QASSERT(0); }


	void	video_definition::output_cached_data(tu_file* out, const cache_options& options)
	{
		// Nothing to do.
		return;
	}
	void	video_definition::input_cached_data(tu_file* in)
	{
		// Nothing to do.
		return;
	}

	movie_interface*	video_definition::create_instance()
	{
		ASSERT( 0, "video_definition::create_instance" );
		return NULL;
	}

	// overloads from character_def
	character*	video_definition::create_character_instance(movie* parent, int id)
		// Create a (mutable) instance of our definition.  The
		// instance is created to live (temporarily) on some level on
		// the parent movie's display list.
	{
		video_instance*	si = new video_instance(this, parent->get_root(), parent, id);

		return si;
	}


	/* video_definition */
	void	video_definition::add_execute_tag(execute_tag* c)
	{
		m_playlist[m_loading_frame].push_back(c);
	}

	/* video_definition */
	void	video_definition::add_init_action(int video_id, execute_tag* c)
	{
		// video def's should not have do_init_action tags in them!  (@@ correct?)
		log_error("video_definition::add_init_action called!  Ignored.\n");
	}

	/* video_definition */
	void	video_definition::add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.  A copy of the name string is made and
		// kept in this object.
	{
		QASSERT(m_loading_frame >= 0 && m_loading_frame < m_frame_count);

		tu_string	n = name;
		int	currently_assigned = 0;
		if (m_named_frames.get(n, &currently_assigned) == true)
		{
			log_error("add_frame_name(%d, '%s') -- frame name already assigned to frame %d; overriding\n",
				m_loading_frame,
				name,
				currently_assigned);
		}
		m_named_frames.set(n, m_loading_frame);	// stores 0-based frame #
	}

	/* video_definition */
	bool	video_definition::get_labeled_frame(const char* label, int* frame_number)
		// Returns 0-based frame #
	{
		return m_named_frames.get(label, frame_number);
	}

	const array<execute_tag*>&	video_definition::get_playlist(int frame_number)
		// frame_number is 0-based
	{
		return m_playlist[frame_number];
	}

	/* video_definition */
	const array<execute_tag*>*	video_definition::get_init_actions(int frame_number)
	{
		// videos do not have init actions in their
		// playlists!  Only the root movie
		// (movie_def_impl) does (@@ correct?)
		return NULL;
	}


	/* video_definition */
	void	video_definition::read(stream* in)
	// Read the video info.  Consists of a series of tags.
	{
		av_register_all();

		m_frame_count = in->read_u16();

		m_width = in->read_u16();
		m_height = in->read_u16();

		m_flags = in->read_u8();
		m_codec = in->read_u8();

		if (m_frame_count < 1)
		{
			m_frame_count = 1;
		}
		m_playlist.resize(m_frame_count);	// need a playlist for each frame

		m_loading_frame = 0;

		if ( m_codec == CODEC_SORENSON )
		{
			m_pFormatCtx = av_alloc_format_context();
			AVStream *st = av_new_stream( m_pFormatCtx, 0 );
			m_pCodecCtx = st->codec;
			m_pCodec=avcodec_find_decoder( CODEC_ID_FLV1 );
			m_pCodecCtx->width = m_width;
			m_pCodecCtx->height = m_height;
			m_pFormatCtx->iformat = &flv_iformat;
			m_pFormatCtx->nb_streams = 1;

			avcodec_open( m_pCodecCtx, m_pCodec );

			m_pFrame = avcodec_alloc_frame();
			m_pFrameRGB = avcodec_alloc_frame();

			const u32	numBytes( avpicture_get_size( PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height ) );

			m_pBuffer = new u8[ numBytes ];

			avpicture_fill( (AVPicture *)m_pFrameRGB, m_pBuffer, PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height );

			//
			//	Create the bitmap for the video stream
			//
			image::rgb *	p_image( image::create_rgb( m_width, m_height ) );

			m_pBitmap = render::create_bitmap_info_rgb( p_image, true );

			delete p_image;
		}
		else
		{
			ASSERT( 0, "Unsupported video codec" );
		}
	}

	void	video_definition::add_frame( stream * in )
	{
		sVideoFrame	frame;
		const s32	frame_num( in->read_u16() );

		frame.m_nSize = in->get_tag_length() - 4;
		frame.m_pData = new u8[ frame.m_nSize ];

		if ( frame.m_pData != NULL )
		{
			for ( u32 i = 0; i < frame.m_nSize; ++i )
			{
				frame.m_pData[ i ] = in->read_u8();
			}
		}

		m_Frames.push_back( frame );
	}


	bool	video_definition::decode_frame( s32 frame_num )
	{
		if ( m_pBitmap != NULL )
		{
			if ( frame_num < m_Frames.size() )
			{
				sVideoFrame frame( m_Frames[ frame_num ] );
				s32			frameFinished( 0 );
				u32			bytesRemaining( frame.m_nSize );
				u8 *		p_video_data( frame.m_pData );

				while ( bytesRemaining > 0 )
				{
					const u32	bytesDecoded( avcodec_decode_video( m_pCodecCtx, m_pFrame, &frameFinished, p_video_data, bytesRemaining ) );

					if ( bytesDecoded < 0 )
					{
						BREAK_POINT( "Error while decoding frame\n" );

						return false;
					}

					bytesRemaining -= bytesDecoded;
					p_video_data += bytesDecoded;

					if ( frameFinished )
					{
						break;
					}
				}

				if ( frameFinished != 0 )
				{
					img_convert( ( AVPicture * )m_pFrameRGB, PIX_FMT_RGB24, ( AVPicture * )m_pFrame, m_pCodecCtx->pix_fmt, m_pCodecCtx->width, m_pCodecCtx->height );

					m_pBitmap->fill_rgb( m_pFrameRGB->data[ 0 ], m_width, m_height );

					return true;
				}
				else
				{
					ASSERT( 0, "Incomplete video frame decode error" );
				}
			}
			else
			{
				ASSERT( 0, "Trying to decode an invalid frame!" );
			}
		}

		return false;
	}

}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
