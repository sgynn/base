#include <base/editor/assetbrowser.h>
#include <base/resources.h>
#include <base/directory.h>
#include <base/gui/menubuilder.h>
#include <base/virtualfilesystem.h>

#include "imageviewer.h"
#include "modelviewer.h"
#include "materialeditor.h"


#include <base/editor/embed.h>
BINDATA(editor_assets_gui, EDITOR_DATA "/assets.xml");
BINDATA(editor_assets_image, EDITOR_DATA "/asseticons.png");

using namespace gui;
using namespace editor;
using namespace base;

class AssetTile : public Button {
	WIDGET_TYPE(AssetTile);
	Asset asset;
	// Skip the Button::onMouseButton() one as it crashes since the callback can delete this widget
	void onMouseButton(const Point& p, int d, int u) override { Widget::onMouseButton(p,d,u); }
};

inline bool operator==(const Asset& a, const Asset& b) {
	if(a.file && (a.file == b.file)) return true;
	return a.type==b.type && a.resource==b.resource && a.file==b.file;
}

const char* AssetBrowser::getAssetName(const Asset& asset) {
	const char* name = asset.resource;
	if(!name) name = asset.file.name;
	if(const char* f = strrchr(name, '/')) name = f+1;
	return name;
}


AssetBrowser::~AssetBrowser() {
	delete m_contextMenu;
	delete m_newItemMenu;
	delete m_typeFilterList;
}

void AssetBrowser::initialise() {
	Root::registerClass<AssetTile>();

	getEditor()->addEmbeddedPNGImage("data/editor/asseticons.png", editor_assets_image, editor_assets_image_len);
	m_panel = loadUI("assets.xml", editor_assets_gui);
	addToggleButton(m_panel, "editors", "browser");

	m_breadcrumbs = m_panel->getWidget("breadcrumbs");
	m_filter = m_panel->getWidget<Textbox>("filter");
	m_items = m_panel->getWidget("items");

	// Default icons for matched file types
	m_fileTypes[".bm"] = "cube";
	m_fileTypes[".obj"] = "cube";
	m_fileTypes[".png"] = "image";
	m_fileTypes[".dds"] = "image";
	m_fileTypes[".glsl"] = "page";
	m_fileTypes[".vert"] = "page";
	m_fileTypes[".frag"] = "page";
	m_fileTypes[".ini"] = "page";
	m_fileTypes[".def"] = "page";
	m_fileTypes[".xml"] = "page";
	m_fileTypes[".mat"] = "sphere";
	m_fileTypes[".wav"] = "sound";
	m_fileTypes[".ogg"] = "sound";

	m_typeFilterList = new Popup();
	m_typeFilterList->setSkin(getEditor()->getGUI()->getSkin("button"));
	m_typeFilterList->setSize(100,100);

	addTypeFilter("Files",     ResourceType::None);
	addTypeFilter("Models",    ResourceType::Model);
	addTypeFilter("Textures",  ResourceType::Texture);
	addTypeFilter("Materials", ResourceType::Material);
	addTypeFilter("Shaders",   ResourceType::Shader);
	addTypeFilter("Particle",  ResourceType::Particle);
	m_panel->getWidget<Button>("types")->eventPressed.bind([this](Button* b) { m_typeFilterList->popup(b); });


	getEditor()->addTransientComponent(new ImageViewer());
	getEditor()->addTransientComponent(new ModelViewer());
	getEditor()->addTransientComponent(new MaterialEditor());
	

	getEditor()->assetChanged.bind([this](const Asset& asset, bool changed) {
		if(!asset.file) return;
		if(!changed) printf("NotifySaved %s\n", asset.file.getFullPath().str());
		for(size_t i=0; i<m_modifiedFiles.size(); ++i) {
			if(m_modifiedFiles[i] == asset.file) {
				if(changed) return;
				m_modifiedFiles[i] = std::move(m_modifiedFiles.back());
				m_modifiedFiles.pop_back();
				refreshItems();
				return;
			}
		}
		if(changed) {
			m_modifiedFiles.push_back(asset.file);
			refreshItems();
		}
	});


	m_filter->eventChanged.bind([this](Textbox*, const char*) { refreshItems(); });
	m_panel->getWidget<Button>("clearfilter")->eventPressed.bind([this](Button*) {
		m_filter->setText("");
		refreshItems();
	});

	if(Button* add = m_panel->getWidget<Button>("new")) add->eventPressed.bind([this](Button* b) {
		if(!m_newItemMenu) populateCreateMenu();
		m_newItemMenu->popup(b);
	});
}

void AssetBrowser::addTypeFilter(const char* text, ResourceType type) {
	m_typeFilterList->addItem(getEditor()->getGUI(), "checkbox", "", text);
	Checkbox* c = cast<Checkbox>(m_typeFilterList->getWidget(m_typeFilterList->getWidgetCount()-1));
	c->setDragSelect(true);
	c->setSelected(m_typeFilter & 1ull << (int)type);
	c->eventChanged.bind([this, type](Button* b) {
		if(b->isSelected()) m_typeFilter |= 1ull<<(int)type;
		else m_typeFilter &= ~(1ull<<(int)type);
		refreshItems();
	});
}

void AssetBrowser::populateCreateMenu() {
	Root* root = m_panel->getRoot();
	m_newItemMenu = new Popup();
	m_newItemMenu->setSize(100, 0);

	AssetCreationBuilder builder;
	for(EditorComponent* handler: getEditor()) handler->assetCreationActions(builder);
	for(auto& i: builder.list) {
		m_newItemMenu->addItem(root, "button", "", i.first, [this, create=i.second](Button*) {
			m_newItemMenu->hide();
			Asset newAsset = create(m_localPath);
			if(!newAsset.file && !newAsset.resource) return; // Nope
			if(!newAsset.file) newAsset.file = getFileSystem().createTransientFile(newAsset.resource);
			assert(newAsset.file);
			getEditor()->assetChanged(newAsset, true);
			openAssetEvent(newAsset);
		});
	}
}

void AssetBrowser::activate() {
	m_panel->setVisible(true);
	if(!m_localPath && m_items && m_items->getWidgetCount()==0) setPath(m_localPath);
}

void AssetBrowser::deactivate() {
}

bool AssetBrowser::isActive() const {
	return m_panel->isVisible();
}

void AssetBrowser::setPath(const char* path) {
	if(!path) path="";
	else if(path[0]=='.' && path[1]=='/') path += 2;
	m_localPath = path;
	if(m_localPath && !m_localPath.endsWith("/")) m_localPath += "/";

	// create breadcrumbs
	if(m_breadcrumbs) {
		auto add = [&](const char* name, int length=0) {
			Button* b = m_breadcrumbs->createChild<Button>("breadcrumb");
			if(b) {
				b->setCaption(length? String(name, length).str(): name);
				b->eventPressed.bind(this, &AssetBrowser::pressedCrumb);
			}
			return b;
		};

		m_breadcrumbs->deleteChildWidgets();
		add("Root");
		while(const char* e = strchr(path, '/')) {
			add(">");
			add(path, e-path);
			path = e+1;
		}
		if(path && path[0]) {
			add(">");
			add(path);
		}
	}

	refreshItems();
}

void AssetBrowser::pressedCrumb(gui::Button* b) {
	int index = b->getIndex();
	if(index == 0) setPath("");
	else if((index&1)==0) {
		char buffer[1024];
		char* e = buffer;
		for(int i=2; i<=index; i+=1) {
			e += sprintf(e, "%s/", m_breadcrumbs->getWidget(i)->as<Button>()->getCaption());
		}
		setPath(buffer);
	}
	else {
		// Show dropdown menu of subfolders
		static char buffer[1024];
		char* e = buffer;
		for(int i=2; i<=index; i+=1) {
			e += sprintf(e, "%s/", m_breadcrumbs->getWidget(i)->as<Button>()->getCaption());
		}

		Popup* popup = new Popup();
		popup->setSize(100, 0);
		popup->setSkin(m_panel->getRoot()->getSkin("panel"));
		for(auto& file: Directory(buffer)) {
			if(file.type == Directory::DIRECTORY) {
				if(file.name[0]=='.') continue;
				e = buffer;
				Popup::ButtonDelegate func;
				func.bind( [this, buffer=e](Button* b) { strcat(buffer, b->getCaption()); setPath(buffer); m_items->setFocus(); });
				popup->addItem(m_panel->getRoot(), "breadcrumb", "", file.name, func);
			}
		}
		popup->popup(b);
	}
}

String AssetBrowser::getUniqueFileName(const char* base, const char* localPath) const {
	if(!localPath) localPath = "";
	const char* ext = strrchr(base, '.');
	int baseLen = ext? ext-base: strlen(base);
	// strip _1 part
	const char* fin = base + baseLen - 1;
	if(baseLen > 2 && fin[-1]=='_' && fin[0]>='0' && fin[0]<='9') baseLen -= 2;

	char buffer[1024];
	sprintf(buffer, "%s%s", localPath, base);
	int index = 1;
	while(getFileSystem().getFile(buffer)) {
		sprintf(buffer, "%s%.*s_%d%s", localPath, baseLen, base, index, ext);
		++index;
	}
	return buffer + strlen(localPath);
}

void AssetBrowser::refresh() {
	refreshItems();
}

void AssetBrowser::refreshItems() {
	m_items->deleteChildWidgets();
	buildResourceTree();
	String filter = m_filter->getText()[0]? String::format("*%s*", m_filter->getText()): nullptr;

	// Scan the folder for files
	if(m_typeFilter & 1) {
		const VirtualFileSystem::Folder& folder = Resources::getInstance()->getFileSystem().getFolder(m_localPath);
		for(auto& file: folder) {
			if(filter && !String::match(file.name, filter)) continue;
			addAssetTile(Asset{ResourceType::None, "", file}, file.isFolder());
		}
	}
	
	// Then get the resource assets
	int pathLength = m_localPath.length();
	for(const Asset& asset: m_resources) {
		if(~m_typeFilter & 1ull<<(int)asset.type) continue;
		if(asset.resource && asset.resource.startsWith(m_localPath) && !strchr(asset.resource+pathLength, '/')) {
			// Make sure it is not already added by the file search
			for(Widget* w: m_items) {
				Asset& tileAsset = w->as<AssetTile>()->asset;
				if(tileAsset == asset) {
					tileAsset.type = asset.type;
					tileAsset.resource = asset.resource;
					goto next;
				}
			}
			if(filter && !String::match(asset.resource + pathLength, filter)) continue;
			addAssetTile(asset);
		}
		next:;
	}

	// Also custom items
	for(const Asset& asset : m_custom) {
		if(~m_typeFilter & 1ull<<(int)asset.type) continue;
		if(asset.resource.startsWith(m_localPath) && !strchr(asset.resource+pathLength, '/')) {
			if(filter && !String::match(asset.resource + pathLength, filter)) continue;
			addAssetTile(asset);
		}
	}

	// Tooltips
	for(Widget* w: m_items) {
		AssetTile* tile = cast<AssetTile>(w);
		if(tile->getIconIndex() == 0) continue; // Folder
		String tip = tile->getCaption();
		switch(tile->asset.type) {
		case ResourceType::Model: tip += "\nModel: " + tile->asset.resource; break;
		case ResourceType::Shader: tip += "\nShader: " + tile->asset.resource; break;
		case ResourceType::Texture: tip += "\nTexture: " + tile->asset.resource; break;
		case ResourceType::Material: tip += "\nMaterial: " + tile->asset.resource; break;
		case ResourceType::Particle: tip += "\nparticle: " + tile->asset.resource; break;
		default:
			if(tile->asset.type > ResourceType::Custom) 
				tip += "\n" + m_customTypes[(int)tile->asset.type-(int)ResourceType::Custom] + ": " + tile->asset.resource;
			break;
		}
		if(tile->asset.file) tip += "\nPath: " + tile->asset.file.getFullPath();
		w->setToolTip(tip);
	}
}

void AssetBrowser::clearCustomItems() {
	m_custom.clear();
}

void AssetBrowser::addCustomItem(const char* type, const char* name, const char* file) {
	unsigned typeIndex = 0;
	for(const String& s: m_customTypes) { if(type == s) break; ++typeIndex; }
	ResourceType customType = (ResourceType) ((int)ResourceType::Custom + typeIndex);
	if(m_customTypes.size() == typeIndex) {
		m_customTypes.push_back(type);
		addTypeFilter(type, customType);
	}
	// FIXME - this file is not in out VirtualFileSystem
	m_custom.push_back(Asset{customType, name, {}});
}

void AssetBrowser::removeCustomItem(const char* type, const char* name) {
	unsigned typeIndex = 0;
	for(const String& s: m_customTypes) { if(type == s) break; ++typeIndex; }
	ResourceType customType = (ResourceType) ((int)ResourceType::Custom + typeIndex);
	for(size_t i=0; i<m_custom.size(); ++i) {
		if(m_custom[i].type == customType && m_custom[i].resource == name) {
			m_custom[i] = std::move(m_custom.back());
			m_custom.pop_back();
		}
	}
}

void AssetBrowser::buildResourceTree() {
	m_resources.clear();
	Resources& res = *Resources::getInstance();
	auto addAsset = [this, &res](ResourceType type, const char* name, ResourceManagerBase& rc) {
		Asset a { type, name };
		a.file = res.getFileSystem().getFile(name);
		m_resources.push_back(a);
	};

	for(const auto& i: res.models)    if(i.value) addAsset(ResourceType::Model, i.key, res.models);
	for(const auto& i: res.textures)  if(i.value) addAsset(ResourceType::Texture, i.key, res.textures);
	for(const auto& i: res.materials) if(i.value) addAsset(ResourceType::Material, i.key, res.materials);
	for(const auto& i: res.particles) if(i.value) addAsset(ResourceType::Particle, i.key, res.particles);
}

Widget* AssetBrowser::addAssetTile(const Asset& asset, bool folder) {
	AssetTile* tile = m_items->createChild<AssetTile>("asset");
	if(!tile) return nullptr;

	tile->eventMouseDown.bind(this, &AssetBrowser::pressItem);
	tile->eventMouseMove.bind(this, &AssetBrowser::dragItem);
	tile->eventMouseUp.bind(this, &AssetBrowser::dropItem);
	
	const char* title = getAssetName(asset);
	const char* ext = strrchr(title, '.');
	if(!ext) ext = "";
	
	tile->setCaption(title);
	if(folder) tile->setIcon("folder");
	else switch(asset.type) {
		case ResourceType::None: tile->setIcon(m_fileTypes.get(ext, "page")); break;
		case ResourceType::Model:     tile->setIcon("cube"); break;
		case ResourceType::Texture:   tile->setIcon("image"); break;
		case ResourceType::Material:  tile->setIcon("sphere"); break;
		case ResourceType::Shader:    tile->setIcon("sphere"); break;
		case ResourceType::ShaderVars:tile->setIcon("page"); break;
		case ResourceType::Particle:  tile->setIcon("page"); break;
		default: break;
	}


	Widget* mod = tile->getTemplateWidget("_modified");
	mod->setVisible(!asset.file);
	if(!mod->isVisible()) for(const auto& s: m_modifiedFiles) {
		if(asset.file == s) {
			mod->setVisible(true);
			break;
		}
	}

	tile->asset = asset;
	return tile;
}

bool AssetBrowser::openAsset(Widget* w) {
	AssetTile* tile = cast<AssetTile>(w);
	if(tile->getIconIndex() == 0) { // Folder
		setPath(m_localPath + tile->getCaption());
		return false;
	}
	return openAssetEvent(tile->asset);
}

bool AssetBrowser::openAssetEvent(const Asset& asset) {
	// Already open ?
	for(Editor& e: m_activeEditors) {
		if(e.asset == asset) {
			e.widget->setVisible(true);
			e.widget->raise();
			Point p = e.widget->getPosition();
			Point max = e.widget->getParent()->getSize() - e.widget->getSize();
			if(p.x>max.x) p.x = max.x;
			if(p.y>max.y) p.y = max.y;
			if(p.x<0) p.x = 0;
			if(p.y<0) p.y = 0;
			e.widget->setPosition(p);
			return true;
		}
	}

	const char* name = getAssetName(asset);

	Widget* window = nullptr;
	for(EditorComponent* c: getEditor()) {
		window = c->openAsset(asset);
		if(window) {
			if(!window->getParent()) {
				window->setVisible(true);
				if(gui::Window* w = cast<gui::Window>(window)) w->setCaption(name);
				m_activeEditors.push_back({asset, window});
				getEditor()->getGUI()->getRootWidget()->add(window);
				// cascade
				bool overlap = true;
				while(overlap) {
					overlap = false;
					for(Widget* w : getEditor()->getGUI()->getRootWidget()) {
						if(w!=window && w->getPosition() == window->getPosition()) {
							overlap = true;
							break;
						}
					}
					if(overlap) window->setPosition(window->getPosition() + 16);
				}
			}
			break;
		}
	}
	if(window) printf("Open file: %s\n", name);
	else printf("No file handlers for %s\n", name);

	if(window) refreshItems(); // Opening an asset can change stuff
	return window;
}

void AssetBrowser::deleteAsset(Widget* item) {
	const Asset& asset = cast<AssetTile>(item)->asset;
	if(asset.file && !asset.file.inArchive()) {
		printf("Delete %s\n", asset.file.getFullPath().str());
		getFileSystem().removeFile(m_localPath + asset.file.name);
		remove(asset.file.getFullPath());
	}
	refreshItems();
}

void AssetBrowser::duplicateAsset(Widget* item) {
	const Asset& asset = cast<AssetTile>(item)->asset;
	const char* name = getAssetName(asset);
	if(!name) name = asset.file.name;
	String src = asset.file.getFullPath();
	if(!src) return;

	String dstName = getUniqueFileName(name, m_localPath);
	VirtualFileSystem::File dst = getFileSystem().createTransientFile(m_localPath + dstName);

	printf("Copy %s -> %s\n", src.str(), dst.getFullPath().str());
	File data = asset.file.read();
	if(!data) return;
	FILE* fp = fopen(dst.getFullPath(), "wb");
	if(fp) {
		fwrite(data, 1, data.size(), fp);
		fclose(fp);
	}
	refreshItems();
}

void AssetBrowser::renameAsset(Widget* item) {
	Textbox* box = item->createChild<Textbox>("editbox");
	Label* lbl = item->getTemplateWidget<Label>("_text");
	if(!box || !lbl) return;

	box->setPosition(0, item->getSize().y - box->getSize().y);
	box->setSize(item->getSize().x, box->getSize().y);
	box->setText(lbl->getCaption());
	box->select(0, 999);
	box->setFocus();
	lbl->setVisible(false);

	box->eventSubmit.bind([this, lbl](Textbox* t) {
		const Asset& asset = cast<AssetTile>(t->getParent())->asset;
		lbl->setVisible(true);
		if(asset.file.inArchive() || !t->getText()[0]) { // Invalid rename
			delete t;
			return;
		}

		getFileSystem().removeFile(m_localPath + asset.file.name);
		String file = asset.file.getFullPath();
		const char* e = strrchr(file, '/');
		String newFile = (e? StringView(file, e-file.str()+1): StringView()) + t->getText();
		Resources& res = *Resources::getInstance();

		if(FILE* fp = fopen(file, "r")) {
			fclose(fp);
			printf("Rename %s -> %s\n", file.str(), newFile.str());
			rename(file, newFile);
			getFileSystem().scanFolders();
		}
		else {	// Not yet saved - need to update the resource data
			switch(asset.type) {
			case ResourceType::Particle: res.particles.rename(asset.resource, t->getText()); break;
			default: break; // TODO: other types ?
			}
		}

		t->removeFromParent();
		refreshItems();
		
		// We may need to update the resource manager FIXME
		res.particles.reload(file);
		delete t;
	});
	box->eventLostFocus.bind([lbl](Widget* t) {
		lbl->setVisible(true);
		t->removeFromParent();
		delete t;
	});
}

inline int manhattenDistance(const Point& a, const Point& b) {
	return abs(a.x-b.x) + abs(a.y-b.y);
}

void AssetBrowser::pressItem(Widget* w, const Point& p, int b) {
	if(b != 1) return;
	if(m_clickCount > 0 && manhattenDistance(p, m_lastClick) >= 8) m_clickCount = 0;
	m_lastClick = p;
	++m_clickCount;
}

void AssetBrowser::dragItem(Widget* w, const Point& p, int b) {
	if(b != 1) return;
	if(abs(p.x-m_lastClick.x) + abs(p.y-m_lastClick.y) < 8) return;
	if(!m_dragWidget) {
		m_dragWidget = w->clone();
		m_dragWidget->setTangible(Tangible::NONE);
	}
	w->getRoot()->getRootWidget()->add(m_dragWidget);
	m_dragWidget->setPosition(w->getRoot()->getMousePos());
	m_clickCount = 0;
	getEditor()->cancelActiveTools(this);
}

void AssetBrowser::dropItem(Widget* w, const Point& p, int b) {
	if(m_dragWidget) {
		m_dragWidget->removeFromParent();
		delete m_dragWidget;
		m_dragWidget = nullptr;
		
		// Drop function
		Point pos = w->getRoot()->getMousePos();
		getEditor()->drop(pos, cast<AssetTile>(w)->asset);
	}
	else if(b==4) { // Item context menu
		m_selectedItem = w;
		if(cast<Button>(w)->getIconIndex() != 0) { // Not a Folder
			const Asset& asset = cast<AssetTile>(w)->asset;
			MenuBuilder menu(w->getRoot(), "button", "button");
			menu.addAction("Open", [this, w]() { openAsset(w); });
			for(EditorComponent* c: getEditor()) {
				if(c->assetActions(menu, asset)) break;
			}
			menu.addAction("Rename",    [this, w]() { renameAsset(w); });
			menu.addAction("Duplicate", [this, w]() { duplicateAsset(w); });
			menu.addAction("Delete",    [this, w]() { deleteAsset(w); });

			delete m_contextMenu;
			m_contextMenu = menu;
			menu.menu()->popup(w->getRoot(), w->getRoot()->getMousePos(), w);
		}
	}
	else if(m_clickCount > 1 && manhattenDistance(p, m_lastClick) < 8) {
		openAsset(w);
		m_clickCount = 0;
	}
}

