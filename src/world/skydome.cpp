#include <base/world/skydome.h>
#include <base/variable.h>
#include <base/drawablemesh.h>
#include <base/material.h>
#include <base/mesh.h>
#include <base/hardwarebuffer.h>
#include <base/resources.h>
#include <base/shader.h>
#include <base/autovariables.h>
#include <cstdio>

using namespace base;


static const char* SkyDomeShaderSrc = R"(#version 330
#pragma vertex_shader

uniform mat4 transform;		// ViewProjection matrix
uniform vec3 camera;		// camera position
uniform vec3 sun;			// Sun position
uniform vec3 invWave; 		// 1 / pow(wavelength, 4) for rgb
uniform float radius; 		// Planet radius
uniform float thickness; 	// Atmosphere depth
uniform float brightness;	// Sun brightness
uniform float rayleigh;		// Rayleigh scattering constant
uniform float mie;			// Mie scattering constant
uniform float scaleDepth;	// Altitude of average atmosphere density [0-1]

in vec3 vertex;
out vec3 direction;
out vec3 colour1;
out vec3 colour2;
out float alpha;

const float fourPI = 12.56637061;
const int samples = 2;
const float samplesf = 2.0;

float scale(float v) {
	float x = 1.0 - v;
	return scaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

void main() {
	// Get direction and atmosphere intersection
	float outer = radius+thickness;
	vec3  ray = normalize(vertex.xyz);
	ray.y = max(0.0, ray.y);
	float b = radius * ray.y;
	float c = radius*radius - outer*outer;
	float far = -b + sqrt(b*b-c);
	vec3  start = vec3(0.0, radius, 0.0);
	float startAngle = dot(ray, start) / length(start);
	float offset = scale(ray.y); //scale(startAngle);

	// Initialise scattering loop
	float sampleLength = far / samplesf;
	float scaledLength = sampleLength / thickness;
	vec3  sampleRay    = ray * sampleLength;
	vec3  samplePoint  = start + sampleRay * 0.5;
	vec3  scatterConstant = invWave * (rayleigh * fourPI) + (mie * fourPI);
	float ssd = 1.0 / (thickness*scaleDepth);

	float alpha = 0.0;
	vec3 colour = vec3(0.0, 0.0, 0.0);
	for(int i=0; i<samples; ++i) {
		float height = length(samplePoint);
		float depth  = exp(ssd * (radius - height));
		float lightAngle = dot(sun, samplePoint) / height;
		float cameraAngle = dot(ray, samplePoint) / height;
		float scatter = offset + depth * (scale(lightAngle) - scale(cameraAngle));
		vec3 attenuate = exp(-scatter * scatterConstant);
		colour += attenuate * (depth * scaledLength);
		alpha += depth * scaledLength;
		samplePoint += sampleRay;
	}

	gl_Position = transform * vec4(vertex + camera, 1.0);
	colour2 = colour * mie * brightness;
	colour1 = colour * (invWave * (rayleigh * brightness));
	alpha = alpha * rayleigh * 100.0;
	direction = -ray;
}


#pragma fragment_shader

uniform vec3 sun;			// Sun position
uniform float asymmetry;	// asymmetry
uniform float scaleDepth;	// Altitude of average atmosphere density [0-1]

in vec3 direction;		// camera ray
in vec3 colour1;
in vec3 colour2;
in float alpha;
out vec4 fragment;

float dither(int x, int y, float value) {
	float key[16] = float[]( 0.0,8.0,2.0,10.0, 12.0,4.0,14.0,6.0, 3.0,11.0,1.0,9.0, 15.0,7.0,13.0,5.0 );
	float limit = (key[x+y*4] + 1.0) / 16.0;
	return value<limit? 1.0: 0.0;
}

void main() {
	float g2 = asymmetry * asymmetry;
	float c = dot(sun, direction) / length(direction);
	float miePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + c*c) / pow(1.0 + g2 - 2.0*asymmetry*c, 1.5);
	fragment.rgb = colour1 + miePhase * colour2;
	
	// Try dithering to reduce banding
	int x = int(mod(gl_FragCoord.x,4.0));
	int y = int(mod(gl_FragCoord.y,4.0));
	fragment.r += dither(x,y,fragment.r) * 0.004 - 0.002;
	fragment.g += dither(x,y,fragment.g) * 0.004 - 0.002;
	fragment.b += dither(x,y,fragment.b) * 0.004 - 0.002;

	fragment.a = alpha + dot(fragment.rgb, vec3(1.0,2.0,1.0));
}
)";


SkyDome::SkyDome(float radius, int res, int queue) {
	setRenderQueue(queue);

	vec3 wave(0.65, 0.57, 0.475);
	vec3 invWave = 1.f / pow(wave, 4);
	float brightness = 30;
	float rayleigh = 0.0035;
	float mie = 0.001;

	m_latitude = 60;
	m_time = 10;
	m_data.link("latitude", m_latitude);
	m_data.link("time", m_time);
	
	createMesh(res*2, res, radius);

	Material* skyMat = Resources::getInstance()->materials.getIfExists("SkyDome");
	if(!skyMat) {
		skyMat = new Material;
		Pass* pass = skyMat->addPass();
		pass->setShader(Shader::create(SkyDomeShaderSrc, SkyDomeShaderSrc));

		ShaderVars& params = pass->getParameters();
		params.setAuto("transform", AUTO_VIEW_PROJECTION_MATRIX);
		params.setAuto("camera", AUTO_CAMERA_POSITION);
		params.set("sun",        0.577, 0.577, 0.577);
		params.set("invWave",    5.96, 9.47, 19.64); // 1 / pow(wavelength.rgb, 4);
		params.set("radius",     100000.f);
		params.set("thickness",  2000.f);
		params.set("scaleDepth", 0.25f);
		params.set("brightness", 30.f);
		params.set("asymmetry", -0.99f);
		params.set("rayleigh",   0.0035f);
		params.set("mie",        0.001f);

		pass->compile();
		Resources::getInstance()->materials.add("SkyDome", skyMat);
	}
	setMaterial(skyMat);

	// Link variables
	ShaderVars& params = skyMat->getPass(0)->getParameters();
	m_data.link("wave", *(vec3*)params.getFloatPointer("invWave"));
	m_data.link("radius", *params.getFloatPointer("radius"));
	m_data.link("thickness", *params.getFloatPointer("thickness"));
	m_data.link("scaleDepth", *params.getFloatPointer("scaleDepth"));
	m_data.link("brightness", *params.getFloatPointer("brightness"));
	m_data.link("asymmetry", *params.getFloatPointer("asymmetry"));
	m_data.link("rayleigh", *params.getFloatPointer("rayleigh"));
	m_data.link("mie", *params.getFloatPointer("mie"));

	// Save constants for getColour()
	m_sconst = invWave * (rayleigh*4*PI) + (mie*4*PI);
	m_cconst = invWave * (rayleigh * brightness);
}

void SkyDome::setSunDirection(const vec3& dir) {
	m_sunDirection = dir.normalised();

	ShaderVars& params = getMaterial()->getPass(0)->getParameters();
	params.set("sun", m_sunDirection);
}

void SkyDome::setLatitude(float degrees) {
	m_latitude = degrees;
}
void SkyDome::setTime(float hour) {
	m_time = hour;
}

void SkyDome::updateSunFromTime() {
	// Calculate sun direction from latitude and time
	static const float toRad = PI/180;
	static const vec3 left(-1,0,0);
	vec3 up(0, sin(m_latitude*toRad), cos(m_latitude*toRad));
	float angle = m_time/24;
	angle -= floor(angle) + 0.5;
	angle *= TWOPI;
	setSunDirection(up * cos(angle) + left * sin(angle));	
}

// ------------------------------------------------------------------------------------- //

void SkyDome::createMesh(int seg, int div, float rad) {
	int s = 3;
	int s1 = seg+1, d1=div+1;
	// build ring
	vec2* ring = new vec2[seg];
	float a = TWOPI / seg;
	for(int i=0; i<seg; ++i) ring[i] = vec2(sin(i*a), cos(i*a));
	// Build vertices
	float* vertices = new float[s * s1 * d1];
	float b = PI/div;
	float* v;
	for(int i=0; i<d1; ++i) {
		float r = sin(i*b);
		float y = cos(i*b);
		for(int j=0; j<seg; ++j) {
			v = vertices + (j+s1*i)*s;
			v[0] = ring[j].x * r * rad;
			v[1] = y*rad;
			v[2] = ring[j].y * r * rad;
		}
		v = vertices+(seg + s1*i)*s;
		memcpy(v, vertices + s1*i*s, s*sizeof(float));
	}
	delete [] ring;

	// Build indices
	int size = 0;
	uint16* indices = new uint16[s1 * div * 2];
	for(int i=0; i<div; ++i) for(int j=0; j<s1; ++j) {
		indices[size] = i*s1 + j + s1;
		indices[size+1] = i*s1 + j;
		size += 2;
	}

	// Create mesh
	m_mesh = new Mesh();
	m_mesh->setPolygonMode(PolygonMode::TRIANGLE_STRIP);

	HardwareVertexBuffer* vertexBuffer = new HardwareVertexBuffer();
	vertexBuffer->attributes.add(VA_VERTEX, VA_FLOAT3);
	vertexBuffer->setData(vertices, s1*d1, s*sizeof(float), true);
	vertexBuffer->createBuffer();
	m_mesh->setVertexBuffer(vertexBuffer);

	HardwareIndexBuffer* indexBuffer = new HardwareIndexBuffer(IndexSize::I16);
	indexBuffer->setData(indices, size, true);
	indexBuffer->createBuffer();
	m_mesh->setIndexBuffer(indexBuffer);

	setMesh(m_mesh);

	m_bounds = BoundingBox(-rad,-rad,-rad, rad, rad, rad);
}

// ------------------------------------------------------------------------------------- //

inline float sscale(float v) {
	float x = 1.0 - v;
	return 0.25 * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

Colour SkyDome::getColour(const vec3& sray) const {
	const int samples = 2;
	float radius     = m_data.get("radius");
	float thickness  = m_data.get("thickness");
	float scaleDepth = m_data.get("scaleDepth");
	float outer      = radius + thickness;
	vec3  ray        = sray.normalised();
	if(ray.y<0) ray.y=0;
	float b          = radius * ray.y;
	float c          = radius*radius - outer*outer;
	float far        = -b + sqrt(b*b-c);
	float offset     = sscale(ray.y);

	float sampleLength = far / samples;
	float scaledLength = sampleLength / thickness;
	float ssd          = 1.0 / (thickness*scaleDepth);
	vec3  sun          = m_sunDirection;
	vec3  rayStep      = ray * sampleLength;
	vec3  point        = vec3(0,radius,0) + rayStep * 0.5;
	vec3  colour;
	for(int i=0; i<samples; ++i) {
		float h = point.length();
		float d = exp(ssd * (radius-h));
		float la = sun.dot(point)/h;
		float ca = ray.dot(point)/h;
		float scatter = offset + d * (sscale(la)-sscale(ca));
		vec3 attenuate( exp(m_sconst.x*-scatter), exp(m_sconst.y*-scatter), exp(m_sconst.z*-scatter));
		colour += attenuate * (d*scaledLength);
		point += rayStep;
	}
	// ...
	float brightness = m_data.get("brightness");
	float mie = m_data.get("mie");
	float g   = m_data.get("asymmetry");
	float g2  = g*g;
	c = sun.dot(ray)*-1;
	float phase = 1.5 * ((1-g2) / (2+g2)) * (1+c*c) / pow(1+g2-2*g*c, 1.5);
	colour = colour * m_cconst;
	colour += colour*(mie*brightness*phase);
	return Colour(fmin(1,colour.x), fmin(1,colour.y), fmin(1,colour.z));
}


