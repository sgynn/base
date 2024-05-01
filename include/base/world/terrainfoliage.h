#pragma once

#include "foliage.h"

namespace base {

class TerrainDrawable;

// Foliage system stuff
class TerrainFoliage : public FoliageSystem {
	public:
	TerrainFoliage(TerrainDrawable* zone, int threads);
	void resolvePosition(const vec3& point, vec3& position, float& height) const override;
	void resolveNormal(const vec3& point, vec3& normal) const override;
	int getActive(const vec3& p, float cs, float range, IndexList& out) const override;
	private:
	TerrainDrawable* m_terrain;
};



}

