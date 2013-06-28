#ifndef _BASE_MESH_
#define _BASE_MESH_

#include "math.h"


namespace base {
	class SMaterial;

namespace model {

	typedef unsigned short IndexType;
	typedef float VertexType;
	class Skeleton;

	/** Vertex or index buffer for meshes - can be used as buffer objects */
	template<typename T>
	struct Buffer {
		Buffer(): data(0), size(0), bufferObject(0), ref(0) {};
		typedef T type;		// Expose data type
		T*    data;			// Buffer data
		uint  size;			// Array size
		uint  bufferObject; // OpenGL Buffer Object
		uint  ref;			// Reference counter for shared data
	};

	/** Vertex formats - use integer bitmasks. Each part takes 4 bits */
	enum VertexFormat {
		VERTEX2   = 0x000002,
		VERTEX3   = 0x000003,
		NORMAL    = 0x000030,
		TEXCOORD  = 0x000200,
		TEXCOORD1 = 0x002000,
		COLOUR3   = 0x030000,
		COLOUR4   = 0x040000,
		TANGENT   = 0x300000,
	};
	/** Some preset vertex formats */
	enum VertexFormatPreset {
		VERTEX_DEFAULT = VERTEX3 | NORMAL | TEXCOORD,
		VERTEX_TANGENT = VERTEX3 | NORMAL | TEXCOORD | TANGENT,
	};

	/** Mesh skin. Maps vertex indices with skin weights */
	struct Skin {
		char*       name;		/**< Bone name to attach this skin to */
		uint        bone;		/**< Bone index (cached) */
		uint        size;		/**< Number of vertices in this skin */
		IndexType*  indices;	/**< Vertex indices that are in this skin */
		float*      weights;	/**< Vertex weights */
		Matrix      offset;		/**< Skin offset matrix */
	};

	/** Mesh class.
	 * Meshes contain vertex data and optionally indices too.
	 * Vertex format and type is configurable.
	 * A mesh has a single material.
	 * */
	class Mesh {
		friend class Model;
		public:
		Mesh();										/**< Default constructor */
		Mesh(const Mesh& mesh);					 	/**< Copy constructor. Can reference or copy vertex data */
		~Mesh();									/**< Destructor */

		Mesh*       grab();								/**< Take a reference to this mesh */
		const Mesh* grab() const;						/**< Take a reference to this mesh */
		int         drop() const;						/**< Drop a reference to this mesh */

		void bindBuffer() const;					/**< Bind buffer objects */
		void useBuffer(bool use=true);				/**< Use buffer objects for this mesh */
		void updateBuffer();						/**< Update vertex buffer if mesh has changd */

		uint getSize() const;						/**< Number of vertices to draw */
		uint getVertexCount() const;				/**< Number of vertices in the mesh */
		uint getFormat() const;						/**< Get the vertex format */
		void setFormat(uint format);				/**< Change the vertex format. Resizes arrays. */
		uint getMode() const;						/**< Get the draw mode */
		void setMode(uint mode);					/**< Set the draw mode. Default GL_TRIANGLES */
		bool hasIndices() const;					/**< Is the mesh indexed */

		const vec3&     getVertex(uint index) const;	/**< Get a vertex */
		const vec3&     getNormal(uint index) const;	/**< get a vertex normal */
		const IndexType getIndex(uint index) const;		/**< Get vertex index */

		const VertexType* getData() const;			/**< Get raw data pointer - used in morphs and skinning */
		VertexType*       getData();				/**< Get raw data pointer - used in morphs and skinning */

		uint             getStride() const;					/**< Get the vertex array stride in bytes */
		const void*      getVertexPointer() const;			/**< Get the vertex pointer for drawing */
		const void*      getNormalPointer() const;			/**< Get the normal pointer for drawing */
		const void*      getTexCoordPointer(int u=0) const;	/**< Get the texture coordinate pointer for drawing */
		const void*      getColourPointer() const;			/**< Get the colour pointer for drawing */
		const void*      getTangentPointer() const;			/**< Get the tangent pointer for drawing */
		const IndexType* getIndexPointer() const;			/**< Get the index pointer for drawing */

		int              getSkinCount() const;				/**< Get the number of skins in this mesh */
		const Skin*      getSkin(int index) const;			/**< Get a skin by index */

		const SMaterial* getMaterial() const;					/**< Get mesh material */
		SMaterial*       getMaterial();							/**< Get mesh material */
		void             setMaterial(SMaterial* material);		/**< Set the material */


		void setVertices(int count, VertexType* data, uint format);			/**< Set the vertex data */
		void setIndices(int count, IndexType* data);						/**< Set the index data */
		void addSkin(const Skin& skin);											/**< Add a shin */

		int  calculateNormals(float smooth=0);			/**< Calculate the vertex normals */
		int  smoothNormals(float angle);				/**< Smooth normals within angle threshold */
		int  calculateTangents();						/**< Calculate the vertex tangents */
		int  normaliseWeights();						/**< Normalise the skin weights */
		int  optimise();								/**< Optimise mesh by removing any duplicate vertices and using an index array */

		const BoundingBox& getBounds() const;					/**< Get the last calculated axis aligned bounding box */
		const BoundingBox& calculateBounds();					/**< Calculate axis aligned bounding box */

		protected:
		mutable int m_ref;			// Reference counter
		uint        m_mode;			// Drawing mode (GL_TRIANGLES)
		uint        m_format;		// Vertex format
		uint        m_stride;		// Vertex data stride in bytes
		void*       m_pointers[8];	// Cached vertex pointers
		int         m_formatSize;	// Number of floats in format;
		SMaterial*  m_material;		// Material
		BoundingBox m_bounds;		// Axis aligned bounding box

		Buffer<VertexType>* m_vertexBuffer;	// Vertex buffer data
		Buffer<IndexType>*  m_indexBuffer;	// Index bufer data
		Buffer<Skin>*       m_skins;		// Skin array. Uses buffer for reference counting

		
		// Processing functions
		void cachePointers();
		static int calculateTangent(VertexType*const* a, VertexType*const* b, VertexType*const* c, VertexType* t);
		static int formatSize(uint format);
	};


	//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////
	
	inline Mesh*       Mesh::grab()       { ++m_ref; return this; }
	inline const Mesh* Mesh::grab() const { ++m_ref; return this; }
	inline int         Mesh::drop() const { if(m_ref>0) return --m_ref; else delete this; return 0; }

	inline uint Mesh::getSize() const        { return hasIndices()? m_indexBuffer->size: getVertexCount(); }
	inline uint Mesh::getVertexCount() const { return m_vertexBuffer->size * sizeof(VertexType) / m_stride; }
	inline uint Mesh::getFormat() const      { return m_format; }
	inline uint Mesh::getMode() const        { return m_mode; }
	inline void Mesh::setMode(uint mode)      { m_mode = mode; }
	inline bool Mesh::hasIndices() const     { return m_indexBuffer; }

	// Does this work?
	inline const vec3&     Mesh::getVertex(uint index) const { return *reinterpret_cast<vec3*>(m_vertexBuffer->data + index*(m_stride/sizeof(VertexType))); }
	inline const vec3&     Mesh::getNormal(uint index) const { return *reinterpret_cast<vec3*>(m_vertexBuffer->data + index*(m_stride/sizeof(VertexType)) + (m_format&0xf)); }
	inline const IndexType Mesh::getIndex(uint index)  const { return m_indexBuffer->data[ index ]; };

	inline const VertexType* Mesh::getData() const       { return m_vertexBuffer? m_vertexBuffer->data: 0; }
	inline       VertexType* Mesh::getData()             { return m_vertexBuffer? m_vertexBuffer->data: 0; }

	inline uint             Mesh::getStride() const               { return m_stride; }
	inline const void*      Mesh::getVertexPointer()   const      { return m_pointers[0]; }
	inline const void*      Mesh::getNormalPointer()   const      { return m_pointers[1]; }
	inline const void*      Mesh::getTexCoordPointer(int u) const { return m_pointers[2+u]; }
	inline const void*      Mesh::getColourPointer()   const      { return m_pointers[4]; }
	inline const void*      Mesh::getTangentPointer()  const      { return m_pointers[5]; }
	inline const IndexType* Mesh::getIndexPointer() const         { return m_indexBuffer && !m_indexBuffer->bufferObject? m_indexBuffer->data: 0; }

	inline int         Mesh::getSkinCount() const                 { return m_skins? m_skins->size: 0; }
	inline const Skin* Mesh::getSkin(int index) const             { return &m_skins->data[index]; }

	inline void             Mesh::setMaterial(SMaterial* m)       { m_material = m; }
	inline SMaterial*       Mesh::getMaterial()                   { return m_material; }
	inline const SMaterial* Mesh::getMaterial() const             { return m_material; }

	inline const BoundingBox& Mesh::getBounds() const                { return m_bounds; }
	
};
};

#endif

