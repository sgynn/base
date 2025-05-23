#pragma once

#include "editor.h"
#include <base/hashmap.h>

namespace gui { class Widget; class Listbox; class TreeView; class TreeNode; class Textbox; class ItemList; class ListItem; class Popup; class Button; }
namespace base { class XMLElement; }

namespace editor {
class AudioEditor : public EditorComponent {
	public:
	~AudioEditor();
	void initialise() override;
	void activate() override;
	void deactivate() override;
	bool isActive() const override;
	void update() override;

	void refreshDataTree();
	void refreshFileList();

	void assetCreationActions(AssetCreationBuilder&) override;
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
	static bool isRootLevelSound(unsigned);

	template<class T> void listItems(Type type, gui::TreeNode*root, const T& list, const char* icon);

	protected:
	gui::Widget* m_panel;
	gui::Listbox* m_bankList;
	gui::TreeView* m_data;
	gui::ItemList* m_variableList;
	gui::Widget* m_properties;
	gui::Popup* m_menu;
	gui::Popup* m_bankMenu;
	gui::Widget* m_preview;
	gui::Textbox* m_filter;
	unsigned char m_typeFilter = -1;
	objectID m_testObject = -1;
	char m_playing = 0;
	int m_loadMessage = 0;
};
}

