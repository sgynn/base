#include "audioclass.h"
#include <base/audio.h>
#include <base/xml.h>
#include <cstdio>

namespace audio {
	// Declare a named audio structure
	template<typename T>
	objectID Data::declare(const char* name, LookupMap& map, vector<T>& list) {
		if(name==0 || name[0]==0) return INVALID;
		LookupMap::iterator it = map.find(name);
		if(it!=map.end()) return it->value;
		unsigned ix = list.size();
		list.push_back( T() );
		map[name] = ix;
		return ix;
	}
	// Declare a sound or group - returns local id
	objectID Data::declareSound(SoundBank* bank, const char* name) {
		if(name[0]) {
			LookupMap::iterator it = m_soundMap.find(name);
			if(it!=m_soundMap.end()) return it->value;
		}
		soundID id = bank->data.size();
		bank->data.push_back(0);
		if(name[0]) m_soundMap[name] = Data::getSoundID(id, bank);
		return id;
	}

	// Debug - Find the name of a sound
	const char* Data::findSoundName(soundID id) const {
		for(LookupMap::const_iterator i=m_soundMap.begin(); i!=m_soundMap.end(); ++i) {
			if(i->value == id) return i->key;
		}
		return 0;
	}

	// Translate name with prefix
	const char* translateName(const char* prefix, const char* name) {
		if(name==0 || name[0]==0) return 0;
		static char tmp[128]; // Not thread safe, but should only run one instance anyway.
		if(prefix==0 || prefix[0]==0) return name;
		sprintf(tmp, "%s.%s", prefix, name);
		return tmp;
	}

	// Parse an optionally variable value
	Value parseVariableValue(const base::XMLElement& e) {
		Value v;
		v.value = e.attribute("value", 1.0f);			// initial value
		v.variance = e.attribute("variance", 0.0f);		// Random variance
		v.variable = INVALID;
		if(e.hasAttribute("variable")) {				// Variable link
			Data* data = Data::instance;
			const char* name = e.attribute("variable");
			if(name) {
				size_t was = data->m_variables.size();
				v.variable = data->declare( name, data->m_variableMap, data->m_variables );
				if(data->m_variables.size() > was) data->m_variables[v.variable].value = v.value;
			}
		}
		return v;
	}

	// Sound source parameters
	void loadSoundParams(Sound* s, const base::XMLElement& e) {
		Data* data = Data::instance;
		for(base::XML::iterator i=e.begin(); i!=e.end(); ++i) {
			if(*i=="source") {
				strcpy( s->file, i->attribute("file"));
				if(i->attribute("stream", false)) s->flags |= STREAM;
			}
			else if(*i=="target") s->targetID = data->declare( i->attribute("mixer"), data->m_mixerMap, data->m_mixers );
			else if(*i=="gain")   s->gain     = parseVariableValue(*i);
			else if(*i=="pitch")  s->pitch    = parseVariableValue(*i);
			else if(*i=="loop")  {
				s->loop  = i->attribute("count", 0);
				s->flags |= LOOP;
			}
			else if(*i=="attenuation") {
				s->attenuationID = data->declare( i->attribute("ref"), data->m_attenuationMap, data->m_attenuations);
			}
			else if(*i=="transition") {
				s->fadeIn  = i->attribute("fadein", 0.f);
				s->fadeOut = i->attribute("fadeout", 0.f);
			}
		}
	}

	// Main xml load function
	void loadGroup(SoundBank* bank, Group* group, const char* prefix, const base::XMLElement& xml, Sound inherit) {
		Data* data = Data::instance;
		
		// Read default sound settings
		loadSoundParams(&inherit, xml);

		// 
		for(base::XML::iterator i = xml.begin(); i!=xml.end(); ++i) {
			if(*i=="sound") {
				// Referenced sound
				if(i->hasAttribute("ref")) {
					if(!group) { printf("Error: Referenced sound outside group\n"); continue; }
					const char* name = i->attribute("ref");
					printf("Sound '%s' referenced from '%s'\n", name, prefix);
					group->sounds.push_back( data->declareSound(bank, name) );
				}
				// Defined sound
				else {
					const char* name = translateName(prefix, i->attribute("name"));
					if(!name) { printf("Error: Sound must have a name\n"); continue; }
					int ix = data->declareSound(bank, name);
					printf("Creating sound '%s' as %d\n", name, ix);
					Sound* s = new Sound(inherit);
					s->type = SOUND;
					if(bank->data[ix]) { printf("Warning: sound %s already exists\n", name); delete bank->data[ix]; }
					bank->data[ix] = s;
					if(group) group->sounds.push_back( ix );

					// Sound source parameters
					loadSoundParams(s, *i);
				}
			}
			else if(*i=="mixer") {
				const char* name = translateName( prefix, i->attribute("name") );
				if(name) {
					int id = data->declare( name, data->m_mixerMap, data->m_mixers );
					if(id==0) { printf("Error: Mixer must be named\n"); continue; }
					printf("Creating mixer '%s' as %d\n", name, id);
					Mixer& m = data->m_mixers[id];
					m.mixerID = id;
					for(base::XML::iterator j=i->begin(); j!=i->end(); ++j) {
						if(*j=="volume") m.volume = parseVariableValue(*j);
						else if(*j=="target") m.targetID = data->declare( j->attribute("mixer"), data->m_mixerMap,data->m_mixers );
					}
				}
			}
			else if(*i=="group") {
				const char* name = translateName( prefix, i->attribute("name") );
				const char* type = i->attribute("type");
				Group* g = new Group();	g->type = GROUP;
				if(strcmp(type, "random")==0) g->type = RANDOM;
				else if(strcmp(type, "sequence")==0) g->type = SEQUENCE;
				else if(strcmp(type, "switch")==0) {
					g->type = SWITCH;
					g->switchID = data->declare( i->attribute("enum"), data->m_enumMap, data->m_enums );
				}
				else if(type[0]) printf("Warning: Undefined group type '%s'\n", type);
				// Add to lists
				int ix = name? data->declareSound(bank, name): bank->data.size();
				if(group) group->sounds.push_back( ix );
				if(name && bank->data[ix]) { printf("Warning: group %s already exists\n", name); delete bank->data[ix]; }
				if(name) bank->data[ix] = g;
				else bank->data.push_back(g);
				const char* grouptypes[] = { "basic", "random", "sequence", "switch" };
				printf("Creating %s group '%s' as %d\n", grouptypes[g->type-GROUP], name, ix);
				// Recurse group
				if(name) {
					char* tmp = strdup(name);
					loadGroup(bank, g, tmp, *i, inherit);
					free(tmp);
				} else loadGroup(bank, g, prefix, *i, inherit);
			}
			else if(*i=="attenuation") {
				const char* name = translateName(prefix, i->attribute("name"));
				int ix = data->declare(name, data->m_attenuationMap, data->m_attenuations);
				if(ix==0) { printf("Invalid attenuation name\n"); continue; }
				data->m_attenuations[ix].falloff  = i->attribute("fallloff", 0.f);
				data->m_attenuations[ix].distance = i->attribute("distance", 0.f);
			}
			else if(*i=="enum") {
				const char* name = translateName(prefix, i->attribute("name"));
				if(!name) { printf("Invalid enum name\n"); continue; }
				int k = 0, ix = data->declare(name, data->m_enumMap, data->m_enums);
				data->m_enums[ix].value = i->attribute("default", 0);
				for(base::XML::iterator j=i->begin(); j!=i->end(); ++j) {
					name = j->text()? j->text(): j->attribute("name");
					if(name[0]) data->m_enums[ix].names[ name ] = k++;
				}
			}
			else if(*i=="variable") {
				const char* name = translateName(prefix, i->attribute("name"));
				int ix = data->declare(name, data->m_variableMap, data->m_variables);
				if(ix==0) { printf("Invalid variable name\n"); continue; }
				data->m_variables[ix].value = i->attribute("value", 0.f);
				data->m_variables[ix].min = i->attribute("min", 0.f);
				data->m_variables[ix].max = i->attribute("max", 0.f);
			}
			else if(*i=="event") {
				const char* name = translateName(prefix, i->attribute("name"));
				if(!name) { printf("Invalid event name\n"); continue; }
				int ix = data->declare(name, data->m_eventMap, data->m_events);
				EventList& list = data->m_events[ix];
				for(base::XML::iterator j=i->begin(); j!=i->end(); ++j) {
					Event e;
					if(*j=="play" || *j=="stop") {
						e.type = *j=="play"? 1: 2;
						e.target = data->declareSound(bank, j->attribute("sound"));
						list.push_back(e);
					} else if(*j=="set") {
						e.type = 3;
						// variable or enum - needs more design.
					}
				}
			}
		}
	}


	bool Data::loadSoundBank(const char* file) {
		base::XML xml = base::XML::load(file);
		if(xml.getRoot().size()==0) return false;
		SoundBank* bank = new SoundBank();
		for(bank->id=0; bank->id < m_banks.size(); ++bank->id) {
			if(strcmp(file, m_banks[ bank->id ]->file)==0) break;
		}
		bank->file = file;
		printf("Loading '%s' as soundbank %u\n", file, bank->id);
		Sound soundTemplate;
		audio::loadGroup(bank, 0, 0, xml.getRoot(), soundTemplate);
		if(bank->id == m_banks.size()) m_banks.push_back(bank);
		else m_banks[ bank->id ] = bank;

		// Try loading the sounds
		printf( "Loading audio data\n");
		for(unsigned i=0; i<bank->data.size(); ++i) {
			if(bank->data[i] && bank->data[i]->type == SOUND) {
				loadSource( static_cast<Sound*>(bank->data[i]) );
			}
		}
		return true;
	}

	bool Data::unloadSoundBank(const char* file) {
		for(unsigned i=0; i<m_banks.size(); ++i) {
			SoundBank* bank = m_banks[i];
			if(strcmp(file, bank->file)==0) {
				unloadSoundBank(bank);
				return true;
			}
		}
		return false;
	}

	void Data::unloadSoundBank(SoundBank* bank) {
		for(unsigned j=0; j<bank->data.size(); ++j) {
			if(bank->data[j]) {
				if(bank->data[j]->type==SOUND) {
					unloadSource( static_cast<Sound*>(bank->data[j]) );
				}
				delete bank->data[j];
			}
		}
		bank->data.clear();
	}
}

