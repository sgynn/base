#include <base/world/landscape.h>
#include <base/camera.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <base/collision.h>
#include <assert.h>

using namespace base;

Landscape::Landscape(float size, const vec3& pos): m_position(pos), m_size(size) {
	m_min         = 0;
	m_max         = 32;
	m_patchLimit  = 1000000;
	m_patchSize   = 9;
	m_patchStep   = 3;
	m_threshold   = 8.f;
	m_root        = nullptr;

	m_selected = nullptr; // Debug
}

Landscape::~Landscape() {
	delete m_root;
}

void Landscape::setHeightFunction( HeightFunc func) {
	m_func = func;
	// Create root here as it needs to be called AFTER HeightFunc is set
	if(!m_root) {
		m_root = new Patch(this);
		m_root->create();
		m_root->build();
	}
	else {
		printf("Warning: Terrain height function changed\n");
	}
}

void Landscape::setPatchCallbacks(PatchFunc create, PatchFunc destroy, PatchFunc updated) {
	if(m_root) printf("Warning: Callbacks set after terrain geometry initialised\n");
	m_createCallback = create;
	m_destroyCallback = destroy;
	m_updateCallback = updated;
}

void Landscape::setLimits(int min, int max, int count) {
	m_min = min;
	m_max = max;
	m_patchLimit = count;
}

void Landscape::setThreshold(float v) {
	m_threshold = v;
}

void Landscape::connect(Landscape* land, int side) {
	if(land) m_root->setAdjacent(land->m_root, side);
	else m_root->clearAdjacent(side);
}

void Landscape::update(const Camera* cam) {
	// Synchronise generation thread
	assert(m_root && "Height function no set up");

	auto SplitCmp = [cam](const Patch* a, const Patch* b) {
		float ea = a->projectError(cam) + (cam->onScreen(a->m_bounds)? 100: 0);
		float eb = b->projectError(cam) + (cam->onScreen(b->m_bounds)? 100: 0);
		return ea>eb;
	};
	
	// Sort split queue
	std::sort(m_splitQueue.begin(), m_splitQueue.end(), SplitCmp);
	for(uint i=0,r=0; i<m_splitQueue.size() && r<10; ++i) r+=m_splitQueue[i]->split();
	for(uint i=0; i<m_mergeQueue.size() && i<10; ++i) m_mergeQueue[i]->merge();
	m_splitQueue.clear();
	m_mergeQueue.clear();
	
	// optimise patches (recursive)
	m_root->update(cam);

	// Build any index arrays
	for(uint i=0; i<m_buildList.size(); ++i) {
		if(m_buildList[i]) m_buildList[i]->updateEdges();
	}
	m_buildList.clear();
}
int Landscape::cull(const Camera* cam) {
	m_geometryList.clear();
	m_root->collect(cam, m_geometryList, 0x7e);
	return m_geometryList.size();
}


int Landscape::visitAllPatches(PatchFunc f) const {
	return m_root->visitAllPatches(f);
}


float Landscape::getHeight(const vec3& pos, bool real) const {
	return real? m_func(pos): m_root->getHeight(pos.x, pos.z, 0);
}
float Landscape::getHeight(float x, float z, bool real) const {
	return real? m_func(vec3(x,0,z)): m_root->getHeight(x, z, 0);
}
float Landscape::getHeight(float x, float z, vec3& normal, bool real) const {
	return real? m_func(vec3(x,0,z)): m_root->getHeight(x, z, &normal);
}

bool Landscape::intersect(const vec3& start, float radius, const vec3& direction, float& t, vec3& normal) const {
	return m_root->intersect(start, radius, direction, t, normal);
}
bool Landscape::intersect(const vec3& start, const vec3& direction, float& t, vec3& normal) const {
	return intersect(start, 0, direction, t, normal);
}
bool Landscape::intersect(const vec3& a, const vec3& b, vec3& point, vec3& normal) const {
	vec3 dir = b-a;
	float t = dir.normaliseWithLength();
	if(intersect(a, 0, dir, t, normal)) {
		point = a + dir * t;
		return 1;
	}
	return 0;
}

void Landscape::updateGeometry(const BoundingBox& box, bool normals) {
	m_root->updateGeometry(box, normals);
}


Landscape::Info Landscape::getInfo() const {
	Info info;
	info.patches        = m_root->getCount();
	info.visiblePatches = m_geometryList.size();
	info.splitQueue     = m_splitQueue.size();
	info.mergeQueue     = m_mergeQueue.size();
	info.triangles      = 0;
	for(uint i=0; i<m_geometryList.size(); ++i) info.triangles += m_geometryList[i]->indexCount-2;
	return info;
}
int Patch::getCount() const {
	int r = 1;
	if(m_split) for(int i=0; i<4; ++i) r+=m_child[i]->getCount();
	return r;
}


//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// 
//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //// 



Patch::Patch(Landscape* land) : m_landscape(land)
	, m_adjacent{0,0,0,0}, m_child{0,0,0,0}, m_parent(nullptr)
	, m_depth(0), m_lod(0), m_split(false), m_error(0)
	, m_changed(0), m_edge{0,0,0,0}
{
	m_lod = 0;
	m_error = 0;

	float s = land->m_size;
	m_corner[0] = land->m_position;
	m_corner[1] = land->m_position + vec3(s,0,0);
	m_corner[2] = land->m_position + vec3(0,0,s);
	m_corner[3] = land->m_position + vec3(s,0,s);
}

Patch::Patch(Patch* parent, int index) : m_landscape(parent->m_landscape)
	, m_adjacent{0,0,0,0}, m_child{0,0,0,0}, m_parent(parent)
	, m_depth(0), m_lod(0), m_split(false), m_error(0)
	, m_changed(0), m_edge{0,0,0,0}
{
	m_depth  = parent->m_depth + 1;

	// Calculate corners from parent
	const vec3* c = parent->m_corner;
	m_corner[index] = parent->m_corner[index];
	m_corner[index^1] = (c[index] + c[index^1]) * 0.5f;
	m_corner[index^2] = (c[index] + c[index^2]) * 0.5f;
	m_corner[index^3] = (c[0] + c[1] + c[2] + c[3]) * 0.25f;

	m_geometry.tag = 0;
	m_lod = 0;
	m_error = 0;
}

Patch::~Patch() {
	if(m_landscape->m_destroyCallback) m_landscape->m_destroyCallback(&m_geometry);
	if(m_geometry.vertices) delete [] m_geometry.vertices;
	if(m_geometry.indices) delete [] m_geometry.indices;
	if(m_landscape->m_selected==this) m_landscape->m_selected = 0; // Debug
}

//////////////////////////////////////////////////////////////////////

void Patch::flagChanged() {
	if(m_split) for(int i=0; i<4; ++i) m_child[i]->flagChanged();
	else        for(int i=0; i<4; ++i) flagChanged(i);
}

void Patch::flagChanged(int edge) {
	if(m_split) {
		getChild(edge, 0)->flagChanged(edge);
		getChild(edge, 1)->flagChanged(edge);
	} else {
		if(!m_changed) m_landscape->m_buildList.push_back(this);
		if(~m_changed & (1<<edge)) {
			m_changed |= 1<<edge;
			if(m_adjacent[edge]) m_adjacent[edge]->flagChanged( getOppositeEdge(edge) );
		}
	}
}

void Patch::update(const Camera* cam) {
	const float maxError = m_landscape->m_threshold;
	if(m_split) {
		// Count children above merge threshold
		int m = 0;
		for(int i=0; i<4; ++i) {
			m_child[i]->update(cam);
			if(!m_child[i]->m_split && m_child[i]->m_lod <= 0) ++m;
		}
		// Merge
		if(m==4) {
			float error = projectError(cam);
			if(error < maxError && m_depth>m_landscape->m_min) {
				m_landscape->m_mergeQueue.push_back(this);
			}
		}
	} else {
		// Calculate error
		float error = projectError(cam);
		float parentError = m_parent? m_parent->projectError(cam): error;
		if(parentError<=error) parentError = error*1.001;
		float target = error==parentError? 1: (maxError-parentError) / (error-parentError);

		// Force split if less than min
		if(m_depth < m_landscape->m_min) target = 1.0;

		// Queue patch for splitting?
		if(target >= 1.0 && m_depth<m_landscape->m_max) {
			m_landscape->m_splitQueue.push_back(this);
		}
		m_lod = target;
	}
}
void Patch::collect(const Camera* cam, Landscape::GList& list, int cullFlags) const {
	int cf = cam->onScreen(m_bounds, cullFlags);
	if(cf==0) return;
	if(m_split) {
		for(int i=0; i<4; ++i) m_child[i]->collect(cam, list, cf);
	} else list.push_back(&m_geometry);
}

int Patch::visitAllPatches(Landscape::PatchFunc f) {
	// Recurse to children
	int count = 1;
	if(m_split) {
		for(int i=0; i<4; ++i) count += m_child[i]->visitAllPatches(f);
	}
	// Call callback
	f(&m_geometry);
	return count;
}

float Patch::projectError(const Camera* cam) const {
	const vec3 centre = m_bounds.centre();
	//float d = cam->getDirection().dot( cam->getPosition() - centre );
	float d = cam->getPosition().distance(centre);
	
	// Closest point on aabb
	//vec3 cp = cam->getPosition();
	//for(int i=0; i<3; ++i) cp[i] = fmax(fmin(cp[i], m_bounds.max[i]), m_bounds.min[i]);
	//float d = cam->getPosition().distance(cp);


	const Matrix& prj = cam->getProjection();
	float p = prj[5] / d;	// 1/tan(fov/2) / d
	float r = (p*0.5) * 768;

	return r * m_error;
}

//////////////////////////////////////////////////////////////////////

inline Patch* Patch::getChild(int edge, int n) const {
	static const int e[8] = { 0,1,0,2,  2,3,1,3 };
	return m_child[ e[n*4+edge] ];
}
inline int Patch::getOppositeEdge(int side) const {
	return side^1;
}

void Patch::setAdjacent(Patch* p, int side) {
	m_adjacent[side] = p;
	int opp = getOppositeEdge(side);
	assert(p && !p->m_adjacent[opp]); // Error - probably connecting wrong side
	p->m_adjacent[opp] = this;
	if(p->m_parent!=m_parent) p->flagChanged();
	// Recurse to child patches on this side
	if(m_split && p && p->m_split) {
		getChild(side, 0)->setAdjacent(p->getChild(opp, 0), side);
		getChild(side, 1)->setAdjacent(p->getChild(opp, 1), side);
	}
}

void Patch::clearAdjacent(int side) {
	if(m_adjacent[side]) {
		Patch* p = m_adjacent[side];
		m_adjacent[side] = 0;
		p->m_adjacent[ getOppositeEdge(side) ] = 0;
		if(m_split) {
			getChild(side, 0)->clearAdjacent(side);
			getChild(side, 1)->clearAdjacent(side);
		}
	}
}



// Split patch
int Patch::split() {
	if(m_split) return 0;
	m_split = true;
	// Create child patches
	m_child[0] = new Patch(this, 0);
	m_child[1] = new Patch(this, 1);
	m_child[2] = new Patch(this, 2);
	m_child[3] = new Patch(this, 3);
	// internal adjacency
	m_child[0]->setAdjacent(m_child[1], 1);
	m_child[0]->setAdjacent(m_child[2], 3);
	m_child[3]->setAdjacent(m_child[1], 2);
	m_child[3]->setAdjacent(m_child[2], 0);
	// External adjacency
	for(int i=0; i<4; ++i) {
		if(m_adjacent[i] && m_adjacent[i]->m_split) {
			int edge = getOppositeEdge(i);
			getChild(i,0)->setAdjacent( m_adjacent[i]->getChild(edge,0), i);
			getChild(i,1)->setAdjacent( m_adjacent[i]->getChild(edge,1), i);
		}
	}

	// Update cached edge data and bounding boxes
	for(Patch* p=this; p; p=p->m_parent) {
		p->cacheEdgeData();
		p->m_bounds.include( m_bounds );
	}

	// Split any adjacent patched that require splitting to be valid
	int a = splitAdjacent();

	// Create patch geometry
	for(int i=0; i<4; ++i) {
		m_child[i]->create();
		m_child[i]->build();
	}

	return a+1;
}

// Split adjacent patches if they need splitting to connect to this patch
int Patch::splitAdjacent() const {
	int count = 0;
	int min = m_depth - m_landscape->m_patchStep + 1; // Minimum lod adjacent nodes can have
	for(int i=0; i<4;) {
		// Get adjacent patch
		Patch* adjacent = 0;
		for(const Patch* p=this; p&&!adjacent; p=p->m_parent) adjacent=p->m_adjacent[i];
		// Do we need to split it?
		if(adjacent && (int)adjacent->m_depth < min) {
			count += adjacent->split();
		} else ++i;
	}
	return count;
}

void Patch::cacheEdgeData() {
	if(m_split) {
		m_edge[0] = fmax(m_child[0]->m_edge[0], m_child[2]->m_edge[0]);
		m_edge[1] = fmax(m_child[1]->m_edge[1], m_child[3]->m_edge[1]);
		m_edge[2] = fmax(m_child[0]->m_edge[2], m_child[1]->m_edge[2]);
		m_edge[3] = fmax(m_child[2]->m_edge[3], m_child[3]->m_edge[3]);
	} else m_edge[0] = m_edge[1] = m_edge[2] = m_edge[3] = m_depth;
}

template<typename T> bool removeItem(typename std::vector<T> list, const T& item) {
	for(typename std::vector<T>::iterator i=list.begin(); i!=list.end(); ++i) {
		if(*i == item) { list.erase(i); return true; }
	}
	return false;
}

int Patch::merge() {
	if(!m_split) return 0;
	// Make sure children are not split
	for(int i=0; i<4; ++i) if(m_child[i]->m_split) return 0;
	// Check adjacent m_edge values to validate merge
	int max = m_depth + m_landscape->m_patchStep;
	for(int i=0; i<4; ++i) {
		if(m_adjacent[i] && m_adjacent[i]->m_edge[ getOppositeEdge(i) ] >= max) return 0;
	}
	// Clear adjacencies
	for(int i=0; i<4; ++i) {
		if(m_adjacent[i] && m_adjacent[i]->m_split) {
			int edge = getOppositeEdge(i);
			m_adjacent[i]->getChild(edge,0)->m_adjacent[edge] = 0;
			m_adjacent[i]->getChild(edge,1)->m_adjacent[edge] = 0;
		}
	}
	// Remove children
	for(int i=0; i<4; ++i) {
		// remove from any lists
		//removeItem(m_landscape->m_buildList, m_child[i]);
		for(uint j=0; j<m_landscape->m_buildList.size(); ++j) {
			if(m_landscape->m_buildList[j]==m_child[i]) m_landscape->m_buildList[j]=0;
		}
		
		delete m_child[i];
		m_child[i] = 0;
	}
	m_split = false;
	for(Patch* p=this; p; p=p->m_parent) p->cacheEdgeData();
	m_lod = 1.0;
	flagChanged();
	return 1;
}


//// //// Vertex buffer creation //// ////


void Patch::create() {
	// Vertex format: position:3, normal:3
	int size = m_landscape->m_patchSize;
	vec3 step = (m_corner[3] - m_corner[0]) / (size-1);

	float* vx = new float[ size * size * stride ];
	m_error = step.x * 0.1;	// factor resolution into error value
	vec3 point;

	int index = 0;
	if(m_parent) for(index=0; index<4; ++index) if(m_parent->m_child[index]==this) break;
	Landscape::HeightFunc func = m_landscape->m_func;
	
	// Create vertices
	for(int x=0; x<size; ++x) {
		point.x = m_corner[0].x + x*step.x;
		for(int y=0; y<size; ++y) {
			float* v = vx + (x + y*size)*stride;
			// Calculate ground position
			point.z = m_corner[0].z + y*step.z;

			v[0] = point.x;
			v[2] = point.z;

			// Copy Get data from parent
			if(m_parent && !((x&1) || (y&1))) {
				int pk = x/2 + (index&1? size/2: 0) + (y/2 + (index&2? size/2: 0)) * size;
				v[1] = m_parent->m_geometry.vertices[pk*stride+1];
			}
			else {
				v[1] = func(point);
			}

			// Update bounds
			if(x+y)	m_bounds.include( vec3(point.x, v[1], point.z) );
			else m_bounds.min = m_bounds.max = vec3(point.x, v[1], point.z);
		}
	}

	// Calculate normals
	vec3 n[6], normal;
	int s = size-1;
	size_t ss = size * stride;
	static const vec3 base[6] = { vec3(0,0,-1), vec3(1,0,-1), vec3(1,0,0), vec3(0,0,1), vec3(-1,0,1), vec3(-1,0,0) };
	for(int i=0; i<6; ++i) n[i] = base[i] * step;
	for(int x=0; x<size; ++x) {
		for(int y=0; y<size; ++y) {
			int k = (x+y*size)*stride + 1;
			float h = vx[k];

			// Get connected vertices
			n[0].y = (y>0?      vx[k-ss]:        func( vec3(vx+k-1) + base[0]*step )) - h;
			n[1].y = (y>0&&x<s? vx[k+stride-ss]: func( vec3(vx+k-1) + base[1]*step )) - h;
			n[2].y = (x<s?      vx[k+stride]:    func( vec3(vx+k-1) + base[2]*step )) - h;
			n[3].y = (y<s?      vx[k+ss]:        func( vec3(vx+k-1) + base[3]*step )) - h;
			n[4].y = (x>0&&y<s? vx[k+ss-stride]: func( vec3(vx+k-1) + base[4]*step )) - h;
			n[5].y = (x>0?      vx[k-stride]:    func( vec3(vx+k-1) + base[5]*step )) - h;

			// combine
			normal.x = normal.y = normal.z = 0;
			for(int i=0; i<6; ++i) normal += n[i].cross( n[(i+1)%6] );
			normal.normalise();

			// Write to vertex array
			vx[k+2] = -normal.x;
			vx[k+3] = -normal.y;
			vx[k+4] = -normal.z;
		}
	}

	// Interpolate vertices to get error value
	for(int x=0; x<size; ++x) {
		for(int y=0; y<size; ++y) {
			float* v = vx + (x + y*size)*stride;
			float* a=0;
			float* b=0;

			// get interpolants
			if((x&1) && (y&1)) {
				a = vx + (x-1 + (y-1)*size) * stride;
				b = vx + (x+1 + (y+1)*size) * stride;
			}
			else if(x&1) {
				a = vx + (x-1 + y*size) * stride;
				b = vx + (x+1 + y*size) * stride;
			}
			else if(y&1) {
				a = vx + (x + (y-1)*size) * stride;
				b = vx + (x + (y+1)*size) * stride;
			}
			
			// Interpolated height
			if(a&&b) {
				float mid = (a[1] + b[1]) * 0.5;
				m_error = fmax( fabs(mid-v[1]), m_error);
			}
		}
	}

	m_geometry.vertexCount = size * size;
	m_geometry.vertices = vx;
	m_geometry.bounds = &m_bounds;
}





//// //// Index Array Generation //// ////




// Build index array
void Patch::build() {
	int size = m_landscape->m_patchSize;

	// Allocate array
	int count = (size * 2 + 4) * (size-1) - 4;
	if(m_geometry.indices) delete [] m_geometry.indices;
	m_geometry.indices = new uint16[ count ];
	m_geometry.indexCount = count;

	// Generate indices
	//for(uint i=0; i<strips; ++i) addStrip(i, step);
	

	uint16* ix = m_geometry.indices;
	for(int y=0; y<size-1; ++y) {
		// connect to previous strip
		if(y>0) {
			ix[0] = ix[1] = ix[-1];
			ix[2] = ix[3] = y*size;
			ix += 4;
		}

		// Create strip
		for(int x=0; x<size; ++x, ix+=2) {
			ix[0] = x + y*size;
			ix[1] = x + (y+1)*size;
		}
	}

	if(m_landscape->m_createCallback) m_landscape->m_createCallback(&m_geometry);
	flagChanged();
}

void Patch::updateEdges() {
	if(m_changed&1) updateEdge(0);
	if(m_changed&2) updateEdge(1);
	if(m_changed&4) updateEdge(2);
	if(m_changed&8) updateEdge(3);
	if(m_changed && m_landscape->m_updateCallback)
		m_landscape->m_updateCallback(&m_geometry);
	m_changed = 0;
}

void Patch::updateEdge(int edge) {
	int size = m_landscape->m_patchSize;
	int rowSize = size * 2 + 4;
	int s = getAdjacentStep(edge);
	s = s>0? 1<<s: 1;

	switch(edge) {
	case 0:
		for(int i=1; i<size-1; ++i) {
			int v = i%s<=s/2? i/s*s: (i/s+1)*s;
			uint16* ix = m_geometry.indices + i*rowSize;
			*ix = v * size;
			ix[1-rowSize] = ix[-1] = ix[-2] = *ix;
		} break;
	case 1:
		for(int i=1; i<size-1; ++i) {
			int v = s==1? i: i%s<s/2? i/s*s: (i/s+1)*s;
			uint16* ix = m_geometry.indices + i*rowSize + rowSize-6;
			*ix = size-1 + v * size;
			ix[1-rowSize] = ix[2-rowSize] = ix[3-rowSize] = *ix;
		} break;
	case 2:
		for(int i=1; i<size-1; ++i) {
			int v = i%s<=s/2? i/s*s: (i/s+1)*s;
			uint16* ix = m_geometry.indices + i*2;
			*ix = v;
		} break;
	case 3:
		for(int i=1; i<size-1; ++i) {
			int v = s==1? i: i%s<s/2? i/s*s: (i/s+1)*s;
			uint16* ix = m_geometry.indices + (size-2)*rowSize+1 + i*2;
			*ix = (size-1)*size + v;
		} break;
	}
	//m_changed &= ~(1<<edge);
}

int Patch::getAdjacentStep(int side) const {
	if(m_adjacent[side]) return 0;
	else {
		if(!m_parent) return -0xffffff;
		return m_parent->getAdjacentStep(side) + 1;
	}
}

// ====================== Update Geometry ================================== //

inline void clampVec(vec2& v, float min, float max) {
	if(v.x<min) v.x=min; else if(v.x>max) v.x=max;
	if(v.y<min) v.y=min; else if(v.y>max) v.y=max;
}

void Patch::updateGeometry(const BoundingBox& box, bool normals) {
	if(m_split) {
		vec3 c = m_child[0]->m_corner[3]; // Patch centre point
		if(box.min.x <= c.x && box.min.z <= c.z) m_child[0]->updateGeometry(box, normals);
		if(box.max.x >= c.x && box.min.z <= c.z) m_child[1]->updateGeometry(box, normals);
		if(box.min.x <= c.x && box.max.z >= c.z) m_child[2]->updateGeometry(box, normals);
		if(box.max.x >= c.x && box.max.z >= c.z) m_child[3]->updateGeometry(box, normals);
	}

	// Update vertex positions
	int  size = m_landscape->m_patchSize;
	vec3 step = (m_corner[3] - m_corner[0]) / (size-1);
	vec2 a = ceil( (box.min - m_corner[0]) / step).xz();
	vec2 b = floor((box.max - m_corner[0]) / step).xz();
	clampVec(a, 0, size-1);
	clampVec(b, 0, size-1);

	// get data from children to avoid calculation
	if(m_split && 0) {
		for(int x=a.x; x<=b.x; ++x) for(int y=a.y; y<=b.y; ++y) {
			float* v = m_geometry.vertices + (x + y*size) * stride;
			int ci = (x>size/2?1:0) | (y>size/2? 2:0);
			int cx = (x - (ci&1? size/2: 0)) * 2;
			int cy = (y - (ci&2? size/2: 0)) * 2;
			float* cv = m_child[ci]->m_geometry.vertices + (cx+cy*size) * stride;
			// Set values
			v[1] = cv[1];	// height
			v[3] = cv[3];	// normal
			v[4] = cv[4];
			v[5] = cv[5];
			v[9] = cv[1];	// interpolated height
			m_bounds.include( vec3(v) );
		}

	}
	else {
		Landscape::HeightFunc func = m_landscape->m_func;
		
		// Update base heights
		for(int x=a.x; x<=b.x; ++x) for(int y=a.y; y<=b.y; ++y) {
			float* v = m_geometry.vertices + (x + y*size) * stride;
			vec3 p(v[0], 0, v[2]);
			v[1] = p.y = func(p);
			m_bounds.include(p);
		}

		// Update normals
		if(normals) {
			vec3 n[6], normal;
			int s = size - 1, ss = size * stride;
			static const vec3 base[6] = { vec3(0,0,-1), vec3(1,0,-1), vec3(1,0,0), vec3(0,0,1), vec3(-1,0,1), vec3(-1,0,0) };
			for(int i=0; i<6; ++i) n[i] = base[i] * step;
			for(int x=a.x; x<=b.x; ++x) for(int y=a.y; y<=b.y; ++y) {
				float* vx = m_geometry.vertices + (x + y*size) * stride;
				// Get connected vertices
				n[0].y = (y>0?      vx[1-ss]:        func( vec3(vx) + base[0]*step )) - vx[1];
				n[1].y = (y>0&&x<s? vx[1+stride-ss]: func( vec3(vx) + base[1]*step )) - vx[1];
				n[2].y = (x<s?      vx[1+stride]:    func( vec3(vx) + base[2]*step )) - vx[1];
				n[3].y = (y<s?      vx[1+ss]:        func( vec3(vx) + base[3]*step )) - vx[1];
				n[4].y = (x>0&&y<s? vx[1+ss-stride]: func( vec3(vx) + base[4]*step )) - vx[1];
				n[5].y = (x>0?      vx[1-stride]:    func( vec3(vx) + base[5]*step )) - vx[1];

				// combine
				normal.x = normal.y = normal.z = 0;
				for(int i=0; i<6; ++i) normal -= n[i].cross( n[(i+1)%6] );
				normal.normalise();
				memcpy(vx+3, normal, sizeof(vec3));
			}
		}
	}

	// Interpolated values to calculate error
	m_error = step.x * 0.3;	// factor resolution into error value
	for(int x=a.x; x<=b.x; ++x) for(int y=a.y; y<=b.y; ++y) {
		float* v = m_geometry.vertices + (x + y*size)*stride;
		float* a=0;
		float* b=0;

		// get interpolants
		if((x&1) && (y&1)) {
			a = v - (size+1) * stride;
			b = v + (size+1) * stride;
		} else if(x&1) {
			a = v - stride;
			b = v + stride;
		} else if(y&1) {
			a = v - size * stride;
			b = v + size * stride;
		}

		// Interpolate values
		if(a&&b) {
			float mid = (a[1] + b[1]) * 0.5;
			m_error = fmax(m_error, mid - v[1]);
		}
	}

	// Update error value from children
	if(m_split) for(int i=0; i<4; ++i) m_error = fmax(m_error, m_child[i]->m_error);
	if(m_landscape->m_updateCallback) m_landscape->m_updateCallback(&m_geometry);
}



//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////
//                             COLLISION                                     //
//// //// //// //// //// //// //// //// //// //// //// //// //// //// //// ////

float Patch::getHeight(float x, float z, vec3* normal) const {
	// Recurse tree if split
	if(m_split) {
		vec3 c = m_child[0]->m_corner[3];
		int k = (x<c.x? 0: 1) | (z<c.z? 0: 2);
		return m_child[k]->getHeight(x,z,normal);
	}

	// Get interpolated height at current lod
	int size = m_landscape->m_patchSize;
	x = (x - m_corner[0].x) / (m_corner[3].x - m_corner[0].x) * (size-1);
	z = (z - m_corner[0].z) / (m_corner[3].z - m_corner[0].z) * (size-1);
	if(x<0||z<0||x>size||z>size) return 0; // Out of bounds
	int ix = floor(x); x-=ix;
	int iz = floor(z); z-=iz;
	if(ix==size-1) --ix, x=1;	// Fix if on the edge
	if(iz==size-1) --iz, z=1;

	const float* v0;
	const float* v1 = m_geometry.vertices + (ix+1 + iz*size) * stride;
	const float* v2 = m_geometry.vertices + (ix + (iz+1)*size) * stride;
	if(x+z>1) {
		float t = x; x = 1 - z; z = 1 - t;
		v0 = m_geometry.vertices  + (ix+1 + (iz+1)*size) * stride;
	} else v0 = m_geometry.vertices + (ix+iz*size) * stride;

	// Debug
	float* end = m_geometry.vertices+size*size*stride;
	if(v0>=end || v1>=end || v2>=end) { printf("Error: Index out of bounds\n"); assert(false); }

	// Interpolate height
	float h = (1-x-z) * v0[1];
	h += x * v1[1];
	h += z * v2[1];

	// Interpolate Normal
	if(normal) {
		*normal = (1-x-z) * vec3(v0+3);
		*normal += x * vec3(v1+3);
		*normal += z * vec3(v2+3);
	}

	return h;
}

const Patch* Patch::intersect(const vec3& start, float radius, const vec3& direction, float& t, vec3& normal) const {
	if(m_split) {
		// Intersect with children in order
		const Patch* hit = 0;
		for(int i=0; i<4; ++i) {
			int k = direction.x>0? i: i^1;
			if(direction.z<0) k^=2;
			if(const Patch* r = m_child[k]->intersect(start, radius, direction, t, normal)) hit = r;
			if(hit && radius==0) return hit; // may need the adjacent one if there is a radius
		}
		return hit;
	}
	else {
		// test bounding box and get intersecting line segment
		float min=0, max=t;
		for(int i=0; i<3; ++i) {
			if(direction[i] == 0) {
				if(start[i] < m_bounds.min[i]-radius || start[i] > m_bounds.max[i]+radius) return 0;
			}
			else {
				float t0 = (m_bounds.min[i] - radius - start[i]) / direction[i];
				float t1 = (m_bounds.max[i] + radius - start[i]) / direction[i];
				if(t0>t1) { float v=t0; t0=t1; t1=v; } // correct order
				if(t0>min) min = t0;
				if(t1<max) max = t1;
				if(min>max) return 0; // Missed
				if(min>t) return 0; // stopped short
			}
		}

		// Geometry
		if(radius<=0 && intersectGeometry(start, direction, t, normal)) return this;
		if(radius>0 && intersectGeometry(start, radius, direction, t, normal)) return this;
	}
	return 0;
}

bool Patch::getTriangle(uint i, vec3& a, vec3& b, vec3& c) const {
	const uint16* ix = m_geometry.indices;
	if(ix[i]==ix[i+1] || ix[i+1]==ix[i+2] || ix[i]==ix[i+2]) return false;
	const int flip = i&1; // Triangle strip needs to flip odd polygons
	const float* vx = m_geometry.vertices;
	const float* pa = vx + ix[i+0] * stride;
	const float* pb = vx + ix[i+1+flip] * stride;
	const float* pc = vx + ix[i+2-flip] * stride;
	a = vec3(pa);
	b = vec3(pb);
	c = vec3(pc);
	return true;
}

// ToDo: Bresenham algorithm. Just brute force for now.
bool Patch::intersectGeometry(const vec3& p, const vec3& d, float& t, vec3& normal) const {
	bool hit = false;
	float value;
	vec3 a,b,c;
	for(uint i=0; i<m_geometry.indexCount-2; ++i) {
		if(getTriangle(i, a,b,c) && base::intersectRayTriangle(p,d, a,b,c, value) && value<t) {
			normal = (b-a).cross(c-a).normalise();
			hit = true;
			t = value;
		}
	}
	return hit;
}
bool Patch::intersectGeometry(const vec3& p, float radius, const vec3& d, float& t, vec3& normal) const {
	//Note: d must be normalised
	bool hit = false;
	float value;
	vec3 a,b,c,s,edge;
	for(uint i=0; i<m_geometry.indexCount-2; ++i) {
		if(getTriangle(i, a,b,c)) {
			vec3 n = (b-a).cross(c-a);
			if(n.dot(d)>0) continue; // wrong side
			n.normalise();
			s = p - n * radius;
			float dn = n.dot(d);
			if(dn != 0) {
				value = (n.dot(a) - n.dot(s)) / dn;
				s += d * value;
				int k = base::closestPointOnTriangle(s, a, b, c, edge);
				if(k==0 && value >= 0) {
					normal = n;
					t = value;
					hit = true;
				}
				else if(base::intersectRaySphere(edge, -d, p, radius, value) && value>=0 && value<=t) {
					normal = p + d * t - edge;
					t = value;
					hit = true;
				}
			}
		}
	}
	return hit;
}



// Debug stuff
bool Landscape::selectPatch(const vec3& start, const vec3& direction) {
	float t;
	vec3 normal;
	if(const Patch* r = m_root->intersect(start, 0, direction.normalised(), t, normal)) {
		m_selected = r;
		return true;
	}
	return false;
}
const PatchGeometry* Landscape::getSelectedGeometry() const {
	return m_selected? &m_selected->getGeometry(): 0;
}
void Landscape::getSelectedInfo() const {
	if(m_selected) m_selected->printInfo();
}
void Patch::printInfo() const {
	// get address
	char address[12];
	int ca = m_depth;
	address[ca] = 0;
	for(const Patch* p = this; p->m_parent; p=p->m_parent) {
		for(int i=0; i<4; ++i) if(p->m_parent->m_child[i] == p) address[--ca] = '0'+i;
	}
	printf("Patch %s [%d] lod:%g error:%g,%g\n", address, m_depth, m_lod, m_error, m_parent->m_error);
}


