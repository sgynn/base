#include <base/animation.h>
#include <base/skeleton.h>
#include <base/thread.h>
#include <cstring>

using namespace base;

Animation::Animation() : m_animations(0), m_name(""), m_length(0), m_fps(0), m_loop(true), m_ref(0) {}
Animation::~Animation() {
	for(uint i=0; i<m_animations.size(); ++i)
		delete m_animations[i];
}

int Animation::getBoneID(const char* name) const {
	for(uint i=0; i<m_animations.size(); ++i) {
		if(m_animations[i] && strcmp(m_animations[i]->name, name)==0) return i;
	}
	return -1;
}
int Animation::addKeySet(const char* name) {
	m_animations.push_back( new KeySet() );
	strcpy(m_animations.back()->name, name);
	return m_animations.size()-1;
}

// --------------------------------------------------------------------------------------------- //

const char* Animation::getMap(const Skeleton* s) const {
	// Find it in list
	unsigned id = s->getMapID();
	for(uint i=0; i<m_maps.size(); ++i) {
		if(m_maps[i].skeletonID == id) return m_maps[i].map;
	}
	// Create it	TODO move this into Skeleton class to fix threading issues and potential memory leak.
	SkeletonMap map;
	map.size = s->getBoneCount();
	map.map = new char[ map.size ];
	map.skeletonID = id;
	for(int i=0; i<map.size; ++i) {
		map.map[i] = getBoneID( s->getBone(i)->getName() );
	}
	static base::Mutex m;
	MutexLock lock(m);
	m_maps.push_back(map);
	return map.map;
}


// --------------------------------------------------------------------------------------------- //

template<int N>
void Animation::addKey( std::vector< Keyframe<N> >& keys, int frame, const float* value) {
	// Add frame to the end of the array
	unsigned int index = 0;
	if(keys.empty() || frame>keys.back().frame) {
		keys.push_back( Keyframe<N>() );
		index = keys.size()-1;
	} else {
		// Splice it in somewhere, or replace one
		for(index=0; index<keys.size(); ++index) {
			if(keys[index].frame == frame) break;
			else if(keys[index].frame>frame) {
				keys.insert( keys.begin()+index, Keyframe<N>());
				break;
			}
		}
	}
	// Set key
	keys[index].frame = frame;
	memcpy(keys[index].data, value, N*sizeof(float));
	if(frame>m_length) m_length = frame;
}
void Animation::addRotationKey(int bone, int frame, const Quaternion& quat) {
	KeySet* set = m_animations[bone];
	addKey(set->rotation, frame, quat);
}
void Animation::addPositionKey(int bone, int frame, const vec3& position) {
	KeySet* set = m_animations[bone];
	addKey(set->position, frame, position);
}
void Animation::addScaleKey(int bone, int frame, const vec3& scale) {
	KeySet* set = m_animations[bone];
	addKey(set->scale, frame, scale);
}

// --------------------------------------------------------------------------------------------- //


template<int N>
bool Animation::removeKey(std::vector< Keyframe<N> >& keys, int frame) {
	for(unsigned int i=0; i<keys.size(); ++i) {
		if(keys[i].frame == frame) {
			keys.erase( keys.begin()+i );
			return true;
		}
	}
	return false;
}
bool Animation::removeRotationKey(int bone, int frame) {
	if(bone<(int)m_animations.size())
		return removeKey( m_animations[bone]->rotation, frame );
	return false;
}
bool Animation::removePositionKey(int bone, int frame) {
	if(bone<(int)m_animations.size())
		return removeKey( m_animations[bone]->position, frame );
	return false;
}
bool Animation::removeScaleKey(int bone, int frame) {
	if(bone<(int)m_animations.size())
		return removeKey( m_animations[bone]->scale, frame );
	return false;
}



// --------------------------------------------------------------------------------------------- //


template<int N, class P>
inline int Animation::getValue( std::vector< Keyframe<N> >& keys, float frame, int hint, float* value, const float* def, P interpolate) const {
	// Trivial cases
	int count = keys.size();
	if(count==0) {
		memcpy(value, def, N*sizeof(float));
	} else if(count==1) {
		memcpy(value, keys[0].data, N*sizeof(float));
	} else {
		// Get nearest keyframes
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>=keys[count-1].frame) hint=count;
			else for(int i=0; i<count; ++i) if(keys[i].frame>frame) { hint = i; break; }
		}
		// Interpolate
		if(hint==0) memcpy(value, keys[0].data, N*sizeof(float));
		else if(hint==count) memcpy(value, keys[count-1].data, N*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			interpolate(value, keys[hint-1].data, keys[hint].data, v);
		}
		return hint;
	}
	return 0;
}


int Animation::getRotation(int bone, float frame, int hint, Quaternion& q) const {
	static SlerpFunc slerp;
	static float def[4] = {1,0,0,0};
	return getValue(m_animations[bone]->rotation, frame, hint, q, def, slerp);
}
int Animation::getPosition(int bone, float frame, int hint, vec3& p) const {
	static LerpFunc lerp;
	static float def[3] = {0,0,0};
	return getValue(m_animations[bone]->position, frame, hint, p, def, lerp);
}
int Animation::getScale(int bone, float frame, int hint, vec3& s) const {
	static LerpFunc lerp;
	static float def[3] = {1,1,1};
	return getValue(m_animations[bone]->scale, frame, hint, s, def, lerp);
}



// --------------------------------------------------------------------------------------------- //



Animation* Animation::subAnimation(int start, int end) const {
	Animation* a = new Animation;
	a->m_length = end - start;
	a->m_fps = m_fps;

	// Create keysets
	for(uint i=0; i<m_animations.size(); ++i) {
		a->addKeySet(m_animations[i]->name);
	}

	// Copy keyframes
	for(uint i=0; i<m_animations.size(); ++i) {
		KeySet* list = m_animations[i];
		KeySet* out = a->m_animations[i];
		// count keys
		int rc=0, pc=0, sc=0;
		int ri=-1, pi=-1, si=-1;
		auto in = [](float v, float a, float b) { return v>=a && v<=b; };
		for(uint j=0; j<list->rotation.size(); ++j) if( in(list->rotation[j].frame,start,end) ) { ++rc; if(ri<0) ri=j; }
		for(uint j=0; j<list->position.size(); ++j) if( in(list->position[j].frame,start,end) ) { ++pc; if(pi<0) pi=j; }
		for(uint j=0; j<list->scale.size(); ++j)    if( in(list->scale[j].frame,start,end) )    { ++sc; if(si<0) si=j; }
		// Rotation
		if(rc) {
			out->rotation.insert(out->rotation.begin(), list->rotation.begin()+ri, list->rotation.begin()+ri+rc);
			for(int j=0; j<rc; ++j) out->rotation[j].frame -= start;
		} else {
			Quaternion q;
			getRotation(i, start, 0, q);
			a->addRotationKey(i, 0, q);
		}
		// Position
		if(rc) {
			out->position.insert(out->position.begin(), list->position.begin()+ri, list->position.begin()+ri+rc);
			for(int j=0; j<rc; ++j) out->position[j].frame -= start;
		} else {
			vec3 p;
			getPosition(i, start, 0, p);
			a->addPositionKey(i, 0, p);
		}
		// Scale
		if(sc) {
			out->scale.insert(out->scale.begin(), list->scale.begin()+ri, list->scale.begin()+ri+rc);
			for(int j=0; j<rc; ++j) out->scale[j].frame -= start;
		} else {
			vec3 p;
			getScale(i, start, 0, p);
			a->addScaleKey(i, 0, p);
		}
	}
	return a;
}


