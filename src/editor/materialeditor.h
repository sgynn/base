#pragma once

#include <base/editor/editor.h>

namespace gui { class Widget; class ItemList; }
namespace base { class Pass; }

namespace editor {

class MaterialEditor : public EditorComponent {
	public:
	~MaterialEditor();
	void initialise() override;
	bool newAsset(const char*& name, const char*& file, const char*& body) const override;
	bool assetActions(gui::MenuBuilder&, const Asset&) override;
	gui::Widget* openAsset(const Asset&) override;
	protected:
	bool drop(gui::Widget*, const Point& p, const Asset&, bool) override;
	void addTexture(gui::Widget* w, base::Pass* pass, const char* key, const char* value);
	void addVariable(gui::Widget* w, base::Pass* pass, const char* name, const char* value);
	void addShared(gui::Widget* w, base::Pass* pass, const char* name);
	protected:
	gui::Widget* m_panel = nullptr;
	std::vector<gui::Widget*> m_editors;
	gui::ItemList* m_autoVarList = nullptr;
};

}

