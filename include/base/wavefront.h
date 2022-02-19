#pragma once

#include <base/math.h>

namespace base {
	class Model;
	class Mesh;

	/** OBJ model format */
	class Wavefront {
		public:
		static Model* load(const char* filename);
		static Model* parse(const char* data);
		static bool   save(Model* model, const char* filename);
		static bool   save(Mesh* model, const char* filename);

		protected:
		static Mesh* buildMesh(int size, Point3* f, vec3* vx, vec2* tx, vec3* nx);
	};

}

