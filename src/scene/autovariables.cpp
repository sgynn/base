#include <base/autovariables.h>
#include <base/camera.h>
#include <base/opengl.h>
#include <assert.h>
#include <cstring>
#include <cstdio>

// full matrix invert function in libbase - should make it Matrix::invert(in, out);
extern int glhInvertMatrixf2(const float* in, float* out);

using namespace base;

AutoVariableSource::AutoVariableSource() 
	: m_derivedMask(0), m_skinSize(0), m_skinCapacity(0), m_skinMatrixVector(0), m_skinMatrices(0) 
	, m_near(0), m_far(0), m_time(0), m_frameTime(1.f/60)
{
	setCustom(0);
}
AutoVariableSource::~AutoVariableSource() {
}

const char* AutoVariableSource::getString(int key) const { return getKeyString(key); }
const char* AutoVariableSource::getKeyString(int key) {
	static const char* autos[] = { 
		"time", "frame_time",
		"model_matrix", "view_matrix", "projection_matrix", 
		"modelview_matrix", "viewprojection_matrix", "modelviewprojection_matrix",
		"inverse_model_matrix", "inverse_view_matrix", "inverse_projection_matrix",
		"inverse_modelview_matrix", "inverse_viewprojection_matrix", "inverse_modelviewprojection_matrix",
		"viewport_size", "far_clip", "near_clip",
		"camera_position", "camera_direction",
		"skin_matrices", "custom" };
	if(key>AUTO_CUSTOM) return 0; // max
	return autos[key];
}

int AutoVariableSource::getData(int key, int& e, int& a, const float*& data) {
	#define return_matrix(m) e=16; a=1; data=&m[0]; return 3;
	#define return_float(f) e=1; a=1; data=&f; return 1;

	switch(key) {
	case AUTO_TIME:       return_float(m_time);
	case AUTO_FRAME_TIME: return_float(m_frameTime);
	case AUTO_NEAR_CLIP:  return_float(m_near);
	case AUTO_FAR_CLIP:   return_float(m_far);
	case AUTO_VIEWPORT_SIZE: e=4; a=1; data=m_viewportSize; return 1;

	case AUTO_MODEL_MATRIX:      return_matrix(m_modelMatrix);
	case AUTO_VIEW_MATRIX:       return_matrix(m_viewMatrix);
	case AUTO_PROJECTION_MATRIX: return_matrix(m_projection);

	case AUTO_CAMERA_POSITION:   e=3; a=1; data=m_cameraPosition; return 1;
	case AUTO_CAMERA_DIRECTION:  e=3; a=1; data=m_cameraDirection; return 1;

	// derived matrices
	case AUTO_MODEL_VIEW_MATRIX:
	case AUTO_VIEW_PROJECTION_MATRIX:
	case AUTO_MODEL_VIEW_PROJECTION_MATRIX:
	case AUTO_INVERSE_MODEL_MATRIX:
	case AUTO_INVERSE_VIEW_MATRIX:
	case AUTO_INVERSE_PROJECTION_MATRIX:	
	case AUTO_INVERSE_MODEL_VIEW_MATRIX:
	case AUTO_INVERSE_VIEW_PROJECTION_MATRIX:
	case AUTO_INVERSE_MODEL_VIEW_PROJECTION_MATRIX:
		return_matrix( deriveMatrix(key) );
	
	// skin data
	case AUTO_SKIN_MATRICES: e=16; a=m_skinSize; data=m_skinMatrices[0]; return 3;

	// Custom data
	case AUTO_CUSTOM: e=4; a=1; data=m_custom; return 1;
	default: return 0;
	}
}

Matrix& AutoVariableSource::deriveMatrix(int key) {
	if(~m_derivedMask & (1<<key)) {
		switch(key) {
		case AUTO_MODEL_VIEW_MATRIX:      m_modelViewMatrix = m_viewMatrix * m_modelMatrix; break;
		case AUTO_VIEW_PROJECTION_MATRIX: m_viewProjection = m_projection * m_viewMatrix; break;
		case AUTO_MODEL_VIEW_PROJECTION_MATRIX: m_modelViewProjection = m_projection * m_viewMatrix * m_modelMatrix; break;
		
		case AUTO_INVERSE_MODEL_MATRIX: Matrix::inverseAffine(m_inverseModel, m_modelMatrix); break;
		case AUTO_INVERSE_VIEW_MATRIX:  Matrix::inverseAffine(m_inverseView, m_viewMatrix); break;
		case AUTO_INVERSE_PROJECTION_MATRIX: glhInvertMatrixf2(m_projection, m_inverseProjection); break;

		case AUTO_INVERSE_MODEL_VIEW_MATRIX: Matrix::inverseAffine(m_inverseModelView, deriveMatrix(AUTO_MODEL_VIEW_MATRIX)); break;
		case AUTO_INVERSE_VIEW_PROJECTION_MATRIX:  glhInvertMatrixf2(deriveMatrix(AUTO_VIEW_PROJECTION_MATRIX), m_inverseViewProjection); break;
		case AUTO_INVERSE_MODEL_VIEW_PROJECTION_MATRIX: glhInvertMatrixf2(deriveMatrix(AUTO_MODEL_VIEW_PROJECTION_MATRIX), m_inverseModelViewProjection); break;
		}
		m_derivedMask |= 1<<key;
	}
	switch(key) {
	case AUTO_MODEL_VIEW_MATRIX:                    return m_modelViewMatrix;
	case AUTO_VIEW_PROJECTION_MATRIX:               return m_viewProjection;
	case AUTO_MODEL_VIEW_PROJECTION_MATRIX:         return m_modelViewProjection;
	
	case AUTO_INVERSE_MODEL_MATRIX:                 return m_inverseModel;
	case AUTO_INVERSE_VIEW_MATRIX:                  return m_inverseView;
	case AUTO_INVERSE_PROJECTION_MATRIX:            return m_inverseProjection;

	case AUTO_INVERSE_MODEL_VIEW_MATRIX:            return m_inverseModelView;
	case AUTO_INVERSE_VIEW_PROJECTION_MATRIX:       return m_inverseViewProjection;
	case AUTO_INVERSE_MODEL_VIEW_PROJECTION_MATRIX: return m_inverseModelViewProjection;
	default: assert(false); // Not a derived matrix
		return m_modelMatrix;	// error
	}
}

void AutoVariableSource::setTime(float t, float f) {
	m_time = t;
	m_frameTime = f;
}

void AutoVariableSource::setCamera(const Camera* cam) {
	m_projection = cam->getProjection();
	m_viewMatrix = cam->getModelview();
	m_near = cam->getNear();
	m_far  = cam->getFar();
	m_derivedMask = 0;
	m_cameraDirection = cam->getDirection();
	m_cameraPosition = cam->getPosition();
	// viewport
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	m_viewportSize[0] = viewport[2];
	m_viewportSize[1] = viewport[3];
	m_viewportSize[2] = 1.f / viewport[2];
	m_viewportSize[3] = 1.f / viewport[3];
}
void AutoVariableSource::setModelMatrix(const Matrix& m) {
	m_modelMatrix = m;
	int mask = 1<<AUTO_MODEL_VIEW_MATRIX | 1<<AUTO_MODEL_VIEW_PROJECTION_MATRIX;
	mask |= 1<<AUTO_INVERSE_MODEL_MATRIX | 1<<AUTO_INVERSE_MODEL_VIEW_MATRIX | 1<<AUTO_INVERSE_MODEL_VIEW_PROJECTION_MATRIX;
	m_derivedMask &= ~mask;
}
void AutoVariableSource::setSkinMatrices(int c, const Matrix* m, int* map) {
	m_skinSize = c;
	if(map) {
		if(m_skinCapacity < c) {
			delete [] m_skinMatrixVector;
			m_skinMatrixVector = new Matrix[c];
			m_skinCapacity = c;
		}
		for(int i=0; i<c; ++i) {
			memcpy( &m_skinMatrixVector[i], &m[ map[i] ], sizeof(Matrix));
		}
		m_skinMatrices = m_skinMatrixVector;
	} else {
		m_skinMatrices = m;
	}
}

void AutoVariableSource::setCustom(const float* v) {
	static float empty[4] = {0,0,0,0};
	m_custom = v? v: empty;
}

