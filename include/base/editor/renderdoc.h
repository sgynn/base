#pragma once

#include "editor.h"

namespace editor {
class RenderDoc : public EditorComponent {
	public:
	void initialise() override;
	void update() override;
};
REGISTER_EDITOR_COMPONENT(RenderDoc);
}
