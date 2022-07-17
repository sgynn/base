#include <base/model.h>
#include <base/skeleton.h>
#include <base/animation.h>
#include <base/animationstate.h>
#include <base/animationcontroller.h>
#include <algorithm>
#include <assert.h>
#include <cstdio>

using namespace base;

HashMap<uint> AnimationKey::s_lookup;
std::vector<const char*> AnimationKey::s_reverse;
uint AnimationKey::lookup(const char* name) {
	if(!name || !name[0]) return ~0u;
	uint r = s_lookup.get(name, s_reverse.size());
	if(r==s_reverse.size()) {
		s_lookup[name] = r;
		s_reverse.push_back(s_lookup.find(name)->key);
	}
	return r;
}
const char* AnimationKey::lookup(uint key) {
	return key<s_reverse.size()? s_reverse[key]: 0;
}


// ============================================================================================= //

AnimationBank::AnimationBank(const char* rootBone, const vec3& forward) : m_forward(forward) {
	m_rootBone = rootBone? strdup(rootBone): 0;
}
AnimationBank::~AnimationBank() {
	free((char*)m_rootBone);
	for(AnimationInfo* a: m_animations) {
		while(a) {
			AnimationInfo* next = a->next;
			delete a;
			a = next;
		}
	}
}

float analyseLoop(Animation* anim, const char* rootName) {
	float result = 1.0;
	for(int i=0; i<anim->getSize(); ++i) {
		Quaternion startRot, endRot;
		anim->getRotation(i, 0,0,startRot);
		anim->getRotation(i, anim->getLength(), 0, endRot);
		startRot.normalise();
		endRot.normalise();
		float dot = startRot.dot(endRot);
		result *= dot;
	}
	return result;
}

void AnimationBank::calculateMeta(AnimationInfo& anim, const char* root) {
	if(!root || anim.animation->getLength()==0) return;
	int set = anim.animation->getBoneID(root);
	if(set>=0) {
		const int length = anim.animation->getLength();

		vec3 endPos;
		Quaternion endRot;
		anim.animation->getPosition(set, 0, 0, anim.startPos);
		anim.animation->getPosition(set, length, length, endPos);
		anim.animation->getRotation(set, 0, 0, anim.startRot);
		anim.animation->getRotation(set, length, length, endRot);

		anim.loopOffset = endPos - anim.startPos;
		anim.loopRotation = endRot * anim.startRot.getInverse();

		anim.speedKey = m_forward.dot(anim.loopOffset) / length * anim.animation->getSpeed();
	}

}

bool AnimationBank::autoDetectMove(const Animation* anim) const {
	// Detect if this it a movment animation loop
	if(!anim || anim->getLength()==0) return false;
	int rootTrack = anim->getBoneID(m_rootBone);
	if(rootTrack<0) return false;
	int last = anim->getLength() + 1;

	// Root track must just move forward.
	// Having lateral offsets can be desired in a move animation but this is just a simple shortcut.
	vec3 va, vb;
	Quaternion qa, qb;
	anim->getPosition(rootTrack, 0, 0, va);
	anim->getPosition(rootTrack, last, 0, vb);
	if(va == vb) return false; // Must have moved
	if(fabs(m_forward.dot((va-vb).normalise())) < 0.999) return false;

	// all other bones must match first frame
	for(int track=0; track<anim->getSize(); ++track) {
		if(track == rootTrack) continue;
		anim->getRotation(track, 0, 0, qa);
		anim->getRotation(track, last, 0, qb);
		if(fabs(qa.dot(qb)-1) > 0.001) return false;
		//if(qa != qb) return false;
	}
	return true;
}

float AnimationBank::getFastestMoveSpeed(int group) const {
	float result = 0;
	uint mask = group<0? ~0u: 1<<group;
	for(AnimationInfo* a: m_movement) {
		if((a->groupMask&mask) && a->speedKey > result) result = a->speedKey;
	}
	return result;
}

void AnimationBank::add(const AnimationKey& key, Animation* anim, uint groupMask, float weight, bool move) {
	if(!anim) return;
	uint k = key;
	if(k >= m_animations.size()) m_animations.resize(k+1, 0);
	AnimationInfo** ptr = &m_animations[k];
	while(*ptr) ptr = &((*ptr)->next);
	AnimationInfo* a = new AnimationInfo;
	a->animation = anim;
	a->speedKey = 0;
	a->weight = weight;
	a->groupMask = groupMask;
	a->key = key;
	a->next = 0;
	*ptr = a;

	calculateMeta(*a, m_rootBone);
	
	if(move) {
		m_movement.push_back(a);
		std::sort(m_movement.begin(), m_movement.end(), [](AnimationInfo* a, AnimationInfo* b) { return a->speedKey<b->speedKey; });
	}
}

const AnimationBank::AnimationInfo* AnimationBank::getAnimation(const AnimationKey& key, int group) const {
	uint k = key;
	uint mask = 1<<group;
	if(k>=m_animations.size() || !m_animations[k]) return 0;
	if(!m_animations[k]->next) return m_animations[k]->groupMask&mask? m_animations[k]: 0;

	// Random Selection
	float total = 0;
	for(AnimationInfo* a=m_animations[k]; a; a=a->next) {
		if(a->groupMask&mask) total += a->weight;
	}
	if(total>0) {
		total *= rand()/RAND_MAX;
		for(AnimationInfo* a=m_animations[k]; a; a=a->next) {
			if(a->groupMask&mask) {
				total -= a->weight;
				if(total<=0) return a;
			}
		}
	}
	return 0;
}


const AnimationBank::AnimationInfo* AnimationBank::getAnimation(float speed, int group) const {
	if(speed==0 || m_movement.empty()) return 0;
	uint m = 1<<group;

	// Find first one
	size_t index = 0;
	while(index<m_movement.size() && !(m_movement[index]->groupMask&m)) ++index;
	if(index == m_movement.size()) return 0; // No move animations in current group

	float low = m_movement[index]->speedKey;
	for(size_t i=index+1; i<m_movement.size(); ++i) {
		if(m_movement[i]->groupMask&m) {
			float high = m_movement[i]->speedKey;
			if(speed<(low+high)/2) return m_movement[index];	// ToDo: add bias ?
			low = high;
			index = i;
		}
	}
	return m_movement[index];
}


// ============================================================================================= //


AnimationController::AnimationController()
	: m_state(0), m_bank(0), m_rootMotion(true), m_group(0),  m_moveSpeed(0)
	, m_idleTrack(-1), m_actionTrack(-1), m_overrideStart(2), m_lastAction(-1)
	, m_fadeTime(0.3), m_rootBone(0)
{
}

AnimationController::~AnimationController() {
	delete getSkeleton();
	delete m_state;
}

void AnimationController::initialise(Model* model, AnimationBank* bank, bool rootMotion) {
	m_bank = bank;
	delete m_state;
	if(model->getSkeleton()) {
		Skeleton* skeleton = new Skeleton(*model->getSkeleton());
		m_state = new AnimationState(skeleton);
		m_rootMotion = rootMotion;
		setAnimationBank(bank);
	}
}

void AnimationController::setAnimationBank(AnimationBank* bank) {
	m_bank = bank;
	if(bank && getSkeleton()) {
		m_rootBone = getSkeleton()->getBoneIndex( bank->getRootBone() );
		if(m_rootBone<0) m_rootBone = 0;
		m_rootRest = getSkeleton()->getRestPose(m_rootBone);
		m_rootSkin = getSkeleton()->getSkinMatrix(m_rootBone);
		m_rootRest.normalise();
		m_rootSkin.normalise();
		updateRootOffset(false);
	}
}

Skeleton* AnimationController::getSkeleton() const {
	return m_state? m_state->getSkeleton(): 0;
}

void AnimationController::setFadeTime(float time) {
	m_fadeTime = time;
}

void AnimationController::setGroup(int g) {
	m_group = g;
	// Validate idle
	if(m_idleTrack>=0 && !(m_meta[m_idleTrack].animation->groupMask&1<<g)) {
		setIdle(m_idle);
	}
}

void AnimationController::setMeta(uint track, const AnimationInfo* anim, MetaType type, ActionMode end) {
	if(track>=m_meta.size()) m_meta.resize(track+1, Meta{0, ACTION, ActionMode::Hold});
	m_meta[track].animation = anim;
	m_meta[track].type = type;
	m_meta[track].mode = end;
}

void AnimationController::setIdle(const AnimationKey& key) {
	m_idle = key;
	const AnimationBank::AnimationInfo* a = m_bank->getAnimation(key, m_group);
	if(!a) m_idleTrack = -1;
	else if(m_idleTrack<0 || m_state->getAnimation(m_idleTrack) != a->animation) {
		float initialWeight = m_idleTrack<0? 1: 0;
		m_idleTrack = allocateActionTrack();
		m_state->play(a->animation, 1, AnimationBlend::Add, initialWeight, true, -1, m_idleTrack);
		setMeta(m_idleTrack, a, IDLE, ActionMode::Loop);
		m_lastPosition += a->startPos;
		m_lastOrientation *= a->startRot;
	}
}

int AnimationController::allocateActionTrack() {
	int track = m_state->getUnusedTrack();
	if(track >= m_overrideStart) {
		track = m_overrideStart;
		setOverrideStartTrack(m_overrideStart+1);
	}
	return track;
}

void AnimationController::playMove(float speed) {
	if(m_moveSpeed == speed) return;
	m_moveSpeed = speed;
	const AnimationBank::AnimationInfo* a = m_bank->getAnimation(speed, m_group);
	if(!a) {
		if(m_actionTrack>=0 && m_meta[m_actionTrack].type == MOVEMENT) {
			//printf("Stop movement\n");
			m_actionTrack = -1;
		}
	}
	else if(m_actionTrack<0 || m_state->getAnimation(m_actionTrack)!=a->animation) {
		float frame = 0;
		for(uint i=0; i<m_meta.size(); ++i) {
			if(m_meta[i].type==MOVEMENT) {
				if(m_meta[i].animation == a) { 
					//printf("Pick up move %s [%u]\n", a->animation->getName(), i);
					m_actionTrack = i;
					frame = -1;
					break;
				}
				else frame = m_state->getFrameNormalised(i);
			}
		}
		if(frame>=0) {
			if(m_actionTrack>=0 && m_meta[m_actionTrack].type==MOVEMENT && m_state->getWeight(m_actionTrack)==0) m_state->stop(m_actionTrack);
			m_actionTrack = allocateActionTrack();
			m_state->play(a->animation, 1, AnimationBlend::Add, 0, true, -1, m_actionTrack);
			if(frame>0) m_state->setFrameNormalised(frame, m_actionTrack);
			setMeta(m_actionTrack, a, MOVEMENT, ActionMode::Loop);
			//printf("Start move animation %s [%d]\n", a->animation->getName(), m_actionTrack);
			//m_lastPosition += a->startPos;
			//m_lastOrientation *= a->startRot;
			updateRootOffset(false);
			// ToDo: initial frame offset for these
		}
	}
	m_action = ~0u;

	// Update move animation speeds
	for(int i=m_state->getNextTrack(); i>=0; i=m_state->getNextTrack(i)) {
		float animSpeed = m_meta[i].animation->speedKey;
		if(m_meta[i].type==MOVEMENT && animSpeed != 0) m_state->setSpeed(speed / animSpeed, i);
	}
}

void AnimationController::playAction(const AnimationKey& key, ActionMode mode, float speed, bool blend) {
	const AnimationBank::AnimationInfo* a = m_bank->getAnimation(key, m_group);
	if(key && !a) printf("Missing animation %s\n", key.getName());
	if(!a) endAction();
	else if(m_actionTrack>=0 && m_state->getAnimation(m_actionTrack)==a->animation) {
		m_meta[m_actionTrack].mode = mode;
		m_state->setLoop(mode==ActionMode::Loop, m_actionTrack);
		m_state->setSpeed(speed, m_actionTrack);
		m_action = key;
		m_moveSpeed = 0;
	}
	else {
		float weight = blend? 0: 1;
		m_actionTrack = allocateActionTrack();
		m_state->play(a->animation, speed, AnimationBlend::Add, weight, mode==ActionMode::Loop, -1, m_actionTrack);
		m_lastAction = m_actionTrack;
		setMeta(m_actionTrack, a, ACTION, mode);
		//printf("Start action %s %d\n", a->animation->getName(), m_actionTrack);
		if(speed < 0) m_state->setFrameNormalised(1, m_actionTrack); // start at end if reversed
		updateRootOffset(false);
		m_action = key;
		m_moveSpeed = 0;
	}
}

void AnimationController::endAction() {
	if(m_actionTrack>=0 && m_meta[m_actionTrack].type != MOVEMENT)
		m_actionTrack = -1;
}

void AnimationController::clear() {
	for(int i=m_state->getNextTrack(); i>=0; i=m_state->getNextTrack(i)) {
		if(i==m_idleTrack) m_state->setWeight(1.0, i);
		else {
			m_state->stop(i);
			setMeta(i, 0, ACTION, ActionMode::Hold);
		}
	}
	m_actionTrack = 0;
	m_action = -1;
	m_moveSpeed = 0;
	updateRootOffset(false);
}


// -------------------------------- //


ActionState AnimationController::getState() const {
	if(m_actionTrack<0) return ActionState::Idle;
	else if(!m_action) return ActionState::Moving;
	else if(m_state->isEnded(m_actionTrack)) return ActionState::Ended;
	else return ActionState::Action;
}
AnimationKey AnimationController::getAction() const {
	return m_action;
}
float AnimationController::getProgress() const {
	if(m_lastAction<0) return 0;
	return m_state->getFrameNormalised(m_lastAction);
}
float AnimationController::getWeight() const {
	if(m_lastAction<0) return 0;
	return m_state->getWeight(m_lastAction);
}
float AnimationController::deriveMoveSpeed() const {
	float weight = 0;
	int channels = m_overrideStart<(int)m_meta.size()? m_overrideStart: m_meta.size();
	for(int i=0; i<channels; ++i) {
		if(m_meta[i].type == MOVEMENT)
			weight += m_state->getWeight(i);
	}
	return m_moveSpeed * weight;
}


void AnimationController::setProgress(float p) {
	if(getState() == ActionState::Action) m_state->setFrameNormalised(p, m_actionTrack);
}
void AnimationController::setSpeed(float s) {
	if(getState() == ActionState::Action) m_state->setSpeed(s, m_actionTrack);
}

// -------------------------------- //

void AnimationController::setOverrideStartTrack(int newStart) {
	if(newStart == m_overrideStart) return;
	int lastIndex = m_meta.size() - 1;
	while(lastIndex && !m_meta[lastIndex].animation) --lastIndex;

	int shift = newStart - m_overrideStart;
	if(shift < 0) {
		for(int i=m_overrideStart; i<lastIndex; ++i) {
			if(m_meta[i].animation) {
				m_meta[i+shift] = m_meta[i];
				m_state->swapTracks(i, i+shift);
			}
		}
	}
	else {
		setMeta(lastIndex+shift, 0, ACTION, ActionMode::Hold);
		for(int i=lastIndex; i>=m_overrideStart; --i) {
			if(m_meta[i].animation) {
				m_meta[i+shift] = m_meta[i];
				m_state->swapTracks(i, i+shift);
			}
		}
	}
	m_overrideStart = newStart;
}

void AnimationController::playOverride(const AnimationKey& key, ActionMode mode, bool fade, bool additive) {
	const AnimationBank::AnimationInfo* a = m_bank->getAnimation(key, m_group);
	if(a) {
		AnimationBlend blend = additive? AnimationBlend::Add: AnimationBlend::Mix;
		int track = m_state->play(a->animation, 1, blend, fade?0:1, mode==ActionMode::Loop?1:0);
		setMeta(track, a, OVERRIDE_IN, mode);
		if(track < m_overrideStart) m_overrideStart = track;
	}
}

void AnimationController::stopOverride(const AnimationKey& key, bool fade) {
	if(!key) return;
	for(uint i=0; i<m_meta.size(); ++i) {
		if(m_meta[i].type >= OVERRIDE && m_meta[i].animation->key == key) {
			m_meta[i].type = OVERRIDE_OUT;
			if(!fade) m_state->setWeight(0, i);
		}
	}
}


void AnimationController::forceOverride(const AnimationKey& key, uint track, float frame, float weight, bool additive) {
	track += m_overrideStart;
	if(!m_state->getAnimation(track)) {
		const AnimationBank::AnimationInfo* a = m_bank->getAnimation(key, m_group);
		if(a) {
			m_state->play(a->animation, 0, additive? AnimationBlend::Add: AnimationBlend::Mix, weight, 0, -1, track);
			m_state->setFrameNormalised(frame, track);
			setMeta(track, a, OVERRIDE, ActionMode::Hold);
		}
	}
	else {
		assert(m_meta[track].type >= OVERRIDE); // Invalid track
		m_state->setFrameNormalised(frame, track);
		m_state->setWeight(weight, track);
	}
}

int AnimationController::findOverride(const AnimationKey& key) const {
	if(!key) return -1;
	for(uint i=0; i<m_meta.size(); ++i) {
		if(m_meta[i].type >= OVERRIDE && m_meta[i].animation->key == key) return i;
	}
	return -1;
}

void AnimationController::clearOverrides(bool fade) {
	for(uint i=0; i<m_meta.size(); ++i) {
		if(m_meta[i].type >= OVERRIDE) {
			m_meta[i].type = OVERRIDE_OUT;
			if(!fade) m_state->setWeight(0, i);
		}
	}
}

// -------------------------------- //

void AnimationController::update(float time, bool finalise) {
	const float fade = time / m_fadeTime;
	m_state->update(time, false);	// Advance animations


	// Animation end handler
	for(int i=m_state->getNextTrack(); i>=0; i=m_state->getNextTrack(i)) {
		if(m_state->isEnded(i)) switch(m_meta[i].mode) {
		case ActionMode::End:
			if(i==m_actionTrack) endAction();
			else if(m_meta[i].type == OVERRIDE_IN) m_meta[i].type = OVERRIDE_OUT;
			break;
		case ActionMode::Reverse:
			m_state->setSpeed(-m_state->getSpeed(i), i);
			break;
		case ActionMode::ReverseHold:
			m_state->setSpeed(-m_state->getSpeed(i), i);
			m_meta[i].mode = ActionMode::Hold;
			break;
		case ActionMode::ReverseEnd:
			m_state->setSpeed(-m_state->getSpeed(i), i);
			m_meta[i].mode = ActionMode::End;
			break;
		default: break;
		}
	}
	

	// Track fading and normalisation for actions, idles and movement
	int endSignal = 0;
	int primary = m_actionTrack>=0? m_actionTrack: m_idleTrack;
	if(primary >= 0) {
		float w = m_state->getWeight(primary) + fade;
		if(w > 1) w = 1;
		m_state->setWeight(w, primary);

		float sum = -w;
		for(int i=m_state->getNextTrack(); i>=0; i=m_state->getNextTrack(i)) {
			if(m_meta[i].type>=OVERRIDE) break;
			sum += m_state->getWeight(i);
		}

		float scale = w<1? (1 - w) / sum: 0;
		for(int i=m_state->getNextTrack(); i>=0; i=m_state->getNextTrack(i)) {
			if(m_meta[i].type>=OVERRIDE) break;
			if(i!=primary) {
				if(scale == 0 && i!=m_idleTrack) {
					//printf("Stop %s [%d]\n", m_state->getAnimation(i)->getName(), i);
					m_state->setWeight(0, i);
					endSignal = i+1;
					if(i==m_lastAction) {
						m_lastAction = -1;
						m_action = -1;
					}
				}
				else if(sum!=0) m_state->setWeight( m_state->getWeight(i)*scale, i);
			}
		}
	}

	// Fade override tracks
	for(int i=m_state->getNextTrack(endSignal-1); i>=0; i=m_state->getNextTrack(i)) {
		if(m_meta[i].type == OVERRIDE_IN) m_state->fadeIn(fade, i);
		else if(m_meta[i].type == OVERRIDE_OUT) m_state->fadeOut(fade, i);
	}


	// Calculate movement deltas
	if(m_rootMotion) {
		updateRootOffset(true);
		Bone* root = m_state->getSkeleton()->getBone(m_rootBone);
		getSkeleton()->resetPose();
		m_state->apply(false);
		root->setPosition( vec3() );
		root->setAngle( Quaternion() );
		if(finalise) getSkeleton()->update();
	}
	else {
		getSkeleton()->resetPose();
		m_state->apply(finalise);
	}


	// Clean up ended tracks
	bool updateOffsets = false;
	for(int i=0; i<endSignal; ++i) {
		if(i==m_idleTrack) continue;
		if(i!=m_actionTrack && m_state->getAnimation(i) && m_state->getWeight(i)==0) {
			updateOffsets |= m_state->getKeyMap(i)[m_rootBone]>=0;
			m_state->stop(i);
			setMeta(i, 0, ACTION, ActionMode::Hold);
		}
		if(!m_state->getAnimation(i) && m_state->getAnimation(i+1) && m_meta[i+1].type < OVERRIDE) {
			//printf("Swap %d -> %d\n", i, i+1);
			m_state->swapTracks(i, i+1);
			m_meta[i] = m_meta[i+1];
			setMeta(i+1, 0, ACTION, ActionMode::Hold);
			if(m_lastAction == i) m_lastAction = -1;
			if(i+1==m_actionTrack) m_actionTrack = i;
			if(i+1==m_lastAction) m_lastAction = i;
			if(i+1==m_idleTrack) m_idleTrack = i;
		}
	}

	// Stop ended overrides - TODO: fill gaps. Perhaps merge with above. Dont change forced tracks
	for(int i=m_state->getNextTrack(endSignal-1); i>=0; i=m_state->getNextTrack(i)) {
		if(m_meta[i].type == OVERRIDE_OUT && m_state->getWeight(i)==0) {
			updateOffsets |= m_state->getKeyMap(i)[m_rootBone]>=0;
			m_state->stop(i);
			setMeta(i, 0, ACTION, ActionMode::Hold);
		}
	}

	if(updateOffsets) updateRootOffset(false);
}


void AnimationController::updateRootOffset(bool output) {
	if(!m_rootMotion) return;
	static const Quaternion identity;

	if(output) {
		m_offset.set(0,0,0);
		m_rotation = identity;
	}

	for(int i=m_state->getNextTrack(); i>=0; i=m_state->getNextTrack(i)) {
		int rootSet = m_state->getKeyMap(i)[m_rootBone];
		if(rootSet>=0) {
			Meta& meta = m_meta[i];

			vec3 pos;
			Quaternion rot;
			m_state->getAnimation(i)->getPosition(rootSet, m_state->getFrame(i), 0, pos);
			m_state->getAnimation(i)->getRotation(rootSet, m_state->getFrame(i), 0, rot);

			if(output) {
				const float weight = m_state->getWeight(i);
				vec3 deltaPos = pos - meta.lastPos;
				Quaternion deltaRot = rot * meta.lastRot.getInverse();

				// Problems if wanting full rotation animation
				if(meta.type != ACTION) {
					deltaPos *= weight;
					if(deltaRot.w < 1)
						deltaRot = slerp(identity, deltaRot, weight); 
				}
				
				// Stop jumping back when animation loops
				if(m_state->getLooped(i)) {
					deltaPos += meta.animation->loopOffset * weight * m_state->getLooped(i);
					if(fabs(meta.animation->loopRotation.w) < 1) {
						for(int i=0; i<m_state->getLooped(i); ++i) deltaRot *= meta.animation->loopRotation;
					}
				}

				// Need to undo object rotation to keep things going in the right direction
				Quaternion base = m_rootRest * rot.getInverse();

				m_offset += base * deltaPos;
				m_rotation *= deltaRot;
			}

			meta.lastPos = pos;
			meta.lastRot = rot;
		}
	}

	if(output) m_rotation = m_rootRest * m_rotation * m_rootSkin; // Object space rotation correction
}

