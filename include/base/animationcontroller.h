#pragma once

#include <base/math.h>
#include <base/hashmap.h>
#include <vector>

namespace base {

class Model;
class Skeleton;
class Animation;
class AnimationState;


/// Name lookup for animations
class AnimationKey {
	static base::HashMap<uint>      s_lookup;
	static std::vector<const char*> s_reverse;
	uint m_key;
	public:
	AnimationKey(const char* name) : m_key(lookup(name)) {}
	AnimationKey(uint key=~0u) : m_key(key) {}
	uint getKey() const { return m_key; }
	const char* getName() const { return lookup(m_key); }
	bool operator==(const AnimationKey& k) const { return m_key==k.m_key; }
	bool operator==(const char* name) const { return *this==AnimationKey(name); }
	bool operator!=(const AnimationKey& k) const { return m_key!=k.m_key; }
	operator uint() const { return m_key; }
	operator const char*() const { return getName(); }
	operator bool() const { return m_key<s_reverse.size(); }
	public:
	static uint lookup(const char*);
	static const char* lookup(uint);
};


// What to do when an action ends
enum class ActionMode {
	Hold,		// Hold final frame
	Loop,		// Loop animation
	End,		// End action
	Reverse,	// set speed = -speed (loops)
	ReverseEnd,	// Reverse at >1.0, End at 0.0
	ReverseHold	// Reverse at >1.0, End at 0.0
};


/// Animations and metadata
class AnimationBank {
	public:
	using Animation = base::Animation;
	struct AnimationInfo {
		AnimationKey key;		// Key
		Animation* animation;	// Animation data
		float speedKey;			// Speed for selecting movement animation
		float weight;			// Selection weight if multiple options
		uint  groupMask;		// Group mask for filtering animations	

		vec3 startPos, loopOffset;
		Quaternion startRot, loopRotation;

		AnimationInfo* next;	// Linked list of animations with the same key
	};

	public:
	AnimationBank(const char* rootBone=0, const vec3& forward = vec3(0,-1,0)); // Note: forward axis in blender
	~AnimationBank();
	bool autoDetectMove(const Animation* anim) const;
	void calculateMeta(AnimationInfo& anim, const char* root);
	void add(const AnimationKey&, Animation*, uint groupMask=~0u, float weight=1, bool move=false);
	const AnimationInfo* getAnimation(const AnimationKey&, int group) const;
	const AnimationInfo* getAnimation(float speed, int group) const;
	const char* getRootBone() const { return m_rootBone; }
	float getFastestMoveSpeed(int group=-1) const;

	private:
	std::vector<AnimationInfo*> m_animations;
	std::vector<AnimationInfo*> m_movement;
	const char* m_rootBone;
	vec3 m_forward;
};



enum class ActionState : uint8 { Idle, Moving, Action, Ended };

/// Animation controller handles all animation stuff.
class AnimationController {
	public:
	typedef AnimationBank::AnimationInfo AnimationInfo;
	AnimationController();
	~AnimationController();

	void initialise(base::Model*, AnimationBank*, bool rootMotion=true);
	void setAnimationBank(AnimationBank*);
	AnimationBank* getAnimations() const { return m_bank; }
	base::Skeleton* getSkeleton() const;
	void setFadeTime(float time);

	void setGroup(int group);
	void setIdle(const AnimationKey&);								// Set idle action (loop)
	void playMove(float speed);										// Play move animation. select based on speed from valid group
	void playAction(const AnimationKey&, ActionMode mode=ActionMode::Hold, float speed=1.0, bool blend=true);	// Play an action
	void endAction();												// End current action
	void clear();													// Immediatly terminate all actions

	AnimationKey getIdle() const;		// Get the assigned idle animation (may not be playing)
	ActionState  getState() const;
	AnimationKey getAction() const;		// Get the active action - It may have ended.
	float        getProgress() const;	// 0-1 value through active animation
	float        getWeight() const;		// Weight of active action
	int          getGroup() const { return m_group; }

	void         setProgress(float);	// Set progress of current action.
	void         setWeight(float);		// Set weight of currrent action
	void         setSpeed(float);		// Set speed of current action

	void playOverride(const AnimationKey&, ActionMode=ActionMode::Hold, float speed=1, bool fade=true, bool additive=false);		// Play an override animation
	void stopOverride(const AnimationKey&, bool fade=true);								// Stop an override animation
	void setOverride(const AnimationKey&, float frame, float weight=1, bool additive=false);
	void setOverrideSpeed(const AnimationKey&, float speed);
	void forceOverride(const AnimationKey&, uint track, float frame, float weight, bool additive=false);
	bool hasOverride(const AnimationKey&) const;
	float getOverrideProgress(const AnimationKey&) const;
	float getOverrideWeight(const AnimationKey&) const;
	void clearOverrides(bool fade=false);

	void              update(float time, bool finalise=true);
	const vec3&       getOffset() const		{ return m_offset; }
	const Quaternion& getRotation() const	{ return m_rotation; }

	bool currentActionAffectsBone(base::Bone*) const;
	bool currentActionAffectsBone(unsigned boneIndex) const;

	float deriveMoveSpeed() const;

	protected:
	enum MetaType { IDLE, ACTION, MOVEMENT, OVERRIDE, OVERRIDE_IN, OVERRIDE_OUT };
	void setMeta(uint track, const AnimationInfo*, MetaType type, ActionMode end);
	void clearMeta(uint track);
	void updateRootOffset(bool output);
	int  findOverride(const AnimationKey&) const;
	void setOverrideStartTrack(int newStart);
	int allocateActionTrack();

	protected:
	AnimationState* m_state;
	AnimationBank* m_bank;
	bool m_rootMotion;		// Root motion enabled
	int m_group;			// animation selection mask
	AnimationKey m_idle;	// Idle action
	AnimationKey m_action;	// Current action
	float m_moveSpeed;		// speed for selecting move animation.
	int m_idleTrack;		// Track playing idle animation - doesn't get removed if weight=0
	int m_actionTrack;		// Track playing active action
	int m_overrideStart;	// override animations must be after action tracks
	int m_lastAction;		// Keep action track until it fades out
	float m_fadeTime;		// Tiime to fade between animations

	int  m_rootBone;		// Bone used for root motion
	vec3 m_lastPosition;	// Previous root position for calculating deltas
	vec3 m_offset;			// Frame root translation
	Quaternion m_lastOrientation;	// Previous orientation for calculating deltas
	Quaternion m_rotation;			// Frame root rotation
	Quaternion m_rootRest;
	Quaternion m_rootSkin;

	struct Meta {
		const AnimationInfo* animation;	// Animation data on this track
		MetaType type;					// Type of animation
		ActionMode mode;				// Behaviour when animation ends
		vec3 lastPos;					// Last position for root motion
		Quaternion lastRot;				// Last orientation for root motion
	};
	std::vector<Meta> m_meta;
};

}

