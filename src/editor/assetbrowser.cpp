#include <base/editor/assetbrowser.h>
#include <base/resources.h>
#include <base/directory.h>
#include <base/gui/menubuilder.h>

#include "imageviewer.h"
#include "modelviewer.h"
#include "materialeditor.h"


#include <base/editor/embed.h>
BINDATA(editor_assets_gui, EDITOR_DATA "/assets.xml");
BINDATA(editor_assets_image, EDITOR_DATA "/asseticons.png");

using namespace gui;
using namespace editor;
using namespace base;

// Wildcard string matching:
bool matchWildcard(const char* s, const char* pattern) {
	const char* rollback = nullptr;
	const char* p = pattern;
	for(const char* c = s; *c; ++c) {
		if(*p=='?' || *p==*c) ++p;
		else if(*p=='*') {
			while(p[1]=='*') ++p; // Clear multiple * characters
			rollback = p;
			if(*c == p[1]) p+=2;
		}
		else if(rollback) p = rollback;
		else return false;
	}
	while(*p=='*') ++p;
	return *p==0;
}

class AssetTile : public Button {
	WIDGET_TYPE(AssetTile);
	AssetTile(const Rect& r, Skin* s) : Super(r,s) {}
	Asset asset;
	// Skip the Button::onMouseButton() one as it crashes since the callback can delete this widget
	void onMouseButton(const Point& p, int d, int u) override { Widget::onMouseButton(p,d,u); }
};

inline bool operator==(const Asset& a, const Asset& b) {
	if(a.file && a.file == b.file) return true;
	return a.type==b.type && a.resource==b.resource && a.file==b.file;
}

const char* AssetBrowser::getAssetName(const Asset& asset) {
	const char* name = asset.resource;
	if(!name) name = asset.file;
	if(const char* f = strrchr(name, '/')) name = f+1;
	return name;
}


AssetBrowser::~AssetBrowser() {
	delete m_contextMenu;
	delete m_newItemMenu;
}

void AssetBrowser::initialise() {
	Root::registerClass<AssetTile>();

	m_rootPath = "./data/";
	getEditor()->addEmbeddedPNGImage("data/editor/asseticons.png", editor_assets_image, editor_assets_image_len);
	m_panel = loadUI("assets.xml", editor_assets_gui);
	addToggleButton(m_panel, "editors", "browser");

	m_breadcrumbs = m_panel->getWidget("breadcrumbs");
	m_filter = m_panel->getWidget<Combobox>("filters");
	m_items = m_panel->getWidget("items");

	// Default icons for matched file types
	m_fileTypes[".bm"] = "cube";
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


	getEditor()->addTransientComponent(new ImageViewer());
	getEditor()->addTransientComponent(new ModelViewer());
	getEditor()->addTransientComponent(new MaterialEditor());
	

	getEditor()->assetChanged.bind([this](const char* asset, bool changed) {
		for(size_t i=0; i<m_modifiedFiles.size(); ++i) {
			if(strcmp(asset, m_modifiedFiles[i])==0) {
				if(changed) return;
				m_modifiedFiles[i] = std::move(m_modifiedFiles.back());
				m_modifiedFiles.pop_back();
				refreshItems();
				return;
			}
		}
		if(changed) {
			m_modifiedFiles.push_back(asset);
			refreshItems();
		}
	});


	m_filter->eventTextChanged.bind([this](Combobox*, const char*) { refreshItems(); });
	m_filter->eventSelected.bind([this](Combobox*, ListItem&) { refreshItems(); });

	if(Button* add = m_panel->getWidget<Button>("new")) add->eventPressed.bind([this](Button* b) {
		if(!m_newItemMenu) populateCreateMenu();
		m_newItemMenu->popup(b);
	});
}

void AssetBrowser::populateCreateMenu() {
	Root* root = m_panel->getRoot();
	m_newItemMenu = new Popup(Rect(0,0,100,100), nullptr);
	for(EditorComponent* handler: getEditor()) {
		const char* name = nullptr;
		const char* file = nullptr;
		const char* data = nullptr;
		if(handler->newAsset(name, file, data)) {
			m_newItemMenu->addItem(root, "button", "", name, [this, file, data](Button*) {
				m_newItemMenu->hide();
				String filename = getUniqueFileName(file, m_localPath);
				printf("New file: %s\n", filename.str());
				if(data) {
					FILE* fp = fopen(filename, "wb");
					fputs(data, fp);
					fclose(fp);
				}
				refreshItems();
			});
		}
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

void AssetBrowser::setRootPath(const char* path) {
	m_rootPath = path;
	if(!m_rootPath) m_rootPath = "./";
	else if(m_rootPath[m_rootPath.length()-1]!='/') m_rootPath += "/";
	refreshItems();
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
			e += sprintf(buffer, "%s/", m_breadcrumbs->getWidget(i)->as<Button>()->getCaption());
		}
		setPath(buffer);
	}
	else {
		// Show dropdown menu of subfolders
		static char buffer[1024];
		char* e = buffer + sprintf(buffer, "%s", m_rootPath.str());
		for(int i=2; i<=index; i+=1) {
			e += sprintf(buffer, "%s/", m_breadcrumbs->getWidget(i)->as<Button>()->getCaption());
		}

		Popup* popup = new Popup(Rect(0,0,100,10), m_panel->getRoot()->getSkin("panel"));
		for(auto& file: Directory(buffer)) {
			if(file.type == Directory::DIRECTORY) {
				if(file.name[0]=='.') continue;
				e = buffer + m_rootPath.length();
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
	sprintf(buffer, "%s%s%s", m_rootPath.str(), localPath, base);
	auto exists = [](const char* s) { FILE* fp = fopen(s, "r"); if(fp) fclose(fp); return fp; };
	int index = 1;
	while(exists(buffer)) {
		sprintf(buffer, "%s%s%.*s_%d%s", m_rootPath.str(), localPath, baseLen, base, index, ext);
		++index;
	}
	return buffer;
}

void AssetBrowser::refreshItems() {
	m_items->deleteChildWidgets();
	buildResourceTree();
	String filter = m_filter->getText()[0]? String::format("*%s*", m_filter->getText()): nullptr;

	// Scan the folder for files
	String fullPath = m_rootPath + m_localPath;
	for(auto& file: Directory(fullPath)) {
		if(file.name[0]=='.') continue;
		if(filter && !matchWildcard(file.name, filter)) continue;
		bool isFolder = file.type == Directory::DIRECTORY;
		addAssetTile(Asset{ResourceType::None, "", fullPath + file.name}, isFolder);
	}
	
	// Then get the resource assets
	int pathLength = m_localPath.length();
	for(const Asset& asset: m_resources) {
		if(asset.resource.startsWith(m_localPath) && !strchr(asset.resource+pathLength, '/')) {
			// Make sure it is not already added by the file search
			for(Widget* w: m_items) {
				Asset& tileAsset = w->as<AssetTile>()->asset;
				if(tileAsset == asset) {
					tileAsset.type = asset.type;
					tileAsset.resource = asset.resource;
					goto next;
				}
			}
			if(filter && !matchWildcard(asset.resource + pathLength, filter)) continue;
			addAssetTile(asset);
		}
		next:;
	}

	// Tooltips
	for(Widget* w: m_items) {
		AssetTile* tile = cast<AssetTile>(w);
		if(tile->getIcon() == 0) continue; // Folder
		String tip = tile->getCaption();
		switch(tile->asset.type) {
		case ResourceType::Model: tip += "\nModel: " + tile->asset.resource; break;
		case ResourceType::Shader: tip += "\nShader: " + tile->asset.resource; break;
		case ResourceType::Texture: tip += "\nTexture: " + tile->asset.resource; break;
		case ResourceType::Material: tip += "\nMaterial: " + tile->asset.resource; break;
		case ResourceType::Particle: tip += "\nparticle: " + tile->asset.resource; break;
		default: break;
		}
		if(tile->asset.file) tip += "\nPath: " + tile->asset.file;
		w->setToolTip(tip);
	}
}

void AssetBrowser::buildResourceTree() {
	m_resources.clear();
	Resources& res = *Resources::getInstance();
	static char buffer[512];
	strcpy(buffer, "./");
	auto addAsset = [this](ResourceType type, const char* name, ResourceManagerBase& rc) {
		Asset a { type, name };
		if(rc.findFile(name, buffer+2, 512)) {
			if(buffer[2] == '.') a.file = buffer + 2;
			else a.file = buffer;
		}
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
	if(!mod->isVisible()) for(const char* s: m_modifiedFiles) {
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
	if(tile->getIcon() == 0) { // Folder
		setPath(m_localPath + tile->getCaption());
		return false;
	}

	const Asset& asset = tile->asset;

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
	if(asset.file) remove(asset.file);
	refreshItems();
}

void AssetBrowser::duplicateAsset(Widget* item) {
	const Asset& asset = cast<AssetTile>(item)->asset;
	const char* name = getAssetName(asset);
	if(!name) name = asset.file;
	String src = asset.file;
	String dst = getUniqueFileName(name, m_localPath.str());
	if(!src) return;
	printf("Copy %s -> %s\n", src.str(), dst.str());
	FILE* sp = fopen(src, "rb");
	FILE* dp = fopen(dst, "wb");
	if(sp && dp) {
		fseek(sp, 0, SEEK_END);
		size_t len = ftell(sp);
		rewind(sp);
		char* buffer = new char[len];
		fread(buffer, 1, len, sp);
		fwrite(buffer, 1, len, dp);
		delete [] buffer;
	}
	fclose(sp);
	fclose(dp);
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
		lbl->setVisible(true);
		String file = m_rootPath + m_localPath + t->getText();
		FILE* fp = fopen(file, "r");
		if(fp) fclose(fp);
		else {
			String src = m_rootPath + m_localPath + lbl->getCaption();
			printf("Rename %s -> %s\n", src.str(), file.str());
			rename(src, file);
			t->removeFromParent();
			refreshItems();
			
			// We may need to update the resource manager
			String asset = m_localPath + t->getText();
			Resources& res = *Resources::getInstance();
			res.particles.reload(asset);

		}
		delete t;
	});
	box->eventLostFocus.bind([lbl](Widget* t) {
		lbl->setVisible(true);
		t->removeFromParent();
		delete t;
	});
}

void AssetBrowser::dragItem(Widget* w, const Point& p, int b) {
	if(b != 1) return;
	if(!m_dragWidget) {
		m_dragWidget = w->clone();
		m_dragWidget->setTangible(Tangible::NONE);
	}
	w->getRoot()->getRootWidget()->add(m_dragWidget);
	m_dragWidget->setPosition(w->getRoot()->getMousePos());
	m_lastClick.set(0,0);
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
		if(cast<Button>(w)->getIcon() != 0) { // Not a Folder
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
	else if(p == m_lastClick) {
		openAsset(w);
		m_lastClick.set(0, 0);
	}
	else m_lastClick = p;
}

