#include <base/editor/gizmo.h>
#include <base/hardwarebuffer.h>
#include <base/autovariables.h>
#include <base/renderer.h>
#include <base/material.h>
#include <base/shader.h>
#include <base/camera.h>
#include <base/opengl.h>
#include <cstdio>

using namespace editor;

MouseRay::MouseRay(base::Camera* cam, int mx, int my, int sw, int sh)
	: start(cam->getPosition())
	, screen(mx, my), camera(cam)
	, m_viewport(sw, sh)
{
	direction = cam->unproject( vec3(mx, my, 1), m_viewport) - start;
	direction.normalise();
}
vec3 MouseRay::project(const vec3& p) const {
	return camera->project(p, m_viewport);
}


// --------------------------------- //


Gizmo::Gizmo() : mScale(1,1,1), mLocalMode(false), mMode(POSITION), mHandle(0), mOffset(0), mHeld(false), mSize(0.2), mCachedScale(1) {
	createGeometry();
}
Gizmo::~Gizmo() {
}

// --------------------------------- //

void Gizmo::setBasis(const Matrix& m) {
	mBasis = m;
}
void Gizmo::setBasis(const Matrix& m, const Quaternion& r) {
	mBasis = m;
	mBaseRot = r;
}
void Gizmo::setScale(const vec3& s) {
	mScale = s;
}
void Gizmo::setPosition(const vec3& p) {
	mPosition = p;
}
void Gizmo::setOrientation(const Quaternion& o) {
	mOrientation = o;
}
const vec3& Gizmo::getScale() const {
	return mScale;
}
const vec3& Gizmo::getPosition() const {
	return mPosition;
}
const Quaternion& Gizmo::getOrientation() const {
	return mOrientation;
}

// ---------------------------------- //

void Gizmo::setMode(Mode m) {
	mMode = m;
}
void Gizmo::setRelative(const Matrix& m) {
	mRelative.fromMatrix(m);
	mLocalMode = false;
}
void Gizmo::setRelative(const Quaternion& q) {
	mRelative = q;
	mLocalMode = false;
}
void Gizmo::setLocalMode() {
	mRelative = Quaternion(mBasis) * mBaseRot * mOrientation;
	mLocalMode = true;
}
bool Gizmo::isLocal() const {
	return mLocalMode;
}

bool Gizmo::isHeld() const {
	return mHeld;
}

// ---------------------------------- //
//          Mouse Control             //
// ---------------------------------- //

bool Gizmo::isOver(const MouseRay& m, float& t) const {
	float offset;
	return pickHandle(m, 6*6, offset, t);
}
bool Gizmo::onMouseDown(const MouseRay& m) {
	float d = 0;
	mHandle = pickHandle(m, 6*6, mOffset, d);
	mHeld = mHandle > 0;
	if(mMode == ORIENTATION && mHeld) {
		mCachedOrientation = mOrientation;
		if(mHandle == 5) mOffset = m.screen.x + m.screen.y * 5000;	// Is this accurate enough?
	}
	return mHeld;
}
bool Gizmo::onMouseMove(const MouseRay& m) {
	float d = 0, o = 0;
	if(mHeld) {
		vec3 axis;
		float scale = mCachedScale; // mSize / sqrt( 1.0 / mPosition.distance2(m.start));
		vec3 position = mBasis * mPosition;
		Quaternion base, local, resolve;
		switch(mMode) {
		case POSITION:
			axis[mHandle-1] = 1;
			o = closestPointOnLine(m, position, position + mRelative * axis);
			o -= mOffset * scale;
			axis = mRelative * axis;
			axis = mBasis.unrotate(axis);
			mPosition = mPosition + axis*o;
			return true;
		case SCALE:
			return false; // Not implemented yet
		case ORIENTATION:
			getSpaceQuaternions(mCachedOrientation, base, local);
			resolve.fromMatrix(mBasis) *= mBaseRot;
			resolve.invert();
			if(mHandle < 4) {
				Quaternion q[3];
				decomposeOrientation(local, q[0], q[1], q[2]);
				if(!overRing(m, position, base * q[mHandle-1], scale, 1e8f, d, o)) return false;
				Quaternion final = resolve * base * q[mHandle-1];
				axis = final.zAxis();
				Quaternion r(axis, o - mOffset);
				mOrientation =  r * mCachedOrientation;
			}
			else if(mHandle == 4) {
				Quaternion q(m.camera->getModelview());
				q.w = -q.w;
				if(!overRing(m, position, q, scale, 1e8f, d, o)) return false;
				axis = (resolve * q).zAxis();
				Quaternion r(axis, o - mOffset);
				mOrientation = r * mCachedOrientation;
			}
			else if(mHandle == 5) {
				// Starting mouse pos encoded in mOffset
				vec2 lp( ((int)mOffset)%5000, ((int)mOffset)/5000);
				Quaternion qy( resolve * m.camera->getLeft(), (m.screen.y - lp.y) * -0.01);
				Quaternion qx( resolve * m.camera->getUp(), (m.screen.x - lp.x) * 0.01);
				mOrientation = qx * qy * mCachedOrientation;
			}
			return true;
		}
	}
	else {
		int h = mHandle;
		mHandle = pickHandle(m, 6*6, mOffset, d);
		return h != mHandle;
	}
	return false;
}
void Gizmo::onMouseUp() {
	mHeld = 0;
}

// ---------------------------------- //
//           Utilities                //
// ---------------------------------- //

void Gizmo::getSpaceQuaternions(const Quaternion& in, Quaternion& base, Quaternion& local) const {
	// Get coordinate space adjustment quaternions
	if(mLocalMode) {
		base = Quaternion(mBasis) * mBaseRot * in;
		local.set(1,0,0,0);
	}
	else {
		Quaternion global = Quaternion(mBasis) * mBaseRot * in;
		local = mRelative.getInverse() * global;
		base = mRelative;
	}
}

int Gizmo::pickHandle(const MouseRay& m, float threshold, float& offset, float& depth) const {
	vec3 position = mBasis * mPosition;
	float scale = mCachedScale; // mSize / sqrt( 1.0 / mPosition.distance2(m.start));
	float d=0, o=0;
	int handle = 0;
	depth = 1e8f;
	offset = 0;

	// Get handle and offset
	if(mMode == ORIENTATION) {
		Quaternion base, local;
		getSpaceQuaternions(mOrientation, base, local);

		Quaternion q[3];
		decomposeOrientation(local, q[0], q[1], q[2]);
		for(int i=0; i<3; ++i) {
			if(overRing(m, position, base * q[i], scale * (1-i*0.05), threshold, d, o) && d < depth) {
				handle = i + 1;
				offset = o;
				depth = d;
			}
		}
		// Final two
		Quaternion s(m.camera->getModelview()); s.w = -s.w;
		for(int i=0; i<2; ++i) {
			if(overRing(m, position, s, scale, threshold, d, o) && d < depth) {
				handle = i + 4;
				offset = o;
				depth = d;
			}
			scale *= 0.1;
		}
		return handle;
	}
	else {
		vec3 spos = m.project(position);
		for(int i=0; i<3; ++i) {
			vec3 axis;
			axis[i] = 1;
			axis = mRelative * axis;
			if(overLine(m.screen, spos, m.project(position+axis*scale), threshold, d, o) && d < depth) {
				handle = i + 1;
				offset = o;
				depth = d;
			}
		}
		// Translate depth
		if(handle) {
			vec3 axis; axis[handle-1] = 1;
			axis = mOrientation * axis;
			depth = m.start.distance(position + axis*offset);
		}

		return handle;
	}
}

void Gizmo::decomposeOrientation(const Quaternion& q, Quaternion& y, Quaternion& p, Quaternion& r) {
	// Calculate yaw, yaw*pitch, yaw*pitch*roll=q
	
	y = Quaternion(0.707107,0.707107,0,0);
	p = Quaternion(.5,.5,.5,.5);

	vec3 d = q.zAxis();
	vec3 u = q.yAxis();
	double yaw = atan2(d.z, d.x) - HALFPI;
	double pitch = -atan2(d.y, sqrt(d.x*d.x + d.z*d.z)) - HALFPI;
	// Flip yaw - FIXME: not if both pitch and roll are > pi/2 (flips back again)
	if(u.y<0) {
		yaw -= PI;
		pitch = TWOPI - pitch;
	}

	y *= Quaternion(vec3(0,0,1), yaw);
	p *= Quaternion(vec3(0,0,1), pitch);

	p = y * p;
	r = q;
}

/// return point on line ab closest to mouse ray, unbounded and unchecked.
float Gizmo::closestPointOnLine(const MouseRay& m, const vec3& a, const vec3& e) {
	vec3 d = e - a;
	float l = d.dot(d);
	float f = m.direction.dot(a - m.start);
	float b = m.direction.dot(d);
	float c = d.dot(a - m.start);
	float denom = l - b*b;
	float s = (b*f - c) / denom;
	return s;
}

bool Gizmo::overLine(const vec2& m, const vec3& a, const vec3& b, float threshold, float& depth, float& offset) {
	// All in screen space
	vec2 d(b.x-a.x, b.y-a.y);
	float t = d.dot(m - a.xy()) / d.dot(d);
	if(threshold < 1e8f) {
		t = t<0? 0: t>1? 1: t;	// clamp;
		// Within range?
		if(m.distance2(a.xy() + d * t) <= threshold) {
			offset = t;
			depth = a.z + (b.z-a.z) * t;	// Note: depth in screen space
			return true;
		}
	}
	else {
		offset = t;
		return true;
	}
	return false;
}

bool Gizmo::overRing(const MouseRay& m, const vec3& centre, const Quaternion& orientation, float radius, float threshold, float& depth, float& offset) {
	// intersect ring plane
	const vec3 normal = orientation.zAxis();
	float dn = normal.dot(m.direction);
	if(dn == 0 && threshold<1e8f) return false;	// Parallel
	float t = (normal.dot(centre) - normal.dot(m.start)) / dn;
	if(t < 0 && threshold<1e8f) return false;	// facing away
	vec3 point = m.start + m.direction * t;

	// Use a distant point if no plane intersection
	if(dn==0 || t<0) {
		point = m.start + m.direction * 1e5f;
		point -= normal * normal.dot(point);
	}


	// Get point on ring
	vec3 v = point - centre;
	v.normalise();
	point = centre + v * radius;

	// screen space distance threshold from ring
	if(threshold < 1e8f) {
		vec3 ss = m.project(point);
		if(m.screen.distance2(ss.xy()) > threshold) return false;
	}

	// Calculate angle
	depth = t;
	const vec3 z = orientation.xAxis();
	vec3 n = z.cross(normal);
	offset = acos(v.dot(z));
	if(v.dot(n)>0) offset = -offset;
	return true;
}


// --------------------------------------- //
//                Drawing                  //
// --------------------------------------- //

using base::HardwareVertexBuffer;
Gizmo::Geometry Gizmo::sGeometry;

void Gizmo::createGeometry() {
	if(sGeometry.arrowData) return;
	int stride = 2*sizeof(float);

	// Arrow
	const float ap[12] = { 0,0,  0,0.9, 0.05,0.9,  0,1,  -0.05,0.9,  0,0.9 };
	sGeometry.arrowData = new HardwareVertexBuffer();
	sGeometry.arrowData->copyData(ap, 6, stride);
	sGeometry.arrowData->attributes.add(base::VA_VERTEX, base::VA_FLOAT2);
	sGeometry.arrowData->createBuffer();
	addBuffer(sGeometry.arrowData);
	sGeometry.arrowBinding = m_binding;
	m_binding = 0;

	// Block
	const float bp[21] = { 0,0,  0,0.9,  0.05,0.9,  0.05,1,  -0.05,1,  -0.05,0.9,  0,0.9 };
	sGeometry.blockData = new HardwareVertexBuffer();
	sGeometry.blockData->copyData(bp, 7, stride);
	sGeometry.blockData->attributes.add(base::VA_VERTEX, base::VA_FLOAT2);
	sGeometry.blockData->createBuffer();
	addBuffer(sGeometry.blockData);
	sGeometry.blockBinding = m_binding;
	m_binding = 0;

	// Ring (with arrow)
	const int segments = 42;
	float step = PI * 2 / segments;
	float* rp = new float[ (segments + 4) * 2 ];
	float* p = rp;
	for(int i=0; i<=segments; ++i) {
		p[0] = sin(i*step);
		p[1] = cos(i*step);
		p += 2;
	}
	const float ra[6] = { 0.06,0.90,  -0.06,0.90,  0,1 };
	memcpy(p, ra, sizeof(ra));
	sGeometry.ringData = new HardwareVertexBuffer();
	sGeometry.ringData->setData(rp, segments+4, stride, true);
	sGeometry.ringData->attributes.add(base::VA_VERTEX, base::VA_FLOAT2);
	sGeometry.ringData->createBuffer();
	addBuffer(sGeometry.ringData);
	sGeometry.ringBinding = m_binding;
	m_binding = 0;
}

base::Material* getGizmoMaterial() {
	static base::Material* mat = 0;
	if(!mat) {
		// Create new default material
		static const char* vs_src =
		"#version 130\n"
		"in vec4 vertex;\n"
		"uniform mat4 transform;\n"
		"void main() { gl_Position=transform * vertex; }\n";
		static const char* fs_src = 
		"#version 130\n"
		"uniform vec4 colour;\n"
		"out vec4 fragment;\n"
		"void main() { fragment = colour; }\n";
		base::ShaderPart* vs = new base::ShaderPart(base::VERTEX_SHADER, vs_src);
		base::ShaderPart* fs = new base::ShaderPart(base::FRAGMENT_SHADER, fs_src);
		base::Shader* shader = new base::Shader();
		shader->attach(vs);
		shader->attach(fs);
		shader->bindAttributeLocation("vertex", 0);

		mat = new base::Material();
		base::Pass* pass = mat->addPass("default");
		pass->setShader(shader);
		pass->getParameters().setAuto("transform", base::AUTO_MODEL_VIEW_PROJECTION_MATRIX);
		pass->getParameters().set("colour", 1,1,1,1);
		pass->state.depthTest = base::DEPTH_ALWAYS;
		pass->blend = base::BLEND_ALPHA;
		pass->compile();
	}
	return mat;
}


inline void makeTransform(const vec3& axis, const vec3& camera, Matrix& out) {
	vec3 x = axis.cross(camera);
	x.normalise();
	vec3 z = x.cross(axis);
	z.normalise();
	out.set(x, axis, z);
}


inline void setColour(float* out, int axis, bool highlight) {
	if(highlight) {
		out[0] = out[1] = out[2] = 0.4;
		out[axis] = 1.0;
	}
	else {
		out[0] = out[1] = out[2] = 0.0;
		out[axis] = 0.7;
	}
}

void Gizmo::draw(base::RenderState& state) {
	const vec3& pos = state.getCamera()->getPosition();
	const vec3& dir = state.getCamera()->getDirection();
	
	if(!m_material) m_material = getGizmoMaterial();
	if(!m_material || !m_material->getPass(0)->getShader() || !m_material->getPass(0)->getShader()->isCompiled()) {
		printf("Gizmo material error\n");
		return;
	}

	state.setMaterial(m_material);
	base::Pass* pass = m_material->getPass(0);
	float* colour = pass->getParameters().getFloatPointer("colour");
	float& alpha = colour[3];
	glLineWidth(1.5);

	// Use non-shader mode for now
	vec3 position = mBasis * mPosition;
	const vec3 cpos = pos - position;
	float scale = mSize * cpos.dot(dir);
	mCachedScale = scale;
	Matrix matrix;

	if(mMode == ORIENTATION) {
		Quaternion base, local;
		getSpaceQuaternions(mOrientation, base, local);

		// Draw PYR discs
		Quaternion q[3];
		decomposeOrientation(local, q[0], q[1], q[2]);

		alpha = 1;
		m_binding = sGeometry.ringBinding;
		int count = sGeometry.ringData->getVertexCount();
		bind();
		for(int i=0; i<3; ++i) {
			setColour(colour, i, i==mHandle-1);
			q[i] = base * q[i];
			q[i].toMatrix(matrix);
			matrix.setTranslation(position);
			matrix.scale(scale * (1.0 - i*0.05));
			
			state.getVariableSource()->setModelMatrix(matrix);
			pass->bindVariables(state.getVariableSource());
			glDrawArrays(GL_LINE_STRIP, 0, count);
		}

		// extra two rings
		vec3 up(0,1,0);
		if(fabs(up.dot(dir)) > 0.8) up.set(1,0,0);
		vec3 x = up.cross(dir);
		x.normalise();
		up = dir.cross(x);
		matrix.set(x,up,dir);
		matrix.setTranslation(position);
		matrix.scale(scale);

		colour[0] = colour[1] = colour[2] = 1.0;
		state.getVariableSource()->setModelMatrix(matrix);
		pass->bindVariables(state.getVariableSource());
		glDrawArrays(GL_LINE_STRIP, 0, count-3);

		matrix.scale(0.1);
		state.getVariableSource()->setModelMatrix(matrix);
		pass->bindVariables(state.getVariableSource(), true);
		glDrawArrays(GL_LINE_STRIP, 0, count-3);
	}
	else {
		vec3 dir = (position - pos).normalise();
		int count = 0;
		if(mMode == POSITION) {
			m_binding = sGeometry.arrowBinding;
			count = sGeometry.arrowData->getVertexCount();
		}
		else {
			m_binding = sGeometry.blockBinding;
			count = sGeometry.blockData->getVertexCount();
		}
		bind();
		// draw three axes
		for(int i=0; i<3; ++i) {
			vec3 axis;
			axis[i] = 1.0;
			axis = mRelative * axis;
			alpha = (1.0 - fabs(axis.dot(dir))) * 80;
			setColour(colour, i, i==mHandle-1);
			makeTransform(axis, dir, matrix);
			matrix.setTranslation(position);
			matrix.scale(scale);

			state.getVariableSource()->setModelMatrix(matrix);
			pass->bindVariables(state.getVariableSource());
			glDrawArrays(GL_LINE_STRIP, 0, count);
		}
	}

	glLineWidth(1);
	m_binding = 0;
}

