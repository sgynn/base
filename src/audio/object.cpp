#include "audioclass.h"
#include "source.h"

#include <AL/al.h>
#include <cstdlib>
#include <cstdio>


using namespace audio;


float Mixer::getVolume() const {
	if(targetID > 0) return volume.value * Data::instance->m_mixers[ targetID ].getVolume();
	else return volume.value;
}
void Mixer::setVar(objectID var, float v) {
	if(volume.variable == var && volume.value != v) {
		volume.value = v;
		updateVolumes( getVolume() );
	}
}
void Mixer::updateVolumes(float v) const {
	v *= volume.value;
	for(size_t i=0; i<children.size(); ++i) {
		Data::instance->m_mixers[ children[i] ].updateVolumes(v);
	}
	for(size_t i=0; i<instances.size(); ++i) {
		Object* object = Data::instance->getObject(instances[i]&0x3ff);
		object->updateVolume( object->playing[ instances[i]>>18 ], v);
	}
}
void Mixer::addInstance(Object* o, int i) {
	unsigned key = o->id | i<<18;
	instances.push_back(key);
}
void Mixer::removeInstance(Object* o, int i) {
	unsigned key = o->id | i<<18;
	// Remove from instances list, order not preserved
	for(size_t i=0; i<instances.size(); ++i) {
		if(instances[i] == key) {
			instances[i] = instances.back();
			instances.pop_back();
			break;
		}
	}
}


// ================================================== //


Object::Object() {
	position[0] = position[1] = position[2] = 0;
	velocity[0] = velocity[1] = velocity[2] = 0;
}
Object::~Object() {
	stopAll();
}

int Object::random(int n) {
	static unsigned a = rand(); // should probably get from system time
	a = 36969 * (a&0xffff) + (a>>16);
	return a % n;
}

int Object::play(soundID id) {
	// get sound data
	const SoundBank* bank = Data::instance->getBank(id);
	const SoundBase* base = Data::instance->lookupSound(id);
	if(!base) return -1; // Sound does not exist / not loaded
	const SoundBase* sound = base;
	
	SoundInstance inst;
	const Group* group;
	unsigned index;

	// Select sound
	while(sound) {
		switch(sound->type) {
		case SOUND:
			inst.sound = static_cast<const Sound*>(sound);
			sound = 0;
			group = 0;
			break;
		case GROUP: return -2;// Should not be playing this
		case RANDOM:
			group = static_cast<const Group*>(sound);
			index = random( group->sounds.size() );
			break;
		case SEQUENCE:
			group = static_cast<const Group*>(sound);
			index = 0;
			break;
		case SWITCH:
			group = static_cast<const Group*>(sound);
			index = Data::instance->m_enums[ group->switchID ].value;
			for(unsigned i=0; i<enums.size(); ++i)
				if(enums[i].id == group->switchID) { index = enums[i].value; break; }
			break;
		}
		if(group) {
			inst.stack.push_back( GroupInstance() );
			inst.stack.back().id = Data::getSoundID( group->sounds[ index ], bank );
			inst.stack.back().loop = group->loop;
			inst.stack.back().index = index;
			sound = bank->data[ group->sounds[index] ];
			if(!sound) {
				soundID id = Data::getSoundID(group->sounds[index], bank);
				const char* name = Data::instance->findSoundName(id);
				printf("Error: Sound %s not defined\n", name);
				return -1;
			}
		}
	}

	if(!inst.sound->source) {
		printf("Error: Sound %s not loaded\n", inst.sound->file);
		return -1;
	}
	
	// Allocate instance index
	for(index=0; index<playing.size(); ++index) if(!playing[index].sound) break;
	if(index < playing.size()) inst.source = playing[index].source;
	
	// Get mixer
	float volume = 1.0;
	if(inst.sound->targetID>0) {
		Mixer& mixer = Data::instance->m_mixers[ inst.sound->targetID ];
		mixer.addInstance(this, index);
		volume = mixer.getVolume();
	}
	
	// Attenuation
	if(inst.sound->attenuationID > 0) {
		// TODO
	}

	
	// We should have a sound instance to start playing at this point
	//printf("Playing %s\n", inst.sound->file);
	if(!inst.source) alGenSources(1, &inst.source);
	alSourcei(inst.source, AL_BUFFER, inst.sound->source->buffers[0]);
	alSourcef(inst.source, AL_GAIN, volume);
	alSourcef(inst.source, AL_PITCH, 1.0);
	alSourcefv(inst.source, AL_POSITION, position);
	alSourcefv(inst.source, AL_VELOCITY, velocity);
	
	if(inst.sound->flags & LOOP) alSourcei(inst.source, AL_LOOPING, AL_TRUE);


	alSourcePlay(inst.source);

	inst.id = id;
	inst.loop = inst.sound->loop;
	inst.endTime = Data::getTimeStamp() + inst.sound->source->length;
	if(index<playing.size()) playing[index] = inst;
	else playing.push_back( inst );

	// ToDo: allow for a 'silence' sound
	
	// Insert into endevents list
	if(!(inst.sound->flags&LOOP) || inst.loop>0) {
		insertEndEvent(index);
	}
	return 0;
}

void Object::insertEndEvent(unsigned index) {
	Data::EndEvent e { playing[index].endTime, this, index };
	vector<Data::EndEvent>& list = Data::instance->m_endEvents;
	if(list.empty()) list.push_back(e);
	else if(e.time <= list.front().time) list.insert(list.begin(), e);
	else if(e.time >= list.back().time) list.push_back(e);
	else {
		size_t a = 0;
		size_t b = list.size()-1;
		while(b > a + 1) {
			size_t c = (a+b)/2;
			if(e.time < list[c].time) b = c;
			else a = c;
		}
		size_t index = a=b? b+1: b;
		list.insert(list.begin()+index, e);
	}
}

void Object::onEndEvent(const Data::EndEvent& e) {
	if(e.index >= playing.size()) return;
	SoundInstance& inst = playing[e.index];
	if(!inst.sound || inst.endTime > e.time) return; // Event no longer valid

	// Loop counting
	if(inst.loop>0) {
		printf("Finished loop %s\n", inst.sound->file);
		if(inst.loop == 1) alSourcei(inst.source, AL_LOOPING, AL_FALSE);
		inst.endTime += inst.sound->source->length;
		insertEndEvent(e.index);
		--inst.loop;
	}

	//printf("Finished playing %s\n", inst.sound->file);
	
	// if we are in the final loop, need to call: alSourcei(alid, AL_LOOPING, 0);
	// if there is nothing else to play, this should not get called.
	// if there is more stuff, load into a second buffer to seamlessly transition over.
	// Need to handle buffer underrun
	
	// Also, need to handle enum changes for playing sounds
	

	if(!inst.stack.empty()) {
		// run the next thing in the stack

	}


	// Stop sound
	if(inst.sound->targetID>0) {
		alSourceStop(inst.source); // to be sure - possibly fixes a bug
		Data::instance->m_mixers[ inst.sound->targetID ].removeInstance(this, e.index);
		inst.sound = 0;
		inst.id = INVALID;
	}
}

int Object::stop(soundID sid) {
	// Get playing sound
	for(size_t i=0; i<playing.size(); ++i) {
		SoundInstance& inst = playing[i];
		if(inst.id == sid) {
			alSourceStop(inst.source);
			alDeleteSources(1, &inst.source); // TODO: Reuse sources !
			if(inst.sound->targetID>0) Data::instance->m_mixers[ inst.sound->targetID ].removeInstance(this, i);
			inst.source = 0;
			inst.sound = 0; // removed
			inst.id = INVALID;
			return 0;
		}
	}
	return 0;
}
void Object::stopAll() {
	for(size_t i=0; i<playing.size(); ++i) {
		SoundInstance& inst = playing[i];
		if(inst.sound) {
			alSourceStop(inst.source);
			alDeleteSources(1, &inst.source);
			if(inst.sound->targetID>0) Data::instance->m_mixers[ inst.sound->targetID ].removeInstance(this, i);
		}
		else if(inst.source) {
			alDeleteSources(1, &inst.source);
		}
	}
	playing.clear();
}

void Object::setPosition(const float* pos) {
	memcpy(position, pos, sizeof(position));
	for(size_t i=0; i<playing.size(); ++i) {
		if(playing[i].sound) {
			alSourcefv(playing[i].source, AL_POSITION, pos);
		}
	}
}

void Object::updateVolume(SoundInstance& s, float volume) {
	alSourcef(s.source, AL_GAIN, volume);
}

int Object::setVar(unsigned id, float value) {
	for(size_t i=0; i<variables.size(); ++i) {
		if(variables[i].id == id) {
			variables[i].value = value;
			// Affect any playing sounds - gain / pitch
			return 0;
		}
	}
	VariableValue v;
	v.id = id;
	v.value=value;
	variables.push_back(v);
	// Affect any playing sounds
	return 0;
}

int Object::setEnum(unsigned id, int value) {
	for(size_t i=0; i<enums.size(); ++i) {
		if(enums[i].id == id) {
			enums[i].value = value;
			// Affect any playing sounds - possibly call a switch to another sound
			return 0;
		}
	}
	EnumValue e;
	e.id = id;
	e.value=value;
	enums.push_back(e);
	// Affect any playing sounds
	return 0;
}


// =============================== //

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <sys/time.h>
TimeStamp Data::getTimeStamp() {
	#ifdef WIN32
	LARGE_INTEGER t, f;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&f);
	TimeStamp tv = ((TimeStamp)t.HighPart << 32) + (TimeStamp)t.LowPart;
	TimeStamp fv = ((TimeStamp)f.HighPart << 32) + (TimeStamp)f.LowPart;
	return tv * 1000000ull / fv;
	#else
	struct timeval now;
	gettimeofday(&now, 0);
	return (TimeStamp)now.tv_sec * 1000000ull + (TimeStamp)now.tv_usec;
	#endif
}


