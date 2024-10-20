#pragma once

#include <base/drawable.h>

namespace base {

class Camera;
class Landscape;
struct PatchGeometry;

// Terrain class wraps landscape module
// if autoUpdate is false, you must manually call landscape->update and landscape->cull.
class TerrainDrawable : public base::Drawable {
	public:
	TerrainDrawable(Landscape* land, Material* mat=nullptr, bool autoUpdate=true);
	void draw(RenderState&) override;
	Landscape* getLandscape() { return m_land; }
	void setAutoUpdate(bool enable) { m_autoUpdate = enable; } 
	private:
	void patchCreated(PatchGeometry*);
	void patchUpdated(PatchGeometry*);
	void patchDetroyed(PatchGeometry*);
	private:
	Landscape* m_land;
	bool m_autoUpdate;
};

}

