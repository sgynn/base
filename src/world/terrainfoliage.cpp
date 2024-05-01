#include <base/world/terrainfoliage.h>
#include <base/world/terraindrawable.h>
#include <base/world/landscape.h>

using namespace base;

TerrainFoliage::TerrainFoliage(TerrainDrawable* z, int threads) : FoliageSystem(threads), m_terrain(z) {
}

void TerrainFoliage::resolvePosition(const vec3& point, vec3& pos, float& height) const {
	vec3 offset = &m_terrain->getTransform()[12]; // Use full transform ?
	height = m_terrain->getLandscape()->getHeight(point-offset, true);
	pos.set(point.x, height, point.z);
}

void TerrainFoliage::resolveNormal(const vec3& point, vec3& normal) const {
	// This should be in Landscape::getNormal(point);
	// The query vectors should be the higest resolution
	// Needs to use source height rather than geometry height as that changes with lod
	vec3 local = point - vec3(&m_terrain->getTransform()[12]); // Use full transform ?
	vec3 a(1,0,0);
	vec3 b(0,0,1);
	const Landscape& land = *m_terrain->getLandscape();
	a.y = land.getHeight(local+a, true) - land.getHeight(local-a, true);
	b.y = land.getHeight(local+b, true) - land.getHeight(local-b, true);
	a.x *= 2;
	b.z *= 2;
	normal = b.cross(a).normalised();
}

int TerrainFoliage::getActive(const vec3& p, float cellSize, float rangeLimit, IndexList& out) const {
	Point a( floor((p.x-rangeLimit)/cellSize), floor((p.z-rangeLimit)/cellSize) );
	Point b( ceil((p.x+rangeLimit)/cellSize), ceil((p.z+rangeLimit)/cellSize) );
	int max = ceil(m_terrain->getLandscape()->getSize() / cellSize);
	a.x = std::max(a.x, 0);
	a.y = std::max(a.y, 0);
	b.x = std::min(b.x, max);
	b.y = std::min(b.y, max);
	for(Point p : range(a, b+1)) out.push_back(p);
	return out.size();
}

