#include <base/editor/assetbrowser.h>
#include <base/resources.h>
#include <base/directory.h>

#include "imageviewer.h"
#include "modelviewer.h"


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



void AssetBrowser::initialise() {
	m_rootPath = "./data/";
	getEditor()->addEmbeddedPNGImage("data/editor/asseticons.png", editor_assets_image, editor_assets_image_len);
	m_panel = loadUI("assets.xml", editor_assets_gui);
	addToggleButton(m_panel, "editors", "browser");

	m_breadcrumbs = m_panel->getWidget("breadcrumbs");
	m_filter = m_panel->getWidget<Combobox>("filters");
	m_items = m_panel->getWidget("items");

	m_fileTypes["bm"] = Asset::Model;
	m_fileTypes["png"] = Asset::Texture;
	m_fileTypes["dds"] = Asset::Texture;
	m_fileTypes["glsl"] = Asset::Shader;
	m_fileTypes["vert"] = Asset::Shader;
	m_fileTypes["frag"] = Asset::Shader;
	m_fileTypes["ini"] = Asset::Text;
	m_fileTypes["def"] = Asset::Text;
	m_fileTypes["xml"] = Asset::Text;
	m_fileTypes["mat"] = Asset::Material;
	m_fileTypes["wav"] = Asset::Sound;
	m_fileTypes["ogg"] = Asset::Sound;


	getEditor()->addTransientComponent(new ImageViewer());
	getEditor()->addTransientComponent(new ModelViewer());
	

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

	Root* root = m_panel->getRoot();

	static auto getSelectedFileName = [this]() { return cast<Button>(m_selectedItem)->getCaption(); };
	m_contextMenu = new Popup(Rect(0,0,100,100), nullptr);
	m_contextMenu->addItem(root, "button", "open", "Open", [this](Button*) {
		m_contextMenu->hide();
		openAsset(m_selectedItem);
	});
	m_contextMenu->addItem(root, "button", "save", "Save", [this](Button*) {
		m_contextMenu->hide();
		String file = m_localPath + getSelectedFileName();
		for(EditorComponent* c: getEditor()) {
			if(c->saveAsset(file)) {
				getEditor()->assetChanged(file, false);
				break;
			}
		}
	});
	m_contextMenu->addItem(root, "button", "rename", "Rename", [this](Button*) {
		m_contextMenu->hide();
		renameFile(m_selectedItem);
	});
	m_contextMenu->addItem(root, "button", "duplicate", "Duplicate", [this](Button*) {
		m_contextMenu->hide();
		String src = m_rootPath + m_localPath + getSelectedFileName();
		String dst = getUniqueFileName(getSelectedFileName(), m_localPath.str());
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
	});
	m_contextMenu->addItem(root, "button", "delete", "Delete", [this](Button*) {
		m_contextMenu->hide();
		remove(m_rootPath + m_localPath + getSelectedFileName());
		refreshItems();
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

	char buffer[1024];
	sprintf(buffer, "%s%s", m_rootPath.str(), m_localPath.str());
	for(auto& file: Directory(buffer)) {
		if(file.name[0]=='.') continue;
		const char* ext = file.name + file.ext;
		int type;
		if(file.type == Directory::DIRECTORY) type = Asset::Folder;
		else type = m_fileTypes.get(ext, -1);

		if(m_filter && m_filter->getText()[0]) {
			String filter = String::format("*%s*", m_filter->getText());
			if(!matchWildcard(file.name, filter)) continue;
		}

		Button* asset = m_items->createChild<Button>("asset");
		if(!asset) return;

		if(type == Asset::Folder) {
			asset->eventPressed.bind([this](Button* b) { setPath(m_localPath + b->getCaption()); });
		}

		asset->eventMouseMove.bind(this, &AssetBrowser::dragItem);
		asset->eventMouseUp.bind(this, &AssetBrowser::dropItem);

		asset->setCaption(file.name);
		switch(type) {
		case Asset::Folder: asset->setIcon("folder"); break;
		case Asset::Text: asset->setIcon("page"); break;
		case Asset::Model: asset->setIcon("cube"); break;
		case Asset::Sound: asset->setIcon("sound"); break;
		case Asset::Texture: asset->setIcon("image"); break;
		}

		Widget* mod = asset->getTemplateWidget("_modified");
		mod->setVisible(false);
		for(const char* s: m_modifiedFiles) {
			if(strcmp(file.name, s)==0) {
				mod->setVisible(true);
				break;
			}
		}
	}
}

bool AssetBrowser::openAsset(Widget* w) {
	const char* file = cast<Button>(w)->getCaption();
	String path = m_localPath + file;
	
	// Already open ?
	for(Editor& e: m_activeEditors) {
		if(e.file == path) {
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


	Widget* window = nullptr;
	for(EditorComponent* c: getEditor()) {
		window = c->openAsset(path);
		if(window) {
			if(!window->getParent()) {
				window->setVisible(true);
				if(gui::Window* w = cast<gui::Window>(window)) w->setCaption(file);
				m_activeEditors.push_back({path, window});
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
	if(window) printf("Open file: %s\n", file);
	else printf("No file handlers for %s\n", file);
	return window;
}

void AssetBrowser::renameFile(Widget* item) {
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
		getEditor()->drop(pos, 0, m_localPath + cast<Button>(w)->getCaption());
	}
	else if(b==4) {
		m_selectedItem = w;
		m_contextMenu->popup(w->getRoot(), w->getRoot()->getMousePos(), w);
	}
	else if(p == m_lastClick) {
		openAsset(w);
		m_lastClick.set(0, 0);
	}
	else m_lastClick = p;
}

