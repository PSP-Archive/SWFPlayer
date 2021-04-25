// gameswf_action.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Implementation and helpers for SWF actions.


#include "gameswf_action.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "gameswf_stream.h"
#include "base/tu_random.h"

#include "gameswf_string.h"
#include "gameswf_movie.h"
#include "gameswf_timers.h"
#include "gameswf_textformat.h"
#include "gameswf_sound.h"

#ifdef HAVE_LIBXML
#include "gameswf_xml.h"
#include "gameswf_xmlsocket.h"
#endif


#ifdef _WIN32
#define snprintf _snprintf
#endif // _WIN32

// NOTES:
//
// Buttons
// on (press)                 onPress
// on (release)               onRelease
// on (releaseOutside)        onReleaseOutside
// on (rollOver)              onRollOver
// on (rollOut)               onRollOut
// on (dragOver)              onDragOver
// on (dragOut)               onDragOut
// on (keyPress"...")         onKeyDown, onKeyUp      <----- IMPORTANT
//
// Sprites
// onClipEvent (load)         onLoad
// onClipEvent (unload)       onUnload                Hm.
// onClipEvent (enterFrame)   onEnterFrame
// onClipEvent (mouseDown)    onMouseDown
// onClipEvent (mouseUp)      onMouseUp
// onClipEvent (mouseMove)    onMouseMove
// onClipEvent (keyDown)      onKeyDown
// onClipEvent (keyUp)        onKeyUp
// onClipEvent (data)         onData

// Text fields have event handlers too!

// Sprite built in methods:
// play()
// stop()
// gotoAndStop()
// gotoAndPlay()
// nextFrame()
// startDrag()
// getURL()
// getBytesLoaded()
// getBytesTotal()

// Built-in functions: (do these actually exist in the VM, or are they just opcodes?)
// Number()
// String()


// TODO builtins
//
// Number.toString() -- takes an optional arg that specifies the base
//
// parseInt(), parseFloat()
//
// Boolean() type cast
//
// typeof operator --> "number", "string", "boolean", "object" (also
// for arrays), "null", "movieclip", "function", "undefined"
//
// isNaN()
//
// Number.MAX_VALUE, Number.MIN_VALUE
//
// String.fromCharCode()


namespace gameswf
{
	//
	//	Key remap table
	//
	static u8	s_key_remap_table[] =
	{
		key::LEFT,		1,
		key::RIGHT,		2,
		key::UP,		14,
		key::DOWN,		15,
		key::INSERT,	45,
		key::DELETEKEY,	6,
		key::BACKSPACE,	8,
		key::ENTER,		13,
		key::PGDN,		17,
		key::PGUP,		16,
		key::TAB,		18,
		key::ESCAPE,	19,
		key::SPACE,		32,
		key::_0,		48,
		key::_1,		49,
		key::_2,		50,
		key::_3,		51,
		key::_4,		52,
		key::_5,		53,
		key::_6,		54,
		key::_7,		55,
		key::_8,		56,
		key::_9,		57,
	};
	static const int	s_num_key_maps( sizeof( s_key_remap_table ) / sizeof( s_key_remap_table[ 0 ] ) );

	
	//
	// action stuff
	//

	void	action_init();

	// Statics.
	bool	s_inited = false;
	smart_ptr<as_object>	s_global;
	fscommand_callback	s_fscommand_handler = NULL;

	smart_ptr<movie>	g_active_entity = NULL;


#define EXTERN_MOVIE
	
#ifdef EXTERN_MOVIE
	void attach_extern_movie(const char* url, const movie* target, const movie* root_movie)
	{
		LOG_LINE;
		tu_string infile = get_workdir();
		LOG_LINE;
		infile += url;

		LOG_LINE;
		movie_definition_sub*	md = create_library_movie_sub(infile.c_str());
		LOG_LINE;
		if (md == NULL)
		{
			LOG_LINE;
			log_error("can't create movie_definition_sub for %s\n", infile.c_str());
			return;
		}

		LOG_LINE;
		gameswf::movie_interface* extern_movie;

		LOG_LINE;
		if (target == root_movie)
		{
			LOG_LINE;
			extern_movie = create_library_movie_inst_sub(md);			
			LOG_LINE;
			if (extern_movie == NULL)
			{
				LOG_LINE;
				log_error("can't create extern root movie_interface for %s\n", infile.c_str());
				return;
			}
			LOG_LINE;
			set_current_root(extern_movie);
			LOG_LINE;
			movie* m = extern_movie->get_root_movie();

			LOG_LINE;
			m->on_event(event_id::LOAD);
		}
		else
		{
			LOG_LINE;
			extern_movie = md->create_instance();
			LOG_LINE;
			if (extern_movie == NULL)
			{
				LOG_LINE;
				log_error("can't create extern movie_interface for %s\n", infile.c_str());
				return;
			}
      
			LOG_LINE;
			save_extern_movie(extern_movie);
      
			LOG_LINE;
			character* tar = (character*)target;
			LOG_LINE;
			const char* name = tar->get_name();
			LOG_LINE;
			Uint16 depth = tar->get_depth();
			LOG_LINE;
			bool use_cxform = false;
			LOG_LINE;
			cxform color_transform =  tar->get_cxform();
			LOG_LINE;
			bool use_matrix = false;
			LOG_LINE;
			matrix mat = tar->get_matrix();
			LOG_LINE;
			float ratio = tar->get_ratio();
			LOG_LINE;
			Uint16 clip_depth = tar->get_clip_depth();
			LOG_LINE;

			LOG_LINE;
			movie* parent = tar->get_parent();
			LOG_LINE;
			movie* new_movie = extern_movie->get_root_movie();
			LOG_LINE;

			LOG_LINE;
			QASSERT(parent != NULL);

			LOG_LINE;
			((character*)new_movie)->set_parent(parent);
       
			LOG_LINE;
			parent->replace_display_object(
				(character*) new_movie,
				name,
				depth,
				use_cxform,
				color_transform,
				use_matrix,
				mat,
				ratio,
				clip_depth);
		}
	}
#endif // EXTERN_MOVIE

	void	register_fscommand_callback(fscommand_callback handler)
	// External interface.
	{
		LOG_LINE;
		s_fscommand_handler = handler;
	}



	static bool string_to_number(double* result, const char* str)
	// Utility.  Try to convert str to a number.  If successful,
	// put the result in *result, and return true.  If not
	// successful, put 0 in *result, and return false.
	{
		LOG_LINE;
		char* tail = 0;
		LOG_LINE;
		*result = strtod(str, &tail);
		LOG_LINE;
		if (tail == str || *tail != 0)
		{
			LOG_LINE;
			// Failed conversion to Number.
			return false;
		}
		LOG_LINE;
		return true;
	}


	//
	// array object
	//


	struct as_array_object : public as_object
	{
// @@ TODO
//		as_array_object()
//		{
//			this->set_member("length", &array_not_impl);
//			this->set_member("join", &array_not_impl);
//			this->set_member("concat", &array_not_impl);
//			this->set_member("slice", &array_not_impl);
//			this->set_member("push", &array_not_impl);
//			this->set_member("unshift", &array_not_impl);
//			this->set_member("pop", &array_not_impl);
//			this->set_member("shift", &array_not_impl);
//			this->set_member("splice", &array_not_impl);
//			this->set_member("sort", &array_not_impl);
//			this->set_member("sortOn", &array_not_impl);
//			this->set_member("reverse", &array_not_impl);
//			this->set_member("toString", &array_not_impl);
//		}
	};

	void	array_not_impl(const fn_call& fn)
	{
		LOG_LINE;
		log_error("array methods not implemented yet\n");
	}


	//
	// as_as_function
	//

	void	as_as_function::operator()(const fn_call& fn)
	// Dispatch.
	{
		LOG_LINE;
		as_environment*	our_env = m_env;
		LOG_LINE;
		if (our_env == NULL)
		{
			LOG_LINE;
			our_env = fn.env;
		}
		LOG_LINE;
		QASSERT(our_env);

		// Set up local stack frame, for parameters and locals.
		LOG_LINE;
		int	local_stack_top = our_env->get_local_frame_top();
		LOG_LINE;
		our_env->add_frame_barrier();

		LOG_LINE;
		if (m_is_function2 == false)
		{
			LOG_LINE;
			// Conventional function.

			// Push the arguments onto the local frame.
			LOG_LINE;
			int	args_to_pass = imin(fn.nargs, m_args.size());
			LOG_LINE;
			for (int i = 0; i < args_to_pass; i++)
			{
				LOG_LINE;
				QASSERT(m_args[i].m_register == 0);
				LOG_LINE;
				our_env->add_local(m_args[i].m_name, fn.arg(i));
			}
		}
		else
		{
			// function2: most args go in registers; any others get pushed.
			
			// Create local registers.
			LOG_LINE;
			our_env->add_local_registers(m_local_register_count);

			// Handle the explicit args.
			LOG_LINE;
			int	args_to_pass = imin(fn.nargs, m_args.size());
			LOG_LINE;
			for (int i = 0; i < args_to_pass; i++)
			{
				LOG_LINE;
				if (m_args[i].m_register == 0)
				{
					// Conventional arg passing: create a local var.
					LOG_LINE;
					our_env->add_local(m_args[i].m_name, fn.arg(i));
				}
				else
				{
					// Pass argument into a register.
					LOG_LINE;
					int	reg = m_args[i].m_register;
					LOG_LINE;
					*(our_env->local_register_ptr(reg)) = fn.arg(i);
				}
			}

			// Handle the implicit args.
			LOG_LINE;
			int	current_reg = 1;
			LOG_LINE;
			if (m_function2_flags & 0x01)
			{
				// preload 'this' into a register.
				LOG_LINE;
				(*(our_env->local_register_ptr(current_reg))).set_as_object_interface(our_env->m_target);
				LOG_LINE;
				current_reg++;
			}

			if (m_function2_flags & 0x02)
			{
				LOG_LINE;
				// Don't put 'this' into a local var.
			}
			else
			{
				// Put 'this' in a local var.
				LOG_LINE;
				our_env->add_local("this", as_value(our_env->m_target));
			}

			// Init arguments array, if it's going to be needed.
			LOG_LINE;
			smart_ptr<as_array_object>	arg_array;
			LOG_LINE;
			if ((m_function2_flags & 0x04) || ! (m_function2_flags & 0x08))
			{
				LOG_LINE;
				arg_array = new as_array_object;

				LOG_LINE;
				as_value	index_number;
				LOG_LINE;
				for (int i = 0; i < fn.nargs; i++)
				{
					LOG_LINE;
					index_number.set_int(i);
					LOG_LINE;
					arg_array->set_member(index_number.to_string(), fn.arg(i));
				}
			}

			LOG_LINE;
			if (m_function2_flags & 0x04)
			{
				// preload 'arguments' into a register.
				LOG_LINE;
				(*(our_env->local_register_ptr(current_reg))).set_as_object_interface(arg_array.get_ptr());
				LOG_LINE;
				current_reg++;
			}

			LOG_LINE;
			if (m_function2_flags & 0x08)
			{
				// Don't put 'arguments' in a local var.
				LOG_LINE;
			}
			else
			{
				// Put 'arguments' in a local var.
				LOG_LINE;
 				our_env->add_local("arguments", as_value(arg_array.get_ptr()));
			}

			LOG_LINE;
			if (m_function2_flags & 0x10)
			{
				// Put 'super' in a register.
				LOG_LINE;
				log_error("TODO: implement 'super' in function2 dispatch (reg)\n");

				LOG_LINE;
				current_reg++;
			}

			LOG_LINE;
			if (m_function2_flags & 0x20)
			{
				// Don't put 'super' in a local var.
				LOG_LINE;
			}
			else
			{
				// Put 'super' in a local var.
				LOG_LINE;
				log_error("TODO: implement 'super' in function2 dispatch (var)\n");
			}

			LOG_LINE;
			if (m_function2_flags & 0x40)
			{
				// Put '_root' in a register.
				LOG_LINE;
				(*(our_env->local_register_ptr(current_reg))).set_as_object_interface(
					our_env->m_target->get_root_movie());
				LOG_LINE;
				current_reg++;
			}

			LOG_LINE;
			if (m_function2_flags & 0x80)
			{
				// Put '_parent' in a register.
				LOG_LINE;
				array<with_stack_entry>	dummy;
				LOG_LINE;
				as_value	parent = our_env->get_variable("_parent", dummy);
				LOG_LINE;
				(*(our_env->local_register_ptr(current_reg))) = parent;
				LOG_LINE;
				current_reg++;
			}

			LOG_LINE;
			if (m_function2_flags & 0x100)
			{
				// Put '_global' in a register.
				LOG_LINE;
				(*(our_env->local_register_ptr(current_reg))).set_as_object_interface(s_global.get_ptr());
				LOG_LINE;
				current_reg++;
			}
		}

		// Execute the actions.
		LOG_LINE;
		m_action_buffer->execute(our_env, m_start_pc, m_length, fn.result, m_with_stack, m_is_function2);

		// Clean up stack frame.
		LOG_LINE;
		our_env->set_local_frame_top(local_stack_top);

		LOG_LINE;
		if (m_is_function2)
		{
			// Clean up the local registers.
			LOG_LINE;
			our_env->drop_local_registers(m_local_register_count);
		}
	}


	//
	// Function/method dispatch.
	//


	as_value	call_method(
		const as_value& method,
		as_environment* env,
		as_object_interface* this_ptr,
		int nargs,
		int first_arg_bottom_index)
	// first_arg_bottom_index is the stack index, from the bottom, of the first argument.
	// Subsequent arguments are at *lower* indices.  E.g. if first_arg_bottom_index = 7,
	// then arg1 is at env->bottom(7), arg2 is at env->bottom(6), etc.
	{
		as_value	val;

		LOG_LINE;
		as_c_function_ptr	func = method.to_c_function();
		LOG_LINE;
		if (func)
		{
			// It's a C function.  Call it.
			LOG_LINE;
			(*func)(fn_call(&val, this_ptr, env, nargs, first_arg_bottom_index));
		}
		else if (as_as_function* as_func = method.to_as_function())
		{
			LOG_LINE;
			// It's an ActionScript function.  Call it.
			(*as_func)(fn_call(&val, this_ptr, env, nargs, first_arg_bottom_index));
		}
		else
		{
			LOG_LINE;
			log_error("error in call_method(): method is not a function\n");
		}

		LOG_LINE;
		return val;
	}


	as_value	call_method0(
		const as_value& method,
		as_environment* env,
		as_object_interface* this_ptr)
	{
		LOG_LINE;
		return call_method(method, env, this_ptr, 0, env->get_top_index() + 1);
	}
		
	const char*	call_method_parsed(
		as_environment* env,
		as_object_interface* this_ptr,
		const char* method_name,
		const char* method_arg_fmt,
		va_list args)
	// Printf-like vararg interface for calling ActionScript.
	// Handy for external binding.
	{
		LOG_LINE;
		log_msg("FIXME(%d): %s\n", __LINE__, __FUNCTION__);

#if 0
		static const int	BUFSIZE = 1000;
		char	buffer[BUFSIZE];
		array<const char*>	tokens;

		// Brutal crap parsing.  Basically null out any
		// delimiter characters, so that the method name and
		// args sit in the buffer as null-terminated C
		// strings.  Leave an intial ' character as the first
		// char in a string argument.
		// Don't verify parens or matching quotes or anything.
		{
			strncpy(buffer, method_call, BUFSIZE);
			buffer[BUFSIZE - 1] = 0;
			char*	p = buffer;

			char	in_quote = 0;
			bool	in_arg = false;
			for (;; p++)
			{
				char	c = *p;
				if (c == 0)
				{
					// End of string.
					break;
				}
				else if (c == in_quote)
				{
					// End of quotation.
					QASSERT(in_arg);
					*p = 0;
					in_quote = 0;
					in_arg = false;
				}
				else if (in_arg)
				{
					if (in_quote == 0)
					{
						if (c == ')' || c == '(' || c == ',' || c == ' ')
						{
							// End of arg.
							*p = 0;
							in_arg = false;
						}
					}
				}
				else
				{
					// Not in arg.  Watch for start of arg.
					QASSERT(in_quote == 0);
					if (c == '\'' || c == '\"')
					{
						// Start of quote.
						in_quote = c;
						in_arg = true;
						*p = '\'';	// ' at the start of the arg, so later we know this is a string.
						tokens.push_back(p);
					}
					else if (c == ' ' || c == ',')
					{
						// Non-arg junk, null it out.
						*p = 0;
					}
					else
					{
						// Must be the start of a numeric arg.
						in_arg = true;
						tokens.push_back(p);
					}
				}
			}
		}
#endif // 0


		// Parse va_list args
		LOG_LINE;
		int	starting_index = env->get_top_index();
		LOG_LINE;
		const char* p = method_arg_fmt;
		for (;; p++)
		{
			LOG_LINE;
			char	c = *p;
			LOG_LINE;
			if (c == 0)
			{
				// End of args.
				LOG_LINE;
				break;
			}
			else if (c == '%')
			{
				LOG_LINE;
				p++;
				LOG_LINE;
				c = *p;
				LOG_LINE;
				// Here's an arg.
				if (c == 'd')
				{
					// Integer.
					LOG_LINE;
					env->push(va_arg(args, int));
				}
				else if (c == 'f')
				{
					// Double
					LOG_LINE;
					env->push(va_arg(args, double));
				}
				else if (c == 's')
				{
					// String
					LOG_LINE;
					env->push(va_arg(args, const char *));
				}
				else if (c == 'l')
				{
					LOG_LINE;
					p++;
					LOG_LINE;
					c = *p;
					LOG_LINE;
					if (c == 's')
					{
						// Wide string.
						LOG_LINE;
						env->push(va_arg(args, const wchar_t *));
					}
					else
					{
						LOG_LINE;
						log_error("call_method_parsed('%s','%s') -- invalid fmt '%%l%c'\n",
							  method_name,
							  method_arg_fmt,
							  c);
					}
				}
				else
				{
					// Invalid fmt, warn.
					LOG_LINE;
					log_error("call_method_parsed('%s','%s') -- invalid fmt '%%%c'\n",
						  method_name,
						  method_arg_fmt,
						  c);
				}
			}
			else
			{
				// Ignore whitespace and commas.
				LOG_LINE;
				if (c == ' ' || c == '\t' || c == ',')
				{
					// OK
					LOG_LINE;
				}
				else
				{
					// Invalid arg; warn.
					LOG_LINE;
					log_error("call_method_parsed('%s','%s') -- invalid char '%c'\n",
						  method_name,
						  method_arg_fmt,
						  c);
				}
			}
		}

		LOG_LINE;
		array<with_stack_entry>	dummy_with_stack;
		LOG_LINE;
		as_value	method = env->get_variable(method_name, dummy_with_stack);

		// check method

		// Reverse the order of pushed args
		LOG_LINE;
		int	nargs = env->get_top_index() - starting_index;
		LOG_LINE;
		for (int i = 0; i < (nargs >> 1); i++)
		{
			LOG_LINE;
			int	i0 = starting_index + 1 + i;
			LOG_LINE;
			int	i1 = starting_index + nargs - i;
			LOG_LINE;
			QASSERT(i0 < i1);

			LOG_LINE;
			swap(&(env->bottom(i0)), &(env->bottom(i1)));
		}

		// Do the call.
		LOG_LINE;
		as_value	result = call_method(method, env, this_ptr, nargs, env->get_top_index());
		LOG_LINE;
		env->drop(nargs);

		// Return pointer to static string for return value.
		LOG_LINE;
		static tu_string	s_retval;
		LOG_LINE;
		s_retval = result.to_tu_string();
		LOG_LINE;
		return s_retval.c_str();
	}


	//
	// sound object
	//

	struct sound_as_object : public as_object
	{
		tu_string sound;
		int sound_id;
	};

	void	movie_load()
	{
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("-- start movie \n"));
	}

	void	sound_start(const fn_call& fn)
	{
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("-- start sound \n"));
		LOG_LINE;
		sound_handler* s = get_sound_handler();
		LOG_LINE;
		if (s != NULL)
		{
			LOG_LINE;
			sound_as_object*	so = (sound_as_object*) (as_object*) fn.this_ptr;
			LOG_LINE;
			QASSERT(so);
			LOG_LINE;
			s->play_sound(so->sound_id, 0);
		}
	}


	void	sound_stop(const fn_call& fn)
	{
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("-- stop sound \n"));
		LOG_LINE;
		sound_handler* s = get_sound_handler();
		LOG_LINE;
		if (s != NULL)
		{
			LOG_LINE;
			sound_as_object*	so = (sound_as_object*) (as_object*) fn.this_ptr;
			LOG_LINE;
			QASSERT(so);
			LOG_LINE;
			s->stop_sound(so->sound_id);
		}
	}

	void	sound_attach(const fn_call& fn)
	{
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("-- attach sound \n"));
		LOG_LINE;
		if (fn.nargs < 1)
		{
			LOG_LINE;
			log_error("attach sound needs one argument\n");
			return;
		}

		LOG_LINE;
		sound_as_object*	so = (sound_as_object*) (as_object*) fn.this_ptr;
		LOG_LINE;
		QASSERT(so);

		LOG_LINE;
		so->sound = fn.arg(0).to_tu_string();

		// check the import.
		LOG_LINE;
		movie_definition_sub*	def = (movie_definition_sub*)
			fn.env->get_target()->get_root_movie()->get_movie_definition();
		LOG_LINE;
		QASSERT(def);
		LOG_LINE;
		smart_ptr<resource> res = def->get_exported_resource(so->sound);
		LOG_LINE;
		if (res == NULL)
		{
			LOG_LINE;
			log_error("import error: resource '%s' is not exported\n", so->sound.c_str());
			return;
		}

		LOG_LINE;
		int si = 0;
		LOG_LINE;
		sound_sample_impl* ss = (sound_sample_impl*) res->cast_to_sound_sample();

		LOG_LINE;
		if (ss != NULL)
		{
			LOG_LINE;
			si = ss->m_sound_handler_id;
		}
		else
		{
			LOG_LINE;
			log_error("sound sample is NULL\n");
			return;
		}

		// sanity check
		LOG_LINE;
		QASSERT(si >= 0 && si < 1000);
		LOG_LINE;
		so->sound_id = si;
	}

	//
	// Built-in objects
	//


	//
	// math object
	//


#if 0
	// One-argument simple functions.
	#define MATH_WRAP_FUNC1(funcname)							\
	void	math_##funcname(as_value* result, as_object_interface* this_ptr,		\
				as_environment* env, int nargs, int first_arg_bottom_index)	\
	{											\
		double	arg = env->bottom(first_arg_bottom_index).to_number();			\
		result->set_double(funcname(arg));						\
	}
#else
	// One-argument simple functions.
	#define MATH_WRAP_FUNC1(funcname)							\
	void	math_##funcname(const fn_call& fn)						\
	{											\
		LOG_LINE;	\
		double	arg = fn.arg(0).to_number();						\
		LOG_LINE;	\
		fn.result->set_double(funcname(arg));						\
	}
#endif

	MATH_WRAP_FUNC1(fabs);
	MATH_WRAP_FUNC1(acos);
	MATH_WRAP_FUNC1(asin);
	MATH_WRAP_FUNC1(atan);
	MATH_WRAP_FUNC1(ceil);
	MATH_WRAP_FUNC1(cos);
	MATH_WRAP_FUNC1(exp);
	MATH_WRAP_FUNC1(floor);
	MATH_WRAP_FUNC1(log);
	MATH_WRAP_FUNC1(sin);
	MATH_WRAP_FUNC1(sqrt);
	MATH_WRAP_FUNC1(tan);

#if 0
	// Two-argument functions.
	#define MATH_WRAP_FUNC2_EXP(funcname, expr)										\
	void	math_##funcname(as_value* result, as_object_interface* this_ptr, as_environment* env, int nargs, int first_arg_bottom_index)	\
	{															\
		double	arg0 = env->bottom(first_arg_bottom_index).to_number();							\
		double	arg1 = env->bottom(first_arg_bottom_index - 1).to_number();						\
		result->set_double(expr);											\
	}
#else
	// Two-argument functions.
	#define MATH_WRAP_FUNC2_EXP(funcname, expr)										\
	void	math_##funcname(const fn_call& fn)										\
	{															\
		LOG_LINE;	\
		double	arg0 = fn.arg(0).to_number();										\
		LOG_LINE;	\
		double	arg1 = fn.arg(1).to_number();										\
		LOG_LINE;	\
		fn.result->set_double(expr);											\
	}
#endif
	MATH_WRAP_FUNC2_EXP(atan2, (atan2(arg0, arg1)));
	MATH_WRAP_FUNC2_EXP(max, (arg0 > arg1 ? arg0 : arg1));
	MATH_WRAP_FUNC2_EXP(min, (arg0 < arg1 ? arg0 : arg1));
	MATH_WRAP_FUNC2_EXP(pow, (pow(arg0, arg1)));

	// A couple of oddballs.
	void	math_random(const fn_call& fn)
	{
		// Random number between 0 and 1.
		LOG_LINE;
		fn.result->set_double(tu_random::next_random() / double(Uint32(0x0FFFFFFFF)));
	}
	void	math_round(const fn_call& fn)
	{
		// round argument to nearest int.
		LOG_LINE;
		double	arg0 = fn.arg(0).to_number();
		LOG_LINE;
		fn.result->set_double(floor(arg0 + 0.5));
	}
	
	void math_init()
	{
		// Create built-in math object.
		LOG_LINE;
		as_object*	math_obj = new as_object;

		// constant
		LOG_LINE;
		math_obj->set_member("e", 2.7182818284590452354);
		LOG_LINE;
		math_obj->set_member("ln2", 0.69314718055994530942);
		LOG_LINE;
		math_obj->set_member("log2e", 1.4426950408889634074);
		LOG_LINE;
		math_obj->set_member("ln10", 2.30258509299404568402);
		LOG_LINE;
		math_obj->set_member("log10e", 0.43429448190325182765);
		LOG_LINE;
		math_obj->set_member("pi", 3.14159265358979323846);
		LOG_LINE;
		math_obj->set_member("sqrt1_2", 0.7071067811865475244);
		LOG_LINE;
		math_obj->set_member("sqrt2", 1.4142135623730950488);

		// math methods
		LOG_LINE;
		math_obj->set_member("abs", &math_fabs);
		LOG_LINE;
		math_obj->set_member("acos", &math_acos);
		LOG_LINE;
		math_obj->set_member("asin", &math_asin);
		LOG_LINE;
		math_obj->set_member("atan", &math_atan);
		LOG_LINE;
		math_obj->set_member("ceil", &math_ceil);
		LOG_LINE;
		math_obj->set_member("cos", &math_cos);
		LOG_LINE;
		math_obj->set_member("exp", &math_exp);
		LOG_LINE;
		math_obj->set_member("floor", &math_floor);
		LOG_LINE;
		math_obj->set_member("log", &math_log);
		LOG_LINE;
		math_obj->set_member("random", &math_random);
		LOG_LINE;
		math_obj->set_member("round", &math_round);
		LOG_LINE;
		math_obj->set_member("sin", &math_sin);
		LOG_LINE;
		math_obj->set_member("sqrt", &math_sqrt);
		LOG_LINE;
		math_obj->set_member("tan", &math_tan);

		LOG_LINE;
		math_obj->set_member("atan2", &math_atan2);
		LOG_LINE;
		math_obj->set_member("max", &math_max);
		LOG_LINE;
		math_obj->set_member("min", &math_min);
		LOG_LINE;
		math_obj->set_member("pow", &math_pow);

		LOG_LINE;
		s_global->set_member("math", math_obj);
	}
		
	void event_test(const fn_call& fn)
	{
		LOG_LINE;
		log_msg("FIXME: %s\n", __FUNCTION__);
	}
	
	//
	// key object
	//


	struct key_as_object : public as_object
	{
		Uint8	m_keymap[key::KEYCOUNT / 8 + 1];	// bit-array
		array<weak_ptr<as_object_interface> >	m_listeners;
		int	m_last_key_pressed;

		key_as_object()
			:
			m_last_key_pressed(0)
		{
			LOG_LINE;
			memset(m_keymap, 0, sizeof(m_keymap));
		}

		bool	is_key_down(int code)
		{
			LOG_LINE;
			if (code < 0 || code >= key::KEYCOUNT) return false;

			LOG_LINE;
			int	byte_index = code >> 3;
			LOG_LINE;
			int	bit_index = code - (byte_index << 3);
			LOG_LINE;
			int	mask = 1 << bit_index;

			LOG_LINE;
			QASSERT(byte_index >= 0 && byte_index < int(sizeof(m_keymap)/sizeof(m_keymap[0])));

			LOG_LINE;
			if (m_keymap[byte_index] & mask)
			{
				LOG_LINE;
				return true;
			}
			else
			{
				LOG_LINE;
				return false;
			}
		}

		void	set_key_down(key::code code)
		{
			LOG_LINE;
			if (code < 0 || code >= key::KEYCOUNT) return;

			LOG_LINE;
			m_last_key_pressed = code;

			LOG_LINE;
			int	byte_index = code >> 3;
			LOG_LINE;
			int	bit_index = code - (byte_index << 3);
			LOG_LINE;
			int	mask = 1 << bit_index;

			LOG_LINE;
			QASSERT(byte_index >= 0 && byte_index < int(sizeof(m_keymap)/sizeof(m_keymap[0])));

			LOG_LINE;
			m_keymap[byte_index] |= mask;

			// Notify listeners.
			int i;
			LOG_LINE;
			int n = m_listeners.size();
			LOG_LINE;
			for (i = 0; i < n; i++)
			{
				LOG_LINE;
				smart_ptr<as_object_interface>	listener = m_listeners[i];
				LOG_LINE;
				as_value	method;
				LOG_LINE;
				if ( listener != NULL && listener->get_member( event_id( event_id::KEY_DOWN ).get_function_name(), &method ) )
				{
					LOG_LINE;
					call_method(method, NULL /* or root? */, listener.get_ptr(), 0, 0);
				}
			}

			//
			//	**HACK** - Send the event to the buttons
			//
			if ( g_active_entity != NULL )
			{
				event_id	key_id( event_id::KEY_PRESS, code );

				g_active_entity->on_button_event( key_id );
			}
		}

		void	set_key_up(int code)
		{
			LOG_LINE;
			if (code < 0 || code >= key::KEYCOUNT) return;

			LOG_LINE;
			int	byte_index = code >> 3;
			LOG_LINE;
			int	bit_index = code - (byte_index << 3);
			LOG_LINE;
			int	mask = 1 << bit_index;

			LOG_LINE;
			QASSERT(byte_index >= 0 && byte_index < int(sizeof(m_keymap)/sizeof(m_keymap[0])));

			LOG_LINE;
			m_keymap[byte_index] &= ~mask;

			// Notify listeners.
			LOG_LINE;
			for (int i = 0, n = m_listeners.size(); i < n; i++)
			{
				LOG_LINE;
				smart_ptr<as_object_interface>	listener = m_listeners[i];

				LOG_LINE;
				as_value	method;
				LOG_LINE;
				if (listener != NULL
				    && listener->get_member(event_id(event_id::KEY_UP).get_function_name(), &method))
				{
					LOG_LINE;
					call_method(method, NULL /* or root? */, listener.get_ptr(), 0, 0);
				}
			}
		}

		void	cleanup_listeners()
		// Remove dead entries in the listeners list.  (Since
		// we use weak_ptr's, listeners can disappear without
		// notice.)
		{
			LOG_LINE;
			for (int i = m_listeners.size() - 1; i >= 0; i--)
			{
				LOG_LINE;
				if (m_listeners[i] == NULL)
				{
					LOG_LINE;
					m_listeners.remove(i);
				}
			}
		}

		void	add_listener(as_object_interface* listener)
		{
			LOG_LINE;
			cleanup_listeners();

			LOG_LINE;
			for (int i = 0, n = m_listeners.size(); i < n; i++)
			{
				LOG_LINE;
				if (m_listeners[i] == listener)
				{
					// Already in the list.
					LOG_LINE;
					return;
				}
			}

			LOG_LINE;
			m_listeners.push_back(listener);
		}

		void	remove_listener(as_object_interface* listener)
		{
			LOG_LINE;
			cleanup_listeners();

			for (int i = m_listeners.size() - 1; i >= 0; i--)
			{
				LOG_LINE;
				if (m_listeners[i] == listener)
				{
					LOG_LINE;
					m_listeners.remove(i);
				}
			}
		}

		int	get_last_key_pressed() const
		{
			LOG_LINE;
			return m_last_key_pressed;
		}
	};


	void	key_add_listener(const fn_call& fn)
	// Add a listener (first arg is object reference) to our list.
	// Listeners will have "onKeyDown" and "onKeyUp" methods
	// called on them when a key changes state.
	{

		LOG_LINE;
		if (fn.nargs < 1)
		{
			LOG_LINE;
			log_error("key_add_listener needs one argument (the listener object)\n");
			return;
		}

		LOG_LINE;
		as_object_interface*	listener = fn.arg(0).to_object();
		if (listener == NULL)
		{
			LOG_LINE;
			log_error("key_add_listener passed a NULL object; ignored\n");
			return;
		}

		LOG_LINE;
		key_as_object*	ko = (key_as_object*) (as_object*) fn.this_ptr;
		QASSERT(ko);

		LOG_LINE;
		ko->add_listener(listener);
	}

	void	key_get_ascii(const fn_call& fn)
	// Return the ascii value of the last key pressed.
	{
		LOG_LINE;
		key_as_object*	ko = (key_as_object*) (as_object*) fn.this_ptr;
		QASSERT(ko);

		LOG_LINE;
		fn.result->set_undefined();

		LOG_LINE;
		int	code = ko->get_last_key_pressed();
		if (code > 0)
		{
			// @@ Crude for now; just jamming the key code in a string, as a character.
			// Need to apply shift/capslock/numlock, etc...
			LOG_LINE;
			char	buf[2];
			buf[0] = (char) code;
			buf[1] = 0;

			LOG_LINE;
			fn.result->set_string(buf);
		}
	}

	void	key_get_code(const fn_call& fn)
	// Returns the keycode of the last key pressed.
	{
		LOG_LINE;
		key_as_object*	ko = (key_as_object*) (as_object*) fn.this_ptr;
		QASSERT(ko);

		LOG_LINE;
		fn.result->set_int(ko->get_last_key_pressed());
	}

	void	key_is_down(const fn_call& fn)
	// Return true if the specified (first arg keycode) key is pressed.
	{
		LOG_LINE;
		if (fn.nargs < 1)
		{
			LOG_LINE;
			log_error("key_is_down needs one argument (the key code)\n");
			return;
		}

		LOG_LINE;
		int	code = (int) fn.arg(0).to_number();

		LOG_LINE;
		key_as_object*	ko = (key_as_object*) (as_object*) fn.this_ptr;
		QASSERT(ko);

		LOG_LINE;
		fn.result->set_bool(ko->is_key_down(code));
	}

	void	key_is_toggled(const fn_call& fn)
	// Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
	// the associated state is on.
	{
		// @@ TODO
		LOG_LINE;
		fn.result->set_bool(false);
	}

	void	key_remove_listener(const fn_call& fn)
	// Remove a previously-added listener.
	{
		LOG_LINE;
		if (fn.nargs < 1)
		{
			LOG_LINE;
			log_error("key_remove_listener needs one argument (the listener object)\n");
			return;
		}

		LOG_LINE;
		as_object_interface*	listener = fn.arg(0).to_object();
		if (listener == NULL)
		{
			LOG_LINE;
			log_error("key_remove_listener passed a NULL object; ignored\n");
			return;
		}

		LOG_LINE;
		key_as_object*	ko = (key_as_object*) (as_object*) fn.this_ptr;
		QASSERT(ko);

		LOG_LINE;
		ko->remove_listener(listener);
	}


	void key_init()
	{
		// Create built-in key object.
		LOG_LINE;
		as_object*	key_obj = new key_as_object;

		// constants
#define KEY_CONST(k) key_obj->set_member(#k, key::k)
		LOG_LINE;
		KEY_CONST(BACKSPACE);
		LOG_LINE;
		KEY_CONST(CAPSLOCK);
		LOG_LINE;
		KEY_CONST(CONTROL);
		LOG_LINE;
		KEY_CONST(DELETEKEY);
		LOG_LINE;
		KEY_CONST(DOWN);
		LOG_LINE;
		KEY_CONST(END);
		LOG_LINE;
		KEY_CONST(ENTER);
		LOG_LINE;
		KEY_CONST(ESCAPE);
		LOG_LINE;
		KEY_CONST(HOME);
		LOG_LINE;
		KEY_CONST(INSERT);
		LOG_LINE;
		KEY_CONST(LEFT);
		LOG_LINE;
		KEY_CONST(PGDN);
		LOG_LINE;
		KEY_CONST(PGUP);
		LOG_LINE;
		KEY_CONST(RIGHT);
		LOG_LINE;
		KEY_CONST(SHIFT);
		LOG_LINE;
		KEY_CONST(SPACE);
		LOG_LINE;
		KEY_CONST(TAB);
		LOG_LINE;
		KEY_CONST(UP);

		// methods
		LOG_LINE;
		key_obj->set_member("addListener", &key_add_listener);
		LOG_LINE;
		key_obj->set_member("getAscii", &key_get_ascii);
		LOG_LINE;
		key_obj->set_member("getCode", &key_get_code);
		LOG_LINE;
		key_obj->set_member("isDown", &key_is_down);
		LOG_LINE;
		key_obj->set_member("isToggled", &key_is_toggled);
		LOG_LINE;
		key_obj->set_member("removeListener", &key_remove_listener);

		LOG_LINE;
		s_global->set_member("Key", key_obj);
	}

	unsigned char	key_to_swf_code( key::code key_code )
	{
		for ( int i = 0; i < s_num_key_maps; ++i )
		{
			if ( s_key_remap_table[ ( i * 2 ) + 0 ] == key_code )
			{
				return s_key_remap_table[ ( i * 2 ) + 1 ];
			}
		}

		if ( key_code >= key::A && key_code <= key::Z )
		{
			return ( 'a' + ( key_code - key::A ) );
		}

		return 0;
	}

	void	notify_key_event(key::code k, bool down)
	// External interface for the host to report key events.
	{
		LOG_LINE;
		action_init();	// @@ put this in some global init somewhere else...

		LOG_LINE;
		static tu_string	key_obj_name("Key");

		LOG_LINE;
		as_value	kval;
		LOG_LINE;
		s_global->get_member(key_obj_name, &kval);
		LOG_LINE;
		if (kval.get_type() == as_value::OBJECT)
		{
			LOG_LINE;
			key_as_object*	ko = (key_as_object*) kval.to_object();
			QASSERT(ko);

			TRACE( "notify_key_event - %d, %d\n", k, down );
			LOG_LINE;
			if (down) ko->set_key_down(k);
			else ko->set_key_up(k);
			LOG_LINE;
		}
		else
		{
			LOG_LINE;
			log_error("gameswf::notify_key_event(): no Key built-in\n");
		}
	}
	

	//
	// global init
	//


	void	as_global_trace(const fn_call& fn)
	{
		LOG_LINE;
		QASSERT(fn.nargs >= 1);

		// Special case for objects: try the toString() method.
		LOG_LINE;
		if (fn.arg(0).get_type() == as_value::OBJECT)
		{
			LOG_LINE;
			as_object_interface* obj = fn.arg(0).to_object();
			QASSERT(obj);

			LOG_LINE;
			as_value method;
			LOG_LINE;
			if (obj->get_member("toString", &method)
			    && method.is_function())
			{
				LOG_LINE;
				as_value result = call_method0(method, fn.env, obj);
				LOG_LINE;
				log_msg("%s\n", result.to_string());

				return;
			}
		}

		// Log our argument.
		//
		// @@ what if we get extra args?
		//
		// @@ Array gets special treatment.
		LOG_LINE;
		const char* arg0 = fn.arg(0).to_string();
		LOG_LINE;
		log_msg("%s\n", arg0);
	}


	void	as_global_sound_ctor(const fn_call& fn)
	// Constructor for ActionScript class Sound.
	{
		LOG_LINE;
		smart_ptr<as_object>	sound_obj(new sound_as_object);

		// methods
		LOG_LINE;
		sound_obj->set_member("attachSound", &sound_attach);
		LOG_LINE;
		sound_obj->set_member("start", &sound_start);
		LOG_LINE;
		sound_obj->set_member("stop", &sound_stop);

		LOG_LINE;
		fn.result->set_as_object_interface(sound_obj.get_ptr());
	}


	void	as_global_object_ctor(const fn_call& fn)
	// Constructor for ActionScript class Object.
	{
		LOG_LINE;
		fn.result->set_as_object_interface(new as_object);
	}

	void	as_global_array_ctor(const fn_call& fn)
	// Constructor for ActionScript class Array.
	{
		LOG_LINE;
		smart_ptr<as_array_object>	ao = new as_array_object;

		LOG_LINE;
		if (fn.nargs == 0)
		{
			// Empty array.
		}
		else if (fn.nargs == 1
			 && fn.arg(0).get_type() == as_value::NUMBER)
		{
			// Create an empty array with the given number of undefined elements.
			//
			// @@ TODO set length property; no need to
			// actually create the elements now, since
			// they're undefined.
			LOG_LINE;
		}
		else
		{
			// Use the arguments as initializers.
			as_value	index_number;
			LOG_LINE;
			for (int i = 0; i < fn.nargs; i++)
			{
				LOG_LINE;
				index_number.set_int(i);
				LOG_LINE;
				ao->set_member(index_number.to_string(), fn.arg(i));
			}

			// @@ TODO set length property
		}

		LOG_LINE;
		fn.result->set_as_object_interface(ao.get_ptr());
	}


	void	as_global_assetpropflags(const fn_call& fn)
	// ASSetPropFlags function
	{
		LOG_LINE;
		const int version = fn.env->get_target()->get_movie_definition()->get_version();

		// Check the arguments
		QASSERT(fn.nargs == 3 || fn.nargs == 4);
		QASSERT((version == 5) ? (fn.nargs == 3) : true);

		// object
		LOG_LINE;
		as_object_interface* const obj = fn.arg(0).to_object();
		LOG_LINE;
		QASSERT(obj != NULL);

		// list of child names
		LOG_LINE;
		as_object_interface* props = fn.arg(1).to_object();
		LOG_LINE;
		if (props == NULL) {
			// tulrich: this fires in test_ASSetPropFlags -- is it correct?
			LOG_LINE;
			QASSERT(fn.arg(1).get_type() == as_value::NULLTYPE);
		}

		// a number which represents three bitwise flags which
		// are used to determine whether the list of child names should be hidden,
		// un-hidden, protected from over-write, un-protected from over-write,
		// protected from deletion and un-protected from deletion
		LOG_LINE;
		int set_true = int(fn.arg(2).to_number()) & as_prop_flags::as_prop_flags_mask;

		// Is another integer bitmask that works like set_true,
		// except it sets the attributes to false. The
		// set_false bitmask is applied before set_true is applied

		// ASSetPropFlags was exposed in Flash 5, however the fourth argument 'set_false'
		// was not required as it always defaulted to the value '~0'. 
		LOG_LINE;
		int set_false = (fn.nargs == 3 ? 
				 (version == 5 ? ~0 : 0) : int(fn.arg(3).to_number()))
			& as_prop_flags::as_prop_flags_mask;

		LOG_LINE;
		// Evan: it seems that if set_true == 0 and set_false == 0, this function
		// acts as if the parameters where (object, null, 0x1, 0) ...
		if (set_false == 0 && set_true == 0)
		{
			LOG_LINE;
			props = NULL;
			LOG_LINE;
			set_false = 0;
			LOG_LINE;
			set_true = 0x1;
		}

		if (props == NULL)
		{
			// Take all the members of the object

			LOG_LINE;
			as_object* object = (as_object*) obj;

			LOG_LINE;
			for ( s32 i = 0; i < object->m_members.size(); ++i )
			{
				LOG_LINE;
				as_prop_flags f = object->m_members[ i ].value.get_member_flags();
				LOG_LINE;
				int oldflags = f.get_flags();
				LOG_LINE;
				int newflags = f.set_flags(set_true, set_false);
				LOG_LINE;
				object->m_members[ i ].value.set_member_flags(f);

				UNUSED( oldflags );
				UNUSED( newflags );
			}

			LOG_LINE;
			if (object->m_prototype != NULL)
			{
				LOG_LINE;
				const as_object* prototype = (as_object*) object->m_prototype;

				LOG_LINE;
				for ( s32 i = 0; i < prototype->m_members.size(); ++i )
				{
					LOG_LINE;
					as_prop_flags f = prototype->m_members[ i ].value.get_member_flags();
					LOG_LINE;
					int oldflags = f.get_flags();
					LOG_LINE;
					int newflags = f.set_flags(set_true, set_false);
					LOG_LINE;
					object->m_members[ i ].value.set_member_flags(f);
					LOG_LINE;

					UNUSED( oldflags );
					UNUSED( newflags );
				}
			}
		}
		else
		{
			BREAK_POINT( "NEED TO IMPLMENENT!!" );
/*			as_object* object = (as_object*) obj;
			as_object* object_props = (as_object*) props;

			stringi_hash<as_member>::iterator it = object_props->m_members.begin();
			while(it != object_props->m_members.end())
			{
				const tu_stringi key = (it.get_value()).get_member_value().to_string();
				stringi_hash<as_member>::iterator it2 = object->m_members.find(key);

				if (it2 != object->m_members.end())
				{
					as_member member = it2.get_value();

					as_prop_flags f = member.get_member_flags();
					const int oldflags = f.get_flags();
					const int newflags = f.set_flags(set_true, set_false);
					member.set_member_flags(f);

					object->m_members.set((it.get_value()).get_member_value().to_string(), member);
				}

				++it;
			}*/
		}
	}


	void	action_init()
	// Create/hook built-ins.
	{
		LOG_LINE;
		if (s_inited == false)
		{
			LOG_LINE;
			s_inited = true;

			// @@ s_global should really be a
			// client-visible player object, which
			// contains one or more actual movie
			// instances.  We're currently just hacking it
			// in as an app-global mutable object :(
			LOG_LINE;
			QASSERT(s_global == NULL);
			LOG_LINE;
			s_global = new as_object;
			LOG_LINE;
			s_global->set_member("trace", as_value(as_global_trace));
			LOG_LINE;
			s_global->set_member("Object", as_value(as_global_object_ctor));
			LOG_LINE;
			s_global->set_member("Sound", as_value(as_global_sound_ctor));
			LOG_LINE;
			s_global->set_member("Array", as_value(as_global_array_ctor));

			LOG_LINE;
			s_global->set_member("TextFormat", as_value(textformat_new));
#ifdef HAVE_LIBXML
			LOG_LINE;
			s_global->set_member("XML", as_value(xml_new));
			//s_global->set_member("XML", as_value(xmlsocket_xml_new));
			LOG_LINE;
			s_global->set_member("XMLSocket", as_value(xmlsocket_new));
#endif // HAVE_LIBXML
			LOG_LINE;
			s_global->set_member("MovieClipLoader", as_value(moviecliploader_new));
			LOG_LINE;
			s_global->set_member("String", as_value(string_ctor));

			// ASSetPropFlags
			LOG_LINE;
			s_global->set_member("ASSetPropFlags", as_global_assetpropflags);

			LOG_LINE;
			math_init();
			LOG_LINE;
			key_init();
		}
	}


	void	action_clear()
	{
		LOG_LINE;
		if (s_inited)
		{
			LOG_LINE;
			s_inited = false;

			LOG_LINE;
			s_global->clear();
			LOG_LINE;
			s_global = NULL;
		}
	}


	//
	// properties by number
	//

	static const tu_string	s_property_names[] =
	{
		tu_string("_x"),
		tu_string("_y"),
		tu_string("_xscale"),
		tu_string("_yscale"),
		tu_string("_currentframe"),
		tu_string("_totalframes"),
		tu_string("_alpha"),
		tu_string("_visible"),
		tu_string("_width"),
		tu_string("_height"),
		tu_string("_rotation"),
		tu_string("_target"),
		tu_string("_framesloaded"),
		tu_string("_name"),
		tu_string("_droptarget"),
		tu_string("_url"),
		tu_string("_highquality"),
		tu_string("_focusrect"),
		tu_string("_soundbuftime"),
		tu_string("@@ mystery quality member"),
		tu_string("_xmouse"),
		tu_string("_ymouse"),
		tu_string("_7_1_M_code"),
	};


	static as_value	get_property(as_object_interface* obj, int prop_number)
	{
		LOG_LINE;
		as_value	val;
		LOG_LINE;
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			LOG_LINE;
			obj->get_member(s_property_names[prop_number], &val);
		}
		else
		{
			LOG_LINE;
			log_error("error: invalid property query, property number %d\n", prop_number);
		}
		LOG_LINE;
		return val;
	}

	static void	set_property(as_object_interface* obj, int prop_number, const as_value& val)
	{
		LOG_LINE;
		if (prop_number >= 0 && prop_number < int(sizeof(s_property_names)/sizeof(s_property_names[0])))
		{
			LOG_LINE;
			obj->set_member(s_property_names[prop_number], val);
		}
		else
		{
			LOG_LINE;
			log_error("error: invalid set_property, property number %d\n", prop_number);
		}
		LOG_LINE;
	}


	//
	// do_action
	//


	// Thin wrapper around action_buffer.
	struct do_action : public execute_tag
	{
		action_buffer	m_buf;

		void	read(stream* in)
		{
			LOG_LINE;
			m_buf.read(in);
		}

		virtual void	execute(movie* m)
		{
			LOG_LINE;
			m->add_action_buffer(&m_buf);
		}

		// Don't override because actions should not be replayed when seeking the movie.
		//void	execute_state(movie* m) {}

		virtual bool	is_action_tag() const
		// Tell the caller that we are an action tag.
		{
			LOG_LINE;
			return true;
		}
	};

	void	do_action_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("tag %d: do_action_loader\n", tag_type));

		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("-- actions in frame %d\n", m->get_loading_frame()));

		LOG_LINE;
		QASSERT(in);
		LOG_LINE;
		QASSERT(tag_type == 12);
		LOG_LINE;
		QASSERT(m);
		
		LOG_LINE;
		do_action*	da = new do_action;
		LOG_LINE;
		da->read(in);

		LOG_LINE;
		m->add_execute_tag(da);
	}

	
	//
	// do_init_action
	//


	void	do_init_action_loader(stream* in, int tag_type, movie_definition_sub* m)
	{
		LOG_LINE;
		QASSERT(tag_type == 59);

		LOG_LINE;
		int	sprite_character_id = in->read_u16();

		LOG_LINE;
		IF_VERBOSE_PARSE(log_msg("  tag %d: do_init_action_loader\n", tag_type));
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("  -- init actions for sprite %d\n", sprite_character_id));

		LOG_LINE;
		do_action*	da = new do_action;
		LOG_LINE;
		da->read(in);
		LOG_LINE;
		m->add_init_action(sprite_character_id, da);
	}


	//
	// action_buffer
	//

	// Disassemble one instruction to the log.
	static void	log_disasm(const unsigned char* instruction_data);

	action_buffer::action_buffer()
		:
		m_decl_dict_processed_at(-1)
	{
		LOG_LINE;
	}


	void	action_buffer::read(stream* in)
	{
		// Read action bytes.
		for (;;)
		{
			int	instruction_start = m_buffer.size();

			int	pc = m_buffer.size();

			int	action_id = in->read_u8();
			m_buffer.push_back(action_id);

			if (action_id & 0x80)
			{
				// Action contains extra data.  Read it.
				int	length = in->read_u16();
				m_buffer.push_back(length & 0x0FF);
				m_buffer.push_back((length >> 8) & 0x0FF);
				for (int i = 0; i < length; i++)
				{
					unsigned char	b = in->read_u8();
					m_buffer.push_back(b);
				}
			}

			IF_VERBOSE_ACTION(log_msg("%4d\t", pc); log_disasm(&m_buffer[instruction_start]); );

			if (action_id == 0)
			{
				// end of action buffer.
				break;
			}
		}
	}


	void	action_buffer::process_decl_dict(int start_pc, int stop_pc)
	// Interpret the decl_dict opcode.  Don't read stop_pc or
	// later.  A dictionary is some static strings embedded in the
	// action buffer; there should only be one dictionary per
	// action buffer.
	//
	// NOTE: Normally the dictionary is declared as the first
	// action in an action buffer, but I've seen what looks like
	// some form of copy protection that amounts to:
	//
	// <start of action buffer>
	//          push true
	//          branch_if_true label
	//          decl_dict   [0]   // this is never executed, but has lots of orphan data declared in the opcode
	// label:   // (embedded inside the previous opcode; looks like an invalid jump)
	//          ... "protected" code here, including the real decl_dict opcode ...
	//          <end of the dummy decl_dict [0] opcode>
	//
	// So we just interpret the first decl_dict we come to, and
	// cache the results.  If we ever hit a different decl_dict in
	// the same action_buffer, then we log an error and ignore it.
	{
		QASSERT(stop_pc <= m_buffer.size());

		LOG_LINE;
		if (m_decl_dict_processed_at == start_pc)
		{
			// We've already processed this decl_dict.
			LOG_LINE;
			int	count = m_buffer[start_pc + 3] | (m_buffer[start_pc + 4] << 8);
			LOG_LINE;
			QASSERT(m_dictionary.size() == count);
			LOG_LINE;
			UNUSED(count);
			LOG_LINE;
			return;
		}

		LOG_LINE;
		if (m_decl_dict_processed_at != -1)
		{
			LOG_LINE;
			log_error("error: process_decl_dict(%d, %d): decl_dict was already processed at %d\n",
				  start_pc,
				  stop_pc,
				  m_decl_dict_processed_at);
			LOG_LINE;
			return;
		}

		LOG_LINE;
		m_decl_dict_processed_at = start_pc;

		// Actual processing.
		LOG_LINE;
		int	i = start_pc;
		LOG_LINE;
		int	length = m_buffer[i + 1] | (m_buffer[i + 2] << 8);
		LOG_LINE;
		int	count = m_buffer[i + 3] | (m_buffer[i + 4] << 8);
		LOG_LINE;
		i += 2;

		LOG_LINE;
		UNUSED(length);

		LOG_LINE;
		QASSERT(start_pc + 3 + length == stop_pc);

		LOG_LINE;
		m_dictionary.resize(count);

		// Index the strings.
		LOG_LINE;
		for (int ct = 0; ct < count; ct++)
		{
			// Point into the current action buffer.
			LOG_LINE;
			m_dictionary[ct] = (const char*) &m_buffer[3 + i];

			LOG_LINE;
			while (m_buffer[3 + i])
			{
				// safety check.
				LOG_LINE;
				if (i >= stop_pc)
				{
					LOG_LINE;
					log_error("error: action buffer dict length exceeded\n");

					// Jam something into the remaining (invalid) entries.
					LOG_LINE;
					while (ct < count)
					{
						LOG_LINE;
						m_dictionary[ct] = "<invalid>";
						LOG_LINE;
						ct++;
					}
					return;
				}
				LOG_LINE;
				i++;
			}
			LOG_LINE;
			i++;
		}
		LOG_LINE;
	}


	void	action_buffer::execute(as_environment* env)
	// Interpret the actions in this action buffer, and evaluate
	// them in the given environment.  Execute our whole buffer,
	// without any arguments passed in.
	{
		LOG_LINE;
		int	local_stack_top = env->get_local_frame_top();
		LOG_LINE;
		env->add_frame_barrier();

		LOG_LINE;
		array<with_stack_entry>	empty_with_stack;
		LOG_LINE;
		execute(env, 0, m_buffer.size(), NULL, empty_with_stack, false /* not function2 */);

		LOG_LINE;
		env->set_local_frame_top(local_stack_top);
	}


	void	action_buffer::execute(
		as_environment* env,
		int start_pc,
		int exec_bytes,
		as_value* retval,
		const array<with_stack_entry>& initial_with_stack,
		bool is_function2)
	// Interpret the specified subset of the actions in our
	// buffer.  Caller is responsible for cleaning up our local
	// stack frame (it may have passed its arguments in via the
	// local stack frame).
	// 
	// The is_function2 flag determines whether to use global or local registers.
	{
		LOG_LINE;
		action_init();	// @@ stick this somewhere else; need some global static init function

		LOG_LINE;
		QASSERT(env);

		LOG_LINE;
		array<with_stack_entry>	with_stack(initial_with_stack);

		// Some corner case behaviors depend on the SWF file version.
		LOG_LINE;
		int version = env->get_target()->get_movie_definition()->get_version();

#if 0
		// Check the time
		LOG_LINE;
		if (periodic_events.expired()) {
			LOG_LINE;
			periodic_events.poll_event_handlers(env);
		}
#endif
		
		LOG_LINE;
		movie*	original_target = env->get_target();
		LOG_LINE;
		UNUSED(original_target);		// Avoid warnings.

		LOG_LINE;
		int	stop_pc = start_pc + exec_bytes;

		LOG_LINE;
		for (int pc = start_pc; pc < stop_pc; )
		{
			// Cleanup any expired "with" blocks.
			LOG_LINE;
			while (with_stack.size() > 0
			       && pc >= with_stack.back().m_block_end_pc)
			{
				// Drop this stack element
				LOG_LINE;
				with_stack.resize(with_stack.size() - 1);
			}

			// Get the opcode.
			LOG_LINE;
			int	action_id = m_buffer[pc];
			LOG_LINE;
			if ((action_id & 0x80) == 0)
			{
				LOG_LINE;
				IF_VERBOSE_ACTION(log_msg("EX:\t"); log_disasm(&m_buffer[pc]));

				// IF_VERBOSE_ACTION(log_msg("Action ID is: 0x%x\n", action_id));
			
				// Simple action; no extra data.
				LOG_LINE;
				switch (action_id)
				{
				default:
					break;

				case 0x00:	// end of actions.
					return;

				case 0x04:	// next frame.
					{
						LOG_LINE;
						env->get_target()->goto_frame(env->get_target()->get_current_frame() + 1);
					}
					break;

				case 0x05:	// prev frame.
					{
						LOG_LINE;
						env->get_target()->goto_frame(env->get_target()->get_current_frame() - 1);
					}
					break;

				case 0x06:	// action play
					{
						LOG_LINE;
						env->get_target()->set_play_state(movie::PLAY);
					}
					break;

				case 0x07:	// action stop
					{
						LOG_LINE;
						env->get_target()->set_play_state(movie::STOP);
					}
					break;

				case 0x08:	// toggle quality
				case 0x09:	// stop sounds
					break;

				case 0x0A:	// add
				{
					LOG_LINE;
					env->top(1) += env->top(0);
					env->drop(1);
					break;
				}
				case 0x0B:	// subtract
				{
					LOG_LINE;
					env->top(1) -= env->top(0);
					env->drop(1);
					break;
				}
				case 0x0C:	// multiply
				{
					LOG_LINE;
					env->top(1) *= env->top(0);
					env->drop(1);
					break;
				}
				case 0x0D:	// divide
				{
					LOG_LINE;
					env->top(1) /= env->top(0);
					env->drop(1);
					break;
				}
				case 0x0E:	// equal
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1) == env->top(0));
					env->drop(1);
					break;
				}
				case 0x0F:	// less than
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1) < env->top(0));
					env->drop(1);
					break;
				}
				case 0x10:	// logical and
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1).to_bool() && env->top(0).to_bool());
					env->drop(1);
					break;
				}
				case 0x11:	// logical or
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1).to_bool() && env->top(0).to_bool());
					env->drop(1);
					break;
				}
				case 0x12:	// logical not
				{
					LOG_LINE;
					env->top(0).set_bool(! env->top(0).to_bool());
					break;
				}
				case 0x13:	// string equal
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1).to_tu_string() == env->top(0).to_tu_string());
					env->drop(1);
					break;
				}
				case 0x14:	// string length
				{
					LOG_LINE;
					env->top(0).set_int(env->top(0).to_tu_string_versioned(version).utf8_length());
					break;
				}
				case 0x15:	// substring
				{
					LOG_LINE;
					int	size = int(env->top(0).to_number());
					LOG_LINE;
					int	base = int(env->top(1).to_number()) - 1;  // 1-based indices
					LOG_LINE;
					const tu_string&	str = env->top(2).to_tu_string_versioned(version);

					LOG_LINE;
					// Keep base within range.
					base = iclamp(base, 0, str.length());

					// Truncate if necessary.
					LOG_LINE;
					size = imin(str.length() - base, size);

					// @@ This can be done without new allocations if we get dirtier w/ internals
					// of as_value and tu_string...
					LOG_LINE;
					tu_string	new_string = str.c_str() + base;
					LOG_LINE;
					new_string.resize(size);

					LOG_LINE;
					env->drop(2);
					LOG_LINE;
					env->top(0).set_tu_string(new_string);

					break;
				}
				case 0x17:	// pop
				{
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x18:	// int
				{
					LOG_LINE;
					env->top(0).set_int(int(floor(env->top(0).to_number())));
					break;
				}
				case 0x1C:	// get variable
				{
					LOG_LINE;
					as_value var_name = env->pop();
					LOG_LINE;
					tu_string var_string = var_name.to_tu_string();

					LOG_LINE;
					as_value variable = env->get_variable(var_string, with_stack);
					LOG_LINE;
					env->push(variable);
					LOG_LINE;
					if (variable.to_object() == NULL)
					{
						LOG_LINE;
						IF_VERBOSE_ACTION(log_msg("-- get var: %s=%s\n",
									  var_string.c_str(),
									  variable.to_tu_string().c_str()));
					} else {
						LOG_LINE;
						IF_VERBOSE_ACTION(log_msg("-- get var: %s=%s at %p\n",
									  var_string.c_str(),
									  variable.to_tu_string().c_str(), variable.to_object()));
					}
					

					break;
				}
				case 0x1D:	// set variable
				{
					LOG_LINE;
					env->set_variable(env->top(1).to_tu_string(), env->top(0), with_stack);
					LOG_LINE;
					IF_VERBOSE_ACTION(log_msg("-- set var: %s \n",
								  env->top(1).to_tu_string().c_str()));

					LOG_LINE;
					env->drop(2);
					break;
				}
				case 0x20:	// set target expression
				{
					LOG_LINE;
					as_object_interface* target_object = env->top(0).to_object();

					LOG_LINE;
					IF_VERBOSE_ACTION(log_msg("-- ActionSetTarget2: %s (%d)",
								  ((character *) target_object)->m_name.c_str(),
								  ((character *) target_object)->m_id));

					LOG_LINE;
					movie* target = env->find_target(target_object);
					LOG_LINE;
					env->set_target (target);
					LOG_LINE;
					break;
				}
				case 0x21:	// string concat
				{
					LOG_LINE;
					env->top(1).convert_to_string_versioned(version);
					LOG_LINE;
					env->top(1).string_concat(env->top(0).to_tu_string_versioned(version));
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x22:	// get property
				{
					LOG_LINE;
					movie*	target = env->find_target(env->top(1));
					LOG_LINE;
					if (target)
					{
						LOG_LINE;
						env->top(1) = get_property(target, (int) env->top(0).to_number());
					}
					else
					{
						LOG_LINE;
						env->top(1) = as_value();
					}
					LOG_LINE;
					env->drop(1);
					break;
				}

				case 0x23:	// set property
				{
					LOG_LINE;
					movie*	target = env->find_target(env->top(2));
					LOG_LINE;
					if (target)
					{
						LOG_LINE;
						set_property(target, (int) env->top(1).to_number(), env->top(0));
					}
					LOG_LINE;
					env->drop(3);
					break;
				}

				case 0x24:	// duplicate clip (sprite?)
				{
					LOG_LINE;
					env->get_target()->clone_display_object(
						env->top(2).to_tu_string(),
						env->top(1).to_tu_string(),
						(int) env->top(0).to_number());
					LOG_LINE;
					env->drop(3);
					break;
				}

				case 0x25:	// remove clip
					LOG_LINE;
					env->get_target()->remove_display_object(env->top(0).to_tu_string());
					LOG_LINE;
					env->drop(1);
					break;

				case 0x26:	// trace
				{
					// Log the stack val.
					LOG_LINE;
					as_global_trace(fn_call(&env->top(0), NULL, env, 1, env->get_top_index()));
					LOG_LINE;
					env->drop(1);
					break;
				}

				case 0x27:	// start drag movie
				{
					movie::drag_state	st;

					LOG_LINE;
					st.m_character = env->find_target(env->top(0));
					LOG_LINE;
					if (st.m_character == NULL)
					{
						LOG_LINE;
						log_error("error: start_drag of invalid target '%s'.\n",
							  env->top(0).to_string());
					}

					LOG_LINE;
					st.m_lock_center = env->top(1).to_bool();
					LOG_LINE;
					st.m_bound = env->top(2).to_bool();
					LOG_LINE;
					if (st.m_bound)
					{
						LOG_LINE;
						st.m_bound_x0 = (float) env->top(6).to_number();
						LOG_LINE;
						st.m_bound_y0 = (float) env->top(5).to_number();
						LOG_LINE;
						st.m_bound_x1 = (float) env->top(4).to_number();
						LOG_LINE;
						st.m_bound_y1 = (float) env->top(3).to_number();
						LOG_LINE;
						env->drop(4);
					}
					LOG_LINE;
					env->drop(3);

					LOG_LINE;
					movie*	root_movie = env->get_target()->get_root_movie();
					LOG_LINE;
					QASSERT(root_movie);

					LOG_LINE;
					if (root_movie && st.m_character)
					{
						LOG_LINE;
						root_movie->set_drag_state(st);
					}
					
					break;
				}

				case 0x28:	// stop drag movie
				{
					LOG_LINE;
					movie*	root_movie = env->get_target()->get_root_movie();
					QASSERT(root_movie);

					LOG_LINE;
					root_movie->stop_drag();

					break;
				}

				case 0x29:	// string less than
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1).to_tu_string() < env->top(0).to_tu_string());
					break;
				}

				case 0x2A:	// throw
				{
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x2B:	// cast_object
				{
					// TODO
					//
					// Pop o1, pop s2
					// Make sure o1 is an instance of s2.
					// If the cast succeeds, push o1, else push NULL.
					//
					// The cast doesn't appear to coerce at all, it's more
					// like a dynamic_cast<> in C++ I think.
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x2C:	// implements
				{
					// Declare that a class s1 implements one or more
					// interfaces (i2 == number of interfaces, s3..sn are the names
					// of the interfaces).
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x30:	// random
				{
					LOG_LINE;
					int	max = int(env->top(0).to_number());
					LOG_LINE;
					if (max < 1) max = 1;
					LOG_LINE;
					env->top(0).set_int(tu_random::next_random() % max);
					break;
				}
				case 0x31:	// mb length
				{
					// @@ TODO
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x32:	// ord
				{
					// ASCII code of first character
					LOG_LINE;
					env->top(0).set_int(env->top(0).to_string()[0]);
					break;
				}
				case 0x33:	// chr
				{
					char	buf[2];
					LOG_LINE;
					buf[0] = int(env->top(0).to_number());
					LOG_LINE;
					buf[1] = 0;
					LOG_LINE;
					env->top(0).set_string(buf);
					break;
				}

				case 0x34:	// get timer
					// Push milliseconds since we started playing.
					LOG_LINE;
					env->push(floorf(env->m_target->get_timer() * 1000.0f));
					break;

				case 0x35:	// mb substring
				{
					// @@ TODO
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x37:	// mb chr
				{
					// @@ TODO
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x3A:	// delete
				{
					// @@ TODO
					
					// Apparently this can be used to remove properties from
					// an object?

					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x3B:	// delete2
				{
					// @@ tulrich: delete is not valid here!  Do we actually just want to 
					// NULL out the object pointer in the environment (to drop the ref)?
					// Should at least check the ref count before deleting anything!!!
//					as_value	obj_name = env->pop();
					LOG_LINE;
					as_value obj_ptr = env->get_variable_raw(env->top(0).to_tu_string(), with_stack);
///x					delete obj_ptr.to_object();
// 	 				log_error("%08X\n", obj_ptr.to_object());
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}

				case 0x3C:	// set local
				{
					LOG_LINE;
					as_value	value = env->pop();
					LOG_LINE;
					as_value	varname = env->pop();
					LOG_LINE;
					env->set_local(varname.to_tu_string(), value);
					break;
				}

				case 0x3D:	// call function
				{
					LOG_LINE;
					as_value	function;
					LOG_LINE;
					if (env->top(0).get_type() == as_value::STRING)
					{
						// Function is a string; lookup the function.
						LOG_LINE;
						const tu_string&	function_name = env->top(0).to_tu_string();
						LOG_LINE;
						function = env->get_variable(function_name, with_stack);

						LOG_LINE;
						if (function.get_type() != as_value::C_FUNCTION
						    && function.get_type() != as_value::AS_FUNCTION)
						{
							LOG_LINE;
							log_error("error in call_function: '%s' is not a function\n",
								  function_name.c_str());
						}
					}
					else
					{
						// Hopefully the actual function object is here.
						LOG_LINE;
						function = env->top(0);
					}
					LOG_LINE;
					int	nargs = (int) env->top(1).to_number();
					LOG_LINE;
					as_value	result = call_method(function, env, NULL, nargs, env->get_top_index() - 2);
					LOG_LINE;
					env->drop(nargs + 1);
					LOG_LINE;
					env->top(0) = result;
					break;
				}
				case 0x3E:	// return
				{
					// Put top of stack in the provided return slot, if
					// it's not NULL.
					LOG_LINE;
					if (retval)
					{
						LOG_LINE;
						*retval = env->top(0);
					}
					LOG_LINE;
					env->drop(1);

					// Skip the rest of this buffer (return from this action_buffer).
					LOG_LINE;
					pc = stop_pc;

					break;
				}
				case 0x3F:	// modulo
				{
					as_value	result;
					LOG_LINE;
					double	y = env->pop().to_number();
					LOG_LINE;
					double	x = env->pop().to_number();
					LOG_LINE;
					if (y != 0)
					{
//						env->top(1).set_double(fmod(env->top(1).to_bool() && env->top(0).to_bool());
//						env->drop(1);
						LOG_LINE;
						result = fmod(x, y);
					}
//					log_error("modulo x=%f, y=%f, z=%f\n",x,y,result.to_number());
					LOG_LINE;
					env->push(result);
					break;
				}
				case 0x40:	// new
				{
					LOG_LINE;
					as_value	classname = env->pop();
					LOG_LINE;
					IF_VERBOSE_ACTION(log_msg("---new object: %s\n",
								  classname.to_tu_string().c_str()));
					LOG_LINE;
					int	nargs = (int) env->pop().to_number();
					LOG_LINE;
					as_value constructor = env->get_variable(classname.to_tu_string(), with_stack);
					LOG_LINE;
					as_value new_obj;
					LOG_LINE;
					if (constructor.get_type() == as_value::C_FUNCTION)
					{
						// C function is responsible for creating the new object and setting members.
						LOG_LINE;
						(constructor.to_c_function())(fn_call(&new_obj, NULL, env, nargs, env->get_top_index()));
					}
					else if (as_as_function* ctor_as_func = constructor.to_as_function())
					{
						// This function is being used as a constructor; make sure
						// it has a prototype object.
						LOG_LINE;
						ctor_as_func->lazy_create_properties();
						LOG_LINE;
						QASSERT(ctor_as_func->m_properties);

						// Set up the prototype.
						as_value	proto;
						LOG_LINE;
						ctor_as_func->m_properties->get_member("prototype", &proto);

						LOG_LINE;
						QASSERT(proto.to_object() != NULL);

						// Create an empty object, with a ref to the constructor's prototype.
						LOG_LINE;
						smart_ptr<as_object>	new_obj_ptr(new as_object(proto.to_object()));

						// Set up the constructor member.
						LOG_LINE;
						new_obj_ptr->set_member("constructor", constructor);
						LOG_LINE;
						new_obj_ptr->set_member_flags("constructor", 1);
						LOG_LINE;
						
						new_obj.set_as_object_interface(new_obj_ptr.get_ptr());

						// Call the actual constructor function; new_obj is its 'this'.
						// We don't need the function result.
						LOG_LINE;
						call_method(constructor, env, new_obj_ptr.get_ptr(), nargs, env->get_top_index());
					}
					else
					{
						LOG_LINE;
						if (classname != "String") {
							log_error("can't create object with unknown class '%s'\n",
								  classname.to_tu_string().c_str());
						} else {
							log_msg("Created special String class\n");
						}
					}

					LOG_LINE;
					env->drop(nargs);
					LOG_LINE;
					env->push(new_obj);
#if 0
					log_msg("new object %s at %p\n", classname.to_tu_string().c_str(), new_obj);
#endif
					break;
				}
				case 0x41:	// declare local
				{
					LOG_LINE;
					const tu_string&	varname = env->top(0).to_tu_string();
					LOG_LINE;
					env->declare_local(varname);
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x42:	// init array
				{
//					LOG_LINE;
//					int	array_size = (int) env->pop().to_number();

					//log_msg("xxx init array: size = %d, top of stack = %d\n",
					//	// array_size, env->get_top_index());//xxxxx

// 					// Call the array constructor, to create an empty array.
// 					as_value	result;
// 					as_global_array_ctor(fn_call(&result, NULL, env, 0, env->get_top_index()));

// 					as_object_interface*	ao = result.to_object();
// 					QASSERT(ao);

// 					// @@ TODO Set array size.
// 					// ao->set_length(whatever); or something

// 					// Fill the elements with the initial values from the stack.
// 					as_value	index_number;
// 					for (int i = 0; i < array_size; i++)
// 					{
// 						// @@ TODO a set_member that takes an int or as_value?
// 						index_number.set_int(i);
// 						ao->set_member(index_number.to_string(), env->pop());
// 					}

// 					env->push(result);

// 					//log_msg("xxx init array end: top of stack = %d, trace(top(0)) =",
// 					//	env->get_top_index());//xxxxxxx

					LOG_LINE;
					as_global_trace(fn_call(NULL, NULL, env, 1, env->get_top_index()));	//xxxx

					break;
				}
				case 0x43:	// declare object
				{
					// @@ TODO
					LOG_LINE;
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x44:	// type of
				{
					LOG_LINE;
					switch(env->top(0).get_type())
					{
					case as_value::UNDEFINED:
						env->top(0).set_string("undefined");
						break;
					case as_value::STRING:
						env->top(0).set_string("string");
						break;
					case as_value::NUMBER:
						env->top(0).set_string("number");
						break;
					case as_value::BOOLEAN:
						env->top(0).set_string("boolean");
						break;
					case as_value::OBJECT:
						env->top(0).set_string("object");
						break;
					case as_value::NULLTYPE:
						env->top(0).set_string("null");
						break;
					case as_value::AS_FUNCTION:
					case as_value::C_FUNCTION:
						env->top(0).set_string("function");
						break;
					default:
						log_error("typeof unknown type: %02X\n", env->top(0).get_type());
						break;
					}
					break;
				}
				case 0x45:	// get target
				{
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				}
				case 0x46:	// enumerate
				{
					LOG_LINE;
					as_value var_name = env->pop();
					LOG_LINE;
					const tu_string& var_string = var_name.to_tu_string();

					LOG_LINE;
					as_value variable = env->get_variable(var_string, with_stack);

					LOG_LINE;
					if (variable.to_object() == NULL)
					{
						break;
					}
					LOG_LINE;
					const as_object* object = (as_object*) (variable.to_object());

					// The end of the enumeration
					LOG_LINE;
					as_value nullvalue;
					LOG_LINE;
					nullvalue.set_null();
					LOG_LINE;
					env->push(nullvalue);
					IF_VERBOSE_ACTION(log_msg("---enumerate - push: NULL\n"));

					LOG_LINE;
					for ( s32 i = 0; i < object->m_members.size(); ++i )
					{
						LOG_LINE;
						const as_member member = object->m_members[ i ].value;

						LOG_LINE;
						if (! member.get_member_flags().get_dont_enum())
						{
							LOG_LINE;
							env->push(as_value(object->m_members[ i ].name));

							LOG_LINE;
							IF_VERBOSE_ACTION(log_msg("---enumerate - push: %s\n",
										  object->m_members[ i ].name.c_str()));
						}
					}

					LOG_LINE;
					const as_object * prototype = (as_object *) object->m_prototype;
					LOG_LINE;
					if (prototype != NULL)
					{
						LOG_LINE;
						for ( s32 i = 0; i < prototype->m_members.size(); ++i )
						{
							LOG_LINE;
							const as_member member = object->m_members[ i ].value;

							LOG_LINE;
							if (! member.get_member_flags().get_dont_enum())
							{
								LOG_LINE;
								env->push(as_value(object->m_members[ i ].name));

								LOG_LINE;
								IF_VERBOSE_ACTION(log_msg("---enumerate - push: %s\n",
											  object->m_members[ i ].name.c_str()));
							}
						};
					}

					break;
				}
				case 0x47:	// add_t (typed)
				{
					LOG_LINE;
					if (env->top(0).get_type() == as_value::STRING
					    || env->top(1).get_type() == as_value::STRING)
					{
						LOG_LINE;
						env->top(1).convert_to_string_versioned(version);
						LOG_LINE;
						env->top(1).string_concat(env->top(0).to_tu_string_versioned(version));
					}
					else
					{
						LOG_LINE;
						env->top(1) += env->top(0);
					}
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x48:	// less than (typed)
				{
					LOG_LINE;
					if (env->top(1).get_type() == as_value::STRING)
					{
						LOG_LINE;
						env->top(1).set_bool(env->top(1).to_tu_string() < env->top(0).to_tu_string());
					}
					else
					{
						LOG_LINE;
						env->top(1).set_bool(env->top(1) < env->top(0));
					}
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x49:	// equal (typed)
				{
					// @@ identical to untyped equal, as far as I can tell...
					LOG_LINE;
					env->top(1).set_bool(env->top(1) == env->top(0));
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x4A:	// to number
				{
					LOG_LINE;
					env->top(0).convert_to_number();
					break;
				}
				case 0x4B:	// to string
				{
					LOG_LINE;
					env->top(0).convert_to_string_versioned(version);
					break;
				}
				case 0x4C:	// dup
				{
					LOG_LINE;
					env->push(env->top(0));
					break;
				}
				
				case 0x4D:	// swap
				{
					LOG_LINE;
					as_value	temp = env->top(1);
					LOG_LINE;
					env->top(1) = env->top(0);
					LOG_LINE;
					env->top(0) = temp;
					break;
				}
				case 0x4E:	// get member
				{
					LOG_LINE;
                                        as_object_interface*    obj = env->top(1).to_object();

                                        // Special case: String has a member "length"
										LOG_LINE;
                                        if (obj == NULL
                                            && env->top(1).get_type() == as_value::STRING
                                            && env->top(0).to_tu_stringi() == "length")
					{
						LOG_LINE;
						int     len = env->top(1).to_tu_string_versioned(version).utf8_length();
                                                env->top(1).set_int(len);
					}
                                        else
					{
						LOG_LINE;
						env->top(1).set_undefined();
						// int	nargs = (int) env->top(1).to_number();
						LOG_LINE;
						if (obj) {
							obj->get_member(env->top(0).to_tu_string(), &(env->top(1)));
							if (env->top(1).to_object() == NULL) {
								IF_VERBOSE_ACTION(log_msg("-- get_member %s=%s\n",
											  env->top(0).to_tu_string().c_str(),
											  env->top(1).to_tu_string().c_str()));
							} else {
								IF_VERBOSE_ACTION(log_msg("-- get_member %s=%s at %p\n",
											  env->top(0).to_tu_string().c_str(),
											  env->top(1).to_tu_string().c_str(), env->top(1).to_object()));
							}
						}
						else
						{
							// @@ log error?
						}
					}
					LOG_LINE;
                                        env->drop(1);
                                        break;
					
				}
				case 0x4F:	// set member
				{
					LOG_LINE;
					as_object_interface*	obj = env->top(2).to_object();
					if (obj)
					{
						LOG_LINE;
						obj->set_member(env->top(1).to_tu_string(), env->top(0));
						LOG_LINE;
						IF_VERBOSE_ACTION(
							log_msg("-- set_member %s.%s=%s\n",
								env->top(2).to_tu_string().c_str(),
								env->top(1).to_tu_string().c_str(),
								env->top(0).to_tu_string().c_str()));
					}
					else
					{
						// Invalid object, can't set.
						LOG_LINE;
						IF_VERBOSE_ACTION(
							log_error("-- set_member %s.%s=%s on invalid object!\n",
								env->top(2).to_tu_string().c_str(),
								env->top(1).to_tu_string().c_str(),
								env->top(0).to_tu_string().c_str()));
					}
					LOG_LINE;
					env->drop(3);
					break;
				}
				case 0x50:	// increment
				{
					LOG_LINE;
					env->top(0) += 1;
					break;
				}
				case 0x51:	// decrement
				{
					LOG_LINE;
					env->top(0) -= 1;
					break;
				}
				case 0x52:	// call method
				{
					LOG_LINE;
					int	nargs = (int) env->top(2).to_number();
					LOG_LINE;
					as_value	result;
					LOG_LINE;
					const tu_string&	method_name = env->top(0).to_tu_string();
					LOG_LINE;
					as_object_interface*	obj = env->top(1).to_object();
					LOG_LINE;
					if (obj)
					{
						LOG_LINE;
						as_value	method;
						LOG_LINE;
						if (obj->get_member(method_name, &method))
						{
							LOG_LINE;
							if (method.get_type() != as_value::C_FUNCTION
							    && method.get_type() != as_value::AS_FUNCTION)
							{
								LOG_LINE;
								log_error("error: call_method: '%s' is not a method\n",
									  method_name.c_str());
							}
							else
							{
								LOG_LINE;
								result = call_method(
									method,
									env,
									obj,
									nargs,
									env->get_top_index() - 3);
							}
						}
						else
						{
							LOG_LINE;
							log_error("error: call_method can't find method %s\n",
								  method_name.c_str());
						}
					}
					else if (env->top(1).get_type() == as_value::STRING)
					{
						LOG_LINE;
						// Handle methods on literal strings.
						string_method(
							fn_call(&result, NULL, env, nargs, env->get_top_index() - 3),
							method_name.to_tu_stringi(),
							env->top(1).to_tu_string_versioned(version));
					}
					else if (env->top(1).get_type() == as_value::C_FUNCTION)
					{
						// Catch method calls on String
						// constructor.  There may be a cleaner
						// way to do this. Perhaps we call the
						// constructor function with a special flag, to
						// indicate that it's a method call?
						LOG_LINE;
						if (env->top(1).to_c_function() == string_ctor)
						{
							LOG_LINE;
							tu_string dummy;
							LOG_LINE;
							string_method(
								fn_call(&result, NULL, env, nargs, env->get_top_index() - 3),
								method_name.to_tu_stringi(),
								dummy);
						}
						else
						{
							log_error("error: method call on unknown c function.\n");
						}
					}
					else
					{
						LOG_LINE;
						if (env->top(1).get_type() == as_value::NUMBER
						    && method_name == "toString")
						{
							// Numbers have a .toString() method.
							LOG_LINE;
							result.set_tu_string(env->top(1).to_tu_string());
						}
						else
						{
							LOG_LINE;
							log_error("error: call_method '%s' on invalid object.\n",
								  method_name.c_str());
						}
					}
					LOG_LINE;
					env->drop(nargs + 2);
					LOG_LINE;
					env->top(0) = result;
					break;
				}
				case 0x53:	// new method
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				case 0x54:	// instance of
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				case 0x55:	// enumerate object
					// @@ TODO
					log_error("todo opcode: %02X\n", action_id);
					break;
				case 0x60:	// bitwise and
				{
					LOG_LINE;
					env->top(1) &= env->top(0);
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x61:	// bitwise or
				{
					LOG_LINE;
					env->top(1) |= env->top(0);
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x62:	// bitwise xor
				{
					LOG_LINE;
					env->top(1) ^= env->top(0);
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x63:	// shift left
				{
					LOG_LINE;
					env->top(1).shl(env->top(0));
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x64:	// shift right (signed)
				{
					LOG_LINE;
					env->top(1).asr(env->top(0));
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x65:	// shift right (unsigned)
				{
					LOG_LINE;
					env->top(1).lsr(env->top(0));
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x66:	// strict equal
				{
					LOG_LINE;
					if (env->top(1).get_type() != env->top(0).get_type())
					{
						// Types don't match.
						LOG_LINE;
						env->top(1).set_bool(false);
						LOG_LINE;
						env->drop(1);
					}
					else
					{
						LOG_LINE;
						env->top(1).set_bool(env->top(1) == env->top(0));
						LOG_LINE;
						env->drop(1);
					}
					break;
				}
				case 0x67:	// gt (typed)
				{
					LOG_LINE;
					if (env->top(1).get_type() == as_value::STRING)
					{
						LOG_LINE;
						env->top(1).set_bool(env->top(1).to_tu_string() > env->top(0).to_tu_string());
					}
					else
					{
						LOG_LINE;
						env->top(1).set_bool(env->top(1).to_number() > env->top(0).to_number());
					}
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x68:	// string gt
				{
					LOG_LINE;
					env->top(1).set_bool(env->top(1).to_tu_string() > env->top(0).to_tu_string());
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x69:	// extends
					log_error("todo opcode: %02X\n", action_id);
					break;

				}
				LOG_LINE;
				pc++;	// advance to next action.
			}
			else
			{
				IF_VERBOSE_ACTION(log_msg("EX:\t"); log_disasm(&m_buffer[pc]));

				// Action containing extra data.
				LOG_LINE;
				int	length = m_buffer[pc + 1] | (m_buffer[pc + 2] << 8);
				LOG_LINE;
				int	next_pc = pc + length + 3;

				LOG_LINE;
				switch (action_id)
				{
				default:
					break;

				case 0x81:	// goto frame
				{
					LOG_LINE;
					int	frame = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					// 0-based already?
					//// Convert from 1-based to 0-based
					//frame--;
					LOG_LINE;
					env->get_target()->goto_frame(frame);
					break;
				}

				case 0x83:	// get url
				{
					// If this is an FSCommand, then call the callback
					// handler, if any.

					// Two strings as args.
					LOG_LINE;
					const char*	url = (const char*) &(m_buffer[pc + 3]);
					LOG_LINE;
					int	url_len = strlen(url);
					LOG_LINE;
					const char*	target = (const char*) &(m_buffer[pc + 3 + url_len + 1]);

					// If the url starts with "FSCommand:", then this is
					// a message for the host app.
					LOG_LINE;
					if (strncmp(url, "FSCommand:", 10) == 0)
					{
						LOG_LINE;
						if (s_fscommand_handler)
						{
							// Call into the app.
							LOG_LINE;
							(*s_fscommand_handler)(env->get_target()->get_root_interface(), url + 10, target);
						}
					}
					else
					{
#ifdef EXTERN_MOVIE
//						log_error("get url: target=%s, url=%s\n", target, url);
            
						LOG_LINE;
						tu_string tu_target = target;
						LOG_LINE;
						movie* target_movie = env->find_target(tu_target);
						LOG_LINE;
						if (target_movie != NULL)
						{
							LOG_LINE;
							movie*	root_movie = env->get_target()->get_root_movie();
							LOG_LINE;
							attach_extern_movie(url, target_movie, root_movie);
						}
						else
						{
							LOG_LINE;
							log_error("get url: target %s not found\n", target);
						}
#endif // EXTERN_MOVIE
					}

					break;
				}

				case 0x87:	// store_register
				{
					LOG_LINE;
					int	reg = m_buffer[pc + 3];
					// Save top of stack in specified register.
					LOG_LINE;
					if (is_function2)
					{
						LOG_LINE;
						*(env->local_register_ptr(reg)) = env->top(0);

						IF_VERBOSE_ACTION(
							log_msg("-------------- local register[%d] = '%s'\n",
								reg,
								env->top(0).to_string()));
					}
					else if (reg >= 0 && reg < 4)
					{
						LOG_LINE;
						env->m_global_register[reg] = env->top(0);
					
						IF_VERBOSE_ACTION(
							log_msg("-------------- global register[%d] = '%s'\n",
								reg,
								env->top(0).to_string()));
					}
					else
					{
						log_error("store_register[%d] -- register out of bounds!", reg);
					}

					break;
				}

				case 0x88:	// decl_dict: declare dictionary
				{
					LOG_LINE;
					int	i = pc;
					//int	count = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					LOG_LINE;
					i += 2;

					LOG_LINE;
					process_decl_dict(pc, next_pc);

					break;
				}

				case 0x8A:	// wait for frame
				{
					// If we haven't loaded a specified frame yet, then we're supposed to skip
					// some specified number of actions.
					//
					// Since we don't load incrementally, just ignore this opcode.
					break;
				}

				case 0x8B:	// set target
				{
					// Change the movie we're working on.
					LOG_LINE;
					const char* target_name = (const char*) &m_buffer[pc + 3];
					LOG_LINE;
					if (target_name[0] == 0) { env->set_target(original_target); }
					else {
//						env->set_target(env->get_target()->find_labeled_target(target_name));
//						if (env->get_target() == NULL) env->set_target(original_target);
					}
					break;
				}

				case 0x8C:	// go to labeled frame, goto_frame_lbl
				{
					LOG_LINE;
					char*	frame_label = (char*) &m_buffer[pc + 3];
					LOG_LINE;
					env->get_target()->goto_labeled_frame(frame_label);
					break;
				}

				case 0x8D:	// wait for frame expression (?)
				{
					// Pop the frame number to wait for; if it's not loaded skip the
					// specified number of actions.
					//
					// Since we don't support incremental loading, pop our arg and
					// don't do anything.
					LOG_LINE;
					env->drop(1);
					break;
				}

				case 0x8E:	// function2
				{
					LOG_LINE;
					as_as_function*	func = new as_as_function(this, env, next_pc, with_stack);
					LOG_LINE;
					func->set_is_function2();

					LOG_LINE;
					int	i = pc;
					i += 3;

					// Extract name.
					// @@ security: watch out for possible missing terminator here!
					LOG_LINE;
					tu_string	name = (const char*) &m_buffer[i];
					i += name.length() + 1;

					// Get number of arguments.
					LOG_LINE;
					int	nargs = m_buffer[i] | (m_buffer[i + 1] << 8);
					i += 2;

					// Get the count of local registers used by this function.
					LOG_LINE;
					uint8	register_count = m_buffer[i];
					LOG_LINE;
					i += 1;
					LOG_LINE;
					func->set_local_register_count(register_count);

					// Flags, for controlling register assignment of implicit args.
					LOG_LINE;
					uint16	flags = m_buffer[i] | (m_buffer[i + 1] << 8);
					LOG_LINE;
					i += 2;
					LOG_LINE;
					func->set_function2_flags(flags);

					// Get the register assignments and names of the arguments.
					LOG_LINE;
					for (int n = 0; n < nargs; n++)
					{
						LOG_LINE;
						int	arg_register = m_buffer[i];
						LOG_LINE;
						i++;

						// @@ security: watch out for possible missing terminator here!
						LOG_LINE;
						func->add_arg(arg_register, (const char*) &m_buffer[i]);
						LOG_LINE;
						i += func->m_args.back().m_name.length() + 1;
					}

					// Get the length of the actual function code.
					LOG_LINE;
					int	length = m_buffer[i] | (m_buffer[i + 1] << 8);
					LOG_LINE;
					i += 2;
					LOG_LINE;
					func->set_length(length);

					// Skip the function body (don't interpret it now).
					LOG_LINE;
					next_pc += length;

					// If we have a name, then save the function in this
					// environment under that name.
					LOG_LINE;
					as_value	function_value(func);
					LOG_LINE;
					if (name.length() > 0)
					{
						// @@ NOTE: should this be m_target->set_variable()???
						LOG_LINE;
						env->set_member(name, function_value);
					}

					// Also leave it on the stack.
					LOG_LINE;
					env->push_val(function_value);

					break;
				}

				case 0x94:	// with
				{
					LOG_LINE;
					int	frame = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					LOG_LINE;
					UNUSED(frame);
					IF_VERBOSE_ACTION(log_msg("-------------- with block start: stack size is %d\n", with_stack.size()));
					LOG_LINE;
					if (with_stack.size() < 8)
					{
						LOG_LINE;
 						int	block_length = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
						LOG_LINE;
 						int	block_end = next_pc + block_length;
						LOG_LINE;
 						as_object_interface*	with_obj = env->top(0).to_object();
						LOG_LINE;
 						with_stack.push_back(with_stack_entry(with_obj, block_end));
					}
					LOG_LINE;
					env->drop(1);
					break;
				}
				case 0x96:	// push_data
				{
					LOG_LINE;
					int i = pc;
					LOG_LINE;
					while (i - pc < length)
					{
						LOG_LINE;
						int	type = m_buffer[3 + i];
						LOG_LINE;
						i++;
						LOG_LINE;
						if (type == 0)
						{
							// string
							LOG_LINE;
							const char*	str = (const char*) &m_buffer[3 + i];
							LOG_LINE;
							i += strlen(str) + 1;
							LOG_LINE;
							env->push(str);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed '%s'\n", str));
						}
						else if (type == 1)
						{
							// float (little-endian)
							LOG_LINE;
							union {
								float	f;
								Uint32	i;
							} u;
							compiler_assert(sizeof(u) == sizeof(u.i));
							LOG_LINE;

							memcpy(&u.i, &m_buffer[3 + i], 4);
							LOG_LINE;
							u.i = swap_le32(u.i);
							LOG_LINE;
							i += 4;

							LOG_LINE;
							env->push(u.f);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed '%f'\n", u.f));
						}
						else if (type == 2)
						{
							LOG_LINE;
							as_value nullvalue;
							LOG_LINE;
							nullvalue.set_null();
							LOG_LINE;
							env->push(nullvalue);	

							IF_VERBOSE_ACTION(log_msg("-------------- pushed NULL\n"));
						}
						else if (type == 3)
						{
							LOG_LINE;
							env->push(as_value());

							IF_VERBOSE_ACTION(log_msg("-------------- pushed UNDEFINED\n"));
						}
						else if (type == 4)
						{
							// contents of register
							LOG_LINE;
							int	reg = m_buffer[3 + i];
							LOG_LINE;
							UNUSED(reg);
							i++;
							LOG_LINE;
							if (is_function2)
							{
								LOG_LINE;
								env->push(*(env->local_register_ptr(reg)));
								IF_VERBOSE_ACTION(
									log_msg("-------------- pushed local register[%d] = '%s'\n",
										reg,
										env->top(0).to_string()));
							}
							else if (reg < 0 || reg >= 4)
							{
								LOG_LINE;
								env->push(as_value());
								log_error("push register[%d] -- register out of bounds!\n", reg);
							}
							else
							{
								LOG_LINE;
								env->push(env->m_global_register[reg]);
								IF_VERBOSE_ACTION(
									log_msg("-------------- pushed global register[%d] = '%s'\n",
										reg,
										env->top(0).to_string()));
							}

						}
						else if (type == 5)
						{
							LOG_LINE;
							bool	bool_val = m_buffer[3 + i] ? true : false;
							LOG_LINE;
							i++;
//							log_msg("bool(%d)\n", bool_val);
							LOG_LINE;
							env->push(bool_val);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed %s\n", bool_val ? "true" : "false"));
						}
						else if (type == 6)
						{
							// double
							// wacky format: 45670123
							union {
								double	d;
								Uint64	i;
								struct {
									Uint32	lo;
									Uint32	hi;
								} sub;
							} u;
							LOG_LINE;
							compiler_assert(sizeof(u) == sizeof(u.i));

							LOG_LINE;
							memcpy(&u.sub.hi, &m_buffer[3 + i], 4);
							LOG_LINE;
							memcpy(&u.sub.lo, &m_buffer[3 + i + 4], 4);
							LOG_LINE;
							u.i = swap_le64(u.i);
							LOG_LINE;
							i += 8;

							LOG_LINE;
							env->push(u.d);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed double %f\n", u.d));
						}
						else if (type == 7)
						{
							LOG_LINE;
							// int32
							Sint32	val = m_buffer[3 + i]
								| (m_buffer[3 + i + 1] << 8)
								| (m_buffer[3 + i + 2] << 16)
								| (m_buffer[3 + i + 3] << 24);
							i += 4;
						
							LOG_LINE;
							env->push(val);

							IF_VERBOSE_ACTION(log_msg("-------------- pushed int32 %d\n", val));
						}
						else if (type == 8)
						{
							LOG_LINE;
							int	id = m_buffer[3 + i];
							i++;
							LOG_LINE;
							if (id < m_dictionary.size())
							{
								LOG_LINE;
								env->push(m_dictionary[id]);

								IF_VERBOSE_ACTION(log_msg("-------------- pushed '%s'\n", m_dictionary[id]));
							}
							else
							{
								LOG_LINE;
								log_error("error: dict_lookup(%d) is out of bounds!\n", id);
								LOG_LINE;
								env->push(0);
								IF_VERBOSE_ACTION(log_msg("-------------- pushed 0\n"));
							}
						}
						else if (type == 9)
						{
							LOG_LINE;
							int	id = m_buffer[3 + i] | (m_buffer[4 + i] << 8);
							LOG_LINE;
							i += 2;
							LOG_LINE;
							if (id < m_dictionary.size())
							{
								LOG_LINE;
								env->push(m_dictionary[id]);
								IF_VERBOSE_ACTION(log_msg("-------------- pushed '%s'\n", m_dictionary[id]));
							}
							else
							{
								log_error("error: dict_lookup(%d) is out of bounds!\n", id);
								LOG_LINE;
								env->push(0);

								IF_VERBOSE_ACTION(log_msg("-------------- pushed 0"));
							}
						}
					}
					
					break;
				}
				case 0x99:	// branch always (goto)
				{
					LOG_LINE;
					Sint16	offset = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);
					LOG_LINE;
					next_pc += offset;
					// @@ TODO range checks
					break;
				}
				case 0x9A:	// get url 2
				{
					LOG_LINE;
					int	method = m_buffer[pc + 3];
					LOG_LINE;
					UNUSED(method);

					LOG_LINE;
					const char*	target = env->top(0).to_string();
					LOG_LINE;
					const char*	url = env->top(1).to_string();

					// If the url starts with "FSCommand:", then this is
					// a message for the host app.
					LOG_LINE;
					if (strncmp(url, "FSCommand:", 10) == 0)
					{
						LOG_LINE;
						if (s_fscommand_handler)
						{
							// Call into the app.
							LOG_LINE;
							(*s_fscommand_handler)(env->get_target()->get_root_interface(), url + 10, target);
						}
					}
					else
					{
#ifdef EXTERN_MOVIE
//            log_error("get url2: target=%s, url=%s\n", target, url);

						LOG_LINE;
						movie* target_movie = env->find_target(env->top(0));
						LOG_LINE;
						if (target_movie != NULL)
						{
							LOG_LINE;
							movie*	root_movie = env->get_target()->get_root_movie();
							LOG_LINE;
							attach_extern_movie(url, target_movie, root_movie);
						}
						else
						{
							LOG_LINE;
							log_error("get url2: target %s not found\n", target);
						}
#endif // EXTERN_MOVIE
					}
					LOG_LINE;
					env->drop(2);
					break;
				}

				case 0x9B:	// declare function
				{
					LOG_LINE;
					as_as_function*	func = new as_as_function(this, env, next_pc, with_stack);

					LOG_LINE;
					int	i = pc;
					i += 3;

					// Extract name.
					// @@ security: watch out for possible missing terminator here!
					LOG_LINE;
					tu_string	name = (const char*) &m_buffer[i];
					LOG_LINE;
					i += name.length() + 1;

					// Get number of arguments.
					LOG_LINE;
					int	nargs = m_buffer[i] | (m_buffer[i + 1] << 8);

					LOG_LINE;
					i += 2;

					// Get the names of the arguments.
					LOG_LINE;
					for (int n = 0; n < nargs; n++)
					{
						// @@ security: watch out for possible missing terminator here!
						LOG_LINE;
						func->add_arg(0, (const char*) &m_buffer[i]);
						LOG_LINE;
						i += func->m_args.back().m_name.length() + 1;
					}

					// Get the length of the actual function code.
					LOG_LINE;
					int	length = m_buffer[i] | (m_buffer[i + 1] << 8);
					LOG_LINE;
					i += 2;
					LOG_LINE;
					func->set_length(length);

					// Skip the function body (don't interpret it now).
					LOG_LINE;
					next_pc += length;

					// If we have a name, then save the function in this
					// environment under that name.
					LOG_LINE;
					as_value	function_value(func);
					LOG_LINE;
					if (name.length() > 0)
					{
						// @@ NOTE: should this be m_target->set_variable()???
						LOG_LINE;
						env->set_member(name, function_value);
					}

					// Also leave it on the stack.
					LOG_LINE;
					env->push_val(function_value);

					break;
				}

				case 0x9D:	// branch if true
				{
					LOG_LINE;
					Sint16	offset = m_buffer[pc + 3] | (m_buffer[pc + 4] << 8);

					LOG_LINE;
					bool	test = env->top(0).to_bool();
					LOG_LINE;
					env->drop(1);
					LOG_LINE;
					if (test)
					{
						LOG_LINE;
						next_pc += offset;

						LOG_LINE;
						if (next_pc > stop_pc)
						{
							log_error("branch to offset %d -- this section only runs to %d\n",
								  next_pc,
								  stop_pc);
						}
					}
					break;
				}
				case 0x9E:	// call frame
				{
					// Note: no extra data in this instruction!
					QASSERT(env->m_target);
					LOG_LINE;
					env->m_target->call_frame_actions(env->top(0));
					LOG_LINE;
					env->drop(1);

					break;
				}

				case 0x9F:	// goto frame expression, goto_frame_exp
				{
					// From Alexi's SWF ref:
					//
					// Pop a value or a string and jump to the specified
					// frame. When a string is specified, it can include a
					// path to a sprite as in:
					// 
					//   /Test:55
					// 
					// When f_play is ON, the action is to play as soon as
					// that frame is reached. Otherwise, the
					// frame is shown in stop mode.

					LOG_LINE;
					unsigned char	play_flag = m_buffer[pc + 3];
					LOG_LINE;
					movie::play_state	state = play_flag ? movie::PLAY : movie::STOP;

					LOG_LINE;
					movie* target = env->get_target();
					bool success = false;

					LOG_LINE;
					if (env->top(0).get_type() == as_value::UNDEFINED)
					{
						// No-op.
					}
					else if (env->top(0).get_type() == as_value::STRING)
					{
						// @@ TODO: parse possible sprite path...
						
						// Also, if the frame spec is actually a number (not a label), then
						// we need to do the conversion...

						LOG_LINE;
						const char* frame_label = env->top(0).to_string();
						LOG_LINE;
						if (target->goto_labeled_frame(frame_label))
						{
							success = true;
						}
						else
						{
							// Couldn't find the label.  Try converting to a number.
							double num;
							LOG_LINE;
							if (string_to_number(&num, env->top(0).to_string()))
							{
								LOG_LINE;
								int frame_number = int(num);
								LOG_LINE;
								target->goto_frame(frame_number);
								LOG_LINE;
								success = true;
							}
							// else no-op.
						}
					}
					else if (env->top(0).get_type() == as_value::OBJECT)
					{
						LOG_LINE;
						// This is a no-op; see test_goto_frame.swf
					}
					else if (env->top(0).get_type() == as_value::NUMBER)
					{
						// Frame numbers appear to be 0-based!  @@ Verify.
						LOG_LINE;
						int frame_number = int(env->top(0).to_number());
						LOG_LINE;
						target->goto_frame(frame_number);
						LOG_LINE;
						success = true;
					}

					LOG_LINE;
					if (success)
					{
						LOG_LINE;
						target->set_play_state(state);
					}
					
					LOG_LINE;
					env->drop(1);

					break;
				}
				
				}
				LOG_LINE;
				pc = next_pc;
			}
		}

		LOG_LINE;
		env->set_target(original_target);
	}


	//
	// as_value -- ActionScript value type
	//

	
	as_value::as_value(as_object_interface* obj)
		:
		m_type(OBJECT),
		m_object_value(obj)
	{
		LOG_LINE;
		if (m_object_value)
		{
			LOG_LINE;
			m_object_value->add_ref();
		}
	}


	as_value::as_value(as_as_function* func)
		:
		m_type(AS_FUNCTION),
		m_as_function_value(func)
	{
		LOG_LINE;
		if (m_as_function_value)
		{
			LOG_LINE;
			m_as_function_value->add_ref();
		}
	}


	const char*	as_value::to_string() const
	// Conversion to string.
	{
		LOG_LINE;
		return to_tu_string().c_str();
	}


	const tu_stringi&	as_value::to_tu_stringi() const
	{
		LOG_LINE;
		return reinterpret_cast<const tu_stringi&>(to_tu_string());
	}


	const tu_string&	as_value::to_tu_string() const
	// Conversion to const tu_string&.
	{
		LOG_LINE;
		if (m_type == STRING) { /* don't need to do anything */ }
		else if (m_type == NUMBER)
		{
			// @@ Moock says if value is a NAN, then result is "NaN"
			// INF goes to "Infinity"
			// -INF goes to "-Infinity"
			LOG_LINE;
			char buffer[50];
			LOG_LINE;
			snprintf(buffer, 50, "%.14g", m_number_value);
			LOG_LINE;
			m_string_value = buffer;
		}
		else if (m_type == UNDEFINED)
		{
			// Behavior depends on file version.  In
			// version 7+, it's "undefined", in versions
			// 6-, it's "".
			//
			// We'll go with the v7 behavior by default,
			// and conditionalize via _versioned()
			// functions.
			LOG_LINE;
			m_string_value = "undefined";
		}
		else if (m_type == NULLTYPE)
		{ 
			LOG_LINE;
			m_string_value = "null";
		}
		else if (m_type == BOOLEAN)
		{
			LOG_LINE;
			m_string_value = this->m_boolean_value ? "true" : "false";
		}
		else if (m_type == OBJECT)
		{
			// @@ Moock says, "the value that results from
			// calling toString() on the object".
			//
			// The default toString() returns "[object
			// Object]" but may be customized.
			//
			// A Movieclip returns the absolute path of the object.

			LOG_LINE;
			const char*	val = NULL;
			if (m_object_value)
			{
				LOG_LINE;
				val = m_object_value->get_text_value();
			}

			if (val)
			{
				LOG_LINE;
				m_string_value = val;
			}
			else
			{
				// Do we have a "toString" method?
				//
				// TODO: we need an environment in order to call toString()!

				// This is the default.
				LOG_LINE;
				m_string_value = "[object Object]";
			}
		}
		else if (m_type == C_FUNCTION)
		{
			LOG_LINE;
			char buffer[50];
			LOG_LINE;
			snprintf(buffer, 50, "<c_function 0x%X>", (unsigned) m_c_function_value);
			LOG_LINE;
			m_string_value = buffer;
		}
		else if (m_type == AS_FUNCTION)
		{
			LOG_LINE;
			char buffer[50];
			LOG_LINE;
			snprintf(buffer, 50, "<as_function 0x%X>", (unsigned) m_as_function_value);
			LOG_LINE;
			m_string_value = buffer;
		}
		else
		{
			LOG_LINE;
			m_string_value = "<bad type>";
			QASSERT(0);
		}
		
		LOG_LINE;
		return m_string_value;
	}


	const tu_string&	as_value::to_tu_string_versioned(int version) const
	// Conversion to const tu_string&.
	{
		LOG_LINE;
		if (m_type == UNDEFINED)
		{
			// Version-dependent behavior.
			LOG_LINE;
			if (version <= 6)
			{
				LOG_LINE;
				m_string_value = "";
			}
			else
			{
				LOG_LINE;
				m_string_value = "undefined";
			}
			LOG_LINE;
			return m_string_value;
		}
		
		LOG_LINE;
		return to_tu_string();
	}

	double	as_value::to_number() const
	// Conversion to double.
	{
		LOG_LINE;
		if (m_type == STRING)
		{
			// @@ Moock says the rule here is: if the
			// string is a valid float literal, then it
			// gets converted; otherwise it is set to NaN.
			//
			// Also, "Infinity", "-Infinity", and "NaN"
			// are recognized.
			LOG_LINE;
			if (! string_to_number(&m_number_value, m_string_value.c_str()))
			{
				// Failed conversion to Number.
				LOG_LINE;
				m_number_value = 0.0;	// TODO should be NaN
			}
			LOG_LINE;
			return m_number_value;
		}
		else if (m_type == NULLTYPE)
		{
 			// Evan: from my tests
			LOG_LINE;
			return 0;
		}
		else if (m_type == BOOLEAN)
		{
			// Evan: from my tests
			LOG_LINE;
			return (this->m_boolean_value) ? 1 : 0;
		}
		else if (m_type == NUMBER)
		{
			LOG_LINE;
			return m_number_value;
		}
		else if (m_type == OBJECT && m_object_value != NULL)
		{
			// @@ Moock says the result here should be
			// "the return value of the object's valueOf()
			// method".
			//
			// Arrays and Movieclips should return NaN.

			// Text characters with var names could get in
			// here.
			LOG_LINE;
			const char* textval = m_object_value->get_text_value();
			if (textval)
			{
				LOG_LINE;
				return atof(textval);
			}

			LOG_LINE;
			return 0.0;
		}
		else
		{
			LOG_LINE;
			return 0.0;
		}
	}


	bool	as_value::to_bool() const
	// Conversion to boolean.
	{
		// From Moock
		LOG_LINE;
		if (m_type == STRING)
		{
			LOG_LINE;
			if (m_string_value == "false")
			{
				LOG_LINE;
				return false;
			}
			else if (m_string_value == "true")
			{
				LOG_LINE;
				return true;
			}
			else
			{
				// @@ Moock: "true if the string can
				// be converted to a valid nonzero
				// number".
				//
				// Empty string --> false
				LOG_LINE;
				return to_number() != 0.0;
			}
		}
		else if (m_type == NUMBER)
		{
			// @@ Moock says, NaN --> false
			LOG_LINE;
			return m_number_value != 0.0;
		}
		else if (m_type == BOOLEAN)
		{
			LOG_LINE;
			return this->m_boolean_value;
		}
		else if (m_type == OBJECT)
		{
			LOG_LINE;
			return m_object_value != NULL;
		}
		else if (m_type == C_FUNCTION)
		{
			LOG_LINE;
			return m_c_function_value != NULL;
		}
		else if (m_type == AS_FUNCTION)
		{
			LOG_LINE;
			return m_as_function_value != NULL;
		}
		else
		{
			QASSERT(m_type == UNDEFINED || m_type == NULLTYPE);
			LOG_LINE;
			return false;
		}
	}

	
	as_object_interface*	as_value::to_object() const
	// Return value as an object.
	{
		LOG_LINE;
		if (m_type == OBJECT)
		{
			// OK.
			LOG_LINE;
			return m_object_value;
		}
		else if (m_type == AS_FUNCTION && m_as_function_value != NULL)
		{
			// Make sure this as_function has properties &
			// a prototype object attached to it, since those
			// may be about to be referenced.
			LOG_LINE;
			m_as_function_value->lazy_create_properties();
			QASSERT(m_as_function_value->m_properties);

			LOG_LINE;
			return m_as_function_value->m_properties;
		}
		else
		{
			return NULL;
		}
	}


	as_c_function_ptr	as_value::to_c_function() const
	// Return value as a C function ptr.  Returns NULL if value is
	// not a C function.
	{
		LOG_LINE;
		if (m_type == C_FUNCTION)
		{
			// OK.
			LOG_LINE;
			return m_c_function_value;
		}
		else
		{
			return NULL;
		}
	}

	as_as_function*	as_value::to_as_function() const
	// Return value as an ActionScript function.  Returns NULL if value is
	// not an ActionScript function.
	{
		LOG_LINE;
		if (m_type == AS_FUNCTION)
		{
			// OK.
			LOG_LINE;
			return m_as_function_value;
		}
		else
		{
			LOG_LINE;
			return NULL;
		}
	}


	void	as_value::convert_to_number()
	// Force type to number.
	{
		LOG_LINE;
		set_double(to_number());
	}


	void	as_value::convert_to_string()
	// Force type to string.
	{
		LOG_LINE;
		to_tu_string();	// init our string data.
		LOG_LINE;
		m_type = STRING;	// force type.
	}


	void	as_value::convert_to_string_versioned(int version)
	// Force type to string.
	{
		LOG_LINE;
		to_tu_string_versioned(version);	// init our string data.
		LOG_LINE;
		m_type = STRING;	// force type.
	}


	void	as_value::set_as_object_interface(as_object_interface* obj)
	{
		LOG_LINE;
		if (m_type != OBJECT || m_object_value != obj)
		{
			LOG_LINE;
			drop_refs();
			LOG_LINE;
			m_type = OBJECT;
			LOG_LINE;
			m_object_value = obj;
			LOG_LINE;
			if (m_object_value)
			{
				LOG_LINE;
				m_object_value->add_ref();
			}
		}
	}

	void	as_value::set_as_as_function(as_as_function* func)
	{
		LOG_LINE;
		if (m_type != AS_FUNCTION || m_as_function_value != func)
		{
			LOG_LINE;
			drop_refs();
			LOG_LINE;
			m_type = AS_FUNCTION;
			LOG_LINE;
			m_as_function_value = func;
			LOG_LINE;
			if (m_as_function_value)
			{
				LOG_LINE;
				m_as_function_value->add_ref();
			}
		}
	}


	bool	as_value::operator==(const as_value& v) const
	// Return true if operands are equal.
	{
		LOG_LINE;
		bool this_nulltype = (m_type == UNDEFINED || m_type == NULLTYPE);
		LOG_LINE;
		bool v_nulltype = (v.get_type() == UNDEFINED || v.get_type() == NULLTYPE);
		LOG_LINE;
		if (this_nulltype || v_nulltype)
		{
			LOG_LINE;
			return this_nulltype == v_nulltype;
		}
		else if (m_type == STRING)
		{
			LOG_LINE;
			return m_string_value == v.to_tu_string();
		}
		else if (m_type == NUMBER)
		{
			LOG_LINE;
			return m_number_value == v.to_number();
		}
		else if (m_type == BOOLEAN)
		{
			LOG_LINE;
			return m_boolean_value == v.to_bool();
		}
		else
		{
			// Evan: what about objects???
			// TODO
			LOG_LINE;
			return m_type == v.m_type;
		}
	}

	
	bool	as_value::operator!=(const as_value& v) const
	// Return true if operands are not equal.
	{
		LOG_LINE;
		return ! (*this == v);
	}

	
	void	as_value::string_concat(const tu_string& str)
	// Sets *this to this string plus the given string.
	{
		LOG_LINE;
		to_tu_string();	// make sure our m_string_value is initialized
		LOG_LINE;
		m_type = STRING;
		LOG_LINE;
		m_string_value += str;
	}

	void	as_value::drop_refs()
	// Drop any ref counts we have; this happens prior to changing our value.
	{
		LOG_LINE;
		if (m_type == AS_FUNCTION)
		{
			LOG_LINE;
			if (m_as_function_value)
			{
				LOG_LINE;
				m_as_function_value->drop_ref();
				LOG_LINE;
				m_as_function_value = 0;
			}
		}
		else if (m_type == OBJECT)
		{
			LOG_LINE;
			if (m_object_value)
			{
				LOG_LINE;
				m_object_value->drop_ref();
				LOG_LINE;
				m_object_value = 0;
			}
		}
	}


	//
	// as_environment
	//


	as_value	as_environment::get_variable(const tu_string& varname, const array<with_stack_entry>& with_stack) const
	// Return the value of the given var, if it's defined.
	{
		// Path lookup rigamarole.
		LOG_LINE;
		movie*	target = m_target;
		tu_string	path;
		tu_string	var;
		LOG_LINE;
		if (parse_path(varname, &path, &var))
		{
			LOG_LINE;
			target = find_target(path);	// @@ Use with_stack here too???  Need to test.
			LOG_LINE;
			if (target)
			{
				LOG_LINE;
				as_value	val;
				LOG_LINE;
				target->get_member(var, &val);
				LOG_LINE;
				return val;
			}
			else
			{
				LOG_LINE;
				log_error("find_target(\"%s\") failed\n", path.c_str());
				return as_value();
			}
		}
		else
		{
			LOG_LINE;
			return this->get_variable_raw(varname, with_stack);
		}
	}


	as_value	as_environment::get_variable_raw(
		const tu_string& varname,
		const array<with_stack_entry>& with_stack) const
	// varname must be a plain variable name; no path parsing.
	{
		QASSERT(strchr(varname.c_str(), ':') == NULL);
		QASSERT(strchr(varname.c_str(), '/') == NULL);
		QASSERT(strchr(varname.c_str(), '.') == NULL);

		as_value	val;

		// Check the with-stack.
		LOG_LINE;
		for (int i = with_stack.size() - 1; i >= 0; i--)
		{
			LOG_LINE;
			as_object_interface*	obj = with_stack[i].m_object.get_ptr();
			LOG_LINE;
			if (obj && obj->get_member(varname, &val))
			{
				// Found the var in this context.
				LOG_LINE;
				return val;
			}
		}

		// Check locals.
		LOG_LINE;
		int	local_index = find_local(varname);
		LOG_LINE;
		if (local_index >= 0)
		{
			// Get local var.
			LOG_LINE;
			return m_local_frames[local_index].m_value;
		}

		// Looking for "this"?
		LOG_LINE;
		if (varname == "this")
		{
			LOG_LINE;
			val.set_as_object_interface(m_target);
			return val;
		}

		// Check movie members.
		LOG_LINE;
		if (m_target->get_member(varname, &val))
		{
			LOG_LINE;
			return val;
		}

		// Check built-in constants.
		LOG_LINE;
		if (varname == "_root" || varname == "_level0")
		{
			LOG_LINE;
			return as_value(m_target->get_root_movie());
		}
		LOG_LINE;
		if (varname == "_global")
		{
			LOG_LINE;
			return as_value(s_global.get_ptr());
		}
		LOG_LINE;
		if (s_global->get_member(varname, &val))
		{
			LOG_LINE;
			return val;
		}
	
		// Fallback.
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("get_variable_raw(\"%s\") failed, returning UNDEFINED.\n", varname.c_str()));
		return as_value();
	}


	void	as_environment::set_variable(
		const tu_string& varname,
		const as_value& val,
		const array<with_stack_entry>& with_stack)
	// Given a path to variable, set its value.
	{
		LOG_LINE;
		IF_VERBOSE_ACTION(log_msg("-------------- %s = %s\n", varname.c_str(), val.to_string()));//xxxxxxxxxx

		// Path lookup rigamarole.
		movie*	target = m_target;
		tu_string	path;
		tu_string	var;
		LOG_LINE;
		if (parse_path(varname, &path, &var))
		{
			LOG_LINE;
			target = find_target(path);
			if (target)
			{
				LOG_LINE;
				target->set_member(var, val);
			}
		}
		else
		{
			LOG_LINE;
			this->set_variable_raw(varname, val, with_stack);
		}
	}


	void	as_environment::set_variable_raw(
		const tu_string& varname,
		const as_value& val,
		const array<with_stack_entry>& with_stack)
	// No path rigamarole.
	{
		// Check the with-stack.
		LOG_LINE;
		for (int i = with_stack.size() - 1; i >= 0; i--)
		{
			LOG_LINE;
			as_object_interface*	obj = with_stack[i].m_object.get_ptr();
			as_value	dummy;
			LOG_LINE;
			if (obj && obj->get_member(varname, &dummy))
			{
				// This object has the member; so set it here.
				LOG_LINE;
				obj->set_member(varname, val);
				return;
			}
		}

		// Check locals.
		LOG_LINE;
		int	local_index = find_local(varname);
		LOG_LINE;
		if (local_index >= 0)
		{
			// Set local var.
			LOG_LINE;
			m_local_frames[local_index].m_value = val;
			return;
		}

		LOG_LINE;
		QASSERT(m_target);

		LOG_LINE;
		m_target->set_member(varname, val);
	}


	void	as_environment::set_local(const tu_string& varname, const as_value& val)
	// Set/initialize the value of the local variable.
	{
		// Is it in the current frame already?
		LOG_LINE;
		int	index = find_local(varname);
		LOG_LINE;
		if (index < 0)
		{
			// Not in frame; create a new local var.

			LOG_LINE;
			QASSERT(varname.length() > 0);	// null varnames are invalid!
    			m_local_frames.push_back(frame_slot(varname, val));
		}
		else
		{
			// In frame already; modify existing var.
			LOG_LINE;
			m_local_frames[index].m_value = val;
		}
	}

	
	void	as_environment::add_local(const tu_string& varname, const as_value& val)
	// Add a local var with the given name and value to our
	// current local frame.  Use this when you know the var
	// doesn't exist yet, since it's faster than set_local();
	// e.g. when setting up args for a function.
	{
		QASSERT(varname.length() > 0);
		LOG_LINE;
		m_local_frames.push_back(frame_slot(varname, val));
	}


	void	as_environment::declare_local(const tu_string& varname)
	// Create the specified local var if it doesn't exist already.
	{
		// Is it in the current frame already?
		LOG_LINE;
		int	index = find_local(varname);
		if (index < 0)
		{
			// Not in frame; create a new local var.
			LOG_LINE;
			QASSERT(varname.length() > 0);	// null varnames are invalid!
    			m_local_frames.push_back(frame_slot(varname, as_value()));
		}
		else
		{
			LOG_LINE;
			// In frame already; don't mess with it.
		}
	}

	
	bool	as_environment::get_member(const tu_stringi& varname, as_value* val) const
	{
		LOG_LINE;
		return m_variables.get(varname, val);
	}


	void	as_environment::set_member(const tu_stringi& varname, const as_value& val)
	{
		LOG_LINE;
		m_variables.set(varname, val);
	}

	as_value*	as_environment::local_register_ptr(int reg)
	// Return a pointer to the specified local register.
	// Local registers are numbered starting with 1.
	//
	// Return value will never be NULL.  If reg is out of bounds,
	// we log an error, but still return a valid pointer (to
	// global reg[0]).  So the behavior is a bit undefined, but
	// not dangerous.
	{
		// We index the registers from the end of the register
		// array, so we don't have to keep base/frame
		// pointers.

		LOG_LINE;
		if (reg <= 0 || reg > m_local_register.size())
		{
			log_error("Invalid local register %d, stack only has %d entries\n",
				  reg, m_local_register.size());

			// Fallback: use global 0.
			LOG_LINE;
			return &m_global_register[0];
		}

		LOG_LINE;
		return &m_local_register[m_local_register.size() - reg];
	}


	int	as_environment::find_local(const tu_string& varname) const
	// Search the active frame for the named var; return its index
	// in the m_local_frames stack if found.
	// 
	// Otherwise return -1.
	{
		// Linear search sucks, but is probably fine for
		// typical use of local vars in script.  There could
		// be pathological breakdowns if a function has tons
		// of locals though.  The ActionScript bytecode does
		// not help us much by using strings to index locals.

		LOG_LINE;
		for (int i = m_local_frames.size() - 1; i >= 0; i--)
		{
			LOG_LINE;
			const frame_slot&	slot = m_local_frames[i];
			if (slot.m_name.length() == 0)
			{
				// End of local frame; stop looking.
				LOG_LINE;
				return -1;
			}
			else if (slot.m_name == varname)
			{
				// Found it.
				LOG_LINE;
				return i;
			}
		}
		LOG_LINE;
		return -1;
	}


	bool	as_environment::parse_path(const tu_string& var_path, tu_string* path, tu_string* var) const
	// See if the given variable name is actually a sprite path
	// followed by a variable name.  These come in the format:
	//
	// 	/path/to/some/sprite/:varname
	//
	// (or same thing, without the last '/')
	//
	// or
	//	path.to.some.var
	//
	// If that's the format, puts the path part (no colon or
	// trailing slash) in *path, and the varname part (no color)
	// in *var and returns true.
	//
	// If no colon, returns false and leaves *path & *var alone.
	{
		// Search for colon.
		LOG_LINE;
		int	colon_index = 0;
		LOG_LINE;
		int	var_path_length = var_path.length();
		LOG_LINE;
		for ( ; colon_index < var_path_length; colon_index++)
		{
			LOG_LINE;
			if (var_path[colon_index] == ':')
			{
				// Found it.
				break;
			}
		}

		LOG_LINE;
		if (colon_index >= var_path_length)
		{
			// No colon.  Is there a '.'?  Find the last
			// one, if any.
			LOG_LINE;
			for (colon_index = var_path_length - 1; colon_index >= 0; colon_index--)
			{
				LOG_LINE;
				if (var_path[colon_index] == '.')
				{
					// Found it.
					break;
				}
			}
			LOG_LINE;
			if (colon_index < 0) return false;
		}

		// Make the subparts.

		// Var.
		LOG_LINE;
		*var = &var_path[colon_index + 1];

		// Path.
		LOG_LINE;
		if (colon_index > 0)
		{
			LOG_LINE;
			if (var_path[colon_index - 1] == '/')
			{
				// Trim off the extraneous trailing slash.
				LOG_LINE;
				colon_index--;
			}
		}
		// @@ could be better.  This whole usage of tu_string is very flabby...
		LOG_LINE;
		*path = var_path;
		LOG_LINE;
		path->resize(colon_index);

		return true;
	}


	movie*	as_environment::find_target(const as_value& val) const
	// Find the sprite/movie represented by the given value.  The
	// value might be a reference to the object itself, or a
	// string giving a relative path name to the object.
	{
		LOG_LINE;
		if (val.get_type() == as_value::OBJECT)
		{
			LOG_LINE;
			if (val.to_object() != NULL)
			{
				LOG_LINE;
				return val.to_object()->to_movie();
			}
			else
			{
				return NULL;
			}
		}
		else if (val.get_type() == as_value::STRING)
		{
			LOG_LINE;
			return find_target(val.to_tu_string());
		}
		else
		{
			LOG_LINE;
			log_error("error: invalid path; neither string nor object");
			return NULL;
		}
	}


	static const char*	next_slash_or_dot(const char* word)
	// Search for next '.' or '/' character in this word.  Return
	// a pointer to it, or to NULL if it wasn't found.
	{
		LOG_LINE;
		for (const char* p = word; *p; p++)
		{
			LOG_LINE;
			if (*p == '.' && p[1] == '.')
			{
				LOG_LINE;
				p++;
			}
			else if (*p == '.' || *p == '/')
			{
				LOG_LINE;
				return p;
			}
		}

		LOG_LINE;
		return NULL;
	}


	movie*	as_environment::find_target(const tu_string& path) const
	// Find the sprite/movie referenced by the given path.
	{
		LOG_LINE;
		if (path.length() <= 0)
		{
			LOG_LINE;
			return m_target;
		}

		LOG_LINE;
		QASSERT(path.length() > 0);

		LOG_LINE;
		movie*	env = m_target;
		QASSERT(env);
		
		LOG_LINE;
		const char*	p = path.c_str();
		tu_string	subpart;

		LOG_LINE;
		if (*p == '/')
		{
			// Absolute path.  Start at the root.
			LOG_LINE;
			env = env->get_relative_target("_level0");
			p++;
		}

		LOG_LINE;
		for (;;)
		{
			LOG_LINE;
			const char*	next_slash = next_slash_or_dot(p);
			LOG_LINE;
			subpart = p;
			LOG_LINE;
			if (next_slash == p)
			{
				LOG_LINE;
				log_error("error: invalid path '%s'\n", path.c_str());
				break;
			}
			else if (next_slash)
			{
				// Cut off the slash and everything after it.
				LOG_LINE;
				subpart.resize(next_slash - p);
			}

			LOG_LINE;
			env = env->get_relative_target(subpart);
			//@@   _level0 --> root, .. --> parent, . --> this, other == character

			LOG_LINE;
			if (env == NULL || next_slash == NULL)
			{
				break;
			}

			LOG_LINE;
			p = next_slash + 1;
		}
		return env;
	}


	//
	// event_id
	//

	const tu_string&	event_id::get_function_name() const
	{
		static tu_string	s_function_names[EVENT_COUNT] =
		{
			"INVALID",		 // INVALID
			"onPress",		 // PRESS
			"onRelease",		 // RELEASE
			"onRelease_Outside",	 // RELEASE_OUTSIDE
			"onRoll_Over",		 // ROLL_OVER
			"onRoll_Out",		 // ROLL_OUT
			"onDrag_Over",		 // DRAG_OVER
			"onDrag_Out",		 // DRAG_OUT
			"onKeyPress",		 // KEY_PRESS
			"onInitialize",		 // INITIALIZE

			"onLoad",		 // LOAD
			"onUnload",		 // UNLOAD
			"onEnterFrame",		 // ENTER_FRAME
			"onMouseDown",		 // MOUSE_DOWN
			"onMouseUp",		 // MOUSE_UP
			"onMouseMove",		 // MOUSE_MOVE
			"onKeyDown",		 // KEY_DOWN
			"onKeyUp",		 // KEY_UP
			"onData",		 // DATA
			// These are for the MoveClipLoader ActionScript only
			"onLoadStart",		 // LOAD_START
			"onLoadError",		 // LOAD_ERROR
			"onLoadProgress",	 // LOAD_PROGRESS
			"onLoadInit",		 // LOAD_INIT
			// These are for the XMLSocket ActionScript only
			"onSockClose",		 // CLOSE
			"onSockConnect",	 // CONNECT
			"onSockXML",		 // XML
			// These are for the XML ActionScript only
			"onXMLLoad",		 // XML_LOAD
			"onXMLData",		 // XML_DATA
			"onTimer",	         // setInterval Timer expired
		};

		LOG_LINE;
		QASSERT(m_id > INVALID && m_id < EVENT_COUNT);
		return s_function_names[m_id];
	}


	// Standard member lookup.
	as_standard_member	get_standard_member(const tu_stringi& name)
	{
		static bool	s_inited = false;
		static stringi_hash<as_standard_member>	s_standard_member_map;
		LOG_LINE;
		if (!s_inited)
		{
			s_inited = true;

			LOG_LINE;
			s_standard_member_map.set_capacity(int(AS_STANDARD_MEMBER_COUNT));

			LOG_LINE;
			s_standard_member_map.add("_x", M_X);
			LOG_LINE;
			s_standard_member_map.add("_y", M_Y);
			LOG_LINE;
			s_standard_member_map.add("_xscale", M_XSCALE);
			LOG_LINE;
			s_standard_member_map.add("_yscale", M_YSCALE);
			LOG_LINE;
			s_standard_member_map.add("_currentframe", M_CURRENTFRAME);
			LOG_LINE;
			s_standard_member_map.add("_totalframes", M_TOTALFRAMES);
			LOG_LINE;
			s_standard_member_map.add("_alpha", M_ALPHA);
			LOG_LINE;
			s_standard_member_map.add("_visible", M_VISIBLE);
			LOG_LINE;
			s_standard_member_map.add("_width", M_WIDTH);
			LOG_LINE;
			s_standard_member_map.add("_height", M_HEIGHT);
			LOG_LINE;
			s_standard_member_map.add("_rotation", M_ROTATION);
			LOG_LINE;
			s_standard_member_map.add("_target", M_TARGET);
			LOG_LINE;
			s_standard_member_map.add("_framesloaded", M_FRAMESLOADED);
			LOG_LINE;
			s_standard_member_map.add("_name", M_NAME);
			LOG_LINE;
			s_standard_member_map.add("_droptarget", M_DROPTARGET);
			LOG_LINE;
			s_standard_member_map.add("_url", M_URL);
			LOG_LINE;
			s_standard_member_map.add("_highquality", M_HIGHQUALITY);
			LOG_LINE;
			s_standard_member_map.add("_focusrect", M_FOCUSRECT);
			LOG_LINE;
			s_standard_member_map.add("_soundbuftime", M_SOUNDBUFTIME);
			LOG_LINE;
			s_standard_member_map.add("_xmouse", M_XMOUSE);
			LOG_LINE;
			s_standard_member_map.add("_ymouse", M_YMOUSE);
			LOG_LINE;
			s_standard_member_map.add("_parent", M_PARENT);
			LOG_LINE;
			s_standard_member_map.add("text", M_TEXT);
			LOG_LINE;
			s_standard_member_map.add("textWidth", M_TEXTWIDTH);
			LOG_LINE;
			s_standard_member_map.add("textColor", M_TEXTCOLOR);
			LOG_LINE;
			s_standard_member_map.add("onLoad", M_ONLOAD);
		}

		LOG_LINE;
		as_standard_member	result = M_INVALID_MEMBER;
		LOG_LINE;
		s_standard_member_map.get(name, &result);

		return result;
	}


	//
	// Disassembler
	//


#define COMPILE_DISASM 1

#ifndef COMPILE_DISASM

	void	log_disasm(const unsigned char* instruction_data)
	// No disassembler in this version...
	{
		LOG_LINE;
		log_msg("<no disasm>\n");
	}

#else // COMPILE_DISASM

	void	log_disasm(const unsigned char* instruction_data)
	// Disassemble one instruction to the log.
	{
		enum arg_format {
			ARG_NONE = 0,
			ARG_STR,
			ARG_HEX,	// default hex dump, in case the format is unknown or unsupported
			ARG_U8,
			ARG_U16,
			ARG_S16,
			ARG_PUSH_DATA,
			ARG_DECL_DICT,
			ARG_FUNCTION2
		};
		struct inst_info
		{
			int	m_action_id;
			const char*	m_instruction;

			arg_format	m_arg_format;
		};

		static inst_info	s_instruction_table[] = {
			{ 0x04, "next_frame", ARG_NONE },
			{ 0x05, "prev_frame", ARG_NONE },
			{ 0x06, "play", ARG_NONE },
			{ 0x07, "stop", ARG_NONE },
			{ 0x08, "toggle_qlty", ARG_NONE },
			{ 0x09, "stop_sounds", ARG_NONE },
			{ 0x0A, "add", ARG_NONE },
			{ 0x0B, "sub", ARG_NONE },
			{ 0x0C, "mul", ARG_NONE },
			{ 0x0D, "div", ARG_NONE },
			{ 0x0E, "equ", ARG_NONE },
			{ 0x0F, "lt", ARG_NONE },
			{ 0x10, "and", ARG_NONE },
			{ 0x11, "or", ARG_NONE },
			{ 0x12, "not", ARG_NONE },
			{ 0x13, "str_eq", ARG_NONE },
			{ 0x14, "str_len", ARG_NONE },
			{ 0x15, "substr", ARG_NONE },
			{ 0x17, "pop", ARG_NONE },
			{ 0x18, "floor", ARG_NONE },
			{ 0x1C, "get_var", ARG_NONE },
			{ 0x1D, "set_var", ARG_NONE },
			{ 0x20, "set_target_exp", ARG_NONE },
			{ 0x21, "str_cat", ARG_NONE },
			{ 0x22, "get_prop", ARG_NONE },
			{ 0x23, "set_prop", ARG_NONE },
			{ 0x24, "dup_sprite", ARG_NONE },
			{ 0x25, "rem_sprite", ARG_NONE },
			{ 0x26, "trace", ARG_NONE },
			{ 0x27, "start_drag", ARG_NONE },
			{ 0x28, "stop_drag", ARG_NONE },
			{ 0x29, "str_lt", ARG_NONE },
			{ 0x2B, "cast_object", ARG_NONE },
			{ 0x30, "random", ARG_NONE },
			{ 0x31, "mb_length", ARG_NONE },
			{ 0x32, "ord", ARG_NONE },
			{ 0x33, "chr", ARG_NONE },
			{ 0x34, "get_timer", ARG_NONE },
			{ 0x35, "substr_mb", ARG_NONE },
			{ 0x36, "ord_mb", ARG_NONE },
			{ 0x37, "chr_mb", ARG_NONE },
			{ 0x3A, "delete", ARG_NONE },
			{ 0x3B, "delete_all", ARG_NONE },
			{ 0x3C, "set_local", ARG_NONE },
			{ 0x3D, "call_func", ARG_NONE },
			{ 0x3E, "return", ARG_NONE },
			{ 0x3F, "mod", ARG_NONE },
			{ 0x40, "new", ARG_NONE },
			{ 0x41, "decl_local", ARG_NONE },
			{ 0x42, "decl_array", ARG_NONE },
			{ 0x43, "decl_obj", ARG_NONE },
			{ 0x44, "type_of", ARG_NONE },
			{ 0x45, "get_target", ARG_NONE },
			{ 0x46, "enumerate", ARG_NONE },
			{ 0x47, "add_t", ARG_NONE },
			{ 0x48, "lt_t", ARG_NONE },
			{ 0x49, "eq_t", ARG_NONE },
			{ 0x4A, "number", ARG_NONE },
			{ 0x4B, "string", ARG_NONE },
			{ 0x4C, "dup", ARG_NONE },
			{ 0x4D, "swap", ARG_NONE },
			{ 0x4E, "get_member", ARG_NONE },
			{ 0x4F, "set_member", ARG_NONE },
			{ 0x50, "inc", ARG_NONE },
			{ 0x51, "dec", ARG_NONE },
			{ 0x52, "call_method", ARG_NONE },
			{ 0x53, "new_method", ARG_NONE },
			{ 0x54, "is_inst_of", ARG_NONE },
			{ 0x55, "enum_object", ARG_NONE },
			{ 0x60, "bit_and", ARG_NONE },
			{ 0x61, "bit_or", ARG_NONE },
			{ 0x62, "bit_xor", ARG_NONE },
			{ 0x63, "shl", ARG_NONE },
			{ 0x64, "asr", ARG_NONE },
			{ 0x65, "lsr", ARG_NONE },
			{ 0x66, "eq_strict", ARG_NONE },
			{ 0x67, "gt_t", ARG_NONE },
			{ 0x68, "gt_str", ARG_NONE },
			
			{ 0x81, "goto_frame", ARG_U16 },
			{ 0x83, "get_url", ARG_STR },
			{ 0x87, "store_register", ARG_U8 },
			{ 0x88, "decl_dict", ARG_DECL_DICT },
			{ 0x8A, "wait_for_frame", ARG_HEX },
			{ 0x8B, "set_target", ARG_STR },
			{ 0x8C, "goto_frame_lbl", ARG_STR },
			{ 0x8D, "wait_for_fr_exp", ARG_HEX },
			{ 0x8E, "function2", ARG_FUNCTION2 },
			{ 0x94, "with", ARG_U16 },
			{ 0x96, "push_data", ARG_PUSH_DATA },
			{ 0x99, "goto", ARG_S16 },
			{ 0x9A, "get_url2", ARG_HEX },
			// { 0x8E, "function2", ARG_HEX },
			{ 0x9B, "func", ARG_HEX },
			{ 0x9D, "branch_if_true", ARG_S16 },
			{ 0x9E, "call_frame", ARG_HEX },
			{ 0x9F, "goto_frame_exp", ARG_HEX },
			{ 0x00, "<end>", ARG_NONE }
		};

		int	action_id = instruction_data[0];
		inst_info*	info = NULL;

		for (int i = 0; ; i++)
		{
			if (s_instruction_table[i].m_action_id == action_id)
			{
				info = &s_instruction_table[i];
			}

			if (s_instruction_table[i].m_action_id == 0)
			{
				// Stop at the end of the table and give up.
				break;
			}
		}

		arg_format	fmt = ARG_HEX;

		// Show instruction.
		if (info == NULL)
		{
			log_msg("<unknown>[0x%02X]", action_id);
		}
		else
		{
			log_msg("%-15s", info->m_instruction);
			fmt = info->m_arg_format;
		}

		// Show instruction argument(s).
		if (action_id & 0x80)
		{
			QASSERT(fmt != ARG_NONE);

			int	length = instruction_data[1] | (instruction_data[2] << 8);

			// log_msg(" [%d]", length);

			if (fmt == ARG_HEX)
			{
				for (int i = 0; i < length; i++)
				{
					log_msg(" 0x%02X", instruction_data[3 + i]);
				}
				log_msg("\n");
			}
			else if (fmt == ARG_STR)
			{
				log_msg(" \"");
				for (int i = 0; i < length; i++)
				{
					log_msg("%c", instruction_data[3 + i]);
				}
				log_msg("\"\n");
			}
			else if (fmt == ARG_U8)
			{
				int	val = instruction_data[3];
				log_msg(" %d\n", val);
			}
			else if (fmt == ARG_U16)
			{
				int	val = instruction_data[3] | (instruction_data[4] << 8);
				log_msg(" %d\n", val);
			}
			else if (fmt == ARG_S16)
			{
				int	val = instruction_data[3] | (instruction_data[4] << 8);
				if (val & 0x8000) val |= ~0x7FFF;	// sign-extend
				log_msg(" %d\n", val);
			}
			else if (fmt == ARG_PUSH_DATA)
			{
				log_msg("\n");
				int i = 0;
				while (i < length)
				{
					int	type = instruction_data[3 + i];
					i++;
					log_msg("\t\t");	// indent
					if (type == 0)
					{
						// string
						log_msg("\"");
						while (instruction_data[3 + i])
						{
							log_msg("%c", instruction_data[3 + i]);
							i++;
						}
						i++;
						log_msg("\"\n");
					}
					else if (type == 1)
					{
						// float (little-endian)
						union {
							float	f;
							Uint32	i;
						} u;
						compiler_assert(sizeof(u) == sizeof(u.i));

						memcpy(&u.i, instruction_data + 3 + i, 4);
						u.i = swap_le32(u.i);
						i += 4;

						log_msg("(float) %f\n", u.f);
					}
					else if (type == 2)
					{
						log_msg("NULL\n");
					}
					else if (type == 3)
					{
						log_msg("undef\n");
					}
					else if (type == 4)
					{
						// contents of register
						int	reg = instruction_data[3 + i];
						i++;
						log_msg("reg[%d]\n", reg);
					}
					else if (type == 5)
					{
						int	bool_val = instruction_data[3 + i];
						i++;
						log_msg("bool(%d)\n", bool_val);
					}
					else if (type == 6)
					{
						// double
						// wacky format: 45670123
						union {
							double	d;
							Uint64	i;
							struct {
								Uint32	lo;
								Uint32	hi;
							} sub;
						} u;
						compiler_assert(sizeof(u) == sizeof(u.i));

						memcpy(&u.sub.hi, instruction_data + 3 + i, 4);
						memcpy(&u.sub.lo, instruction_data + 3 + i + 4, 4);
						u.i = swap_le64(u.i);
						i += 8;

						log_msg("(double) %f\n", u.d);
					}
					else if (type == 7)
					{
						// int32
						Sint32	val = instruction_data[3 + i]
							| (instruction_data[3 + i + 1] << 8)
							| (instruction_data[3 + i + 2] << 16)
							| (instruction_data[3 + i + 3] << 24);
						i += 4;
						log_msg("(int) %d\n", val);
					}
					else if (type == 8)
					{
						int	id = instruction_data[3 + i];
						i++;
						log_msg("dict_lookup[%d]\n", id);
					}
					else if (type == 9)
					{
						int	id = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
						i += 2;
						log_msg("dict_lookup_lg[%d]\n", id);
					}
				}
			}
			else if (fmt == ARG_DECL_DICT)
			{
				int	i = 0;
				int	count = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
				i += 2;

				log_msg(" [%d]\n", count);

				// Print strings.
				for (int ct = 0; ct < count; ct++)
				{
					log_msg("\t\t");	// indent

					log_msg("\"");
					while (instruction_data[3 + i])
					{
						// safety check.
						if (i >= length)
						{
							log_msg("<disasm error -- length exceeded>\n");
							break;
						}

						log_msg("%c", instruction_data[3 + i]);
						i++;
					}
					log_msg("\"\n");
					i++;
				}
			}
			else if (fmt == ARG_FUNCTION2)
			{
				// Signature info for a function2 opcode.
				int	i = 0;
				const char*	function_name = (const char*) &instruction_data[3 + i];
				i += strlen(function_name) + 1;

				int	arg_count = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
				i += 2;

				int	reg_count = instruction_data[3 + i];
				i++;

				log_msg("\n\t\tname = '%s', arg_count = %d, reg_count = %d\n",
					function_name, arg_count, reg_count);

				uint16	flags = (instruction_data[3 + i]) | (instruction_data[3 + i + 1] << 8);
				i += 2;

				// @@ What is the difference between "super" and "_parent"?

				bool	preload_global = (flags & 0x100) != 0;
				bool	preload_parent = (flags & 0x80) != 0;
				bool	preload_root   = (flags & 0x40) != 0;
				bool	suppress_super = (flags & 0x20) != 0;
				bool	preload_super  = (flags & 0x10) != 0;
				bool	suppress_args  = (flags & 0x08) != 0;
				bool	preload_args   = (flags & 0x04) != 0;
				bool	suppress_this  = (flags & 0x02) != 0;
				bool	preload_this   = (flags & 0x01) != 0;

				log_msg("\t\t        pg = %d\n"
					"\t\t        pp = %d\n"
					"\t\t        pr = %d\n"
					"\t\tss = %d, ps = %d\n"
					"\t\tsa = %d, pa = %d\n"
					"\t\tst = %d, pt = %d\n",
					int(preload_global),
					int(preload_parent),
					int(preload_root),
					int(suppress_super),
					int(preload_super),
					int(suppress_args),
					int(preload_args),
					int(suppress_this),
					int(preload_this));

				for (int argi = 0; argi < arg_count; argi++)
				{
					int	arg_register = instruction_data[3 + i];
					i++;
					const char*	arg_name = (const char*) &instruction_data[3 + i];
					i += strlen(arg_name) + 1;

					log_msg("\t\targ[%d] - reg[%d] - '%s'\n", argi, arg_register, arg_name);
				}

				int	function_length = instruction_data[3 + i] | (instruction_data[3 + i + 1] << 8);
				i += 2;

				log_msg("\t\tfunction length = %d\n", function_length);
			}
		}
		else
		{
			log_msg("\n");
		}
	}

#endif // COMPILE_DISASM


	void	as_object::set_member(const tu_stringi& name, const as_value& val )
	{
//		if ( name == "hitTest" )
//		{
//			ASSERT( 0, "set_member( hitTest )" );
//		}

		LOG_LINE;
		if (name == "prototype")
		{
			LOG_LINE;
			if (m_prototype) m_prototype->drop_ref();
			LOG_LINE;
			m_prototype = val.to_object();
			LOG_LINE;
			if (m_prototype) m_prototype->add_ref();
		}
		else
		{
			as_value	value;

			LOG_LINE;
			if ( get_member( name, &value ) == true )
			{
				log_error( "Need to handle this!!!" );
			}
			else
			{
				LOG_LINE;
				sMember	member;

				LOG_LINE;
				member.name = name;
				LOG_LINE;
				member.value = val;

				LOG_LINE;
				m_members.push_back( member );
			}
		}
	}

	bool	as_object::get_member(const tu_stringi& name, as_value* val)
	{
//		if ( name == "hitTest" )
//		{
//			ASSERT( 0, "get_member( hitTest )" );
//		}

		LOG_LINE;
		if (name == "prototype")
		{
			LOG_LINE;
			val->set_as_object_interface(m_prototype);

			return true;
		}
		else
		{
			LOG_LINE;
			for ( s32 i = 0; i < m_members.size(); ++i )
			{
				LOG_LINE;
				if ( m_members[ i ].name == name )
				{
					LOG_LINE;
					*val = m_members[ i ].value.get_member_value();

//					if ( name == "hitTest" )
//					{
//						ASSERT( 0, "SUCCESS! get_member( hitTest )" );
//					}

					return true;
				}
			}

			LOG_LINE;
			if (m_prototype != NULL)
			{
				LOG_LINE;
//				if ( name == "hitTest" )
//				{
//					ASSERT( 0, "PROTOTYPE! get_member( hitTest )" );
//				}
				return m_prototype->get_member(name, val);
			}

			LOG_LINE;
			return false;
		}
	}

	bool as_object::get_member(const tu_stringi& name, as_member* member) const
	{
		QASSERT(member != NULL);

//		if ( name == "hitTest" )
//		{
//			ASSERT( 0, "get_member( hitTest )" );
//		}

		LOG_LINE;
		for ( s32 i = 0; i < m_members.size(); ++i )
		{
			LOG_LINE;
			if ( m_members[ i ].name == name )
			{
				LOG_LINE;
				*member = m_members[ i ].value;

//				if ( name == "hitTest" )
//				{
//					ASSERT( 0, "SUCCESS! get_member( hitTest )" );
//				}

				return true;
			}
		}

		LOG_LINE;
		return false;
	}

	bool	as_object::set_member_flags(const tu_stringi& name, const int flags)
	{
//		if ( name == "hitTest" )
//		{
//			ASSERT( 0, "set_member_flags( hitTest )" );
//		}

		LOG_LINE;
		for ( s32 i = 0; i < m_members.size(); ++i )
		{
			LOG_LINE;
			if ( m_members[ i ].name == name )
			{
				LOG_LINE;
				as_prop_flags f = m_members[ i ].value.get_member_flags();
				LOG_LINE;
				f.set_flags(flags);
				LOG_LINE;
				m_members[ i ].value.set_member_flags(f);

//				if ( name == "hitTest" )
//				{
//					ASSERT( 0, "SUCCESS! set_member_flags( hitTest )" );
//				}

				return true;
			}
		}

		return false;
	}

	bool	IsKeyDown( key::code code )
	{
		as_value	kval;

		if ( s_global->get_member( "Key", &kval ) == true )
		{
			if ( kval.get_type() == as_value::OBJECT )
			{
				key_as_object*	ko = (key_as_object*) kval.to_object();
				QASSERT(ko);

				if ( ko != NULL )
				{
					return ko->is_key_down( code );
				}
			}
		}

		return false;
	}

};


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
