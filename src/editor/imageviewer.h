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
	gui::Widget* openAsset(const char* asset) override {
		const char* ext = strrchr(asset, '.');
		if(ext && (strcmp(ext, ".png")==0 || strcmp(ext, ".dds")==0)) {

			gui::Widget* previewTemplate = getEditor()->getGUI()->getWidget("filepreview");
			if(!previewTemplate) return nullptr;
			gui::Renderer& r = *getEditor()->getGUI()->getRenderer();

			int img = r.getImage(asset);
			if(img < 0) {
				base::Texture* tex = base::Resources::getInstance()->textures.get(asset);
				if(tex) img = r.addImage(asset, tex->width(), tex->height(), tex->unit());
			}
			if(img > 0) {
				Point size = r.getImageSize(img);
				Point border = previewTemplate->getSize() - previewTemplate->getClientRect().size();
				gui::Widget* preview = previewTemplate->clone();
				preview->getWidget(0)->as<gui::Image>()->setImage(img);
				preview->setSize(size + border);
				return preview;
			}
		}
		return nullptr;
	}
};

}

