#pragma once

namespace base {
	class XMLElement;
	class Model;
	class Mesh;
	class Bone;
	class Skeleton;
	class Animation;
	class ModelExtension;

	class BMLoader {
		public:
		static Model*     load(const char* file);
		static Model*     parse(const char* data);
		static Model*     loadModel(const XMLElement& e);
		static Mesh*      loadMesh(const XMLElement& e);
		static Skeleton*  loadSkeleton(const XMLElement& e);
		static Animation* loadAnimation(const XMLElement& e);

		static void registerExtension(const char* key, ModelExtension*(*)(const XMLElement& e));

		// Shorthand for registering extensions that take XMLElement in constructor
		template<class T>
		static void registerExtension(const char* key) {
			registerExtension(key, [](const XMLElement& e)->ModelExtension*{return new T(e); });
		}

		private:
		BMLoader() {}

		static void addBone(const XMLElement& e, Skeleton* skeleton, Bone* parent);
	};
}


