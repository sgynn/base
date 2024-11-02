#include <base/audio.h>
#include <base/thread.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <list>

#include "audioclass.h"

namespace audio {
	enum MessageType { 
		LOAD_BANK, UNLOAD_BANK, LOAD_SOUND,
		GLOBAL_VALUE, SET_VALUE, SET_ENUM, 
		SET_POSITION, SET_VELOCITY, PLAY,
		LISTENER_POS, LISTENER_ROT,
		STOP, STOP_ALL, EVENT, SHUTDOWN
	};
	struct Message {
		MessageType type;
		objectID object;
		objectID target;
		char* string;
		char  flags;
		union {
			char*   stringValue;
			int     intValue;
			float   floatValue;
			float   vecValue[3];
		};
	};

	std::list<Message> messages;
	base::Thread mainThread;
	base::Mutex  mutex;
	base::Mutex  deleteMutex;
	void audioMainLoop(int);
	void setupAudioSystem();
	void destroyAudioSystem();
	bool processAudioMessages();
	void pushMessage(const Message&);
	bool singleThreadedMode = false;
	Data* Data::instance = 0;
	ALCcontext* context = 0;
	ALCdevice* device = 0;
};

extern void audioLogMessage(const char* message);


//// Setup ////
int audio::initialise(bool threaded) {
	if(mainThread.running()) return -1;
	Data::instance = new Data();
	Data::instance->m_mixers.push_back( Mixer() );
	Data::instance->m_mixers[0].volume.value = 1.0;
	Data::instance->m_mixers[0].volume.variable = 0;
	Data::instance->m_mixers[0].targetID = 0;
	Data::instance->m_mixerMap["master"] = 0;
	if(threaded) mainThread.begin(audioMainLoop, 0);
	else setupAudioSystem();
	singleThreadedMode = !threaded;
	audioLogMessage("Audio systems initialised\n");
	return 0;
}
int audio::shutdown() {
	Message m;
	m.type = SHUTDOWN;
	m.string = 0;
	pushMessage(m);
	if(singleThreadedMode) processAudioMessages();
	mainThread.join();
	delete Data::instance;
	Data::instance = 0;
	if(singleThreadedMode) destroyAudioSystem();
	audioLogMessage("Audio systems shut down\n");
	return 0;
}
void audio::pushMessage(const Message& m) {
	base::MutexLock lock(audio::mutex);
	audio::messages.push_back(m);
}

//// MAIN INTERFACE ////
void audio::playSound(objectID obj, soundID s) {
	Message m;
	m.type    = PLAY;
	m.object  = obj;
	m.target  = s;
	m.string  = 0;
	m.flags   = 0;
	pushMessage(m);
}
void audio::playSound(objectID obj, const char* s) {
	Message m;
	m.type    = PLAY;
	m.object  = obj;
	m.target  = 0;
	m.string  = strdup(s);
	m.flags   = 0;
	pushMessage(m);
}
void audio::stopSound(objectID obj, soundID s) {
	Message m;
	m.type    = STOP;
	m.object  = obj;
	m.target  = s;
	m.string  = 0;
	m.flags   = 0;
	pushMessage(m);
}
void audio::stopSound(objectID obj, const char* s) {
	Message m;
	m.type    = STOP;
	m.object  = obj;
	m.target  = 0;
	m.string  = strdup(s);
	m.flags   = 0;
	pushMessage(m);
}
void audio::stopAll(objectID obj) {
	Message m;
	m.type    = STOP_ALL;
	m.object  = obj;
	m.string  = 0;
	pushMessage(m);
}
void audio::fireEvent(objectID obj, const char* event) {
	Message m;
	m.type    = EVENT;
	m.object  = obj;
	m.target  = 0;
	m.string  = strdup(event);
	m.flags   = 0;
	pushMessage(m);
}

void audio::setEnum(objectID obj, const char* e, const char* value) {
	Message m;
	m.type    = SET_ENUM;
	m.object  = obj;
	m.target  = 0;
	m.string  = strdup(e);
	m.flags   = 1;
	m.stringValue = strdup(value);
	pushMessage(m);
}
void audio::setEnum(objectID obj, const char* e, int value) {
	Message m;
	m.type     = SET_ENUM;
	m.object   = obj;
	m.target   = 0;
	m.string   = strdup(e);
	m.flags    = 0;
	m.intValue = value;
	pushMessage(m);
}
void audio::setValue(objectID obj, const char* variable, float value) {
	Message m;
	m.type    = SET_VALUE;
	m.object  = obj;
	m.target  = 0;
	m.string  = strdup(variable);
	m.flags   = 0;
	m.floatValue = value;
	pushMessage(m);
}
void audio::setPosition(objectID obj, const float* pos) {
	Message m;
	m.type     = SET_POSITION;
	m.object   = obj;
	m.target   = 0;
	m.string   = 0;
	m.flags    = 0;
	m.vecValue[0] = pos[0];
	m.vecValue[1] = pos[1];
	m.vecValue[2] = pos[2];
	pushMessage(m);
}
void audio::setPosition(objectID obj, const float* pos, const float* dir) {
	setPosition(obj, pos);
}

//// //// GLOBAL MESSAGES //// ////

void audio::setValue(const char* name, float value) {
	Message m;
	m.type    = GLOBAL_VALUE;
	m.string  = strdup(name);
	m.floatValue = value;
	pushMessage(m);
}
void audio::setListener(const float* p) {
	Message m;
	m.type     = LISTENER_POS;
	m.string   = 0;
	memcpy(m.vecValue, p, 3*sizeof(float));
	pushMessage(m);

	if(singleThreadedMode) processAudioMessages();
}

//// //// LOAD BANKS //// ////

void audio::load(const char* file) {
	Message m;
	m.type     = LOAD_BANK;
	m.string   = strdup(file);
	messages.push_back(m);
}
void audio::unload(const char* file) {
	Message m;
	m.type     = UNLOAD_BANK;
	m.string   = strdup(file);
	messages.push_back(m);
}


//// //// OBJECT MANAGEMENT //// ////
audio::objectID audio::createObject() {
	Object* o = new Object();
	memset(o->position, 0, 6*sizeof(float));
	vector<Object*>& objects = Data::instance->m_objects;
	// Can we fill any holes?
	for(objectID id=1; id<objects.size(); ++id) {
		if(!objects[id]) {
			o->id = id;
			objects[id] = o;
			printf("Created audio object %u\n", id);
			return id;
		}
	}
	o->id = objects.size();
	objects.push_back(o);
	printf("Created audio object %u\n", o->id);
	return o->id;
}
void audio::destroyObject(objectID id) {
	vector<Object*>& objects = Data::instance->m_objects;
	if(id<objects.size() && objects[id]) {
		printf("Destroyed audio object %u\n", id);
		objects[id]->stopAll();

		// stop any sounds
		deleteMutex.lock();
		Data::instance->m_deleted.push_back(objects[id]);
		objects[id] = 0;
		deleteMutex.unlock();
	}
}






//// MAIN AUDIO LOOP ////
void audio::audioMainLoop(int) {
	setupAudioSystem();
	while(processAudioMessages()) {
		#ifndef EMSCRIPTEN
		base::Thread::sleep(1);
		#endif
	}
	destroyAudioSystem();
}

void audio::setupAudioSystem() {
	// Initialise audio
	device = alcOpenDevice(0);
	if(!device) {
		audioLogMessage("Failed to open audio device\n");
		return;
	}
	context = alcCreateContext(device, 0);
	if(!context) {
		alcCloseDevice(device);
		audioLogMessage("Failed to create context\n");
		return;
	}
	if(!alcMakeContextCurrent(context)) {
		alcDestroyContext(context);
		alcCloseDevice(device);
		audioLogMessage("Failed to initialise audio\n");
		return;
	}

	printf("E: %#x\n", alGetError());

	// Set up listener
	float f[] = { 0, 0, 0, 1, 0, 1, 0 };
	alListenerfv(AL_POSITION, f);		// f3
	alListenerfv(AL_ORIENTATION, f+1);	// f6 (at, up)
	alListenerfv(AL_VELOCITY, f);		// f3
}

bool audio::processAudioMessages() {
	bool running = true;
	Object* object = 0;
	Data* data = Data::instance;
	while(!messages.empty()) {
		mutex.lock();
		Message m = messages.front();
		messages.pop_front();
		mutex.unlock();
		// Handle message
		switch(m.type) {
		case LOAD_BANK:		// background load
			data->loadSoundBank(m.string);
			break;

		case UNLOAD_BANK:	// Background unload
			data->unloadSoundBank(m.string);
			break;

		case LOAD_SOUND:	// Load a sound file
			printf("LOAD_SOUND not implemented\n");
			break;

		case LISTENER_POS:	// Listener position
			alListenerfv(AL_POSITION, m.vecValue);
			break;
		case LISTENER_ROT:	// Listener forward vector, and up vector. Must be orthogonal FIXME
			alListenerfv(AL_ORIENTATION, m.vecValue);
			break;

		case GLOBAL_VALUE:	// Mixer variables ?
			m.target = Data::lookup( m.string, Data::instance->m_variableMap );
			if(m.target == INVALID) printf("Error: variable %s not found\n", m.string);
			else {
				data->m_variables[m.target].value = m.floatValue;
				for(size_t i=0; i<data->m_mixers.size(); ++i) {
					Mixer& mixer = data->m_mixers[i];
					mixer.setVar(m.target, m.floatValue);
				}
			}
			break;

		case SET_VALUE:
			object = data->getObject(m.object);
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			m.target = Data::lookup( m.string, Data::instance->m_variableMap );
			if(m.target==INVALID) printf("Error: variable %s not found\n", m.string);
			if(object && m.target!=INVALID) object->setVar(m.target, m.floatValue);
			break;
			
		case SET_ENUM:
			object = data->getObject(m.object);
			m.target = Data::lookup( m.string, Data::instance->m_enumMap );
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			if(m.target==INVALID) printf("Error: variable %s not found\n", m.string);
			if(m.target!=INVALID && object) {
				size_t value = m.intValue;
				if(m.flags) {
					value = Data::lookup( m.stringValue, data->m_enums[m.target].names );
					if(value==INVALID) printf("Error: Enum %s has no member %s\n", m.string, m.stringValue);
					free(m.stringValue);
				}
				if(value != INVALID) object->setEnum(m.target, value);
			}
			break;

		case SET_POSITION:
			object = data->getObject(m.object);
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			if(object) object->setPosition(m.vecValue);
			break;
		case SET_VELOCITY: // TODO
			break;

		case PLAY:
			object = data->getObject(m.object);
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			if(m.string) m.target = Data::lookup( m.string, data->m_soundMap );
			if(m.target==INVALID) printf("Error: Sound %s not found\n", m.string);
			if(object && m.target!=INVALID) object->play(m.target);
			break;

		case STOP:
			object = data->getObject(m.object);
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			if(m.string) m.target = Data::lookup( m.string, data->m_soundMap );
			if(object && m.target!=INVALID) object->stop(m.target);
			break;

		case STOP_ALL:
			object = data->getObject(m.object);
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			if(object && m.target!=INVALID) object->stopAll();
			break;

		case EVENT:
			object = data->getObject(m.object);
			if(!object) printf("Error: Object %d does not exist\n", m.object);
			if(m.string) m.target = Data::lookup( m.string, data->m_eventMap );
			// Fire event
			break;

		case SHUTDOWN:
			for(Object* o: Data::instance->m_objects) delete o;
			for(SoundBank* bank: data->m_banks) data->unloadSoundBank(bank);
			running = false;
			break;
		}
		if(m.string) free(m.string);
	}

	// Update streams etc
	


	// End events
	if(!data->m_endEvents.empty()) {
		TimeStamp time = data->getTimeStamp();
		while(time >= data->m_endEvents.front().time) {
			Object* o = data->m_endEvents.front().object;
			if(o) o->onEndEvent(data->m_endEvents.front());
			data->m_endEvents.erase(data->m_endEvents.begin());
			if(data->m_endEvents.empty()) break;
		}
	}

	// Delete objects
	if(!data->m_deleted.empty()) {
		deleteMutex.lock();
		for(unsigned i=0; i<data->m_deleted.size(); ++i) {
			for(Data::EndEvent& e : data->m_endEvents) if(e.object == data->m_deleted[i]) e.object = 0;
			delete data->m_deleted[i];
		}
		data->m_deleted.clear();
		deleteMutex.unlock();
	}

	return running;
}

void audio::destroyAudioSystem() {
	alcDestroyContext(context);
	alcCloseDevice(device);
}

