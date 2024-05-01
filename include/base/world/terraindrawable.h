#pragma once

#include <base/drawable.h>

namespace base {

class Landscape;
struct PatchGeometry;

// Terrain class wraps landscape module
class TerrainDrawable : public base::Drawable {
	public:
	TerrainDrawable(Landscape*);
	void draw(RenderState&) override;
	Landscape* getLandscape() { return m_land; }
	private:
	void patchCreated(PatchGeometry*);
	void patchUpdated(PatchGeometry*);
	void patchDetroyed(PatchGeometry*);
	private:
	Landscape* m_land;
};

}

