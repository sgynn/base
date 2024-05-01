#ifndef _SCENE_EDITOR_
#define _SCENE_EDITOR_

#include <base/gui/tree.h>
#include <base/gui/widgets.h>
#include <base/scene.h>

namespace editor {


// Shown as an icon - base class
class Resource {
	public:
	enum BaseType { SCENE, TEXT, IMAGE };
	Resource();
	virtual ~Resource();
	virtual void createIcon();
	virtual void createPreview();
	virtual const char* getTypeName() = 0;
	const char*  getName();
	int          getIcon();

	protected:
	gui::String m_name;
	gui::String m_fullName;
	int         m_icon;
};

class ResourceBrowser {
	public:
	ResourceBrowser(gui::Widget*);
	~ResourceBrowser();

	void setPath(const char*);
	void refresh();

	protected:
	void breadcrumButton(gui::Button*);
	void pathChanged(gui::Textbox*, const char*);
	void itemClicked(gui::Button*);

	protected:
	gui::Widget* m_panel;
	gui::Widget* m_breadcrums;
	gui::String  m_path;

	std::vector<Resource*> m_items;
};


class ResourcePreview {
	public:
	ResourcePreview(gui::Widget*);
	~ResourcePreview();
	void setResource(Resource*);

	private:
	base::Scene* m_scene;
	Resource* m_resource;
};

}

#endif
