#pragma once

#include <base/audio.h>
#include <base/hashmap.h>
#include <vector>

namespace audio {
	class Source;
	struct Object;
	using std::vector;

	static const objectID INVALID = -1;

	typedef unsigned long long TimeStamp;
	typedef base::HashMap<unsigned> LookupMap;

	/** Variables */
	struct Variable { float value=0, min=0, max=1; };
	struct Enum { int value=0; LookupMap names; };

	/** Value that can be attached to a variable */
	struct Value { float value=0; objectID variable=INVALID; float variance=0; };

	enum SoundFlags { LOOP=1, TYPE_3D=2, STREAM=4 };
	enum SoundType  { SOUND=1, GROUP, RANDOM, SEQUENCE, SWITCH };

	/** Abstract base class for sounds and sound groups */
	struct SoundBase {
		int type = 0;	// SoundType enum
		int loop = 0;	// count, 0=infinite
	};

	/** Sound definition */
	struct Sound : public SoundBase {
		Source* source = nullptr;
		char    file[64];
		soundID attenuationID = INVALID;
		soundID targetID = 0;
		Value   gain;
		Value   pitch;
		int     flags = 0;
		float   fadeIn = 0, fadeOut = 0;
	};

	/** Sound groups can cause sounds to play differetly */
	struct Group : public SoundBase {
		unsigned switchID = INVALID;	// Switch runtime ID if type==SWITCH
		vector<unsigned> sounds;		// Can be a sound or a group - must be from in current soundbank
	};


	/** Mixers hopefully can amalgamate lots of sounds */
	struct Mixer {
		soundID mixerID;	// ID of this object
		soundID targetID;	// Target mixer for chaining
		Value   volume;		// Master volume

		vector<unsigned> instances;		// Active sounds (objectID | instaceIndex<<18)
		vector<objectID> children;		// Child mixers
		float getVolume() const;		// derive volume of mixer chain
		void  setVar(objectID var, float value);
		void  updateVolumes(float) const;
		void  addInstance(Object*,int);		// Add sound instance to this mixer
		void  removeInstance(Object*,int);	// Remove sound instance from this mixer
	};

	// Attenuation object
	struct Attenuation {
		float falloff = 0;
		float distance = 0;
	};

	/** Soundbanks hold sound data */
	struct SoundBank {
		const char* file;			// Soundbank name
		vector<SoundBase*> data;	// Sound and group data
		unsigned id;				// Bank index
	};

	/** Events can make multiple changes in a single call */
	struct Event {
		int type = 0;			// Play, Stop, Set
		int target = INVALID;	// SoundID or variableID
		float value = 0;		// Value to set if type==set
	};
	typedef vector<Event> EventList;


	/** A class to hold instanciated data */
	class Data {
		public:
		vector<SoundBank*>      m_banks;		// Sound data
		vector<Attenuation>     m_attenuations;	// Attenuation data
		vector<Mixer>           m_mixers;		// Mixer data
		vector<Variable>        m_variables;	// Variable definitions
		vector<Enum>            m_enums;		// Enum definitions
		vector<EventList>       m_events;		// Event info

		LookupMap m_soundMap;
		LookupMap m_attenuationMap;
		LookupMap m_mixerMap;
		LookupMap m_variableMap;
		LookupMap m_enumMap;
		LookupMap m_eventMap;
		
		base::HashMap<Source*(*)()> m_plugins;		// Sound loader plugins
		bool loadSoundBank(const char* file);
		bool unloadSoundBank(const char* file);
		void unloadSoundBank(SoundBank*);
		bool loadSource(Sound*);
		void unloadSource(Sound*);

		static Data* instance;
		template<typename T> objectID declare(const char* name, LookupMap& map, vector<T>& list);
		objectID declareSound(SoundBank* bank, const char* name, bool create=false);

		static soundID   getSoundID(unsigned local, const SoundBank* bank) { return local | bank->id<<20; }
		static unsigned  getLocalIndex(soundID id) { return id&0xfffff; }
		const SoundBase* lookupSound(soundID id) const { return getBank(id)->data[ getLocalIndex(id) ]; }
		const SoundBank* getBank(soundID id) const { return m_banks[id>>20]; }
		const char*      findSoundName(soundID) const;	// For debug - slow

		static size_t lookup(const char* name, const LookupMap& map) {
			LookupMap::const_iterator i = map.find(name);
			return i==map.end()? INVALID: i->value;
		}

		inline Object* getObject(objectID id) {
			return id<Data::instance->m_objects.size()? Data::instance->m_objects[id]: 0;
		}

		static TimeStamp getTimeStamp();
		vector<Object*> m_objects;
		vector<Object*> m_deleted;
		

		struct EndEvent { TimeStamp time; Object* object; unsigned index; };
		vector<EndEvent> m_endEvents;
	};


	/** Playlist information */
	struct GroupInstance {
		soundID id;
		unsigned char loop;
		unsigned char index;
	};

	/** Playing sound instance */
	struct SoundInstance {
		SoundInstance() : sound(0), source(0), loop(0), stack(4) {}
		const Sound*    sound;		// Sound data
		unsigned        source;		// OpenAL Source
		unsigned char   loop;		// Loop number ?
		soundID         id;			// Sound ID (for events and stopping)
		TimeStamp       endTime;	// Time when the sound ends

		vector<GroupInstance> stack;	// Group stack for sequences etc
	};

	struct EnumValue     { unsigned id; int   value; };
	struct VariableValue { unsigned id; float value; };

	/** Game object */
	struct Object {
		objectID id = 0;
		float position[3];
		float velocity[3];

		vector<EnumValue>     enums;
		vector<VariableValue> variables;
		vector<SoundInstance> playing;

		int   play(soundID sound);
		int   stop(soundID sound);
		void  stopAll();
		int   setEnum(unsigned id, int value);
		int   setVar(unsigned id, float value);
		float getValue(const Value& v) const;
		void  setPosition(const float* pos);
		void  updateVolume(SoundInstance&, float);
		void  onEndEvent(const Data::EndEvent&);

		Object();
		~Object();

		protected:
		static int random(int n);
		void insertEndEvent(unsigned);
		int finished(SoundInstance& inst);
	};

}

