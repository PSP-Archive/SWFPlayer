// gameswf_sprite.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#include "gameswf_sprite.h"
#include "gameswf_movie_root.h"
#include "gameswf_stream.h"
#include "gameswf.h"
#include "gameswf_impl.h"
#include "gameswf_timers.h"

namespace gameswf
{
	extern hash<int, loader_function>	s_tag_loaders;

	// For built-in sprite ActionScript methods.
	as_object*	s_sprite_builtins = 0;	// shared among all sprites.

	//
	// sprite built-in ActionScript methods
	//

	void	sprite_play(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);
		LOG_LINE;
		sprite->set_play_state(movie_interface::PLAY);
	}

	void	sprite_stop(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);
		LOG_LINE;
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_goto_and_play(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);

		LOG_LINE;
		if (fn.nargs < 1)
		{
			log_error("error: sprite_goto_and_play needs one arg\n");
			return;
		}

		LOG_LINE;
		int	target_frame = int(fn.arg(0).to_number() - 1);	// Convert to 0-based

		LOG_LINE;
		sprite->goto_frame(target_frame);
		LOG_LINE;
		sprite->set_play_state(movie_interface::PLAY);
	}

	void	sprite_goto_and_stop(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);

		LOG_LINE;
		if (fn.nargs < 1)
		{
			log_error("error: sprite_goto_and_stop needs one arg\n");
			return;
		}

		LOG_LINE;
		int	target_frame = int(fn.arg(0).to_number() - 1);	// Convert to 0-based

		LOG_LINE;
		sprite->goto_frame(target_frame);
		LOG_LINE;
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_next_frame(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		LOG_LINE;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);

		LOG_LINE;
		int frame_count = sprite->get_frame_count();
		LOG_LINE;
		int current_frame = sprite->get_current_frame();
		LOG_LINE;
		if (current_frame < frame_count)
		{
			LOG_LINE;
			sprite->goto_frame(current_frame + 1);
		}
		LOG_LINE;
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_prev_frame(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);

		LOG_LINE;
		int current_frame = sprite->get_current_frame();
		LOG_LINE;
		if (current_frame > 0)
		{
			LOG_LINE;
			sprite->goto_frame(current_frame - 1);
		}
		LOG_LINE;
		sprite->set_play_state(movie_interface::STOP);
	}

	void	sprite_get_bytes_loaded(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);

		LOG_LINE;
		fn.result->set_int(sprite->get_root()->get_file_bytes());
	}

	void	sprite_get_bytes_total(const fn_call& fn)
	{
		LOG_LINE;
		sprite_instance* sprite = (sprite_instance*) fn.this_ptr;
		if (sprite == NULL)
		{
			LOG_LINE;
			sprite = (sprite_instance*) fn.env->get_target();
		}
		QASSERT(sprite);

		LOG_LINE;
		fn.result->set_int(sprite->get_root()->get_file_bytes());
	}


	void	sprite_builtins_init()
	{
		if (s_sprite_builtins)
		{
			return;
		}


		LOG_LINE;
		s_sprite_builtins = new as_object;
		LOG_LINE;
		s_sprite_builtins->set_member("play", &sprite_play);
		LOG_LINE;
		s_sprite_builtins->set_member("stop", &sprite_stop);
		LOG_LINE;
		s_sprite_builtins->set_member("gotoAndStop", &sprite_goto_and_stop);
		LOG_LINE;
		s_sprite_builtins->set_member("gotoAndPlay", &sprite_goto_and_play);
		LOG_LINE;
		s_sprite_builtins->set_member("nextFrame", &sprite_next_frame);
		LOG_LINE;
		s_sprite_builtins->set_member("prevFrame", &sprite_prev_frame);
		LOG_LINE;
		s_sprite_builtins->set_member("getBytesLoaded", &sprite_get_bytes_loaded);
		LOG_LINE;
		s_sprite_builtins->set_member("getBytesTotal", &sprite_get_bytes_loaded);

		// @TODO
		//		s_sprite_builtins->set_member("startDrag", &sprite_start_drag);
		//		s_sprite_builtins->set_member("stopDrag", &sprite_stop_drag);
		//		s_sprite_builtins->set_member("getURL", &sprite_get_url);
		//		s_sprite_builtins->set_member("swapDepths", &sprite_swap_depths);
	}

	void	sprite_builtins_clear()
	{
		LOG_LINE;
		if (s_sprite_builtins)
		{
			LOG_LINE;
			delete s_sprite_builtins;
			s_sprite_builtins = 0;
		}
	}


  	static void	execute_actions(as_environment* env, const array<action_buffer*>& action_list)
  	// Execute the actions in the action list, in the given
  	// environment.
  	{
  		LOG_LINE;
  		for (int i = 0; i < action_list.size(); i++)
  		{
  			LOG_LINE;
  			action_list[i]->execute(env);
  		}
  	}
  
  

	
	
	//
  	// sprite_instance
  	//
	sprite_instance::sprite_instance(movie_definition_sub* def, movie_root* r, movie* parent, int id)
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
  		LOG_LINE;
  		QASSERT(m_def != NULL);
  		QASSERT(m_root != NULL);
  		
  		//m_root->add_ref();	// @@ circular!
  		LOG_LINE;
  		m_as_environment.set_target(this);

  		LOG_LINE;
  		sprite_builtins_init();

  		// Initialize the flags for init action executed.
  		LOG_LINE;
  		m_init_actions_executed.resize(m_def->get_frame_count());
  		LOG_LINE;
  		memset(&m_init_actions_executed[0], 0,
  			    sizeof(m_init_actions_executed[0]) * m_init_actions_executed.size());
  	}

  	// sprite instance of add_interval_handler()
  	int    sprite_instance::add_interval_timer(void *timer)
  	{
  		// log_error("FIXME: %s:\n", __FUNCTION__);
  		LOG_LINE;
  		return m_root->add_interval_timer(timer);
  	}

  	void    sprite_instance::clear_interval_timer(int x)
  	{
  		// log_error("FIXME: %s:\n", __FUNCTION__);
  		LOG_LINE;
  		m_root->clear_interval_timer(x);
  	}


  	/* sprite_instance */
  	void    sprite_instance::do_something(void *timer)
  	{
  		as_value	val;
  		as_object      *obj, *this_ptr;
  		as_environment *as_env;

  		//printf("FIXME: %s:\n", __FUNCTION__);
  		LOG_LINE;
  		Timer *ptr = (Timer *)timer;
  		//log_error("INTERVAL ID is %d\n", ptr->getIntervalID());

  		LOG_LINE;
  		const as_value&	timer_method = ptr->getASFunction();
  		LOG_LINE;
  		as_env = ptr->getASEnvironment();
  		LOG_LINE;
  		this_ptr = ptr->getASObject();
  		LOG_LINE;
  		obj = ptr->getObject();
  		//m_as_environment.push(obj);
  		
  		LOG_LINE;
  		as_c_function_ptr	cfunc = timer_method.to_c_function();
  		if (cfunc) {
  			// It's a C function. Call it.
  			//log_error("Calling C function for interval timer\n");
  			//(*cfunc)(&val, obj, as_env, 0, 0);
  			LOG_LINE;
  			(*cfunc)(fn_call(&val, obj, &m_as_environment, 0, 0));
  			
  		} else if (as_as_function* as_func = timer_method.to_as_function()) {
  			// It's an ActionScript function. Call it.
  			LOG_LINE;
  			as_value method;
  			//log_error("Calling ActionScript function for interval timer\n");
  			LOG_LINE;
  			(*as_func)(fn_call(&val, (as_object_interface *)this_ptr, as_env, 0, 0));
  			//(*as_func)(&val, (as_object_interface *)this_ptr, &m_as_environment, 1, 1);
  		} else {
  			log_error("error in call_method(): method is not a function\n");
  		}    
  	}	

  	sprite_instance::~sprite_instance()
  	{
  		LOG_LINE;
  		m_display_list.clear();
  		//m_root->drop_ref();
  	}

  	movie_interface*	sprite_instance::get_root_interface() { return m_root; }
  	movie_root*	sprite_instance::get_root() { return m_root; }
  	movie*	sprite_instance::get_root_movie() { return m_root->get_root_movie(); }

  	movie_definition*	sprite_instance::get_movie_definition() { return m_def.get_ptr(); }

  	float	sprite_instance::get_width()
  	{
  		LOG_LINE;
  		float	w = 0;
  		LOG_LINE;
  		int i, n = m_display_list.get_character_count();
  		LOG_LINE;
  		character* ch;
  		LOG_LINE;
  		for (i = 0; i < n; i++)
  		{
  			LOG_LINE;
  			ch = m_display_list.get_character(i);
  			LOG_LINE;
  			if (ch != NULL)
  			{
  				LOG_LINE;
  				float ch_w = ch->get_width();
  				if (ch_w > w)
  				{
  					LOG_LINE;
  					w = ch_w;
  				}
  			}
  		}

  		LOG_LINE;
  		return w;
  	}



  	float	sprite_instance::get_height()
  	{
  		LOG_LINE;
  		float	h = 0; 
  		LOG_LINE;
  		int i, n = m_display_list.get_character_count();
  		LOG_LINE;
  		character* ch;
  		LOG_LINE;
  		for (i=0; i < n; i++)
  		{
  			LOG_LINE;
  			ch = m_display_list.get_character(i);
  			LOG_LINE;
  			if (ch != NULL)
  			{
  				LOG_LINE;
  				float	ch_h = ch->get_height();
  				LOG_LINE;
  				if (ch_h > h)
  				{
  					LOG_LINE;
  					h = ch_h;
  				}
  			}
  		}
  		LOG_LINE;
  		return h;
  	}

  	int	sprite_instance::get_current_frame() const { return m_current_frame; }
  	int	sprite_instance::get_frame_count() const
  	{
  		LOG_LINE;
  		return m_def->get_frame_count();
  	}

  	void	sprite_instance::set_play_state(play_state s)
  	// Stop or play the sprite.
  	{
  		LOG_LINE;
  		if (m_play_state != s)
  		{
  			LOG_LINE;
  			m_time_remainder = 0;
  		}

  		LOG_LINE;
  		m_play_state = s;
  	}
  	sprite_instance::play_state	sprite_instance::get_play_state() const
  	{
  		LOG_LINE;
  		return m_play_state;
  	}


  	character*	sprite_instance::get_character(int character_id)
  	{
//			return m_def->get_character_def(character_id);
  		// @@ TODO -- look through our dlist for a match
  		LOG_LINE;
  		return NULL;
  	}

  	float	sprite_instance::get_background_alpha() const
  	{
  		// @@ this doesn't seem right...
  		LOG_LINE;
  		return m_root->get_background_alpha();
  	}

  	float	sprite_instance::get_pixel_scale() const
  	{
  		LOG_LINE;
  		return m_root->get_pixel_scale();
  	}

  	void	sprite_instance::get_mouse_state(int* x, int* y, int* buttons)
  	{
  		LOG_LINE;
  		m_root->get_mouse_state(x, y, buttons);
  	}

  	void	sprite_instance::set_background_color(const rgba& color)
  	{
  		LOG_LINE;
  		m_root->set_background_color(color);
  	}

  	float	sprite_instance::get_timer() const
  	{
  		LOG_LINE;
  		return m_root->get_timer();
  	}

  	void	sprite_instance::restart()
  	{
  		LOG_LINE;
  		m_current_frame = 0;
  		LOG_LINE;
  		m_time_remainder = 0;
  		LOG_LINE;
  		m_update_frame = true;
  		LOG_LINE;
  		m_has_looped = false;
  		LOG_LINE;
  		m_play_state = PLAY;

  		LOG_LINE;
  		execute_frame_tags(m_current_frame);
  		LOG_LINE;
  		m_display_list.update();
  	}


  	bool	sprite_instance::has_looped() const { return m_has_looped; }

  	bool	sprite_instance::get_accept_anim_moves() const { return m_accept_anim_moves; }

  	bool sprite_instance::can_handle_mouse_event()
  	// Return true if we have any mouse event handlers.
  	{
  		// We should cache this!
  		LOG_LINE;
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
  		LOG_LINE;
  		for (int i = 0; i < (int)ARRAYSIZE(FN_NAMES); i++) {
  			LOG_LINE;
  			if (get_member(FN_NAMES[i], &dummy)) {
  				LOG_LINE;
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
  		{for (int i = 0; i < (int)ARRAYSIZE(EH_IDS); i++) {
  			LOG_LINE;
  			if (get_event_handler(EH_IDS[i], &dummy)) {
  				LOG_LINE;
  				return true;
  			}
  		}}

  		return false;
  	}
  	

  	/* sprite_instance */
  	movie*	sprite_instance::get_topmost_mouse_entity(float x, float y)
  	// Return the topmost entity that the given point
  	// covers that can receive mouse events.  NULL if
  	// none.  Coords are in parent's frame.
  	{
  		LOG_LINE;
  		if (get_visible() == false) {
  			return NULL;
  		}

  		LOG_LINE;
  		matrix	m = get_matrix();
  		point	p;
  		LOG_LINE;
  		m.transform_by_inverse(&p, point(x, y));

  		LOG_LINE;
  		int i, n = m_display_list.get_character_count();
  		// Go backwards, to check higher objects first.
  		LOG_LINE;
  		for (i = n - 1; i >= 0; i--)
  		{
  			LOG_LINE;
  			character* ch = m_display_list.get_character(i);
  			
  			LOG_LINE;
  			if (ch != NULL && ch->get_visible())
  			{
  				LOG_LINE;
  				movie*	te = ch->get_topmost_mouse_entity(p.m_x, p.m_y);
  				LOG_LINE;
  				if (te)
  				{
  					// The containing entity that 1) is closest to root and 2) can
  					// handle mouse events takes precedence.
  					LOG_LINE;
  					if (can_handle_mouse_event()) {
  						LOG_LINE;
  						return this;
  					} else {
  						LOG_LINE;
  						return te;
  					}
  				}
  			}
  		}

  		LOG_LINE;
  		return NULL;
  	}


  	/* sprite_instance */
  	void	sprite_instance::increment_frame_and_check_for_loop()
  	// Increment m_current_frame, and take care of looping.
  	{
  		LOG_LINE;
  		m_current_frame++;

  		LOG_LINE;
  		int	frame_count = m_def->get_frame_count();
  		LOG_LINE;
  		if (m_current_frame >= frame_count)
  		{
  			// Loop.
  			LOG_LINE;
  			m_current_frame = 0;
  			LOG_LINE;
  			m_has_looped = true;
  			LOG_LINE;
  			if (frame_count > 1)
  			{
  				LOG_LINE;
  				m_display_list.reset();
  			}
  		}
  	}

  	/* sprite_instance */
  	void	sprite_instance::advance(float delta_time)
  	{
  		// Keep this (particularly m_as_environment) alive during execution!
  		LOG_LINE;
  		smart_ptr<as_object_interface>	this_ptr(this);

  		LOG_LINE;
  		QASSERT(m_def != NULL && m_root != NULL);

  		// Advance everything in the display list.
  		LOG_LINE;
  		m_display_list.advance(delta_time);

  		// mouse drag.
  		LOG_LINE;
  		character::do_mouse_drag();

  		LOG_LINE;
  		m_time_remainder += delta_time;

  		LOG_LINE;
  		const float	frame_time = 1.0f / m_root->get_frame_rate();	// @@ cache this

  		// Check for the end of frame
  		LOG_LINE;
  		if (m_time_remainder >= frame_time)
  		{
  			LOG_LINE;
  			m_time_remainder -= frame_time;

  			// Update current and next frames.
  			LOG_LINE;
  			if (m_play_state == PLAY)
  			{
  				LOG_LINE;
  				int	current_frame0 = m_current_frame;
  				LOG_LINE;
  				increment_frame_and_check_for_loop();

  				// Execute the current frame's tags.
  				LOG_LINE;
  				if (m_current_frame != current_frame0)
  				{
  					LOG_LINE;
  					execute_frame_tags(m_current_frame);
  				}
  			}

  			// Dispatch onEnterFrame event.
  			LOG_LINE;
  			on_event(event_id::ENTER_FRAME);

  			LOG_LINE;
  			do_actions();

  			// Clean up display list (remove dead objects).
  			LOG_LINE;
  			m_display_list.update();
  		}

  		// Skip excess time.  TODO root caller should
  		// loop to prevent this happening; probably
  		// only root should keep m_time_remainder, and
  		// advance(dt) should be a discrete tick()
  		// with no dt.
  		LOG_LINE;
  		m_time_remainder = fmod(m_time_remainder, frame_time);
  	}

  	/*sprite_instance*/
  	void	sprite_instance::execute_frame_tags(int frame, bool state_only)
  	// Execute the tags associated with the specified frame.
  	// frame is 0-based
  	{
  		// Keep this (particularly m_as_environment) alive during execution!
  		LOG_LINE;
  		smart_ptr<as_object_interface>	this_ptr(this);

  		QASSERT(frame >= 0);
  		QASSERT(frame < m_def->get_frame_count());

  		// Execute this frame's init actions, if necessary.
  		LOG_LINE;
  		if (m_init_actions_executed[frame] == false)
  		{
  			LOG_LINE;
  			const array<execute_tag*>*	init_actions = m_def->get_init_actions(frame);
  			LOG_LINE;
  			if (init_actions && init_actions->size() > 0)
  			{
  				// Need to execute these actions.
  				LOG_LINE;
  				for (int i= 0; i < init_actions->size(); i++)
  				{
  					LOG_LINE;
  					execute_tag*	e = (*init_actions)[i];
  					LOG_LINE;
  					e->execute(this);
  				}

  				// Mark this frame done, so we never execute these init actions
  				// again.
  				LOG_LINE;
  				m_init_actions_executed[frame] = true;
  			}
  		}

  		LOG_LINE;
  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);

  		LOG_LINE;
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			LOG_LINE;
  			execute_tag*	e = playlist[i];
  			if (state_only)
  			{
  				LOG_LINE;
  				e->execute_state(this);
  			}
  			else
  			{
  				LOG_LINE;
  				e->execute(this);
  			}
  		}
  	}


  	/*sprite_instance*/
  	void	sprite_instance::execute_frame_tags_reverse(int frame)
  	// Execute the tags associated with the specified frame, IN REVERSE.
  	// I.e. if it's an "add" tag, then we do a "remove" instead.
  	// Only relevant to the display-list manipulation tags: add, move, remove, replace.
  	//
  	// frame is 0-based
  	{
  		// Keep this (particularly m_as_environment) alive during execution!
  		LOG_LINE;
  		smart_ptr<as_object_interface>	this_ptr(this);

  		QASSERT(frame >= 0);
  		QASSERT(frame < m_def->get_frame_count());

  		LOG_LINE;
  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
  		LOG_LINE;
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			LOG_LINE;
  			execute_tag*	e = playlist[i];
  			LOG_LINE;
  			e->execute_state_reverse(this, frame);
  		}
  	}

  	
  	/*sprite_instance*/
  	execute_tag*	sprite_instance::find_previous_replace_or_add_tag(int frame, int depth, int id)
  	{
  		LOG_LINE;
  		uint32	depth_id = ((depth & 0x0FFFF) << 16) | (id & 0x0FFFF);

  		LOG_LINE;
  		for (int f = frame - 1; f >= 0; f--)
  		{
  			LOG_LINE;
  			const array<execute_tag*>&	playlist = m_def->get_playlist(f);
  			LOG_LINE;
  			for (int i = playlist.size() - 1; i >= 0; i--)
  			{
  				LOG_LINE;
  				execute_tag*	e = playlist[i];
  				LOG_LINE;
  				if (e->get_depth_id_of_replace_or_add_tag() == depth_id)
  				{
  					LOG_LINE;
  					return e;
  				}
  			}
  		}

  		LOG_LINE;
  		return NULL;
  	}


  	/*sprite_instance*/
  	void	sprite_instance::execute_remove_tags(int frame)
  	// Execute any remove-object tags associated with the specified frame.
  	// frame is 0-based
  	{
  		QASSERT(frame >= 0);
  		QASSERT(frame < m_def->get_frame_count());

  		LOG_LINE;
  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame);
  		LOG_LINE;
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			LOG_LINE;
  			execute_tag*	e = playlist[i];
  			LOG_LINE;
  			if (e->is_remove_tag())
  			{
  				LOG_LINE;
  				e->execute_state(this);
  			}
  		}
  	}


  	/*sprite_instance*/
  	void	sprite_instance::do_actions()
  	// Take care of this frame's actions.
  	{
  		// Keep m_as_environment alive during any method calls!
  		LOG_LINE;
  		smart_ptr<as_object_interface>	this_ptr(this);

  		LOG_LINE;
  		execute_actions(&m_as_environment, m_action_list);
  		LOG_LINE;
  		m_action_list.resize(0);
  	}


  	/*sprite_instance*/
  	void	sprite_instance::goto_frame(int target_frame_number)
  	// Set the sprite state at the specified frame number.
  	// 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
  	{
//			IF_VERBOSE_DEBUG(log_msg("sprite::goto_frame(%d)\n", target_frame_number));//xxxxx

  		LOG_LINE;
  		target_frame_number = iclamp(target_frame_number, 0, m_def->get_frame_count() - 1);

  		LOG_LINE;
  		if (target_frame_number < m_current_frame)
  		{
  			LOG_LINE;
  			for (int f = m_current_frame; f > target_frame_number; f--)
  			{
  				LOG_LINE;
  				execute_frame_tags_reverse(f);
  			}

  			LOG_LINE;
  			execute_frame_tags(target_frame_number, false);
  			LOG_LINE;
  			m_display_list.update();
  		}
  		else if (target_frame_number > m_current_frame)
  		{
  			LOG_LINE;
  			for (int f = m_current_frame + 1; f < target_frame_number; f++)
  			{
  				LOG_LINE;
  				execute_frame_tags(f, true);
  			}

  			LOG_LINE;
  			execute_frame_tags(target_frame_number, false);
  			LOG_LINE;
  			m_display_list.update();
  		}

  		LOG_LINE;
  		m_current_frame = target_frame_number;      

  		// goto_frame stops by default.
  		LOG_LINE;
  		m_play_state = STOP;
  	}


  	bool	sprite_instance::goto_labeled_frame(const char* label)
  	// Look up the labeled frame, and jump to it.
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

  	
  	/*sprite_instance*/
  	void	sprite_instance::display()
  	{
  		LOG_LINE;
  		if (get_visible() == false)
  		{
  			// We're invisible, so don't display!
  			LOG_LINE;
  			return;
  		}

  		LOG_LINE;
  		m_display_list.display();

  		LOG_LINE;
  		do_display_callback();
  	}

  	/*sprite_instance*/
  	character*	sprite_instance::add_display_object(
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

  		LOG_LINE;
  		character_def*	cdef = m_def->get_character_def(character_id);
  		LOG_LINE;
  		if (cdef == NULL)
  		{
  			LOG_LINE;
  			log_error("sprite::add_display_object(): unknown cid = %d\n", character_id);
  			return NULL;
  		}

  		// If we already have this object on this
  		// plane, then move it instead of replacing
  		// it.
  		LOG_LINE;
  		character*	existing_char = m_display_list.get_character_at_depth(depth);
  		LOG_LINE;
  		if (existing_char
  			&& existing_char->get_id() == character_id
  			&& ((name == NULL && existing_char->get_name().length() == 0)
  			|| (name && existing_char->get_name() == name)))
  		{
//				IF_VERBOSE_DEBUG(log_msg("add changed to move on depth %d\n", depth));//xxxxxx
  			LOG_LINE;
  			move_display_object(depth, true, color_transform, true, matrix, ratio, clip_depth);
  			return NULL;
  		}

  		QASSERT(cdef);
  		LOG_LINE;
  		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
  		QASSERT(ch != NULL);
  		LOG_LINE;
  		if (name != NULL && name[0] != 0)
  		{
  			LOG_LINE;
  			ch->set_name(name);
  		}

  		// Attach event handlers (if any).
  		LOG_LINE;
  		{for (int i = 0, n = event_handlers.size(); i < n; i++)
  		{
  			LOG_LINE;
  			event_handlers[i]->attach_to(ch.get_ptr());
  		}}

  		LOG_LINE;
  		m_display_list.add_display_object(
  			ch.get_ptr(),
  			depth,
  			replace_if_depth_is_occupied,
  			color_transform,
  			matrix,
  			ratio,
  			clip_depth);

  		LOG_LINE;
  		QASSERT(ch == NULL || ch->get_ref_count() > 1);
  		return ch.get_ptr();
  	}


  	/*sprite_instance*/
  	void	sprite_instance::move_display_object(
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
  		LOG_LINE;
  		m_display_list.move_display_object(depth, use_cxform, color_xform, use_matrix, mat, ratio, clip_depth);
  	}


  	/*sprite_instance*/
  	void	sprite_instance::replace_display_object(
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

  		LOG_LINE;
  		character_def*	cdef = m_def->get_character_def(character_id);
  		if (cdef == NULL)
  		{
  			LOG_LINE;
  			log_error("sprite::replace_display_object(): unknown cid = %d\n", character_id);
  			return;
  		}
  		LOG_LINE;
  		QASSERT(cdef);

  		LOG_LINE;
  		smart_ptr<character>	ch = cdef->create_character_instance(this, character_id);
  		QASSERT(ch != NULL);

  		LOG_LINE;
  		if (name != NULL && name[0] != 0)
  		{
  			LOG_LINE;
  			ch->set_name(name);
  		}

  		LOG_LINE;
  		m_display_list.replace_display_object(
  			ch.get_ptr(),
  			depth,
  			use_cxform,
  			color_transform,
  			use_matrix,
  			mat,
  			ratio,
  			clip_depth);
  		LOG_LINE;
  	}


  	/*sprite_instance*/
  	void	sprite_instance::replace_display_object(
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

  		LOG_LINE;
  		if (name != NULL && name[0] != 0)
  		{
  			ch->set_name(name);
  		}

  		LOG_LINE;
  		m_display_list.replace_display_object(
  			ch,
  			depth,
  			use_cxform,
  			color_transform,
  			use_matrix,
  			mat,
  			ratio,
  			clip_depth);
  		LOG_LINE;
  	}


  	/*sprite_instance*/
  	void	sprite_instance::remove_display_object(Uint16 depth, int id)
  	// Remove the object at the specified depth.
  	// If id != -1, then only remove the object at depth with matching id.
  	{
  		LOG_LINE;
  		m_display_list.remove_display_object(depth, id);
  	}


  	/*sprite_instance*/
  	void	sprite_instance::add_action_buffer(action_buffer* a)
  	// Add the given action buffer to the list of action
  	// buffers to be processed at the end of the next
  	// frame advance.
  	{
  		LOG_LINE;
  		m_action_list.push_back(a);
  	}


  	/*sprite_instance*/
  	int	sprite_instance::get_id_at_depth(int depth)
  	// For debugging -- return the id of the character at the specified depth.
  	// Return -1 if nobody's home.
  	{
  		LOG_LINE;
  		int	index = m_display_list.get_display_index(depth);
  		if (index == -1)
  		{
  			LOG_LINE;
  			return -1;
  		}

  		LOG_LINE;
  		character*	ch = m_display_list.get_display_object(index).m_character.get_ptr();

  		LOG_LINE;
  		return ch->get_id();
  	}


  	//
  	// ActionScript support
  	//


  	/* sprite_instance */
  	void	sprite_instance::set_variable(const char* path_to_var, const char* new_value)
  	{
  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		LOG_LINE;
  		if (path_to_var == NULL)
  		{
  			log_error("error: NULL path_to_var passed to set_variable()\n");
  			return;
  		}
  		LOG_LINE;
  		if (new_value == NULL)
  		{
  			log_error("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
  			return;
  		}

  		LOG_LINE;
  		array<with_stack_entry>	empty_with_stack;
  		LOG_LINE;
  		tu_string	path(path_to_var);
  		LOG_LINE;
  		as_value	val(new_value);

  		LOG_LINE;
  		m_as_environment.set_variable(path, val, empty_with_stack);
  	}

  	/* sprite_instance */
  	void	sprite_instance::set_variable(const char* path_to_var, const wchar_t* new_value)
  	{
  		LOG_LINE;
  		if (path_to_var == NULL)
  		{
  			log_error("error: NULL path_to_var passed to set_variable()\n");
  			return;
  		}
  		LOG_LINE;
  		if (new_value == NULL)
  		{
  			log_error("error: NULL passed to set_variable('%s', NULL)\n", path_to_var);
  			return;
  		}

  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		LOG_LINE;
  		array<with_stack_entry>	empty_with_stack;
  		LOG_LINE;
  		tu_string	path(path_to_var);
  		LOG_LINE;
  		as_value	val(new_value);

  		LOG_LINE;
  		m_as_environment.set_variable(path, val, empty_with_stack);
  	}

  	/* sprite_instance */
  	const char*	sprite_instance::get_variable(const char* path_to_var) const
  	{
  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		LOG_LINE;
  		array<with_stack_entry>	empty_with_stack;
  		LOG_LINE;
  		tu_string	path(path_to_var);

  		// NOTE: this is static so that the string
  		// value won't go away after we return!!!
  		// It'll go away during the next call to this
  		// function though!!!  NOT THREAD SAFE!
  		LOG_LINE;
  		static as_value	val;

  		LOG_LINE;
  		val = m_as_environment.get_variable(path, empty_with_stack);

  		LOG_LINE;
  		return val.to_string();	// ack!
  	}

  	
  	/* sprite_instance */
  	void	sprite_instance::set_member(const tu_stringi& name, const as_value& val)
  	// Set the named member to the value.  Return true if we have
  	// that member; false otherwise.
  	{
  		LOG_LINE;
  		as_standard_member	std_member = get_standard_member(name);
  		LOG_LINE;
  		switch (std_member)
  		{
  		default:
  		case M_INVALID_MEMBER:
  			break;
  		case M_X:
  			//if (name == "_x")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();
  			LOG_LINE;
  			m.m_[0][2] = (float) PIXELS_TO_TWIPS(val.to_number());
  			LOG_LINE;
  			set_matrix(m);

  			LOG_LINE;
  			m_accept_anim_moves = false;

  			return;
  		}
  		case M_Y:
  			//else if (name == "_y")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();
  			LOG_LINE;
  			m.m_[1][2] = (float) PIXELS_TO_TWIPS(val.to_number());
  			LOG_LINE;
  			set_matrix(m);

  			LOG_LINE;
  			m_accept_anim_moves = false;

  			return;
  		}
  		case M_XSCALE:
  			//else if (name == "_xscale")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();

  			// Decompose matrix and insert the desired value.
  			LOG_LINE;
  			float	x_scale = (float) val.to_number() / 100.f;	// input is in percent
  			LOG_LINE;
  			float	y_scale = m.get_y_scale();
  			LOG_LINE;
  			float	rotation = m.get_rotation();
  			LOG_LINE;
  			m.set_scale_rotation(x_scale, y_scale, rotation);

  			LOG_LINE;
  			set_matrix(m);
  			LOG_LINE;
  			m_accept_anim_moves = false;
  			LOG_LINE;
  			return;
  		}
  		case M_YSCALE:
  			//else if (name == "_yscale")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();

  			// Decompose matrix and insert the desired value.
  			LOG_LINE;
  			float	x_scale = m.get_x_scale();
  			LOG_LINE;
  			float	y_scale = (float) val.to_number() / 100.f;	// input is in percent
  			LOG_LINE;
  			float	rotation = m.get_rotation();
  			LOG_LINE;
  			m.set_scale_rotation(x_scale, y_scale, rotation);

  			LOG_LINE;
  			set_matrix(m);
  			LOG_LINE;
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_ALPHA:
  			//else if (name == "_alpha")
  		{
  			// Set alpha modulate, in percent.
  			LOG_LINE;
  			cxform	cx = get_cxform();
  			LOG_LINE;
  			cx.m_[3][0] = float(val.to_number()) / 100.f;
  			LOG_LINE;
  			set_cxform(cx);
  			LOG_LINE;
  			m_accept_anim_moves = false;
  			LOG_LINE;
  			return;
  		}
  		case M_VISIBLE:
  			//else if (name == "_visible")
  		{
  			LOG_LINE;
  			set_visible(val.to_bool());
  			LOG_LINE;
  			m_accept_anim_moves = false;
  			LOG_LINE;
  			return;
  		}
  		case M_WIDTH:
  			//else if (name == "_width")
  		{
  			// @@ tulrich: is parameter in world-coords or local-coords?
  			LOG_LINE;
  			matrix	m = get_matrix();
  			LOG_LINE;
  			m.m_[0][0] = float(PIXELS_TO_TWIPS(val.to_number()));
  			LOG_LINE;
  			float w = get_width();
  			LOG_LINE;
  			if (fabsf(w) > 1e-6f)
  			{
  				LOG_LINE;
  					m.m_[0][0] /= w;
  			}
  			LOG_LINE;
  			set_matrix(m);
  			LOG_LINE;
  			m_accept_anim_moves = false;
  			return;
  		}
  		case M_HEIGHT:
  			//else if (name == "_height")
  		{
  			// @@ tulrich: is parameter in world-coords or local-coords?
  			LOG_LINE;
  			matrix	m = get_matrix();
  			LOG_LINE;
  			m.m_[1][1] = float(PIXELS_TO_TWIPS(val.to_number()));
  			LOG_LINE;
  			float h = get_width();
  			LOG_LINE;
  			if (fabsf(h) > 1e-6f)
  			{
  				LOG_LINE;
  				m.m_[1][1] /= h;
  			}
  			LOG_LINE;
  			set_matrix(m);
  			LOG_LINE;
  			m_accept_anim_moves = false;
  			LOG_LINE;
  			return;
  		}
  		case M_ROTATION:
  			//else if (name == "_rotation")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();

  			// Decompose matrix and insert the desired value.
  			LOG_LINE;
  			float	x_scale = m.get_x_scale();
  			LOG_LINE;
  			float	y_scale = m.get_y_scale();
  			LOG_LINE;
  			float	rotation = (float) val.to_number() * float(M_PI) / 180.f;	// input is in degrees
  			LOG_LINE;
  			m.set_scale_rotation(x_scale, y_scale, rotation);

  			LOG_LINE;
  			set_matrix(m);
  			LOG_LINE;
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
  		LOG_LINE;
  		bool	text_val = val.get_type() == as_value::STRING
  			|| val.get_type() == as_value::NUMBER;
  		LOG_LINE;
  		if (text_val)
  		{
  			LOG_LINE;
  			bool	success = false;
  			LOG_LINE;
  			for (int i = 0, n = m_display_list.get_character_count(); i < n; i++)
  			{
  				LOG_LINE;
  				character*	ch = m_display_list.get_character(i);
  				// CASE INSENSITIVE compare.  In ActionScript 2.0, this
  				// changes to CASE SENSITIVE!!!
  				LOG_LINE;
  				if (name == ch->get_text_name())
  				{
  					LOG_LINE;
  					const char* text = val.to_string();
  					LOG_LINE;
  					ch->set_text_value(text);
  					LOG_LINE;
  					success = true;
  				}
  			}
  			LOG_LINE;
  			if (success) return;
  		}

  		// If that didn't work, set a variable within this environment.
  		LOG_LINE;
  		m_as_environment.set_member(name, val);
  	}


  	/* sprite_instance */
  	bool	sprite_instance::get_member(const tu_stringi& name, as_value* val)
  	// Set *val to the value of the named member and
  	// return true, if we have the named member.
  	// Otherwise leave *val alone and return false.
  	{
  		LOG_LINE;
  		as_standard_member	std_member = get_standard_member(name);
  		LOG_LINE;
  		switch (std_member)
  		{
  		default:
  		case M_INVALID_MEMBER:
  			break;
  		case M_X:
  			//if (name == "_x")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();
  			LOG_LINE;
  			val->set_double(TWIPS_TO_PIXELS(m.m_[0][2]));
  			return true;
  		}
  		case M_Y:
  			//else if (name == "_y")
  		{
  			LOG_LINE;
  			matrix	m = get_matrix();
  			LOG_LINE;
  			val->set_double(TWIPS_TO_PIXELS(m.m_[1][2]));
  			return true;
  		}
  		case M_XSCALE:
  			//else if (name == "_xscale")
  		{
  			LOG_LINE;
  			matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
  			LOG_LINE;
  			float xscale = m.get_x_scale();
  			LOG_LINE;
  			val->set_double(xscale * 100);		// result in percent
  			return true;
  		}
  		case M_YSCALE:
  			//else if (name == "_yscale")
  		{
  			LOG_LINE;
  			matrix m = get_matrix();	// @@ or get_world_matrix()?  Test this.
  			LOG_LINE;
  			float yscale = m.get_y_scale();
  			LOG_LINE;
  			val->set_double(yscale * 100);		// result in percent
  			return true;
  		}
  		case M_CURRENTFRAME:
  			//else if (name == "_currentframe")
  		{
  			LOG_LINE;
  			val->set_int(m_current_frame + 1);
  			return true;
  		}
  		case M_TOTALFRAMES:
  			//else if (name == "_totalframes")
  		{
  			// number of frames.  Read only.
  			LOG_LINE;
  			val->set_int(m_def->get_frame_count());
  			return true;
  		}
  		case M_ALPHA:
  			//else if (name == "_alpha")
  		{
  			// Alpha units are in percent.
  			LOG_LINE;
  			val->set_double(get_cxform().m_[3][0] * 100.f);
  			return true;
  		}
  		case M_VISIBLE:
  			//else if (name == "_visible")
  		{
  			LOG_LINE;
  			val->set_bool(get_visible());
  			return true;
  		}
  		case M_WIDTH:
  			//else if (name == "_width")
  		{
  			LOG_LINE;
  			matrix	m = get_world_matrix();
  			rect	transformed_rect;

  			// @@ not sure about this...
  			LOG_LINE;
  			rect	source_rect;
  			LOG_LINE;
  			source_rect.m_x_min = 0;
  			LOG_LINE;
  			source_rect.m_y_min = 0;
  			LOG_LINE;
  			source_rect.m_x_max = (float) get_width();
  			LOG_LINE;
  			source_rect.m_y_max = (float) get_height();

  			LOG_LINE;
  			transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
  			LOG_LINE;
  			val->set_double(TWIPS_TO_PIXELS(transformed_rect.width()));
  			LOG_LINE;
  			return true;
  		}
  		case M_HEIGHT:
  			//else if (name == "_height")
  		{
  			rect	transformed_rect;

  			// @@ not sure about this...
  			rect	source_rect;
  			LOG_LINE;
  			source_rect.m_x_min = 0;
  			LOG_LINE;
  			source_rect.m_y_min = 0;
  			LOG_LINE;
  			source_rect.m_x_max = (float) get_width();
  			LOG_LINE;
  			source_rect.m_y_max = (float) get_height();

  			LOG_LINE;
  			transformed_rect.enclose_transformed_rect(get_world_matrix(), source_rect);
  			LOG_LINE;
  			val->set_double(TWIPS_TO_PIXELS(transformed_rect.height()));
  			return true;
  		}
  		case M_ROTATION:
  			//else if (name == "_rotation")
  		{
  			// Verified against Macromedia player using samples/test_rotation.swf
  			LOG_LINE;
  			float	angle = get_matrix().get_rotation();

  			// Result is CLOCKWISE DEGREES, [-180,180]
  			LOG_LINE;
  			angle *= 180.0f / float(M_PI);

  			LOG_LINE;
  			val->set_double(angle);
  			return true;
  		}
  		case M_TARGET:
  			//else if (name == "_target")
  		{
  			// Full path to this object; e.g. "/_level0/sprite1/sprite2/ourSprite"
  			LOG_LINE;
  			val->set_string("/_root");
  			return true;
  		}
  		case M_FRAMESLOADED:
  			//else if (name == "_framesloaded")
  		{
  			LOG_LINE;
  			val->set_int(m_def->get_frame_count());
  			return true;
  		}
  		case M_NAME:
  			//else if (name == "_name")
  		{
  			LOG_LINE;
  			val->set_tu_string(get_name());
  			return true;
  		}
  		case M_DROPTARGET:
  			//else if (name == "_droptarget")
  		{
  			// Absolute path in slash syntax where we were last dropped (?)
  			// @@ TODO
  			LOG_LINE;
  			val->set_string("/_root");
  			return true;
  		}
  		case M_URL:
  			//else if (name == "_url")
  		{
  			// our URL.
  			LOG_LINE;
  			val->set_string("gameswf");
  			return true;
  		}
  		case M_HIGHQUALITY:
  			//else if (name == "_highquality")
  		{
  			// Whether we're in high quality mode or not.
  			LOG_LINE;
  			val->set_bool(true);
  			return true;
  		}
  		case M_FOCUSRECT:
  			//else if (name == "_focusrect")
  		{
  			// Is a yellow rectangle visible around a focused movie clip (?)
  			LOG_LINE;
  			val->set_bool(false);
  			return true;
  		}
  		case M_SOUNDBUFTIME:
  			//else if (name == "_soundbuftime")
  		{
  			// Number of seconds before sound starts to stream.
  			LOG_LINE;
  			val->set_double(0.0);
  			return true;
  		}
  		case M_XMOUSE:
  			//else if (name == "_xmouse")
  		{
  			// Local coord of mouse IN PIXELS.
  			int	x, y, buttons;
  			QASSERT(m_root);
  			LOG_LINE;
  			m_root->get_mouse_state(&x, &y, &buttons);

  			LOG_LINE;
  			matrix	m = get_world_matrix();

  			LOG_LINE;
  			point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
  			point	b;
  			
  			LOG_LINE;
  			m.transform_by_inverse(&b, a);

  			LOG_LINE;
  			val->set_double(TWIPS_TO_PIXELS(b.m_x));
  			return true;
  		}
  		case M_YMOUSE:
  			//else if (name == "_ymouse")
  		{
  			// Local coord of mouse IN PIXELS.
  			int	x, y, buttons;
  			QASSERT(m_root);
  			LOG_LINE;
  			m_root->get_mouse_state(&x, &y, &buttons);

  			LOG_LINE;
  			matrix	m = get_world_matrix();

  			LOG_LINE;
  			point	a(PIXELS_TO_TWIPS(x), PIXELS_TO_TWIPS(y));
  			point	b;
  			
  			LOG_LINE;
  			m.transform_by_inverse(&b, a);

  			LOG_LINE;
  			val->set_double(TWIPS_TO_PIXELS(b.m_y));
  			return true;
  		}
  		case M_PARENT:
  		{
  			LOG_LINE;
  			val->set_as_object_interface(static_cast<as_object_interface*>(m_parent));
  			return true;
  		}
  		case M_ONLOAD:
  		{
  			LOG_LINE;
  			if (m_as_environment.get_member(name, val))
  			{
  				return true;
  			}
  			// Optimization: if no hit, don't bother looking in the display list, etc.
  			return false;
  		}
  		}	// end switch

  		// Try variables.
  		LOG_LINE;
  		if (m_as_environment.get_member(name, val))
  		{
  			return true;
  		}

  		// Not a built-in property.  Check items on our
  		// display list.
  		LOG_LINE;
  		character*	ch = m_display_list.get_character_by_name_i(name);
  		if (ch)
  		{
  			// Found object.
  			LOG_LINE;
  			val->set_as_object_interface(static_cast<as_object_interface*>(ch));
  			return true;
  		}

  		// Try static builtin functions.
  		QASSERT(s_sprite_builtins);
  		LOG_LINE;
  		if (s_sprite_builtins->get_member(name, val))
  		{
  			return true;
  		}

  		return false;
  	}


  	/* sprite_instance */
  	movie*	sprite_instance::get_relative_target(const tu_string& name)
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
  		LOG_LINE;
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
  			LOG_LINE;
  			return m_root->m_movie.get_ptr();
  		}

  		LOG_LINE;
  		// See if we have a match on the display list.
  		return m_display_list.get_character_by_name(name);
  	}


  	/* sprite_instance */
  	void	sprite_instance::call_frame_actions(const as_value& frame_spec)
  	// Execute the actions for the specified frame.  The
  	// frame_spec could be an integer or a string.
  	{
  		int	frame_number = -1;

  		// Figure out what frame to call.
  		LOG_LINE;
  		if (frame_spec.get_type() == as_value::STRING)
  		{
  			LOG_LINE;
  			if (m_def->get_labeled_frame(frame_spec.to_string(), &frame_number) == false)
  			{
  				// Try converting to integer.
  				LOG_LINE;
  				frame_number = (int) frame_spec.to_number();
  			}
  		}
  		else
  		{
  			// convert from 1-based to 0-based
  			LOG_LINE;
  			frame_number = (int) frame_spec.to_number() - 1;
  		}

  		LOG_LINE;
  		if (frame_number < 0 || frame_number >= m_def->get_frame_count())
  		{
  			// No dice.
  			LOG_LINE;
  			log_error("error: call_frame('%s') -- unknown frame\n", frame_spec.to_string());
  			return;
  		}

  		LOG_LINE;
  		int	top_action = m_action_list.size();

  		// Execute the actions.
  		LOG_LINE;
  		const array<execute_tag*>&	playlist = m_def->get_playlist(frame_number);
  		LOG_LINE;
  		for (int i = 0; i < playlist.size(); i++)
  		{
  			LOG_LINE;
  			execute_tag*	e = playlist[i];
  			if (e->is_action_tag())
  			{
  				LOG_LINE;
  				e->execute(this);
  			}
  		}

  		// Execute any new actions triggered by the tag,
  		// leaving existing actions to be executed.
  		LOG_LINE;
  		while (m_action_list.size() > top_action)
  		{
  			LOG_LINE;
  			m_action_list[top_action]->execute(&m_as_environment);
  			LOG_LINE;
  			m_action_list.remove(top_action);
  		}
  		QASSERT(m_action_list.size() == top_action);
  	}


  	/* sprite_instance */
  	void	sprite_instance::set_drag_state(const drag_state& st)
  	{
  		LOG_LINE;
  		m_root->m_drag_state = st;
  	}

  	/* sprite_instance */
  	void	sprite_instance::stop_drag()
  	{
  		LOG_LINE;
  		QASSERT(m_parent == NULL);	// we must be the root movie!!!
  		
  		m_root->stop_drag();
  	}


  	/* sprite_instance */
  	void	sprite_instance::get_drag_state(drag_state* st)
  	{
  		LOG_LINE;
  		*st = m_root->m_drag_state;
  	}


  	void	sprite_instance::clone_display_object(const tu_string& name, const tu_string& newname, Uint16 depth)
  	// Duplicate the object with the specified name and add it with a new name 
  	// at a new depth.
  	{
  		LOG_LINE;
  		character* ch = m_display_list.get_character_by_name(name);
  		if (ch)
  		{
  			LOG_LINE;
  			array<swf_event*>	dummy_event_handlers;

  			LOG_LINE;
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


  	void	sprite_instance::remove_display_object(const tu_string& name)
  	// Remove the object with the specified name.
  	{
  		LOG_LINE;
  		character* ch = m_display_list.get_character_by_name(name);
  		if (ch)
  		{
  			// @@ TODO: should only remove movies that were created via clone_display_object --
  			// apparently original movies, placed by anim events, are immune to this.
  			LOG_LINE;
  			remove_display_object(ch->get_depth(), ch->get_id());
  		}
  	}

  	
  	/* sprite_instance */
  	bool	sprite_instance::on_event(event_id id)
  	// Dispatch event handler(s), if any.
  	{
  		// Keep m_as_environment alive during any method calls!
  		LOG_LINE;
  		smart_ptr<as_object_interface>	this_ptr(this);

  		bool called = false;
  		
  		// First, check for built-in event handler.
  		{
  			as_value	method;
  			LOG_LINE;
  			if (get_event_handler(id, &method))
  			{
  				// Dispatch.
  				LOG_LINE;
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
  			LOG_LINE;
  			const tu_stringi&	method_name = id.get_function_name().to_tu_stringi();
  			LOG_LINE;
  			if (method_name.length() > 0)
  			{
  				LOG_LINE;
  				as_value	method;
  				if (get_member(method_name, &method))
  				{
  					LOG_LINE;
  					call_method0(method, &m_as_environment, this);
  					called = true;
  				}
  			}
  		}

  		LOG_LINE;
  		return called;
  	}


  	/*sprite_instance*/
  	void	sprite_instance::on_event_load()
  	// Do the events that (appear to) happen as the movie
  	// loads.  frame1 tags and actions are executed (even
  	// before advance() is called).  Then the onLoad event
  	// is triggered.
  	{
  		LOG_LINE;
  		execute_frame_tags(0);
  		LOG_LINE;
  		do_actions();
  		LOG_LINE;
  		on_event(event_id::LOAD);
  	}

  	// Do the events that happen when there is XML data waiting
  	// on the XML socket connection.
  	void	sprite_instance::on_event_xmlsocket_onxml()
  	{
  		LOG_LINE;
  		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
  		LOG_LINE;
  		on_event(event_id::SOCK_XML);
  	}
  	
  	// Do the events that (appear to) happen on a specified interval.
  	void	sprite_instance::on_event_interval_timer()
  	{
  		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
  		LOG_LINE;
  		on_event(event_id::TIMER);
  	}

  	// Do the events that happen as a MovieClip (swf 7 only) loads.
  	void	sprite_instance::on_event_load_progress()
  	{
  		log_error("FIXME: %s: unimplemented\n", __FUNCTION__);
  		LOG_LINE;
  		on_event(event_id::LOAD_PROGRESS);
  	}

/*sprite_instance*/
  	const char*	sprite_instance::call_method_args(const char* method_name, const char* method_arg_fmt, va_list args)
  	{
  		// Keep m_as_environment alive during any method calls!
  		LOG_LINE;
  		smart_ptr<as_object_interface>	this_ptr(this);

  		LOG_LINE;
  		return call_method_parsed(&m_as_environment, this, method_name, method_arg_fmt, args);
  	}

  	/* sprite_instance */
  	void	sprite_instance::attach_display_callback(const char* path_to_object, void (*callback)(void*), void* user_ptr)
  	{
  		QASSERT(m_parent == NULL);	// should only be called on the root movie.

  		LOG_LINE;
  		array<with_stack_entry>	dummy;
  		LOG_LINE;
  		as_value	obj = m_as_environment.get_variable(tu_string(path_to_object), dummy);
  		LOG_LINE;
  		as_object_interface*	as_obj = obj.to_object();
  		LOG_LINE;
  		if (as_obj)
  		{
  			LOG_LINE;
  			movie*	m = as_obj->to_movie();
  			if (m)
  			{
  				LOG_LINE;
  				m->set_display_callback(callback, user_ptr);
  			}
  		}
  	}
  




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

	sprite_definition::sprite_definition(movie_definition_sub* m)
		:
	m_movie_def(m),
		m_frame_count(0),
		m_loading_frame(0)
	{
		LOG_LINE;
		QASSERT(m_movie_def);
	}

	sprite_definition::~sprite_definition()
	{
		// Release our playlist data.
		LOG_LINE;
		{for (int i = 0, n = m_playlist.size(); i < n; i++)
		{
			LOG_LINE;
			for (int j = 0, m = m_playlist[i].size(); j < m; j++)
			{
				LOG_LINE;
				delete m_playlist[i][j];
			}
		}}
	}

	// overloads from movie_definition
	float	sprite_definition::get_width_pixels() const { return 1; }
	float	sprite_definition::get_height_pixels() const { return 1; }
	int	sprite_definition::get_frame_count() const { return m_frame_count; }
	float	sprite_definition::get_frame_rate() const
	{
		LOG_LINE;
		return m_movie_def->get_frame_rate();
	}
	int	sprite_definition::get_loading_frame() const
	{
		LOG_LINE;
		return m_loading_frame;
	}
	int	sprite_definition::get_version() const { return m_movie_def->get_version(); }
	void	sprite_definition::add_character(int id, character_def* ch) { log_error("add_character tag appears in sprite tags!\n"); }
	void	sprite_definition::add_font(int id, font* ch) { log_error("add_font tag appears in sprite tags!\n"); }
	font*	sprite_definition::get_font(int id) { return m_movie_def->get_font(id); }
	void	sprite_definition::set_jpeg_loader(jpeg::input* j_in) { QASSERT(0); }
	jpeg::input*	sprite_definition::get_jpeg_loader() { return NULL; }
	bitmap_character_def*	sprite_definition::get_bitmap_character(int id) { return m_movie_def->get_bitmap_character(id); }
	void	sprite_definition::add_bitmap_character(int id, bitmap_character_def* ch) { log_error("add_bc appears in sprite tags!\n"); }
	sound_sample*	sprite_definition::get_sound_sample(int id) { return m_movie_def->get_sound_sample(id); }
	void	sprite_definition::add_sound_sample(int id, sound_sample* sam) { log_error("add sam appears in sprite tags!\n"); }

	// @@ would be nicer to not inherit these...
	create_bitmaps_flag	sprite_definition::get_create_bitmaps() const { QASSERT(0); return DO_LOAD_BITMAPS; }
	create_font_shapes_flag	sprite_definition::get_create_font_shapes() const { QASSERT(0); return DO_LOAD_FONT_SHAPES; }
	int	sprite_definition::get_bitmap_info_count() const { QASSERT(0); return 0; }
	bitmap_info*	sprite_definition::get_bitmap_info(int i) const { QASSERT(0); return NULL; }
	void	sprite_definition::add_bitmap_info(bitmap_info* bi) { QASSERT(0); }

	void	sprite_definition::export_resource(const tu_string& symbol, resource* res) { log_error("can't export from sprite\n"); }
	smart_ptr<resource>	sprite_definition::get_exported_resource(const tu_string& sym) { return m_movie_def->get_exported_resource(sym); }
	void	sprite_definition::add_import(const char* source_url, int id, const char* symbol) { QASSERT(0); }
	void	sprite_definition::visit_imported_movies(import_visitor* v) { QASSERT(0); }
	void	sprite_definition::resolve_import(const char* source_url, movie_definition* d) { QASSERT(0); }
	character_def*	sprite_definition::get_character_def(int id)
	{
		LOG_LINE;
		return m_movie_def->get_character_def(id);
	}
	void	sprite_definition::generate_font_bitmaps() { QASSERT(0); }


	void	sprite_definition::output_cached_data(tu_file* out, const cache_options& options)
	{
		// Nothing to do.
		return;
	}
	void	sprite_definition::input_cached_data(tu_file* in)
	{
		// Nothing to do.
		return;
	}

	movie_interface*	sprite_definition::create_instance()
	{
		return NULL;
	}

	// overloads from character_def
	character*	sprite_definition::create_character_instance(movie* parent, int id)
		// Create a (mutable) instance of our definition.  The
		// instance is created to live (temporarily) on some level on
		// the parent movie's display list.
	{
		LOG_LINE;
		sprite_instance*	si = new sprite_instance(this, parent->get_root(), parent, id);

		return si;
	}


	/* sprite_definition */
	void	sprite_definition::add_execute_tag(execute_tag* c)
	{
		LOG_LINE;
		m_playlist[m_loading_frame].push_back(c);
	}

	/* sprite_definition */
	void	sprite_definition::add_init_action(int sprite_id, execute_tag* c)
	{
		// Sprite def's should not have do_init_action tags in them!  (@@ correct?)
		log_error("sprite_definition::add_init_action called!  Ignored.\n");
	}

	/* sprite_definition */
	void	sprite_definition::add_frame_name(const char* name)
		// Labels the frame currently being loaded with the
		// given name.  A copy of the name string is made and
		// kept in this object.
	{
		QASSERT(m_loading_frame >= 0 && m_loading_frame < m_frame_count);

		LOG_LINE;
		tu_string	n = name;
		LOG_LINE;
		int	currently_assigned = 0;
		LOG_LINE;
		if (m_named_frames.get(n, &currently_assigned) == true)
		{
			LOG_LINE;
			log_error("add_frame_name(%d, '%s') -- frame name already assigned to frame %d; overriding\n",
				m_loading_frame,
				name,
				currently_assigned);
		}
		LOG_LINE;
		m_named_frames.set(n, m_loading_frame);	// stores 0-based frame #
	}

	/* sprite_definition */
	bool	sprite_definition::get_labeled_frame(const char* label, int* frame_number)
		// Returns 0-based frame #
	{
		LOG_LINE;
		return m_named_frames.get(label, frame_number);
	}

	const array<execute_tag*>&	sprite_definition::get_playlist(int frame_number)
		// frame_number is 0-based
	{
		LOG_LINE;
		return m_playlist[frame_number];
	}

	/* sprite_definition */
	const array<execute_tag*>*	sprite_definition::get_init_actions(int frame_number)
	{
		// Sprites do not have init actions in their
		// playlists!  Only the root movie
		// (movie_def_impl) does (@@ correct?)
		return NULL;
	}


	/* sprite_definition */
	void	sprite_definition::read(stream* in)
		// Read the sprite info.  Consists of a series of tags.
	{
		LOG_LINE;
		int	tag_end = in->get_tag_end_position();

		LOG_LINE;
		m_frame_count = in->read_u16();
		// ALEX: some SWF files have been seen that have 0-frame sprites.
		// The Macromedia player behaves as if they have 1 frame.
		if (m_frame_count < 1)
		{
			m_frame_count = 1;
		}
		LOG_LINE;
		m_playlist.resize(m_frame_count);	// need a playlist for each frame

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  frames = %d\n", m_frame_count));

		LOG_LINE;
		m_loading_frame = 0;

		LOG_LINE;
		while ((Uint32) in->get_position() < (Uint32) tag_end)
		{
			LOG_LINE;
			int	tag_type = in->open_tag();
			LOG_LINE;
			loader_function lf = NULL;
			LOG_LINE;
			if (tag_type == 1)
			{
				// show frame tag -- advance to the next frame.
				IF_VERBOSE_PARSE(log_msg("  show_frame (sprite)\n"));
				LOG_LINE;
				m_loading_frame++;
			}
			else if (s_tag_loaders.get(tag_type, &lf))
			{
				// call the tag loader.  The tag loader should add
				// characters or tags to the movie data structure.
				LOG_LINE;
				(*lf)(in, tag_type, this);
			}
			else
			{
				// no tag loader for this tag type.
				IF_VERBOSE_PARSE(log_error("*** no tag loader for type %d\n", tag_type));
			}

			LOG_LINE;
			in->close_tag();
		}

		IF_VERBOSE_PARSE(log_msg("  -- sprite END --\n"));
	}

}	// end namespace gameswf


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
