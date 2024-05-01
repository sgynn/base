#pragma once

// Generic heightmap procedural foliage

#include <base/bounds.h>
#include <base/point.h>
#include <base/thread.h>
#include <base/scene.h>
#include <vector>
#include <map>

// Windows being infuriating again. ToDo: perhaps use an enum class.
#undef ABSOLUTE
#undef RELATIVE

namespace base { class Material; class DrawableMesh; }
namespace base {  class Mesh;  class HardwareVertexBuffer; }
class FoliageSystem;

class FoliageMap {
	public:
	FoliageMap(int w, int h, unsigned char* data=0, int stride=1);
	~FoliageMap();
	void  setData(int w, int h, unsigned char* data, int stride=1);
	float getValue(float x, float y) const; // Works on normalised 0-1 values
	protected:
	int m_width;
	int m_height;
	unsigned char* m_data;
	private:
	friend class FoliageLayer;
	int m_ref;
};

// For getting tree locations
struct FoliageItemRef {
	Point cell;
	uint index;
	vec3 point;
};

// -------------------------------------------------------------------------------------- //

class FoliageLayer : protected base::SceneNode {
	public:
	FoliageLayer(float chunkSize, float range);
	virtual ~FoliageLayer();
	virtual void update(const vec3& context);

	void setViewRange(float); // set visible distance from context
	void setHeightRange(float, float);
	void setSlopeRange(float, float);
	void setScaleRange(float, float);
	void setDensity(float);
	void setMaterial(base::Material*);
	void setMapBounds(const BoundingBox2D& bounds);
	void setDensityMap(FoliageMap*);
	void clear();
	void regenerate();
	void regenerate(const vec3& centre, float radius);
	void shift(const vec3& offset);
	void setName(const char* name);

	// Allow removing individual items - chopping down trees etc
	const std::vector<FoliageItemRef> getItems(const vec3& point, float radius) const;
	void removeItems(const Point& cell, const std::vector<int>& indices);
	void removeItem(const FoliageItemRef& item);
	void restoreItem(const FoliageItemRef& item);

	protected:
	friend class FoliageSystem;
	FoliageSystem*   m_parent;
	base::Material*  m_material;
	float            m_chunkSize;
	float            m_range;
	float            m_density;
	Rangef           m_heightRange;
	Rangef           m_slopeRange;
	Rangef           m_scaleRange;
	BoundingBox2D    m_mapBounds;
	FoliageMap*      m_densityMap;

	protected:
	typedef Point Index;
	typedef std::vector<Index> IndexList;
	enum ChunkState { EMPTY, GENERATING, GENERATED, COMPLETE };
	struct Geometry { base::Mesh* mesh; base::HardwareVertexBuffer* instances; size_t count; };
	struct Chunk { base::DrawableMesh* drawable; Geometry geometry; ChunkState state; bool active; };
	std::map<Index, Chunk*> m_chunks;
	std::map<Index, std::vector<uint16>> m_removedItems;

	protected:
	struct GenPoint { vec3 position, normal; };
	typedef std::vector<GenPoint> PointList;
	float getMapValue(const FoliageMap* map, const vec3& point) const;
	int generatePoints(const Index& index, int count, vec3* corners, const vec3& up, PointList& out) const;
	int generatePoints(const Index& index, PointList& points, vec3& up) const;
	virtual Geometry generateGeometry(const Point&) const = 0;
	virtual void destroyChunk(Chunk&) const {};
	void regenerateChunk(const Index&, Chunk&);
	bool deleteChunk(Chunk*);
	void deleteMap(FoliageMap*&);
	FoliageMap* referenceMap(FoliageMap*);
};

// -------------------------------------------------------------------------------------- //

class FoliageInstanceLayer : public FoliageLayer {
	public:
	enum OrientaionMode { VERTICAL, NORMAL, ABSOLUTE, RELATIVE };
	FoliageInstanceLayer(float chunkSize, float range);
	void setMesh(base::Mesh*);
	void setAlignment(OrientaionMode mode, const Rangef& range=0);
	protected:
	virtual Geometry generateGeometry(const Index& page) const override;
	protected:
	base::Mesh* m_mesh;
	Rangef              m_alignRange; // RELATIVE: lerp range between normal and up vector, ABSOLUTE: lerp between sideways and up.
	OrientaionMode      m_alignMode;
};

// -------------------------------------------------------------------------------------- //

class GrassLayer : public FoliageLayer {
	public:
	GrassLayer(float chunkSize, float range);
	~GrassLayer();
	void setSpriteSize(float w, float h, int tiles=1);
	void setScaleMap(FoliageMap* map, float low=0, float high=1);
	protected:
	virtual Geometry generateGeometry(const Index& page) const override;
	virtual void destroyChunk(Chunk&) const override;
	protected:
	vec2        m_size;
	int         m_tiles;
	FoliageMap* m_scaleMap;
	Rangef      m_scaleMapRange;
};

// -------------------------------------------------------------------------------------- //



class FoliageSystem : public base::SceneNode {
public:
	friend class FoliageLayer;
	typedef FoliageLayer::Index Index;
	typedef FoliageLayer::IndexList IndexList;

	FoliageSystem(int threads=0);
	virtual ~FoliageSystem();
	virtual void update(const vec3& context);
	virtual void addLayer(FoliageLayer* layer);
	virtual void removeLayer(FoliageLayer* layer);

	typedef std::vector<FoliageLayer*>::const_iterator FoliageLayerIterator;
	FoliageLayerIterator begin() const { return m_layers.begin(); }
	FoliageLayerIterator end() const { return m_layers.end(); }

protected:
	/// Get the list of active chunks from a context
	virtual int getActive(const vec3& context, float chunkSize, float range, IndexList& out) const;
	/// Get the four corners of a chunk. Foliage points will use interpolated values from these corners. Corners should be at height=0
	virtual void  getCorners(const Index&, float chunkSize, vec3* out, vec3& up) const;
	/// Resolve the position and height of a generated point
	virtual void resolvePosition(const vec3& point, vec3& position, float& height) const = 0;
	/// Resolve the normal and slope of a generated point
	virtual void resolveNormal(const vec3& point, vec3& normal) const = 0;
	/// Get a value from a texture map
	virtual float getMapValue(const FoliageMap* map, const BoundingBox2D& bounds, const vec3& position) const;
	/// Random seed for chunk
	virtual unsigned getSeed(const Index&, float size) const;

private:
	void queueChunk(FoliageLayer*, const FoliageLayer::Index&, FoliageLayer::Chunk*);
	bool cancelChunk(FoliageLayer::Chunk*);

protected:
	std::vector<FoliageLayer*> m_layers;
	
	
private:
	base::Thread* m_threads;
	base::Mutex   m_mutex;
	int     m_threadCount;
	bool    m_running;
	struct GenChunk { FoliageLayer* layer; FoliageLayer::Index index; FoliageLayer::Chunk* chunk; };
	std::vector<GenChunk> m_queue;
	void threadFunc(int index);
};

