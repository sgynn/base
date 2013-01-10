#include "base/model.h"
#include "modelmath.h"

using namespace base;
using namespace model;

Animation::Animation() : m_animations(0), m_count(0), m_start(0), m_end(0) {}
Animation::~Animation() {}
Animation::JointAnimation& Animation::getJointAnimation(int jointID) {
	if(jointID>=m_count) {
		//resize array
		ResizeArray(JointAnimation, m_animations, m_count, jointID+1)
		//initialise new animations
		for(int i=m_count; i<=jointID; i++) {
			m_animations[i].rotationCount=0;
			m_animations[i].positionCount=0;
			m_animations[i].scaleCount=0;
		}
		m_count = jointID+1;
	}
	return m_animations[jointID];
}

int Animation::getRotation(float*rot, int joint, float frame, unsigned int& hint) const {
	unsigned int count = m_animations[joint].rotationCount;
	KeyFrame4* keys = m_animations[joint].rotationKeys;
	
	//trivial cases if there is only one keyframe, or none at all.
	if(count==0) {
		rot[0]=0; rot[1]=0; rot[2]=0; rot[3]=0;
	} else if(count==1) {
		memcpy(rot, keys[0].data, 4*sizeof(float));
	} else {	
		//get keyframe index
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>keys[count-1].frame) hint=count;
			else for(unsigned int i=0; i<count; i++) if(keys[i].frame>frame) { hint = i; break; }
		}
		//interpolate
		if(hint==0) memcpy(rot, keys[0].data, 4*sizeof(float));
		else if(hint==count) memcpy(rot, keys[count-1].data, 4*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			Math::slerp(rot, keys[hint-1].data, keys[hint].data, v);
		}
	}
	return 1;
}
int Animation::getPosition(float*pos, int joint, float frame, unsigned int& hint) const {
	unsigned int count = m_animations[joint].positionCount;
	KeyFrame3* keys = m_animations[joint].positionKeys;
	
	//trivial cases if there is only one keyframe, or none at all.
	if(count==0) {
		pos[0]=0; pos[1]=0; pos[2]=0;
	} else if(count==1) {
		memcpy(pos, keys[0].data, 3*sizeof(float));
	} else {	
		//get keyframe index
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>keys[count-1].frame) hint=count;
			else for(unsigned int i=0; i<count; i++) if(keys[i].frame>frame) { hint = i; break; }
		}
		//interpolate
		if(hint==0) memcpy(pos, keys[0].data, 3*sizeof(float));
		else if(hint==count) memcpy(pos, keys[count-1].data, 3*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			Math::lerp(pos, keys[hint-1].data, keys[hint].data, v);
		}
	}
	return 1;
}
int Animation::getScale(float*scl, int joint, float frame, unsigned int& hint) const {
	unsigned int count = m_animations[joint].scaleCount;
	KeyFrame3* keys = m_animations[joint].scaleKeys;
	
	//trivial cases if there is only one keyframe, or none at all.
	if(count==0) {
		scl[0]=1; scl[1]=1; scl[2]=1;
	} else if(count==1) {
		memcpy(scl, keys[0].data, 3*sizeof(float));
	} else {	
		//get keyframe index
		if((hint<count && keys[hint].frame<frame) || (hint>0 && keys[hint-1].frame>frame)) {
			if(frame>keys[count-1].frame) hint=count;
			else for(unsigned int i=0; i<count; i++) if(keys[i].frame>frame) { hint = i; break; }
		}
		//interpolate
		if(hint==0) memcpy(scl, keys[0].data, 3*sizeof(float));
		else if(hint==count) memcpy(scl, keys[count-1].data, 3*sizeof(float));
		else {
			float v = (frame - keys[hint-1].frame) / (keys[hint].frame-keys[hint-1].frame);
			Math::lerp(scl, keys[hint-1].data, keys[hint].data, v);
		}
	}
	return 1;
}

int Animation::getRange(int& start, int& end) {
	if(m_start==0 && m_end==0) {
		for(int i=0; i<m_count; i++) {
			int k0 = m_animations[i].positionCount? m_animations[i].positionKeys[ m_animations[i].positionCount-1 ].frame: 0;
			int k1 = m_animations[i].rotationCount? m_animations[i].rotationKeys[ m_animations[i].rotationCount-1 ].frame: 0;
			int k2 =m_animations[i].scaleCount?  m_animations[i].scaleKeys[ m_animations[i].scaleCount-1 ].frame: 0;
			if(k0>m_end) m_end=k0;
			if(k1>m_end) m_end=k1;
			if(k2>m_end) m_end=k2;
		}
	}
	start=m_start;
	end=m_end;
	return m_end-m_start;
}

