#pragma once

#include <base/math.h>
#include <base/thread.h>
#include <vector>

#include <base/gui/delegate.h>

namespace base {

class Material;
class Camera;
class Patch;

/** Patch Indexing
 *
 *          Edge 2
 *        _________
 *        | 0 | 1 |
 * Edge 0 |___|___| Edge 1
 *        | 2 | 3 |
 *        |___|___|
 *          Edge 3
 *
 * */


/** Geometry data of patch */
struct PatchGeometry {
	size_t       vertexCount;	// Number of vertices
	size_t       indexCount;	// Number of indices
	float*       vertices;      // Vertex data - Format: POSITION3 NORMAL3 BLEND_NORMAL3 BLEND_HEIGHT:1 (stride:10)
	uint16*      indices;		// Index data
	BoundingBox* bounds;		// Bounding box
	void*        tag;			// User data
};


/** Geo-Mipmap Landscape - uses separate thread for generation, trilinear filtering */
class Landscape {
	public:
	using HeightFunc = Delegate<float(const vec3&)>;
	using PatchFunc = Delegate<void(PatchGeometry*)>;

	typedef std::vector<const PatchGeometry*> GList;

	Landscape(float size, const vec3& position=vec3());
	~Landscape();

	/** Set lod limits. Deafult: 0-32 */
	void setLimits(int minLod=0, int maxLod=32, int patchLimit=0x100000);

	/** Set the detail error threshold. Default: 8 */
	void setThreshold(float value);

	/** Allow multiple landscapes to be stiched together */
	void connect(Landscape*, int side);

	/** Update all */
	void update(const base::Camera*);

	/** Cull patches */
	int  cull(const base::Camera*);

	float getSize() const { return m_size; }

	/** Get height at a point */
	float getHeight(const vec3&, bool real=false) const;
	float getHeight(float x, float z, bool real=false) const;
	float getHeight(float x, float z, vec3& normal, bool real=false) const;

	/** Set the height generation function */
	void setHeightFunction(HeightFunc);

	/** Set the material callbacks */
	void setPatchCallbacks(PatchFunc created, PatchFunc destroyed, PatchFunc updated);

	/** Get all visible geometry for rendering */
	const GList& getGeometry() const { return m_geometryList; }

	// Visit all patches
	int visitAllPatches(PatchFunc callback) const;
	
	/** Additional collision routines */
	bool intersect(const vec3& start, const vec3& end, vec3& point, vec3& normal) const;
	bool intersect(const vec3& start, const vec3& normalisedDirection, float& t, vec3& normal) const;
	bool intersect(const vec3& start, float radius, const vec3& normalisedDirection, float& t, vec3& normal) const;

	/** Information */
	struct Info { int patches, visiblePatches, triangles, splitQueue, mergeQueue; };
	Info getInfo() const;

	/** Editing functions */
	void updateGeometry(const BoundingBox& box, bool normals);


	// Debug:
	bool selectPatch(const vec3& start, const vec3& direction); // Select patch from ray intersection
	const PatchGeometry* getSelectedGeometry() const;
	void                 getSelectedInfo() const;

	protected:
	vec3  m_position;	// Plane minimum corner position
	float m_size;		// Plane width and height (square)
	HeightFunc m_func;	// Height calculation callback

	PatchFunc  m_createCallback;	// Callback when a patch is created
	PatchFunc  m_destroyCallback;	// Callback when a patch is destroyed
	PatchFunc  m_updateCallback;	// Callback when index array was updated


	uint  m_min, m_max;	// Patch lod limits
	uint  m_patchLimit;	// Maximum patch count
	uint  m_patchSize;	// Vertices in a patch (power of 2 plus 1)
	uint  m_patchStep;	// Maxumum level step between adjacent patches (log2(patchSize-1));
	float m_threshold;	// Error threshold in screen pixels for determining lod value
	
	Patch* m_root;					// Root patch
	GList m_geometryList;			// Output geometry
	GList m_allGeometry;			// List of all patches
	std::vector<Patch*> m_buildList;// Patches to update index data

	std::vector<Patch*> m_splitQueue;
	std::vector<Patch*> m_mergeQueue;

	const Patch* m_selected; // Debug - selected patch

	friend class Patch;
};


/** Landscape segment */
class Patch {
	public:
	friend class Landscape;
	Patch(Landscape* parent);
	Patch(Patch* parent, int index);
	~Patch();

	/** Create vertex data from height function */
	void create();	// create vertex array
	int  split();	// split patch into four sub-patches
	int  merge();	// delete sub-patches
	void build();	// build vertex array for current lod
	void updateEdges();

	void update(const base::Camera*);
	void collect(const base::Camera*, Landscape::GList& list, int cullFlags) const;

	/** Set adjacent patch for LOD blending */
	void setAdjacent(Patch*, int side);

	/** Clear adjacent patches. Used for disconnecting terrain tiles */
	void clearAdjacent(int side);
	
	/** get the current lod height at a point */
	float getHeight(float x, float z, vec3* normal) const;

	/** intersect with current geometry */
	const Patch* intersect(const vec3& start, float radius, const vec3& direction, float& t, vec3& normal) const;

	/** Get geometry for rendering */
	const PatchGeometry& getGeometry() const { return m_geometry; }

	/** Change the geometry */
	void    updateGeometry(const BoundingBox& box, bool normals);

	/** Recursivly call callback for each patch */
	int visitAllPatches(Landscape::PatchFunc callback);

	/** Recursively get the number of patches */
	int getCount() const;

	// Debug:
	void printInfo() const;
	
	protected:
	Landscape*  m_landscape;// Landscape patch is part of
	BoundingBox m_bounds;	// Patch AABB
	Patch* m_adjacent[4];	// Connected patches
	Patch* m_child[4];		// Child patches
	Patch* m_parent;		// Parent patch
	uint   m_depth;			// Patch level
	float  m_lod;			// Current patch LOD - [0-1]
	bool   m_split;			// Is this patch split
	float  m_error;			// Error amount for lod
	vec3   m_corner[4];		// Patch corners
	uint8  m_changed;		// Does the patch need reconnecting
	uint8  m_edge[4];		// Max lod per edge (cached from children)

	PatchGeometry  m_geometry; // Output geometry
	static const int stride = 6;


	protected:
	int getAdjacentStep(int side) const;
	void updateEdge(int edge);	// Update edge indices to connect to neighbouring patch
	bool getTriangle(uint index, vec3& a, vec3& b, vec3& c) const;
	bool intersectGeometry(const vec3& p, const vec3& d, float& t, vec3& normal) const;
	bool intersectGeometry(const vec3& p, float radius, const vec3& d, float& t, vec3& normal) const;

	float   projectError(const base::Camera* cam) const;
	int     minLOD() const;	// Get the minimum lod level this patch can be due to adjacent patches
	Patch*  getChild(int edge, int n) const; // get child patch n on an edge
	int     getOppositeEdge(int edge) const;
	int     splitAdjacent() const;
	void    cacheEdgeData();
	void    flagChanged();
	void    flagChanged(int edge);
};

}

