#ifndef _WAVEFRONT_
#define _WAVEFRONT_

#include <base/math.h>

namespace base {
	class SMaterial;

namespace model {
	class Model;
	class Mesh;

	/** OBJ model format */
	class Wavefront {
		public:
		static Model* load(const char* filename);
		static Model* parse(const char* string);
		static bool   save(Model* model, const char* filename);

		static void setMaterialFunc( SMaterial*(*)(const char* name));

		protected:
		static Mesh* build(int size, Point3* f, vec3* vx, vec2* tx, vec3* nx, const char* mat);
		static SMaterial*(*s_matFunc)(const char*);
	};

}
}


#endif

