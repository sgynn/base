#include <base/editor/audioeditor.h>
#include "../audio/audioclass.h"	// Private engine header
#include <base/camera.h>
#include <base/directory.h>
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <base/gui/tree.h>
#include <base/xml.h>
#include <algorithm>

#include "menubuilder.h"

#include <base/editor/embed.h>
BINDATA(editor_audio_gui, EDITOR_DATA "/audio.xml")
BINDATA(editor_audio_icons, EDITOR_DATA "/audio.png")

using namespace editor;
using namespace gui;
using namespace audio;


class VariableWidget : public Widget {
	WIDGET_TYPE(VariableWidget)
	VariableWidget(const Rect& r, Skin* s) : Widget(r,s) {}
	void initialise(const Root*, const PropertyMap&) override {
		getTemplateWidget<SpinboxFloat>("value")->eventChanged.bind([this](SpinboxFloat*, float v) {
			if(m_variable->variable == INVALID) m_variable->value = v;
			if(eventChanged) eventChanged(*m_variable);
		});
		getTemplateWidget<Combobox>("variable")->eventSelected.bind([this](Combobox*, ListItem& i) {
			m_variable->variable = i.getValue(1, INVALID);
			SpinboxFloat* value = getTemplateWidget<SpinboxFloat>("value");
			value->setEnabled(m_variable->variable == INVALID);
			if(m_variable->variable == INVALID) value->setValue(m_variable->value);
			else value->setValue(audio::Data::instance->m_variables[m_variable->variable].value);
			if(eventChanged) eventChanged(*m_variable);
		});
	}
	void setVariable(Value& v) {
		m_variable= &v;
		SpinboxFloat* value = getTemplateWidget<SpinboxFloat>("value");
		Combobox* list = getTemplateWidget<Combobox>("variable");
		value->setValue(v.value);
		value->setEnabled(v.variable == INVALID);
		ListItem* var = list->findItem(1, v.variable);
		list->selectItem(var? var->getIndex(): -1);
	}
	Delegate<void(audio::Value&)> eventChanged;
	private:
	audio::Value* m_variable;
};

AudioEditor::~AudioEditor() {
	delete m_menu;
}

void AudioEditor::initialise() {
	Root::registerClass<VariableWidget>();
	getEditor()->addEmbeddedPNGImage("data/editor/audio.png", editor_audio_icons, editor_audio_icons_len);
	m_panel = loadUI("audio.xml", editor_audio_gui);
	addToggleButton(m_panel, "editors", "audio");
	m_panel->setVisible(false);
	m_data = m_panel->getWidget<TreeView>("objects");
	m_data->eventSelected.bind(this, &AudioEditor::selectObject);
	TabbedPane* tabs = m_panel->getWidget<TabbedPane>("tabs");
	m_properties = tabs->getTab(0)->getWidget("objectdata");
	setupPropertyEvents();

	// Context menu
	MenuBuilder menu(m_panel->getRoot(), "button", "menuitem", "submenu", "audioicons");
	MenuBuilder& addMenu = menu.addMenu("Add");
	menu.addAction("Delete", "delete", [this]() { deleteItem(); });
	addMenu.addAction("Mixer", "mixer", [this]() { addItem(Type::Mixer); });
	addMenu.addAction("Group", "folder", [this]() { addItem(Type::Folder); });
	addMenu.addAction("Random", "random", [this]() { addItem(Type::Random); });
	addMenu.addAction("Sequence", "sequence", [this]() { addItem(Type::Sequence); });
	addMenu.addAction("Switch", "switch", [this]() { addItem(Type::Switch); });
	addMenu.addAction("Sound", "sound", [this]() { addItem(Type::Sound); });
	addMenu.addAction("Attenuation", "attenuation", [this]() { addItem(Type::Attenuation); });
	addMenu.addAction("Variable", "variable", [this]() { addItem(Type::Variable); });
	addMenu.addAction("Enum", "variable", [this]() { addItem(Type::Enum); });
	addMenu.addAction("Event", "event", [this]() { addItem(Type::Event); });
	m_menu = menu;

	m_data->eventMouseDown.bind([this](Widget* w, const Point& p, int b) {
		if(b==4) {
			TreeNode* over = m_data->getNodeAt(p);
			if(over) over->select();
			else m_data->clearSelection();
			bool sel = getSelectedObject().id != INVALID;
			m_menu->getWidget("Add")->setEnabled(audio::Data::instance);
			m_menu->getWidget("Delete")->setEnabled(sel);
			m_menu->popup(w->getRoot(), w->getDerivedTransform().transform(p));
		}
	});

	
	m_preview = m_panel->getWidget("preview");
	m_preview->getWidget<Button>("play")->eventPressed.bind(this, &AudioEditor::playSound);


	// Link variable lists
	ItemList* varList = nullptr;
	for(Widget* w: m_properties) {
		for(Widget* c : w) {
			if(VariableWidget* v = cast<VariableWidget>(c)) {
				v->eventChanged.bind([this](Value&) { setupSoundCaster(getSelectedObject()); });
				Combobox* list = v->getTemplateWidget<Combobox>("variable");
				if(!varList) varList = list;
				else list->shareList(varList);
			}
		}
	}
}
void AudioEditor::activate() { m_panel->setVisible(true); refreshDataTree(); refreshFileList(); }
void AudioEditor::deactivate() { }
bool AudioEditor::isActive() const { return m_panel->isVisible(); }

bool AudioEditor::newAsset(const char*& name, const char*& file, const char*& body) const {
	name = "Soundbank";
	file = "audio.xml";
	body = "<?xml version='1.0'/>\n<soundbank>\n</soundbank>\n";
	return true;
}

Widget* AudioEditor::openAsset(const Asset& asset) {
	return nullptr;
}

bool AudioEditor::assetActions(MenuBuilder& menu, const Asset& asset) {
	return false;
}

TreeNode* AudioEditor::findNode(Object obj, TreeNode* node) {
	for(TreeNode* n: *node) {
		Object o = getObject(n);
		if(o.type == obj.type && o.id == obj.id) return n;
		if(TreeNode* child = findNode(obj, n)) return child;
	}
	return nullptr;
}

void AudioEditor::refreshDataTree() {
	Data* data = audio::Data::instance;
	if(!data) return;

	Object selected = getSelectedObject();
	

	TreeNode* root = m_data->getRootNode();
	root->clear();
	for(const auto& i: data->m_mixerMap) root->add(i.key, i.value, "mixer", Type::Mixer);
	for(const auto& i: data->m_variableMap) root->add(i.key, i.value, "variable", Type::Variable);
	for(const auto& i: data->m_enumMap) root->add(i.key, i.value, "variable", Type::Enum);
	for(const auto& i: data->m_eventMap) root->add(i.key, i.value, "event", Type::Event);
	for(const auto& i: data->m_attenuationMap) root->add(i.key, i.value, "attenuation", Type::Attenuation);

	const char* typeIcon[] = { "none", "sound", "folder", "random", "sequence", "switch" };
	std::vector<LookupMap::Pair> queue;
	for(const auto& i: data->m_soundMap) queue.push_back(i);
	for(size_t i=0; i<queue.size(); ++i) {
		TreeNode* node = root;
		const char* name = queue[i].key;
		const char* split = strchr(name, '.');
		while(split && node) {
			int len = split - name;
			TreeNode* next = nullptr;
			for(TreeNode* n: *node) {
				Type type = n->getValue<Type>(3, Type::Folder);
				if(isSoundType(type) && strncmp(name, n->getText(), len)==0) { next=n; break; }
			}
			node = next;
			name = split+1;
			split = strchr(name, '.');
		}

		if(!node) { queue.push_back(queue[i]); continue; } // Come back later

		// prefix: Random "56% Name"; Switch: "Key: name";

		const SoundBase* sound = data->lookupSound(queue[i].value);
		node->add(name, queue[i].value, typeIcon[sound->type], (Type)sound->type);
	}

	// Refresh other comboboxes
	ItemList* varList = m_properties->getWidget<VariableWidget>("volume")->getTemplateWidget<Combobox>("variable");
	varList->clearItems();
	varList->addItem("", audio::INVALID);
	for(const auto& i: data->m_variableMap) varList->addItem(i.key, i.value);

	ItemList* targets = m_properties->getWidget<Combobox>("target");
	targets->clearItems();
	for(const auto& i: data->m_mixerMap) targets->addItem(i.key, i.value);


	// Sort everything
	std::sort(root->begin(), root->end(), [](TreeNode* a, TreeNode* b) {
		Type ta = a->getValue<Type>(3, Type::Mixer);
		Type tb = b->getValue<Type>(3, Type::Mixer);
		return ta<tb || (ta==tb && strcmp(a->getText(), b->getText()) < 0);
	});

	// Selection
	if(TreeNode* sel = findNode(selected, root)) {
		sel->ensureVisible();
		sel->select();
	}
	else selectObject(m_data, nullptr);
}

void scanAudioFiles(Combobox* list, const char* folder) {
	char path[128];
	for(auto& i: base::Directory(folder)) {
		if(i.type == base::Directory::DIRECTORY && i.name[0] != '.') {
			sprintf(path, "%s/%s", folder, i.name);
			scanAudioFiles(list, path);
		}
		else if(strcmp(i.name + i.ext, "wav")==0 || strcmp(i.name + i.ext, "ogg")==0) {
			sprintf(path, "%s/%s", folder, i.name);
			list->addItem(path+2);
		}
	}
}
void AudioEditor::refreshFileList() {
	Combobox* list = m_properties->getWidget<Combobox>("source");
	list->clearItems();
	scanAudioFiles(list, ".");
}


inline const char* getNameFromID(const LookupMap& map, soundID id) {
	for(auto i: map) if(i.value == id) return i.key;
	return "";
}

inline void setValidSkin(Widget* w, bool valid) {
	if(!w) return;
	if(Widget* t = w->getTemplateWidget("_text")) w = t;
	w->setSkin(w->getRoot()->getSkin(valid? "panel": "invalid"));
}

AudioEditor::Object AudioEditor::getObject(const TreeNode* n) const {
	if(n) return Object{ n->getValue<Type>(3, Type::Folder), n->getValue<objectID>(1, INVALID)};
	else return Object{Type::Folder, INVALID};
}

AudioEditor::Object AudioEditor::getSelectedObject() const {
	return getObject(m_data->getSelectedNode());
}

LookupMap& AudioEditor::getLookupMap(Type type) {
	Data& data = *audio::Data::instance;
	switch(type) {
	case Type::Mixer: return data.m_mixerMap;
	case Type::Attenuation: return data.m_attenuationMap;
	case Type::Variable: return data.m_variableMap;
	case Type::Enum: return data.m_enumMap;
	case Type::Event: return data.m_eventMap;
	case Type::Folder: case Type::Random: case Type::Sequence: case Type::Switch: case Type::Sound: return data.m_soundMap;
	}
	// Shouldn't get here, appease compiler
	static LookupMap invalid;
	return invalid;
}

void AudioEditor::deleteItem() {
	m_menu->hide();
	Object object = getSelectedObject();
	if(object.id == INVALID) return;
	if(isSoundType(object.type)) {
		// remove from parent container
		Object parent = getObject(m_data->getSelectedNode()->getParent());
		if(parent.id != INVALID) {
			Group* group = ((Group*)audio::Data::instance->lookupSound(parent.id));
			std::remove(group->sounds.begin(), group->sounds.end(), object.id);
			// FIXME: Keep switch indices
		}
	}
	LookupMap& map = getLookupMap(object.type);
	map.erase(getNameFromID(map, object.id));
	refreshDataTree();
}

void AudioEditor::addItem(Type type) {
	char newName[512];
	static const char* typeNames[] = { "mixer", "sound", "group", "random", "sequence", "switch", "attenuation", "variable", "enum", "event" };
	newName[0] = 0;

	Object item { type, INVALID };
	Data& data = *audio::Data::instance;
	switch(type) {
		case Type::Mixer:
			item.id = data.m_mixers.size();
			data.m_mixers.push_back({item.id, item.id});
			break;
		case Type::Variable:
			item.id = data.m_variables.size();
			data.m_variables.push_back({0.f, 0.f, 1.f});
			break;
		case Type::Attenuation:
			item.id = data.m_attenuations.size();
			data.m_attenuations.push_back({1.f, 10.f});
			break;
		case Type::Enum:
			item.id = data.m_enums.size();
			data.m_enums.push_back({0});
			break;
		case Type::Event:
			item.id = data.m_events.size();
			data.m_events.push_back({});
			break;
		// Audio objects are more complicated
		// They need to be in a soundbank, and registered with their parent
		case Type::Folder: case Type::Random: case Type::Sequence: case Type::Switch: case Type::Sound:
		{
			Object parent = getSelectedObject();
			if(!isSoundType(parent.type)) parent.id = INVALID;
			else if(parent.type == Type::Sound) parent = getObject(m_data->getSelectedNode()->getParent());
			int bankIndex = parent.id==INVALID? 0: parent.id >> 20;
			item.id = data.m_banks[bankIndex]->data.size() | bankIndex << 20;

			// Add to parent container and add name prefix
			if(parent.id != INVALID) {
				((Group*)data.lookupSound(parent.id))->sounds.push_back(item.id&0xfffff);
				sprintf(newName, "%s.", getNameFromID(data.m_soundMap, parent.id));
			}

			// Create the audio object
			if(type==Type::Sound) {
				Sound* s = new Sound;
				s->type = SOUND;
				s->loop = -1;
				s->source = nullptr;
				s->file[0] = 0;
				s->attenuationID = INVALID;
				s->targetID = 0;
				s->flags = 0;
				s->fadeIn = s->fadeOut = 0;
				s->gain = Value{0, INVALID, 0};
				s->pitch = Value{0, INVALID, 0};
				data.m_banks[bankIndex]->data.push_back(s);
			}
			else {
				Group* g = new Group;
				g->switchID = INVALID;
				g->type = (int)type;
				g->loop = -1;
				data.m_banks[bankIndex]->data.push_back(g);
			}
			break;
		}
		default: break;
	}

	// Add to name maps
	strcat(newName, typeNames[(int)type]);
	LookupMap& map = getLookupMap(type);
	int index = 0;
	char* ext = newName + strlen(newName);
	while(map.contains(newName)) sprintf(ext, "_%d", ++index);
	map[newName] = item.id;
	m_menu->hide();
	refreshDataTree();
	if(TreeNode* n = findNode(item, m_data->getRootNode())) {
		n->ensureVisible();
		n->select();
		selectObject(m_data, n);
	}
}


const char* AudioEditor::getObjectName(Type type, uint id) {
	if(id == INVALID) return "";
	const char* name = getNameFromID(getLookupMap(type), id);
	if(isSoundType(type)) {
		const char* e = strrchr(name, '.');	// Map contains full path. Extract name
		if(e) name = e+1;
	}
	return name;
}

int AudioEditor::renameObject(Object o, const char* newName) {
	const char* oldName = getObjectName(o.type, o.id);
	if(strcmp(newName, oldName) == 0) return 0;
	Data& data = *audio::Data::instance;
	if(isSoundType(o.type)) { // SOund types are heirarchical - need to update children too
		const char* fullName = getNameFromID(data.m_soundMap, o.id);
		char buffer[256];
		sprintf(buffer, "%.*s%s", (int)(oldName-fullName), fullName, newName);
		if(o.type!=Type::Sound) fullName = strdup(fullName); // erase deletes this
		data.m_soundMap.erase(fullName);
		data.m_soundMap[buffer] = o.id;
		// Update children
		if(o.type != Type::Sound) {
			int part = strlen(fullName);
			int newPart = strlen(buffer);
			std::vector<const char*> rename;
			for(auto& i: data.m_soundMap) {
				if(strncmp(i.key, fullName, part)==0 && i.key[part]=='.') rename.push_back(i.key);
			}
			for(const char* s: rename) {
				sprintf(buffer+newPart, "%s", s+part);
				data.m_soundMap[buffer] = data.m_soundMap[s];
				data.m_soundMap.erase(s);
			}
			free((char*)fullName);
			return rename.size() + 1;
		}
	}
	else { // Non sound types are global
		LookupMap& map = getLookupMap(o.type);
		map.erase(oldName);
		map[newName] = o.id;
	}
	return 1;
}

bool AudioEditor::isSoundType(Type type) {
	return type==Type::Sound || type==Type::Folder || type==Type::Random || type==Type::Sequence || type==Type::Switch;
}

objectID AudioEditor::findMixer(const TreeNode* node) {
	Type type = node->getValue(3, Type::Folder);
	if(type == Type::Sound) {
		objectID id = node->getValue<objectID>(1, 0);
		const Sound* sound = static_cast<const Sound*>(audio::Data::instance->lookupSound(id));
		return sound->targetID;
	}
	else {
		constexpr objectID unknown = -2;
		objectID mixer = unknown;
		for(TreeNode* n: *node) {
			objectID m = findMixer(n);
			if(m == INVALID) return INVALID;
			else if(mixer == unknown) mixer = m;
			else if(mixer != m) return INVALID;
		}
		return mixer;
	}
}


void AudioEditor::selectObject(gui::TreeView*, gui::TreeNode* node) {
	for(Widget* w: m_properties) if(!cast<Label>(w)) w->setVisible(false);
	if(!node) return;

	auto show = [](Widget* w) { w->getParent()->setVisible(true); };
	#define findWidget(Type, name) if(Type* w=m_properties->getWidget<Type>(#name)) show(w), w

	Data& data = *audio::Data::instance;
	objectID id = node->getValue<objectID>(1, 0);
	Type type = node->getValue(3, Type::Folder);

	m_properties->pauseLayout();
	findWidget(Textbox, name)->setText(getObjectName(type, id));

	switch(type) {
	case Type::Mixer:
		if(id) findWidget(Combobox, target)->setText(getObjectName(Type::Mixer, data.m_mixers[id].targetID));
		findWidget(VariableWidget, volume)->setVariable(data.m_mixers[id].volume);
		break;
	case Type::Sound:
		{
		Sound* sound = (Sound*)(data.lookupSound(id));
		findWidget(Combobox, source)->setText(sound->file);
		findWidget(Combobox, target)->setText(getObjectName(Type::Mixer, sound->targetID));
		findWidget(VariableWidget, gain)->setVariable(sound->gain);
		findWidget(VariableWidget, pitch)->setVariable(sound->pitch);
		findWidget(Checkbox, loop)->setChecked(sound->loop >= 0);
		findWidget(Spinbox, loopcount)->setValue(sound->loop > 0? sound->loop: 0);
		setValidSkin(m_properties->getWidget<Combobox>("source"), sound->source);
		}
		break;
	case Type::Folder:
	case Type::Random:
	case Type::Sequence:
	case Type::Switch:
		findWidget(Combobox, target)->setText(getObjectName(Type::Mixer, findMixer(node)));
		if(type == Type::Switch) {
			Group* group = (Group*)(data.lookupSound(id));
			findWidget(Combobox, enum)->selectItem(group->switchID);
		}
		break;
	case Type::Attenuation:
		findWidget(SpinboxFloat, distance)->setValue(data.m_attenuations[id].distance);
		findWidget(SpinboxFloat, falloff)->setValue(data.m_attenuations[id].falloff);
		break;
	case Type::Variable:
		findWidget(SpinboxFloat, value)->setValue(data.m_variables[id].value);
		break;
	case Type::Enum:
	case Type::Event:
		break;
	}

	#undef findWidget
	m_properties->resumeLayout();
	setupSoundCaster({type, id});
}




void AudioEditor::setupPropertyEvents() {
	#define CONNECT(Type, Name, Event, Lambda) if(Type* w=m_properties->getWidget<Type>(#Name)) w->Event.bind(Lambda);

	// Object name
	CONNECT(Textbox, name, eventSubmit, [this](Textbox* t) {
		if(renameObject(getSelectedObject(), t->getText())) refreshDataTree();
	});

	// Target mixer
	CONNECT(Combobox, target, eventSelected, [this](Combobox*, ListItem& item) {
		objectID value = item.getValue(1, INVALID);
		Object o = getSelectedObject();
		switch(o.type) {
		case Type::Mixer:
			audio::Data::instance->m_mixers[o.id].targetID = value; // TODO: Validate and update children
			break;
		case Type::Sound:
			((Sound*)audio::Data::instance->lookupSound(o.id))->targetID = value; // Do we need to fix playing sound instances
			break;
		case Type::Folder: case Type::Random: case Type::Sequence: case Type::Switch:
			// Recursively set mixer
			break;
		default: break;
		}
	});

	// Variable default value
	CONNECT(SpinboxFloat, value, eventChanged, [this](SpinboxFloat*, float v) {
		Object o = getSelectedObject();
		if(o.type==Type::Variable) Data::instance->m_variables[o.id].value = v;
	});

	// Sound source file
	CONNECT(Combobox, source, eventSubmit, [this](Combobox* c) {
		Object o = getSelectedObject();
		if(Sound* s = ((Sound*)audio::Data::instance->lookupSound(o.id))) {
			strncpy(s->file, c->getText(), 63);
			audio::Data::instance->unloadSource(s);
			bool ok = audio::Data::instance->loadSource(s);
			setValidSkin(c, ok);
		}
	});
	CONNECT(Combobox, source, eventSelected, [this](Combobox* c, ListItem& i) {
		if(i.isValid()) c->eventSubmit(c);
	});
}


// ======================================================================================== //

void AudioEditor::setupSoundCaster(Object sound) {
	if(isSoundType(sound.type) && sound.type != Type::Folder && sound.id != INVALID) {
		if(m_testObject == INVALID) m_testObject = audio::createObject();
		Data& data = *audio::Data::instance;
		m_preview->setVisible(true);
		// Lift all relevant variables
		Widget* list = m_preview->getWidget("values");
		list->deleteChildWidgets();
		auto addVariable = [this, list](uint var) {
			if(var==INVALID) return;
			const char* name = getObjectName(Type::Variable, var);
			if(list->getWidget(name)) return; // Already added
			Widget* w = list->createChild<Widget>("testvalue", name);
			w->getTemplateWidget<Label>("_name")->setCaption(name);
			w->getTemplateWidget<Scrollbar>("_value")->eventChanged.bind([this](Scrollbar* s, int v) {
				audio::setValue(m_testObject, s->getParent()->getName(), v / 100.f);
			});
		};
		std::vector<Object> stack;
		stack.push_back(sound);
		for(size_t i=0; i<stack.size(); ++i) {
			Object o = stack[i];
			switch(o.type) {
			case Type::Random:
			case Type::Switch:
			case Type::Sequence:
			{
				const Group* g = static_cast<const Group*>(data.lookupSound(o.id));
				for(uint s: g->sounds) {
					uint id = s | (o.id&0xfff00000);
					if(const SoundBase* so = data.lookupSound(id)) stack.push_back({(Type)so->type, id});
				}
				if(o.type == Type::Switch && g->switchID != INVALID) {
					const char* name = getObjectName(Type::Enum, g->switchID);
					Widget* var = list->createChild<Widget>("testswitch", name);
					var->getTemplateWidget<Label>("_name")->setCaption(name);
					var->getTemplateWidget<Combobox>("_value")->eventSelected.bind([this](Combobox* c, ListItem& i) {
						audio::setEnum(m_testObject, c->getParent()->getName(), i.getValue<uint>(1, 0));
					});
				}
				break;
			}
			case Type::Sound:
				if(const Sound* s = static_cast<const Sound*>(data.lookupSound(o.id))) {
					stack.push_back({Type::Mixer, s->targetID});
					addVariable(s->gain.variable);
					addVariable(s->pitch.variable);
				}
				break;
			case Type::Mixer:
				//addVariable(data.m_mixers[o.id].volume.variable);
				break;
			default: break;
			}
		}
	}
	else {
		m_preview->setVisible(false);
	}
}

void AudioEditor::playSound(Button*) {
	Object o = getSelectedObject();
	Button* play = m_panel->getWidget<Button>("play");
	if(!m_playing) {
		audio::playSound(m_testObject, o.id);
		audio::setPosition(m_testObject, getEditor()->getCamera()->getPosition());
		play->getWidget(0)->setVisible(true);
		play->getWidget(1)->setVisible(false);
		m_playing = 1;
	}
	else {
		audio::stopAll(m_testObject);
		play->getWidget(0)->setVisible(false);
		play->getWidget(1)->setVisible(true);
		m_playing = 0;
	}
}

void AudioEditor::update() {
	if(m_playing) {
		bool isPlaying = false;
		audio::Object* object = audio::Data::instance->getObject(m_testObject);
		for(const SoundInstance& i: object->playing) if(i.sound) isPlaying=true;
		if(!isPlaying && ++m_playing > 4) playSound(0);
	}
}


// ======================================================================================== //


void AudioEditor::save(const char* file) {
	if(!audio::Data::instance) return;

	using base::XMLElement;
	base::XML xml;
	XMLElement& root = xml.getRoot();
	Data& data = *audio::Data::instance;

	auto writeVariable = [&data, this](XMLElement& e, const char* name, const Value& var) {
		XMLElement& v = e.add(name);
		v.setAttribute("value", var.value);
		if(var.variance!=0) v.setAttribute("variance", var.variance);
		if(var.variable != INVALID) v.setAttribute("variable", getNameFromID(data.m_variableMap, var.variable));
	};


	for(const auto& i: data.m_mixerMap) {
		XMLElement& m = root.add("mixer");
		m.setAttribute("name", i.key);
		writeVariable(m, "volume", data.m_mixers[i.value].volume);
	}
	for(const auto& i: data.m_variableMap) {
		XMLElement& m = root.add("variable");
		m.setAttribute("name", i.key);
		m.setAttribute("value", data.m_variables[i.value].value);
	}
	for(const auto& i: data.m_enumMap) {
		XMLElement& m = root.add("enum");
		m.setAttribute("name", i.key);
	}
	for(const auto& i: data.m_eventMap) {
		XMLElement& m = root.add("event");
		m.setAttribute("name", i.key);
	}
	for(const auto& i: data.m_attenuationMap) {
		XMLElement& m = root.add("attenuation");
		m.setAttribute("name", i.key);
		m.setAttribute("falloff", data.m_attenuations[i.value].falloff);
		m.setAttribute("distance", data.m_attenuations[i.value].distance);
	}

	const char* soundType[] = { "none", "sound", "folder", "random", "sequence", "switch" };
	std::vector<LookupMap::Pair> queue;
	for(const auto& i: data.m_soundMap) queue.push_back(i);
	for(size_t i=0; i<queue.size(); ++i) {
		XMLElement* node = &root;
		const char* name = queue[i].key;
		const char* split = strchr(name, '.');
		while(split && node) {
			int len = split - name;
			XMLElement* next = nullptr;
			for(XMLElement& n: *node) {
				const char* nodeName = n.attribute("name");
				if(strncmp(name, nodeName, len)==0 && nodeName[len]==0) {
					for(int t=1; t<6; ++t) if(n == soundType[t]) { next=&n; break; }
					if(next) break;
				}
			}
			node = next;
			name = split+1;
			split = strchr(name, '.');
		}

		if(!node) { queue.push_back(queue[i]); continue; } // Come back later
		
		const SoundBase* sound = data.lookupSound(queue[i].value);
		XMLElement& m = node->add(sound->type == SOUND? "sound": "group");
		m.setAttribute("name", name);
		if(sound->type == SOUND) {
			const Sound* s = static_cast<const Sound*>(sound);
			m.add("source").setAttribute("file", s->file);
			m.add("target").setAttribute("mixer", getNameFromID(data.m_mixerMap, s->targetID));
			if(s->attenuationID != INVALID) {
				m.add("attenuation").setAttribute("ref", getNameFromID(data.m_attenuationMap, s->attenuationID));
			}
			if(s->fadeIn!=0 || s->fadeOut!=0) {
				XMLElement& fade = m.add("transition");
				fade.setAttribute("fadein", s->fadeIn);
				fade.setAttribute("fadeout", s->fadeOut);
			}
			writeVariable(m, "pitch", s->pitch);
			writeVariable(m, "gain", s->gain);
		}
		else {
			const Group* g = static_cast<const Group*>(sound);
			m.setAttribute("type", soundType[g->type]);
			if(g->type == SWITCH) m.setAttribute("variable", getNameFromID(data.m_enumMap, g->switchID));
		}

		if(sound->loop>=0) {
			XMLElement& loop = m.add("loop");
			if(sound->loop>0) loop.setAttribute("count", sound->loop);
		}
	}


	xml.save(file);
}


