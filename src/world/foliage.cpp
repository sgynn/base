#include <base/world/foliage.h>
#include <base/drawablemesh.h>
#include <base/mesh.h>
#include <base/hardwarebuffer.h>
#include <cstdio>
#include <algorithm>

using base::Mesh;
using base::DrawableMesh;
using base::Material;
using namespace base;

class RNG {
	unsigned m_seed;
	public:
	RNG(unsigned seed=0) : m_seed(seed) {}
	unsigned rand() { m_seed = m_seed * 1103515245 + 12345; return m_seed&0x7fffffff; }
	float    randf() { return (float)rand() / (float)0x7fffffff; }
	float    randf(const Rangef& r) { return r.min + randf() * r.size(); }
};

// ===================================================================================================== //

FoliageMap::FoliageMap(int w, int h, const unsigned char* data, int stride, bool copy) : m_data(0), m_ref(0) {
	setData(w,h,data,stride,copy);
}
FoliageMap::~FoliageMap() {
	if(m_owned) delete [] m_data;
}
void FoliageMap::setData(int w, int h, const unsigned char* data, int stride, bool copy) {
	if(m_owned) delete [] m_data;
	if(copy) {
		int length = w * h;
		unsigned char* owned = new unsigned char[length];
		for(int i=0; i<length; ++i) owned[i] = data[i*stride];
		m_data = owned;
		m_stride = 1;
	}
	else {
		m_data = data;
		m_stride = stride;
		m_owned = false;
	}
	m_width = w;
	m_height = h;
}
float FoliageMap::getValue(float x, float y) const {
	x *= m_width - 1;
	y *= m_height - 1;
	x = x<0? 0: x>=m_width? m_width-1.0001: x;
	y = y<0? 0: y>=m_height? m_height-1.0001: y;
	// Values
	int ix = floor(x);
	int iy = floor(y);
	const unsigned char* k = m_data + (ix + iy * m_width) * m_stride;
	float a = k[0] * (ix-x+1) + k[m_width*m_stride] * (x-ix);
	float b = k[m_stride] * (ix-x+1) + k[m_width*m_stride+m_stride] * (x-ix);
	return (a * (iy-y+1) + b * (y-iy)) / 255.f;
}

// ===================================================================================================== //

FoliageLayer::FoliageLayer(float cs, float r) : m_parent(0), m_material(0), m_chunkSize(cs), m_range(r), m_density(1), m_scaleRange(1), m_densityMap(0) {
	m_heightRange.set(-1e8f, 1e8f);
	m_slopeRange.set(-1e8f, 1e8f);
}
FoliageLayer::~FoliageLayer() {
	if(m_parent && m_parent != getParent()) m_parent->removeLayer(this);
	deleteMap(m_densityMap);
	clear();
}

void FoliageLayer::setName(const char* name) { SceneNode::setName(name); }
void FoliageLayer::setViewRange(float r) { m_range = r; }
void FoliageLayer::setDensity(float d) { m_density = d; }
void FoliageLayer::setHeightRange(float min, float max) { m_heightRange.set(min, max); }
void FoliageLayer::setSlopeRange(float min, float max)  { m_slopeRange.set(min, max); }
void FoliageLayer::setScaleRange(float min, float max)  { m_scaleRange.set(min, max); }

void FoliageLayer::setCluster(int seed, float density, const Rangef& radius, float falloff) {
	m_clusterSeed = seed;
	m_clusterDensity = density;
	m_clusterRadius = radius;
	m_clusterFalloff = falloff;
}
void FoliageLayer::setClusterGap(const Rangef& r) { m_clusterGap = r; }
void FoliageLayer::setClusterShape(int points, float scale) {
	m_clusterShapeOctaves = points;
	m_clusterShapeMult = fmax(0, 1 - scale);
}

void FoliageLayer::setMaterial(Material* m) {
	m_material = m;
	for(auto& c: m_chunks) {
		if(c.second->drawable) c.second->drawable->setMaterial(m);
	}
}
void FoliageLayer::setMapBounds(const BoundingBox2D& b) { m_mapBounds = b; }
void FoliageLayer::setDensityMap(FoliageMap* map) {
	deleteMap(m_densityMap);
	m_densityMap = referenceMap(map);
	m_densityMap = map;
}

void FoliageLayer::clear() {
	for(auto& i:m_chunks) deleteChunk(i.second);
	m_chunks.clear();
}

void FoliageLayer::regenerate() {
	for(auto& i:m_chunks) {
		if(i.second->state != EMPTY) {
			detach(i.second->drawable);
			delete i.second->drawable;
			i.second->drawable = 0;
			destroyChunk(*i.second);
			i.second->state = EMPTY;
			m_parent->queueChunk(this, i.first, i.second);
		}
	}
}

FoliageMap* FoliageLayer::referenceMap(FoliageMap* map) {
	if(map) ++map->m_ref;
	return map;
}
void FoliageLayer::deleteMap(FoliageMap*& map) {
	if(map && --map->m_ref<=0) delete map;
	map = 0;
}

// ----------------------------------------------------------------------------------------------------- //

inline float FoliageLayer::getMapValue(const FoliageMap* map, const vec3& point) const {
	return m_parent->getMapValue(map, m_mapBounds, point);
}

int FoliageLayer::generatePoints(const Index& index, int count, vec3* corners, const vec3& up, PointList& out) const {
	bool testHeight = m_heightRange.isValid();
	bool testSlope = m_slopeRange.isValid();

	const vec3 offset(&getDerivedTransform()[12]); // Threading issue if we move the node when generating
	RNG rng( m_parent->getSeed(index, m_chunkSize) );
	vec3 t0, t1, pos;
	GenPoint point;
	float height;
	size_t start = out.size();
	out.reserve(start + count);
	for(int i=0; i<count; ++i) {
		float rx = rng.randf();
		float ry = rng.randf();
		t0 = lerp(corners[0], corners[1], rx);
		t1 = lerp(corners[2], corners[3], rx);
		pos = lerp(t0, t1, ry);
		pos += offset;

		m_parent->resolvePosition(pos, point.position, height);
		if(testHeight && !m_heightRange.contains(height)) continue;
		m_parent->resolveNormal(pos, point.normal);
		if(testSlope && !m_slopeRange.contains(1 - point.normal.dot(up))) continue;
		point.position -= offset;

		if(m_densityMap) {
			float d = getMapValue(m_densityMap, point.position);
			if(rng.randf() > d) continue;
		}

		out.push_back(point);
	}
	return out.size() - start;
}

int FoliageLayer::generatePoints(const Index& index, PointList& points, vec3& up) const {
	vec3 corners[4];
	RNG rng(m_parent->getSeed(index, m_chunkSize));

	GenPoint point;
	bool testHeight = m_heightRange.isValid();
	bool testSlope = m_slopeRange.isValid();
	auto addPointIfValid = [this, testHeight, testSlope, &point, &points, &up, &rng](const vec3& pos) {
		float height;
		const vec3& offset = *reinterpret_cast<const vec3*>(&getDerivedTransform()[12]);
		m_parent->resolvePosition(pos+offset, point.position, height);
		if(testHeight && !m_heightRange.contains(height)) return false;
		m_parent->resolveNormal(pos, point.normal);
		if(testSlope && !m_slopeRange.contains(1 - point.normal.dot(up))) return false;
		point.position -= offset;
		if(m_densityMap) {
			float d = getMapValue(m_densityMap, point.position);
			if(rng.randf() > d) return false;
		}
		points.push_back(point);
		return true;
	};


	size_t start = points.size();
	if(m_clusterDensity > 0) {
		float clusterAmount = m_clusterDensity * m_chunkSize * m_chunkSize;
		if(rng.randf() < clusterAmount-floor(clusterAmount)) ++clusterAmount;
		if(clusterAmount < 1) return 0;

		// Generate cluster points
		PointList clusters;
		m_parent->getCorners(index, m_chunkSize, corners, up);
		if(!generatePoints(index, (int)clusterAmount, corners, up, clusters)) return 0;

		// Generate the actual points around the clusters
		const vec3 dx = (corners[1] - corners[0]).normalise();
		const vec3 dz = (corners[2] - corners[0]).normalise();
		int shapePoints = m_clusterShapeMult<1 && m_clusterShapeOctaves>0? m_clusterShapeOctaves: 0;
		float* shape = shapePoints? new float[shapePoints+1]: nullptr;
		for(const GenPoint& centre: clusters) {
			for(int i=1; i<=shapePoints; ++i) shape[i] = rng.randf();
			if(shapePoints) shape[0] = shape[shapePoints];
			float min = rng.randf(m_clusterGap);
			float max = rng.randf(m_clusterRadius);
			float area = PI*max*max - PI*min*min;
			int amount = m_density * area;
			if(amount <= 0) return 0;
			float variance = max - min;
			vec3 pos;
			while(--amount>=0) {
				float angle = rng.randf() * TWOPI;
				float distance = min + rng.randf() * variance;
				if(shapePoints) {
					float t = shapePoints * angle / TWOPI;
					float a = shape[(int)floor(t)];
					float b = shape[(int)ceil(t)];
					distance *= flerp(a, b, t-floor(t)) * m_clusterShapeMult;
				}
				pos.set(distance * cos(angle), 0, distance * sin(angle));
				pos = centre.position + dx * pos.x + dz * pos.z;
				addPointIfValid(pos);
			}
		}
		delete [] shape;
		return points.size() - start;
	}
	else { // Uniform
		float amount = m_density * m_chunkSize * m_chunkSize;
		if(rng.randf() < amount-floor(amount)) ++amount;
		if(amount < 1) return 0;
		
		m_parent->getCorners(index, m_chunkSize, corners, up);
		return generatePoints(index, (int)amount, corners, up, points);
	}
}

// ----------------------------------------------------------------------------------------------------- //

void FoliageLayer::update(const vec3& context) {
	IndexList active;
	for(auto& ci: m_chunks) ci.second->active = false;
	vec3 localContext = getDerivedTransform().untransform(context);
	m_parent->getActive(localContext, m_chunkSize, m_range, active);
	// Activate any new chunks
	for(Index& index: active) {
		auto c = m_chunks.find(index);
		if(c == m_chunks.end()) {
			Chunk* chunk = new Chunk{0, Geometry{0,0,0}, EMPTY, false};
			c = m_chunks.insert( std::make_pair(index, chunk) ).first;
			m_parent->queueChunk(this, index, chunk);
		}
		c->second->active = true;
		// Completed
		if(c->second->state == GENERATED) {
			c->second->state = COMPLETE;
			Geometry& g = c->second->geometry;
			if(g.mesh) {
				g.mesh->getVertexBuffer()->createBuffer();
				g.mesh->getIndexBuffer()->createBuffer();
				DrawableMesh* d = c->second->drawable = new DrawableMesh(g.mesh, m_material);

				if(g.instances) {
					g.instances->createBuffer();
					d->setInstanceBuffer(g.instances);
					d->setInstanceCount(g.count);
				}
				attach(d);
			}
		}
	}
	// delete inactive chunks
	for(auto i=m_chunks.begin(); i!=m_chunks.end();) {
		if(!i->second->active) {
			if(i->second->drawable) detach(i->second->drawable);
			deleteChunk(i->second);
			i = m_chunks.erase(i);
		}
		else ++i;
	}
}

bool FoliageLayer::deleteChunk(Chunk* chunk) {
	if(m_parent->cancelChunk(chunk)) {
		delete chunk->drawable;
		destroyChunk(*chunk);
		delete chunk;
		return true;
	}
	return false;
}

void FoliageLayer::shift(const vec3& offset) {
	setPosition(getPosition() + offset);
	getDerivedTransformUpdated();
	// ToDo: cancel generating chunks
}

// ===================================================================================================== //

FoliageInstanceLayer::FoliageInstanceLayer(float cs, float r) : FoliageLayer(cs,r), m_mesh(0), m_alignMode(VERTICAL) {}
void FoliageInstanceLayer::setMesh(Mesh* mesh) { m_mesh = mesh; }
void FoliageInstanceLayer::setAlignment(OrientaionMode m, const Rangef& r) { m_alignMode = m; m_alignRange = r; }
FoliageLayer::Geometry FoliageInstanceLayer::generateGeometry(const Index& index) const {
	if(!m_mesh || !m_material) return Geometry{0,0,0};
	vec3 up;
	PointList points;
	generatePoints(index, points, up);
	if(points.empty()) return Geometry{0,0,0};
	static const vec3 unitY(0,1,0);
	Quaternion q, a, rot = Quaternion::arc(unitY, up);

	RNG rng(0);
	float* data = new float[ points.size() * 8 ]; // Instance buffer data
	float* vx = data;
	for(GenPoint& point: points) {
		float angle = rng.randf() * PI;
		a.w = cos(angle);
		a.y = sin(angle);

		switch(m_alignMode) {
		case VERTICAL: q = rot; break;
		case NORMAL:   q = Quaternion::arc( unitY, point.normal); break;
		case ABSOLUTE: q.fromAxis( up.cross(point.normal), rng.randf(m_alignRange) ); break;
		case RELATIVE: q = slerp(rot, Quaternion::arc( unitY, point.normal ), rng.randf(m_alignRange)); break;
		}
		q *= a;

		memcpy(vx+0, point.position, sizeof(vec3));
		vx[3] = rng.randf(m_scaleRange);
		vx[4] = q.x;
		vx[5] = q.y;
		vx[6] = q.z;
		vx[7] = q.w;
		vx += 8;
	}

	// Create instance buffer
	HardwareVertexBuffer* buffer = new HardwareVertexBuffer();
	buffer->setData(data, points.size(), 32, true);
	buffer->attributes.add(base::VA_CUSTOM, base::VA_FLOAT4, 0, "loc", 1);
	buffer->attributes.add(base::VA_CUSTOM, base::VA_FLOAT4, 16, "rot", 1);

	return Geometry{m_mesh, buffer, points.size()};
}

// ----------------------------------------------------------------------------------------------------- //

void FoliageInstanceLayer::removeItem(const FoliageItemRef& ref) {
	m_removedItems[ref.cell].push_back(ref.index);
	// flag changed
}
void FoliageInstanceLayer::removeItems(const Point& cell, const std::vector<uint16>& indices) {
	std::vector<uint16>& rm = m_removedItems[cell];
	rm.insert(rm.end(), indices.begin(), indices.end());
	// flag changed
}
void FoliageInstanceLayer::restoreItem(const FoliageItemRef& ref) {
	auto it = m_removedItems.find(ref.cell);
	if(it == m_removedItems.end()) return;
	for(size_t i=0; i<it->second.size(); ++i) {
		if(it->second[i] == ref.index) {
			it->second.erase(it->second.begin() + i);
			// flag changed
			break;
		}
	}
}

const std::vector<FoliageItemRef> FoliageInstanceLayer::getItems(const vec3& point, float radius, bool includeUnloaded) const {
	std::vector<FoliageItemRef> result;
	IndexList cells;
	PointList points;
	vec3 up;
	m_parent->getActive(point, m_chunkSize, radius, cells);
	for(const Index& cellIndex: cells) {
		auto it = m_chunks.find(cellIndex);
		if(it!=m_chunks.end() && it->second->state == COMPLETE) {
			result.reserve(result.size() + it->second->geometry.count);
			for(size_t i=0; i<it->second->geometry.count; ++i) {
				const float* vx = it->second->geometry.instances->getVertex(i);
				result.push_back({cellIndex, (uint16)i, vx, vx[3]});
				// FIXME: adjust index for removed items
			}
		}
		else if(includeUnloaded) {
			points.clear();
			generatePoints(cellIndex, points, up);
			result.reserve(result.size() + points.size());
			RNG rng(0); // Match generateGeometry to get scales
			for(size_t i=0; i<points.size(); ++i) {
				rng.rand();
				if(m_alignMode >= ABSOLUTE) rng.randf();
				float scale = rng.randf(m_scaleRange);
				result.push_back({cellIndex, (uint16)i, points[i].position, scale});
			}
		}
	}
	return result;
}




// ===================================================================================================== //

GrassLayer::GrassLayer(float cs, float r): FoliageLayer(cs,r), m_size(1,1), m_tiles(1), m_scaleMap(0) {}
GrassLayer::~GrassLayer() { deleteMap(m_scaleMap); }
void GrassLayer::setSpriteSize(float w, float h, int tile) {
	m_size.set(w,h);
	m_tiles = tile<1? 1: tile;
}
void GrassLayer::setScaleMap(FoliageMap* map, float min, float max) {
	deleteMap(m_scaleMap);
	m_scaleMap = referenceMap(map);
	m_scaleMapRange.set(min, max);
}
FoliageLayer::Geometry GrassLayer::generateGeometry(const Index& index) const {
	if(!m_material) return Geometry{0,0,0};
	vec3 up;
	PointList points;
	generatePoints(index, points, up);
	if(points.empty()) return Geometry{0,0,0};
	Quaternion rot = Quaternion::arc( vec3(0,1,0), up );
	float hw = m_size.x / 2;
	float lean = 0.4;
	up *= m_size.y;

	RNG rng(0);
	vec3 direction;
	float* vdata = new float[ points.size() * 4 * 11 ];
	unsigned short* idata = new unsigned short[ points.size() * 6 ];
	float* vx = vdata;
	unsigned short* ix = idata;
	unsigned short k = 0;
	for(GenPoint& point: points) {
		// Random direction
		float angle = rng.randf() * TWOPI;
		float s = rng.randf(m_scaleRange);
		if(m_scaleMap) {
			float ms = getMapValue(m_scaleMap, point.position);
			s *= m_scaleMapRange.min + ms * m_scaleMapRange.size();
		}

		direction.set(sin(angle)*hw*s, 0, cos(angle)*hw*s);
		direction = rot * direction;
		vec3 top = up * s + vec3(direction.z*lean,0,-direction.x*lean);
		// Positiion
		memcpy(vx+0,  point.position - direction,       sizeof(vec3));
		memcpy(vx+11, point.position - direction + top, sizeof(vec3));
		memcpy(vx+22, point.position + direction + top, sizeof(vec3));
		memcpy(vx+33, point.position + direction,       sizeof(vec3));
		// Normal
		memcpy(vx+3,  point.normal, sizeof(vec3));
		memcpy(vx+14, point.normal, sizeof(vec3));
		memcpy(vx+25, point.normal, sizeof(vec3));
		memcpy(vx+36, point.normal, sizeof(vec3));
		// Tangent
		memcpy(vx+6,  direction, sizeof(vec3));
		memcpy(vx+17, direction, sizeof(vec3));
		memcpy(vx+28, direction, sizeof(vec3));
		memcpy(vx+39, direction, sizeof(vec3));
		// UVs
		vx[9]  = vx[20] = vx[21] = vx[32] = 0;
		vx[10] = vx[31] = vx[42] = vx[43] = 1;
		// Index
		ix[0] = ix[3] = k;
		ix[1] = k+1;
		ix[2] = ix[4] = k+2;
		ix[5] = k+3;
		// Next
		k += 4;
		ix += 6;
		vx += 44;
	}

	// Build buffer objects
	HardwareVertexBuffer* vbuffer = new HardwareVertexBuffer();
	vbuffer->attributes.add(VA_VERTEX, VA_FLOAT3);
	vbuffer->attributes.add(VA_NORMAL, VA_FLOAT3);
	vbuffer->attributes.add(VA_TANGENT, VA_FLOAT3);
	vbuffer->attributes.add(VA_TEXCOORD, VA_FLOAT2);
	vbuffer->setData(vdata, points.size()*4, vbuffer->attributes.getStride(), true);

	HardwareIndexBuffer* ibuffer = new HardwareIndexBuffer();
	ibuffer->setData(idata, points.size()*6, true);

	// Drawable
	Mesh* mesh = new Mesh();
	mesh->setPolygonMode(base::PolygonMode::TRIANGLES);
	mesh->setVertexBuffer(vbuffer);
	mesh->setIndexBuffer(ibuffer);
	return Geometry{ mesh, nullptr, 0 };
}

void GrassLayer::destroyChunk(Chunk& chunk) const {
	if(chunk.geometry.mesh) {
		delete chunk.geometry.mesh;
		chunk.geometry.mesh = 0;
	}
}


// ===================================================================================================== //


FoliageSystem::FoliageSystem(int threads) : m_threads(0), m_threadCount(threads), m_running(true) {
	if(threads) m_threads = new Thread[threads];
	for(int i=0; i<threads; ++i) m_threads[i].begin(this, &FoliageSystem::threadFunc, i);
}
FoliageSystem::~FoliageSystem() {
	m_running = false;
	for(int i=0; i<m_threadCount; ++i) m_threads[i].join();
	for(FoliageLayer* layer: m_layers) {
		delete layer;
	}
	delete [] m_threads;
}
void FoliageSystem::addLayer(FoliageLayer* l) {
	l->m_parent = this;
	m_layers.push_back(l);
	addChild(l);
}

void FoliageSystem::removeLayer(FoliageLayer* l) {
	removeChild(l);
	l->m_parent = nullptr;
	for(size_t i=0; i<m_layers.size(); ++i) {
		if(m_layers[i] == l) {
			m_layers[i] = m_layers.back();
			m_layers.pop_back();
			break;
		}
	}
}

void FoliageSystem::update(const vec3& context) {
	if(!m_sorted) {
		MutexLock scopedLock(m_mutex);
		std::sort(m_queue.begin(), m_queue.end(), [context](const GenChunk& a, const GenChunk& b) { return a.centre.distance2(context) > b.centre.distance2(context); });
		m_sorted = true;
	}
	// Single thread version
	if(!m_threads) {
		for(int i=0; i<10 && !m_queue.empty(); ++i) {
			m_queue.back().chunk->geometry = m_queue.back().layer->generateGeometry(m_queue.back().index);
			m_queue.back().chunk->state = FoliageLayer::GENERATED;
			m_queue.pop_back();
		}
	}

	for(FoliageLayer* layer : m_layers) layer->update(context);
}


// ----------------------------------------------------------------------------------------------------- //

void FoliageSystem::queueChunk(FoliageLayer* layer, const Index& index, FoliageLayer::Chunk* chunk) {
	MutexLock scopedLock(m_mutex);
	static vec3 corners[5];
	getCorners(index, layer->m_chunkSize, corners, corners[4]);
	vec3 centre = (corners[0] + corners[2]) * 0.5;
	m_queue.push_back( GenChunk { layer, index, chunk, centre } );
	m_sorted = false;
}
bool FoliageSystem::cancelChunk(FoliageLayer::Chunk* chunk) {
	if(chunk->state > FoliageLayer::GENERATING) return true;
	if(chunk->state == FoliageLayer::GENERATING) return false;
	MutexLock scopedLock(m_mutex);
	for(size_t i=0; i<m_queue.size(); ++i) {
		if(m_queue[i].chunk == chunk) {
			m_queue[i] = m_queue.back();
			m_queue.pop_back();
			return true;
		}
	}
	return true;
}
void FoliageSystem::threadFunc(int index) {
	GenChunk current{0,Index(),0};
	while(m_running) {
		m_mutex.lock();
		if(m_queue.empty()) current.chunk = 0;
		else {
			current = m_queue.back();
			m_queue.pop_back();
			current.chunk->state = FoliageLayer::GENERATING;
		}
		m_mutex.unlock();

		// Generate this chunk
		if(current.chunk) {
			current.chunk->geometry = current.layer->generateGeometry(current.index);
			current.chunk->state = FoliageLayer::GENERATED;
		}
		else Thread::sleep(1);
	}
}

// ----------------------- Default interface --------------------------- //


int FoliageSystem::getActive(const vec3& p, float cs, float range, IndexList& out) const {
	Point a( floor((p.x-range)/cs), floor((p.z-range)/cs) );
	Point b( ceil((p.x+range)/cs), ceil((p.z+range)/cs) );
	for(Point r=a; r.x<b.x; ++r.x) for(r.y=a.y; r.y<b.y; ++r.y) out.push_back(r);
	return out.size();
}

void FoliageSystem::getCorners(const Index& ix, float cs, vec3* out, vec3& up) const {
	up.set(0,1,0);
	out[0].set(ix.x*cs,    0, ix.y*cs);
	out[1].set(ix.x*cs+cs, 0, ix.y*cs);
	out[2].set(ix.x*cs,    0, ix.y*cs+cs);
	out[3].set(ix.x*cs+cs, 0, ix.y*cs+cs);
}

float FoliageSystem::getMapValue(const FoliageMap* map, const BoundingBox2D& bounds, const vec3& pos) const {
	vec2 p = (pos.xz() - bounds.min) / bounds.size();
	return map->getValue(p.x, p.y);
}

unsigned FoliageSystem::getSeed(const Point& index, float size) const {
	return index.x * 54321 + index.y + 7126;
}

