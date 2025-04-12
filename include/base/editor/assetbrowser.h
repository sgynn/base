#pragma once

#include "editor.h"
#include <base/gui/widgets.h>
#include <base/gui/lists.h>

namespace editor {

class AssetBrowser : public EditorComponent {
	public:
	~AssetBrowser();
	void initialise() override;
	bool isActive() const override;
	void activate() override;
	void deactivate() override;
	void refresh() override;
	void setRootPath(const char* path);
	void setPath(const char* path);
	void refreshItems();

	gui::String getUniqueFileName(const char* baseName, const char* localPath) const;

	void clearCustomItems();
	void addCustomItem(const char* type, const char* name, const char* file=nullptr);
	void removeCustomItem(const char* type, const char* name);

	protected:
	void populateCreateMenu();
	int createTextureIcon(const char* name);
	int createModelIcon(const char* name);
	int createShaderIcon(const char* name);
	int allocateIcon(const char* name);

	bool openAssetEvent(const Asset& asset);
	bool openAsset(gui::Widget*);
	void duplicateAsset(gui::Widget*);
	void renameAsset(gui::Widget*);
	void deleteAsset(gui::Widget*);
	void pressedCrumb(gui::Button*);
	void dragItem(gui::Widget*, const Point&, int);
	void dropItem(gui::Widget*, const Point&, int);
	void addTypeFilter(const char* text, ResourceType type);

	void buildResourceTree();
	gui::Widget* addAssetTile(const Asset& asset, bool isFolder=false);
	static const char* getAssetName(const Asset&);
	using EditorComponent::openAsset;
	protected:
	gui::String m_localPath; // always ends with / and never starts with ./
	gui::String m_search;
	unsigned long long m_typeFilter = -1;

	// Icons are a grid
	struct IconData { int index; bool used; bool visible; };
	std::vector<IconData> m_iconData;
	base::HashMap<int> m_iconMap;
	int m_iconImage;

	base::HashMap<const char*> m_fileTypes; // Icon names for file extensions
	std::vector<Asset> m_resources;
	std::vector<Asset> m_custom;
	std::vector<gui::String> m_customTypes;

	struct Editor { Asset asset; gui::Widget* widget; };
	std::vector<Editor> m_activeEditors;
	std::vector<base::VirtualFileSystem::File> m_modifiedFiles; // Files tagged as modified
	std::vector<gui::String> m_newFiles; // Files requested for new assets not yet saved

	gui::Widget* m_panel = nullptr;
	gui::Widget* m_breadcrumbs;
	gui::Textbox* m_filter;
	gui::Widget* m_items;
	gui::Widget* m_selectedItem = nullptr;
	gui::Widget* m_dragWidget = nullptr;
	gui::Popup*  m_newItemMenu = nullptr;
	gui::Popup*  m_contextMenu = nullptr;
	gui::Popup*  m_typeFilterList = nullptr;
	Point m_lastClick;
};


}

