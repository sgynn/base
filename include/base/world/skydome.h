#pragma once

#include <base/drawablemesh.h>
#include <base/variable.h>
#include <base/colour.h>

namespace script { class Variable; }

// Dynamic skydome with atmospheric scattering shader
class SkyDome : public base::DrawableMesh {
	public:
	SkyDome(float radius, int resolution=16, int queue=8);
	void setLatitude(float degrees);
	void setTime(float hour);
	void setSunDirection(const vec3&);
	const vec3& getSunDirection() const { return m_sunDirection; }
	Colour getColour(const vec3& ray) const;	// Software get colour for lighting
	script::Variable& getData() { return m_data; }
	void updateSunFromTime();
	private:
	void createMesh(int segments, int slices, float radius);
	script::Variable m_data;
	vec3 m_sunDirection;
	float m_latitude;
	float m_time;
	vec3 m_sconst;
	vec3 m_cconst;
};

