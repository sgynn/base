#include <base/model.h>
#include <cstdio>

using namespace base;


AnimationState::AnimationState(Skeleton* s) : m_skeleton(s) {
}
AnimationState::~AnimationState() {
}

int AnimationState::getUnusedTrack() const {
	for(size_t i=0; i<m_animations.size(); ++i) {
		if(!m_animations[i].animation) return i;
	}
	return m_animations.size();
}

int AnimationState::allocateTrack() {
	int track = getUnusedTrack();
	allocateTrack(track);
	return track;
}

int AnimationState::allocateTrack(unsigned t) {
	if(t>=m_animations.size()) {
		AnimationInstance empty;
		empty.animation = 0;
		m_animations.resize(t+1, empty);
	}
	return t;
}

int AnimationState::play(const Animation* anim, float speed, AnimationBlend blend, float weight, char loop, int bone, int track) {
	if(!anim) return -1;
	if(track<0) track = allocateTrack();
	else allocateTrack(track);

	AnimationInstance& a = m_animations[track];
	a.animation = anim;
	a.blend     = blend;
	a.bone      = bone;
	a.weight    = weight;
	a.speed     = speed;
	a.frame     = 0;
	a.loop      = loop<0? anim->isLoop(): loop;
	a.looped    = 0;
	a.changed   = true;
	a.keyMap    = m_skeleton? anim->getMap(m_skeleton): 0;
	return track;
}

int AnimationState::getTrack(const Animation* anim, int bone) const {
	for(size_t i=0; i<m_animations.size(); ++i) {
		if(m_animations[i].animation == anim && m_animations[i].bone==bone) return i;
	}
	return -1;
}

void AnimationState::stopAll() {
	m_animations.clear();
}

void AnimationState::stop(int track) {
	if((uint) track >= m_animations.size()) return;
	m_animations[track].animation = 0;
}
void AnimationState::setSpeed(float speed, int track) {
	m_animations[track].speed = speed;
}
void AnimationState::setFrame(float frame, int track) {
	m_animations[track].changed |= frame != m_animations[track].frame;
	m_animations[track].frame = frame;
}
void AnimationState::setFrameNormalised(float frame, int track) {
	frame *= m_animations[track].animation->getLength() - 1;
	m_animations[track].frame = frame;
}
void AnimationState::setWeight(float weight, int track) {
	m_animations[track].changed |= weight != m_animations[track].weight;
	m_animations[track].weight = weight;
}

float AnimationState::getSpeed(int track) const {
	return m_animations[track].speed;
}
float AnimationState::getFrame(int track) const {
	return m_animations[track].frame;
}
float AnimationState::getFrameNormalised(int track) const {
	const AnimationInstance& state = m_animations[track];
	return state.frame / (state.animation->getLength() - 1);
}
float AnimationState::getWeight(int track) const {
	return m_animations[track].weight;
}

AnimationBlend AnimationState::getBlend(int track) const {
	return m_animations[track].blend;
}

bool AnimationState::getLoop(int track) const {
	return m_animations[track].loop;
}
void AnimationState::setLoop(bool loop, int track) {
	m_animations[track].loop = loop;
}
int AnimationState::getLooped(int track) const {
	return m_animations[track].looped;
}
const char* AnimationState::getKeyMap(int track) const {
	return m_animations[track].keyMap;
}

bool AnimationState::isEnded(int track) const {
	if((uint) track >= m_animations.size()) return true;
	const AnimationInstance& inst = m_animations[track];
	if(!inst.animation) return true;
	if(inst.loop) return false;
	if(inst.frame==inst.animation->getLength() && inst.speed>0) return true;
	if(inst.frame==0 && inst.speed<0) return true;
	return false;
}

const Animation* AnimationState::getAnimation(int track) const {
	if((uint) track >= m_animations.size()) return 0;
	return m_animations[track].animation;
}

void AnimationState::swapTracks(int a, int b) {
	if(a==b) return;
	allocateTrack(a);
	allocateTrack(b);
	AnimationInstance tmp = m_animations[a];
	m_animations[a] = m_animations[b];
	m_animations[b] = tmp;
}

int AnimationState::getNextTrack(int t) const {
	while(++t < (int)m_animations.size())
		if(m_animations[t].animation) return t;
	return -1;
}

bool AnimationState::fadeIn(float v, int track) {
	float w = getWeight(track) + v;
	setWeight(w<1? w: 1, track);
	return w<1;
}
bool AnimationState::fadeOut(float v, int track) {
	float w = getWeight(track) - v;
	setWeight(w>0? w: 0, track);
	return w>0;
}

void AnimationState::advance(float amount, int track) {
	AnimationInstance& state = m_animations[track];
	if(state.animation) {
		state.frame += amount;
		const float length = state.animation->getLength();
		if(state.loop) {
			state.frame -= floor(state.frame/length) * length;	// Wrap
		}
		else {
			if(state.frame<0) state.frame = 0;
			else if(state.frame>length) state.frame = length;
		}
	}
}

bool AnimationState::update(float t, bool applyPose) {
	// Update animation frames
	int changed = 0;
	for(size_t i=0; i<m_animations.size(); ++i) {
		AnimationInstance& state = m_animations[i];
		if(state.animation) {
			if(state.speed != 0) {
				float delta = state.speed * state.animation->getSpeed() * t;
				float lastFrame = state.frame;
				state.frame += delta;

				// loop or clamp animation
				int length = state.animation->getLength();
				if(state.loop && length) {
					state.looped = floor(state.frame/length);
					state.frame -= state.looped * length;	// Wrap
				} else {
					if(state.frame<0) state.frame = 0;
					else if(state.frame>length) state.frame = length;
				}
				state.changed |= state.frame != lastFrame;
			}
			else state.looped = 0;
			if(state.changed) ++changed;
			state.changed = false;
		}
	}
	if(applyPose && changed) return apply();
	return changed;
}

bool AnimationState::apply(bool update) const {
	int changed = 0;
	//m_skeleton->resetPose();	// Only need to do this if there are additive layers
	for(size_t i=0; i<m_animations.size(); ++i) {
		const AnimationInstance& state = m_animations[i];
		if(state.animation && (state.weight!=0 || state.blend==AnimationBlend::Set)) {
			changed += m_skeleton->applyPose(state.animation, state.frame, state.bone, (int)state.blend, state.weight, state.keyMap);
		}
	}

	// Calculate skeleton matrices
	if(!update) return changed;
	else if(changed) {
		return m_skeleton->update();
	}
	else return false;
}

