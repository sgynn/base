#pragma once

#include "editor.h"
#include <base/hashmap.h>

namespace gui { class Widget; class TreeView; class TreeNode; class ItemList; class ListItem; class Popup; class Button; }

namespace editor {
class AudioEditor : public EditorComponent {
	public:
	void initialise() override;
	void activate() override;
	void deactivate() override;
	bool isActive() const override;
	void update() override;

	void refreshDataTree();
	void refreshFileList();

	bool newAsset(const char*& name, const char*& file, const char*& body) const override;
	bool assetActions(gui::MenuBuilder&, const Asset&) override;
	gui::Widget* openAsset(const Asset&) override;
	
	protected:
	using objectID = unsigned;
	enum class Type { Mixer, Sound, Folder, Random, Sequence, Switch, Attenuation, Variable, Enum, Event };
	struct Object { Type type; objectID id; };
	Object getSelectedObject() const;
	Object getObject(const gui::TreeNode*) const;
	static const char* getObjectName(Type, objectID);
	static base::HashMap<unsigned>& getLookupMap(Type);
	static bool isSoundType(Type type);
	void selectObject(gui::TreeView*, gui::TreeNode*);
	void setupPropertyEvents();
	int renameObject(Object object, const char* newName);
	objectID findMixer(const gui::TreeNode* node);
	gui::TreeNode* findNode(Object, gui::TreeNode*);
	void deleteItem();
	void addItem(Type type);

	void setupSoundCaster(Object);
	void playSound(gui::Button*);
	void save(const char* file);

	protected:
	gui::Widget* m_panel;
	gui::TreeView* m_data;
	gui::ItemList* m_variableList;
	gui::Widget* m_properties;
	gui::Popup* m_menu;
	gui::Widget* m_preview;
	objectID m_testObject = -1;
	char m_playing = 0;
};
}

