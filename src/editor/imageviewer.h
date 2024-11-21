#pragma once

#include <base/editor/editor.h>
#include <base/resources.h>
#include <base/texture.h>
#include <base/gui/gui.h>
#include <base/gui/renderer.h>

namespace editor {

class ImageViewer : public EditorComponent {
	public:
	void initialise() override {}
	gui::Widget* openAsset(const Asset& asset) override {
		if(asset.type == ResourceType::None) {
			if(!asset.file.name.endsWith(".png") && !asset.file.name.endsWith(".dds")) return nullptr;
		}
		else if(asset.type != ResourceType::Texture) return nullptr;

		gui::Widget* previewTemplate = getEditor()->getGUI()->getWidget("filepreview");
		if(!previewTemplate) return nullptr;
		gui::Renderer& r = *getEditor()->getGUI()->getRenderer();

		const char* name = asset.resource;
		if(!name) name = asset.file.name;

		int img = r.getImage(name);
		if(img < 0) {
			base::Texture* tex = base::Resources::getInstance()->textures.get(name);
			if(tex) img = r.addImage(name, tex->width(), tex->height(), tex->unit());
		}
		if(img > 0) {
			Point size = r.getImageSize(img);
			if(size.x==1 && size.y==1) size.set(32,32);
			Point border = previewTemplate->getSize() - previewTemplate->getClientRect().size();
			gui::Widget* preview = previewTemplate->clone();
			preview->getWidget(0)->setVisible(false);
			preview->getWidget(1)->as<gui::Image>()->setImage(img);
			preview->setSize(size + border);
			preview->setName(name);
			return preview;
		}
		return nullptr;
	}
};

}

