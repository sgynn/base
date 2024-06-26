#pragma once

#include "animation.h"
#include "skeleton.h"


namespace base {

enum class AnimationBlend {
	Set,	// Discards any previous data
	Mix,	// Interpolatve animation blending
	Add	// Additive animation blending
};


/** Animation state holds the current frame and other info for active animations.
 * It can be shader between multiple instances of a model
 * */
class AnimationState {
	public:
	AnimationState(Skeleton* skeleton);
	~AnimationState();
	bool  update(float time, bool applyPose=true);
	bool  apply(bool update=true) const;	// apply animations to skeleton
	int   play(const Animation*, float speed=1, AnimationBlend=AnimationBlend::Set, float weight=1, char loop=-1, int bone=-1, int track=-1);

	void  stopAll();
	void  stop(int track=0);

	void  swapTracks(int, int);

	int              getTrack(const Animation*, int bone=-1) const;	// Find track running animation
	int	             getUnusedTrack() const;						// Get first unused track
	const Animation* getAnimation(int track=0) const;				// Get animation on track

	void  setSpeed (float speed,  int track=0);
	void  setWeight(float weight, int track=0);
	void  setFrame (float frame,  int track=0);
	void  setFrameNormalised(float frame,  int track=0);

	bool  fadeIn(float amount, int track=0);		// Helper function for increasing weight to 1.0. Returns if at 1
	bool  fadeOut(float amount, int track=0);		// Helper function for decreasing weight to 0.0. Returns if at 0
	void  advance(float amount, int track=0);		// advance current frame by this amount

	AnimationBlend getBlend(int track=0) const;
	float getFrame(int track=0) const;
	float getFrameNormalised(int track=0) const;
	float getSpeed(int track=0) const;
	float getWeight(int track=0) const;
	bool  isEnded(int track=0) const;

	bool getLoop(int track=0) const;
	void setLoop(bool loop, int track=0);
	int  getLooped(int track=0) const;		// Did animation loop in last update

	Skeleton* getSkeleton() const { return m_skeleton; }

	const unsigned char* getKeyMap(int track=0) const;	// Get bone -> animationset map for an animation
	int getNextTrack(int last=-1) const;	// Get next active track. ends with -1;

	//protected:
	int allocateTrack();
	int allocateTrack(unsigned);
	struct AnimationInstance {
		const Animation* animation;	// animation data
		AnimationBlend   blend;		// animation blend mode
		int              bone;		// starting bone
		int              mesh;		// mesh applied to
		float            weight;	// blend weight
		float            speed;		// animation speed
		float            frame;		// current frame
		bool             loop;		// is animation looped
		bool             changed;	// Force update, even if data looks the same
		int              looped;	// If animation looped in last frame
		const unsigned char* keyMap;// animation keyset -> bone index map
	};
	std::vector<AnimationInstance> m_animations;
	Skeleton* m_skeleton;
};

}

