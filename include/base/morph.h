#ifndef _BASE_MORPH_
#define _BASE_MORPH_

#include "mesh.h"

namespace base {
namespace model {

	/** Morph class. Information to deform a single mesh 
	 *	Morph can affect all vertex data values
	 * */
	class Morph {
		public:
		Morph();								/**< Default constructor */
		Morph(const Morph&);					/**< Copy constructor */
		~Morph();								/**< Destructor */

		Morph*       grab();					/** Grab morph reference */
		const Morph* grab() const;				/** Grab morph reference */
		int          drop() const;				/** Drop reference. Object is deleted if last */

		const char* getName() const;			/** Get the name of this morph */
		void        setName(const char* name);	/** Set the name of this morph */
		void        setData(int size, IndexType* indices, VertexType* data, uint format);	/** Set morph data */
		void        apply(const Mesh* input, Mesh* output, float value) const;				/** Apply morph to output from input */
		void        apply(Mesh* mesh, float value) const;									/** Apply morph to a mesh */ 

		static Morph* create(Mesh* source, Mesh* target, uint format);						/** Create a morph between two meshes */

		protected:
		const char* m_name;			// Morph name
		int         m_size;			// Vertex count
		IndexType   m_max;			// Highest vertex index for mesh validation
		VertexType* m_data;			// Morph data
		IndexType*  m_indices;		// Morph to Mesh vertex index map
		uint        m_format;		// Internal vertex format
		int         m_offset[8];	// Vertex format data offsets
		int         m_parts[8];		// Vertex format part sizes
		int         m_formatSize;   // Total size of vertex format
		mutable int m_ref;			// Reference counter
	};

	// Inline inplementation
	inline Morph*       Morph::grab()                 { ++m_ref; return this; }
	inline const Morph* Morph::grab() const           { ++m_ref; return this; }
	inline int          Morph::drop() const           { if(--m_ref>0) return m_ref; else delete this; return 0; }
	inline const char*  Morph::getName() const        { return m_name; }
	inline void         Morph::setName(const char* n) { m_name = n; }
	inline void         Morph::apply(Mesh* m, float v) const { apply(m,m,v); }

};
};

#endif

