// gameswf_movie_def_impl.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.


#ifndef GAMESWF_MOVIE_DEF_IMPL_H
#define GAMESWF_MOVIE_DEF_IMPL_H

#include "gameswf_impl.h"

namespace gameswf
{
	//
	// Helper for movie_def_impl
	//

	struct import_info
	{
		tu_string	m_source_url;
		int	        m_character_id;
		tu_string	m_symbol;

		import_info()
			:
		m_character_id(-1)
		{
			LOG_LINE;
		}

		import_info(const char* source, int id, const char* symbol)
			:
		m_source_url(source),
			m_character_id(id),
			m_symbol(symbol)
		{
			LOG_LINE;
		}
	};


	//
	// movie_def_impl
	//
	// This class holds the immutable definition of a movie's
	// contents.  It cannot be played directly, and does not hold
	// current state; for that you need to call create_instance()
	// to get a movie_instance.
	//

	struct movie_def_impl : public movie_definition_sub
	{
		hash<int, smart_ptr<character_def> >	m_characters;
		hash<int, smart_ptr<font> >	 m_fonts;
		hash<int, smart_ptr<bitmap_character_def> >	m_bitmap_characters;
		hash<int, smart_ptr<sound_sample> >	m_sound_samples;
		array<array<execute_tag*> >	   m_playlist;	// A list of movie control events for each frame.
		array<array<execute_tag*> >	   m_init_action_list;	// Init actions for each frame.
		stringi_hash<int>	           m_named_frames;	// 0-based frame #'s
		stringi_hash<smart_ptr<resource> > m_exports;

		// Items we import.
		array<import_info>	m_imports;

		// Movies we import from; hold a ref on these, to keep them alive
		array<smart_ptr<movie_definition> >	m_import_source_movies;

		// Bitmaps used in this movie; collected in one place to make
		// it possible for the host to manage them as textures.
		array<smart_ptr<bitmap_info> >	m_bitmap_list;

		create_bitmaps_flag	m_create_bitmaps;
		create_font_shapes_flag	m_create_font_shapes;

		rect	m_frame_size;
		float	m_frame_rate;
		int	m_frame_count;
		int	m_version;
		int	m_loading_frame;
		uint32	m_file_length;

		jpeg::input*	m_jpeg_in;


		movie_def_impl(create_bitmaps_flag cbf, create_font_shapes_flag cfs);
		~movie_def_impl();

		movie_interface*	create_instance();

		// ...
		int	get_frame_count() const;
		float	get_frame_rate() const;
		float	get_width_pixels() const;
		float	get_height_pixels() const;

		virtual int	get_version() const;

		virtual int	get_loading_frame() const;

		uint32	get_file_bytes() const;

		virtual create_bitmaps_flag	get_create_bitmaps() const;
		virtual create_font_shapes_flag	get_create_font_shapes() const;
		virtual void	add_bitmap_info(bitmap_info* bi);
		virtual int	get_bitmap_info_count() const;
		virtual bitmap_info*	get_bitmap_info(int i) const;
		virtual void	export_resource(const tu_string& symbol, resource* res);
		virtual smart_ptr<resource>	get_exported_resource(const tu_string& symbol);
		virtual void	add_import(const char* source_url, int id, const char* symbol);
		bool	in_import_table(int character_id);
		virtual void	visit_imported_movies(import_visitor* visitor);
		virtual void	resolve_import(const char* source_url, movie_definition* source_movie);
		void	add_character(int character_id, character_def* c);
		character_def*	get_character_def(int character_id);
		bool	get_labeled_frame(const char* label, int* frame_number);
		void	add_font(int font_id, font* f);
		font*	get_font(int font_id);

		bitmap_character_def*	get_bitmap_character(int character_id);
		void	add_bitmap_character(int character_id, bitmap_character_def* ch);
		sound_sample*	get_sound_sample(int character_id);
		virtual void	add_sound_sample(int character_id, sound_sample* sam);
		void	add_execute_tag(execute_tag* e);
		void	add_init_action(int sprite_id, execute_tag* e);
		void	add_frame_name(const char* name);
		void	set_jpeg_loader(jpeg::input* j_in);
		jpeg::input*	get_jpeg_loader();
		virtual const array<execute_tag*>&	get_playlist(int frame_number);
		virtual const array<execute_tag*>*	get_init_actions(int frame_number);
		void	read(tu_file* in);

		void	get_owned_fonts(array<font*>* fonts);
		void	generate_font_bitmaps();

		void	output_cached_data(tu_file* out, const cache_options& options);
		void	input_cached_data(tu_file* in);
	};

}	// end namespace gameswf


#endif // GAMESWF_MOVIE_DEF_IMPL_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
