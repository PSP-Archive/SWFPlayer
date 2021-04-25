// gameswf_morph.h -- Mike Shaver <shaver@off.net> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.
//
// Morph directives for shape tweening.

#ifndef GAMESWF_MORPH_H
#define GAMESWF_MORPH_H

#include "gameswf_shape.h"
#include "gameswf_styles.h"
#include "gameswf_tesselate.h"

namespace gameswf
{
	struct morph_path
	{
		morph_path();
		morph_path(float ax, float ay, int fill0, int fill1, int line);
		bool is_empty() const { return m_edges[0].size() == 0; }
		void tesselate(float ratio) const;

		int m_fill0, m_fill1, m_line;
		float m_ax[2], m_ay[2];
		array<edge> m_edges[2];
		bool m_new_shape;
	};

        struct shape_morph_def : public character_def
        {
                shape_morph_def();
                virtual ~shape_morph_def();
                virtual void display(character *instance_info);
                void read(stream* in, int tag_type, bool with_style,
			  movie_definition_sub* m);
		virtual void tesselate(float error_tolerance, tesselate::trapezoid_accepter *accepter, float ratio) const;

        private:
		void read_edge(stream* in, edge& e, float& x, float& y);
		int read_shape_record(stream* in, movie_definition_sub* m,
				      bool start);

		rect	m_bound_orig, m_bound_target;
		array<morph_fill_style> m_fill_styles;
		array<morph_line_style> m_line_styles;
		array<morph_path> m_paths;

		float m_last_ratio;
		mesh_set *m_last_mesh;
        };

	struct morph_tesselating_shape : public tesselate::tesselating_shape
	{
		morph_tesselating_shape(shape_morph_def *sh, float ratio) :
			m_sh(sh), m_ratio(ratio) { }
		virtual void tesselate(float error_tolerance, tesselate::trapezoid_accepter *accepter) const
		{
			m_sh->tesselate(error_tolerance, accepter, m_ratio);
		}

		
	private:
		shape_morph_def *m_sh;
		float            m_ratio;
	};
		

}

#endif /* GAMESWF_MORPH_H */
// Local Variables:
// mode: C++
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
