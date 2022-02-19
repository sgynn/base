#pragma once

#include <base/math.h>
#include <base/matrix.h>

namespace base {
	class Camera;

	/** Automatic shader variables.*/
	enum AutoVariable {
		AUTO_TIME,							// float
		AUTO_FRAME_TIME,					// float
		AUTO_MODEL_MATRIX,					// mat4 
		AUTO_VIEW_MATRIX,					// mat4
		AUTO_PROJECTION_MATRIX,				// mat4

		AUTO_MODEL_VIEW_MATRIX,
		AUTO_VIEW_PROJECTION_MATRIX,
		AUTO_MODEL_VIEW_PROJECTION_MATRIX,

		AUTO_INVERSE_MODEL_MATRIX,			// mat4
		AUTO_INVERSE_VIEW_MATRIX,			// mat4
		AUTO_INVERSE_PROJECTION_MATRIX,		// mat4

		AUTO_INVERSE_MODEL_VIEW_MATRIX,
		AUTO_INVERSE_VIEW_PROJECTION_MATRIX,
		AUTO_INVERSE_MODEL_VIEW_PROJECTION_MATRIX,

		AUTO_VIEWPORT_SIZE,					// vec4( width, height, 1/width, 1/height )
		AUTO_FAR_CLIP,						// float
		AUTO_NEAR_CLIP,						// float

		AUTO_CAMERA_POSITION,				// vec3
		AUTO_CAMERA_DIRECTION,				// vec3

		AUTO_SKIN_MATRICES,					// mat3x4[]

		AUTO_CUSTOM,						// Custom vec4 - somes from drawable data
	};

	
	/** Source for auto parameters - should be extensible */
	class AutoVariableSource {
		public:
		AutoVariableSource();
		virtual ~AutoVariableSource();
		virtual int getData(int key, int& elements, int& array, const float*& data);
		virtual void setTime(float time, float frameTime=1.f/60);
		virtual void setCamera(const base::Camera*);
		virtual void setModelMatrix(const Matrix&);
		virtual void setSkinMatrices(int count, const Matrix* data, int* map=0);
		virtual void setCustom(const float*);
		virtual const char* getString(int key) const; // Lower case, no prefix.
		static const char* getKeyString(int key);

		protected:
		Matrix m_projection;
		Matrix m_modelMatrix;
		Matrix m_viewMatrix;

		// derived matrices
		Matrix& deriveMatrix(int key);
		Matrix m_modelViewMatrix;
		Matrix m_modelViewProjection;
		Matrix m_viewProjection;
		Matrix m_inverseView;
		Matrix m_inverseModel;
		Matrix m_inverseProjection;
		Matrix m_inverseModelView;
		Matrix m_inverseViewProjection;
		Matrix m_inverseModelViewProjection;
		int m_derivedMask;

		// skin
		int m_skinSize;
		int m_skinCapacity;
		Matrix* m_skinMatrixVector;
		const Matrix* m_skinMatrices;
		

		// Other stuff
		float m_near, m_far;
		float m_time;
		float m_frameTime;
		float m_viewportSize[4];
		vec3  m_cameraPosition;;
		vec3  m_cameraDirection;;
		const float* m_custom;
	};
}

