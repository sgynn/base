#pragma once

#include <base/drawablemesh.h>
#include <base/variable.h>
#include <base/colour.h>

namespace base {

// Dynamic skydome with atmospheric scattering shader
class SkyDome : public base::DrawableMesh {
	public:
	SkyDome(float radius, bool stars=false, int resolution=16, int queue=8);
	void setLatitude(float degrees, float axialTilt=23);
	void setTime(float hour, float season=0, bool updateSun=true); // Season: 0-4, 0 being spring exquinox
	void setSunDirection(const vec3&);
	const vec3& getSunDirection() const { return m_sunDirection; }
	Colour getColour(const vec3& ray) const;	// Software get colour for lighting
	script::Variable& getData() { return m_data; }
	void updateSunFromTime();
	private:
	void createMesh(int segments, int slices, float radius);
	script::Variable m_data;
	vec3 m_sunDirection;
	float m_axialTilt;
	float m_latitude;
	float m_time;
	float m_season;
	vec3 m_sconst;
	vec3 m_cconst;
};

}

