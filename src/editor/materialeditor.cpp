#include "materialeditor.h"
#include <base/gui/widgets.h>
#include <base/gui/lists.h>
#include <base/resources.h>
#include <base/material.h>
#include <base/shader.h>
#include <base/autovariables.h>

#include <base/editor/embed.h>
BINDATA(editor_material_gui, EDITOR_DATA "/material.xml")

using namespace gui;
using namespace base;
using namespace editor;

void updateListInfo(Widget* list) {
	Label* info = list->getTemplateWidget<Label>("info");
	if(list->getWidgetCount() == 0) info->setCaption("Empty");
	else if(list->getWidgetCount() == 1) info->setCaption("1 Item");
	else info->setCaption(String::format("%d Items", list->getWidgetCount()));
}

void MaterialEditor::initialise() {
	m_panel = loadUI("src/data/material.xml", editor_material_gui);
	if(m_panel) m_panel->setVisible(false);

	m_autoVarList = new ItemList();
	for(int i=0; i<AUTO_CUSTOM; ++i) m_autoVarList->addItem(AutoVariableSource::getKeyString(i));
}

MaterialEditor::~MaterialEditor() {
	delete m_autoVarList;
}

bool MaterialEditor::newAsset(const char*& name, const char*& file, const char*& body) const {
	name = "Material";
	file = "material.mat";
	return true;
}

bool MaterialEditor::assetActions(MenuBuilder& menu, const Asset& asset) {
	return false;
}

Widget* MaterialEditor::openAsset(const Asset& asset) {
	if(asset.type == ResourceType::None) {
		StringView ext = strrchr(asset.file, '.');
		if(ext != ".mat") return nullptr;
	}
	else if(asset.type != ResourceType::Material) return nullptr;

	const char* name = asset.resource;
	if(!name) name = getResourceNameFromFile(Resources::getInstance()->materials, asset.file);
	Material* mat = Resources::getInstance()->materials.get(name);
	if(!mat) return nullptr;

	Widget* w = m_panel->clone();
	cast<gui::Window>(w)->setCaption(name);

	Combobox* tech = w->getWidget<Combobox>("technique");
	for(Pass* pass: *mat) tech->addItem(pass->getName());
	tech->eventSelected.bind([this, mat, w](Combobox*, ListItem& item) {
		Resources& res = *Resources::getInstance();
		Pass* pass = mat->getPass(item);
		ShaderVars& params = pass->getParameters();
		Shader::UniformList vars;
		if(Shader* s = pass->getShader()) {
			const char* shaderName = res.shaders.getName(s);
			w->getWidget<Combobox>("shader")->setText(shaderName);
			vars = s->getUniforms();
		}

		// Textures
		Widget* textureList = w->getWidget("textures");
		std::vector<const char*> varNames = params.getNames();
		for(size_t i=0; i<pass->getTextureCount(); ++i) {
			const Texture* tex = pass->getTexture(i);
			if(tex) {
				const char* texName = res.textures.getName(tex);
				const char* texVar = "";
				// Find variable name
				for(const char* n: varNames) {
					if(const int* ip = params.getSamplerPointer(n)) {
						if(*ip == (int)i) { texVar = n; break; }
					}
				}
				// Add widget
				addTexture(textureList, pass, texVar, texName);
			}
		}
		textureList->getTemplateWidget<Button>("add")->eventPressed.bind([this, pass](Button* b){addTexture(b->getParent(), pass, 0,0);});
		updateListInfo(textureList);
		

		
		// Variables
		Widget* variableList = w->getWidget("variables");
		for(const char* name: varNames) {
			int a = params.getAutoKey(name);
			if(a>=0) addVariable(variableList, pass, name, AutoVariableSource::getKeyString(a));
			else if(const int* ip = params.getIntPointer(name)) {
				switch(params.getElements(name)) {
				case 1: addVariable(variableList, pass, name, String::format("%d", ip[0])); break;
				case 2: addVariable(variableList, pass, name, String::format("%d %d", ip[0], ip[1])); break;
				case 3: addVariable(variableList, pass, name, String::format("%d %d %d", ip[0], ip[1], ip[2])); break;
				case 4: addVariable(variableList, pass, name, String::format("%d %d %d %d", ip[0], ip[1], ip[2], ip[3])); break;
				}
			}
			else if(const float* fp = params.getFloatPointer(name)) {
				switch(params.getElements(name)) {
				case 1: addVariable(variableList, pass, name, String::format("%g", fp[0])); break;
				case 2: addVariable(variableList, pass, name, String::format("%g %g", fp[0], fp[1])); break;
				case 3: addVariable(variableList, pass, name, String::format("%g %g %g", fp[0], fp[1], fp[2])); break;
				case 4: addVariable(variableList, pass, name, String::format("%g %g %g %g", fp[0], fp[1], fp[2], fp[3])); break;
				}
			}
		}
		variableList->getTemplateWidget<Button>("add")->eventPressed.bind([this, pass](Button* b){addVariable(b->getParent(), pass, 0,0);});
		updateListInfo(variableList);

		// Shared variables
		Widget* sharedList = w->getWidget("shared");
		for(const auto& i: res.shaderVars) {
			if(pass->hasShared(i.value)) addShared(sharedList, pass, i.key);
		}
		sharedList->getTemplateWidget<Button>("add")->eventPressed.bind([this, pass](Button* b){addShared(b->getParent(), pass, 0);});
		updateListInfo(sharedList);
		
		// Other properties
		w->getWidget<Combobox>("cull")->selectItem((int)pass->state.cullMode);
		w->getWidget<Combobox>("depthtest")->selectItem((int)pass->state.depthTest);
		w->getWidget<Checkbox>("depthmask")->setChecked(pass->state.depthWrite);
		w->getWidget<Checkbox>("redmask")->setChecked(pass->state.colourMask&MASK_RED);
		w->getWidget<Checkbox>("greenmask")->setChecked(pass->state.colourMask&MASK_GREEN);
		w->getWidget<Checkbox>("bluemask")->setChecked(pass->state.colourMask&MASK_BLUE);
		w->getWidget<Checkbox>("alphamask")->setChecked(pass->state.colourMask&MASK_ALPHA);
		w->getWidget<Checkbox>("wireframe")->setChecked(pass->state.wireframe);
		w->getWidget<SpinboxFloat>("depthoffset")->setValue(pass->state.depthOffset);

		// Change events
		w->getWidget<Combobox>("cull")->eventSelected.bind([pass](Combobox*, ListItem& i) { pass->state.cullMode = (CullMode)i.getIndex(); });
		w->getWidget<Combobox>("depthtest")->eventSelected.bind([pass](Combobox*, ListItem& i) { pass->state.depthTest = (DepthTest)i.getIndex(); });
		w->getWidget<Checkbox>("depthmask")->eventChanged.bind([pass](Button* b) { pass->state.depthWrite = b->isSelected(); });
		w->getWidget<Checkbox>("redmask")  ->eventChanged.bind([pass](Button* b) { (pass->state.colourMask &= 0xe) |= (b->isSelected()?1:0); });
		w->getWidget<Checkbox>("greenmask")->eventChanged.bind([pass](Button* b) { (pass->state.colourMask &= 0xd) |= (b->isSelected()?2:0); });
		w->getWidget<Checkbox>("bluemask") ->eventChanged.bind([pass](Button* b) { (pass->state.colourMask &= 0xa) |= (b->isSelected()?4:0); });
		w->getWidget<Checkbox>("alphamask")->eventChanged.bind([pass](Button* b) { (pass->state.colourMask &= 0x7) |= (b->isSelected()?8:0); });
		w->getWidget<Checkbox>("wireframe")->eventChanged.bind([pass](Button* b) { pass->state.wireframe = b->isSelected(); });
		w->getWidget<SpinboxFloat>("depthoffset")->eventChanged.bind([pass](SpinboxFloat*, float v){ pass->state.depthOffset = v; });


	});

	tech->selectItem(0, true);
	m_editors.push_back(w);
	return w;
}



void MaterialEditor::addTexture(Widget* w, Pass* pass, const char* key, const char* value) {
	Widget* entry = getEditor()->getGUI()->createWidget("textureslot");
	entry->getTemplateWidget<Textbox>("name")->setText(key);
	entry->getTemplateWidget<Textbox>("tex")->setText(value);
	entry->getTemplateWidget<Button>("del")->eventPressed.bind([pass](Button* b) {
		Widget* list = b->getParent()->getParent();
		b->getParent()->removeFromParent();
		delete b->getParent();
		updateListInfo(list);
	});
	w->add(entry);
	if(!value) updateListInfo(w);
}

void MaterialEditor::addVariable(Widget* w, Pass* pass, const char* name, const char* value) {
	Widget* entry = getEditor()->getGUI()->createWidget("variableslot");
	entry->getTemplateWidget<Textbox>("name")->setText(name);
	entry->getTemplateWidget<Combobox>("value")->setText(value);
	entry->getTemplateWidget<Combobox>("value")->shareList(m_autoVarList);
	entry->getTemplateWidget<Button>("del")->eventPressed.bind([pass](Button* b) {
		Widget* list = b->getParent()->getParent();
		b->getParent()->removeFromParent();
		delete b->getParent();
		updateListInfo(list);
	});
	w->add(entry);
	if(!value) updateListInfo(w);
}

void MaterialEditor::addShared(Widget* w, Pass* pass, const char* name) {
	Widget* entry = getEditor()->getGUI()->createWidget("textureslot");
	entry->getTemplateWidget<Textbox>("name")->setVisible(false);
	entry->getTemplateWidget<Textbox>("tex")->setText(name);
	entry->getTemplateWidget<Button>("del")->eventPressed.bind([pass](Button* b) {
		const char* name = b->getParent()->getTemplateWidget<Textbox>("tex")->getText();
		if(ShaderVars* v = Resources::getInstance()->shaderVars.get(name)) pass->removeShared(v);
		Widget* list = b->getParent()->getParent();
		b->getParent()->removeFromParent();
		delete b->getParent();
		updateListInfo(list);
	});
	w->add(entry);
	if(!name) updateListInfo(w);
}



bool MaterialEditor::drop(Widget* target, const Point& p, const Asset& asset, bool apply) {
	if(!target) return false;
	Resources& res = *Resources::getInstance();
	StringView ext = asset.file? strrchr(asset.file, '.'): 0;

	if(Combobox* c = cast<Combobox>(target)) {
		if(c->getName() == "shader") {
			if(asset.type == ResourceType::Shader || (asset.type == ResourceType::None && ext==".glsl")) {
				const char* name = asset.resource;
				if(!name) name = getResourceNameFromFile(res.textures, asset.file);
				if(name[0] == '/') ++name;
				c->setText(name);
				if(c->eventSubmit) c->eventSubmit(c);
				return true;
			}
		}
		return false;
	}

	
	Widget* list = target->getParent();
	if(!cast<CollapsePane>(list)) return false;

	if(list->getName() == "textures") {
		if(asset.type == ResourceType::Texture || (asset.type == ResourceType::None && (ext==".png" || ext==".dds"))) {
			const char* name = asset.resource;
			if(!name) name = getResourceNameFromFile(res.textures, asset.file);
			if(name[0] == '/') ++name;
			Textbox* tex = target->getTemplateWidget<Textbox>("tex");
			tex->setText(name);
			if(tex->eventSubmit) tex->eventSubmit(tex);
			return true;
		}
	}
	else if(list->getName() == "shared") {
		if(asset.type == ResourceType::ShaderVars) {
			Textbox* tex = target->getTemplateWidget<Textbox>("tex");
			tex->setText(asset.resource + 1);
			if(tex->eventSubmit) tex->eventSubmit(tex);
			return true;
		}
	}

	return false;
}

