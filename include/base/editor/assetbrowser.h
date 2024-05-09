#pragma once

#include "editor.h"
#include <base/gui/widgets.h>
#include <base/gui/lists.h>

namespace editor {

struct Asset {
	enum Type { Folder, Texture, Shader, Model, Material, Text, Sound };
	gui::String name;
	int icon;
	Type type;
	bool changed = false;
};


class AssetBrowser : public EditorComponent {
	public:
	void initialise() override;
	bool isActive() const override;
	void activate() override;
	void deactivate() override;
	void setRootPath(const char* path);
	void setPath(const char* path);
	void refreshItems();

	gui::String getUniqueFileName(const char* baseName, const char* localPath) const;

	protected:
	void populateCreateMenu();
	int createTextureIcon(const char* name);
	int createModelIcon(const char* name);
	int createShaderIcon(const char* name);
	int allocateIcon(const char* name);

	bool openAsset(gui::Widget*);
	void renameFile(gui::Widget*);
	void pressedCrumb(gui::Button*);
	void dragItem(gui::Widget*, const Point&, int);
	void dropItem(gui::Widget*, const Point&, int);

	protected:
	gui::String m_rootPath;
	gui::String m_localPath;
	gui::String m_search;
	int m_typeFilter;

	// Icons are a grid
	struct IconData { int index; bool used; bool visible; };
	std::vector<IconData> m_iconData;
	base::HashMap<int> m_iconMap;
	int m_iconImage;

	base::HashMap<int> m_fileTypes;

	struct Editor { gui::String file; gui::Widget* widget; };
	std::vector<Editor> m_activeEditors;
	std::vector<gui::String> m_modifiedFiles;

	gui::Widget* m_panel = nullptr;
	gui::Widget* m_breadcrumbs;
	gui::Combobox* m_filter;
	gui::Widget* m_items;
	gui::Widget* m_selectedItem = nullptr;
	gui::Widget* m_dragWidget = nullptr;
	gui::Popup*  m_newItemMenu = nullptr;
	gui::Popup*  m_contextMenu = nullptr;
	Point m_lastClick;
};


}

