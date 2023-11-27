#include <base/bmloader.h>
#include <base/hardwarebuffer.h>
#include <base/model.h>
#include <base/xml.h>
#include <cstdio>
#include <cstring>


using namespace base;

namespace base {
	struct BMExtension { const char* key; ModelExtension*(*loader)(const XMLElement&); };
	static std::vector<BMExtension> bmExtensions;
}

void BMLoader::registerExtension(const char* key, ModelExtension*(*loader)(const XMLElement&)) {
	bmExtensions.push_back( BMExtension {key,loader} );
}

// ----------------------------------------------------------------------------------------------------------- //


Model* BMLoader::load(const char* file) {
	XML xml = XML::load(file);
	if(xml.getRoot() != "model") {
		printf("Invalid model file '%s'\n", file);
		return 0;
	}
	return loadModel( xml.getRoot() );
}

Model* BMLoader::parse(const char* data) {
	XML xml = XML::parse(data);
	if(xml.getRoot() != "model") {
		printf("Invalid model data\n");
		return 0;
	}
	return loadModel( xml.getRoot() );
}

// ----------------------------------------------------------------------------------------------------------- //

Model* BMLoader::loadModel(const XMLElement& e) {
	Model* model = new Model();
	float version = e.attribute("version", 1.f);
	if(version<2 && e.find("animation").name()) printf("Warning: Old model version. Animations will be broken\n");

	for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
		if(*i == "mesh") {
			Mesh* mesh = loadMesh(*i);
			if(mesh) {
				const char* material = i->find("material").attribute("name");
				int r = model->addMesh(i->attribute("name"), mesh);
				model->setMaterialName(r, material);
			}
		}

		else if(*i == "skeleton") {
			Skeleton* skeleton = loadSkeleton(*i);
			if(skeleton) model->setSkeleton(skeleton);
		}

		else if(*i == "animation") {
			Animation* a = loadAnimation(*i);
			model->addAnimation(a);
		}
		else if(*i == "layout") {
			ModelLayout* lay = loadLayout(*i);
			model->setLayout(lay);
		}

		for(BMExtension& e: bmExtensions) {
			if(*i == e.key) {
				ModelExtension* ext = e.loader(*i);
				if(ext) model->addExtension(ext);
			}
		}
	}

	return model;
}


// ----------------------------------------------------------------------------------------------------------- //

size_t parseValues(VertexType* list, size_t count, const char* src) {
	char* e = 0;
	for(size_t i=0; i<count; ++i) {
		list[i] = strtof(src, &e);
		if(e==src) return i;
		src = e;
		while(*src==' ' || *src=='\n') ++src;
	}
	return count;
}
size_t parseValues(IndexType* list, size_t count, const char* src) {
	char* e = 0;
	for(size_t i=0; i<count; ++i) {
		list[i] = strtol(src, &e, 10);
		if(e==src) return i;
		src = e;
		while(*src==' ' || *src=='\n') ++src;
	}
	return count;
}

// ----------------------------------------------------------------------------------------------------------- //

Mesh* BMLoader::loadMesh(const XMLElement& e) {
	// Get format
	HardwareVertexBuffer* vdata = new HardwareVertexBuffer();
	std::vector<const XMLElement*> parts;
	size_t offset = 0;
	for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
		AttributeSemantic type;
		AttributeType s = VA_INVALID;

		if(*i=="vertices")       s = VA_FLOAT3, type = VA_VERTEX;
		else if(*i=="normals")   s = VA_FLOAT3, type = VA_NORMAL;
		else if(*i=="texcoords") s = VA_FLOAT2, type = VA_TEXCOORD;
		else if(*i=="tangents")  {
			s = (AttributeType)i->attribute("elements", 3);
			type = VA_TANGENT;
		}
		else if(*i=="colours") {
			type = VA_COLOUR;
			const char* t = i->attribute("type", "rgba");
			if(strcmp(t, "rgb")==0) s = VA_FLOAT3;
			else if(strcmp(t, "rgba")==0) s = VA_FLOAT4;
			else printf("Error: invalid vertex colour format\n");
		}

		// Add to list
		if(s!=VA_INVALID) {
			parts.push_back(&(*i));
			vdata->attributes.add(type, s, offset);
			offset += s * sizeof(float);
		}
	}

	size_t size = offset / sizeof(float);
	size_t count = e.attribute("size", 0);

	Mesh* mesh = new Mesh();
	VertexType* vx = new VertexType[size * count];
	VertexType* tmp = new VertexType[count * 4];
	memset(vx, 0, sizeof(VertexType) * size * count); // Just in case

	// Compile vertex array
	for(uint i=0; i<parts.size(); ++i) {
		size_t offset = vdata->attributes.get(i).offset;
		size_t elements = (size_t)vdata->attributes.get(i).type;
		if(parseValues(tmp, count*elements, parts[i]->text())==count*elements) {
			size_t bytes = elements * sizeof(VertexType);
			VertexType* tvx = vx + (offset / sizeof(VertexType));
			for(size_t j=0; j<count; ++j)
				memcpy(tvx+j*size, tmp+j*elements, bytes);
		}
		else printf("Error: mesh has incorrect number of '%s' values\n", parts[i]->name());
	}
	vdata->setData(vx, count, size*sizeof(float));
	mesh->setVertexBuffer(vdata);


	// Index buffer
	const XMLElement& indices = e.find("polygons");
	if(indices.name()) {
		size_t count = indices.attribute("size", 0) * 3;
		IndexType* ix = new IndexType[count];
		parseValues(ix, count, indices.text());
		HardwareIndexBuffer* ibuffer = new HardwareIndexBuffer();
		ibuffer->setData(ix, count);
		mesh->setIndexBuffer(ibuffer);
	}

	
	// Skins buffer
	const XMLElement& skins = e.find("skin");
	if(skins.size()) {
		parts[0] = parts[1] = 0;
		int k = 0;
		int skinCount = skins.attribute("size", 0);
		int weightsPerVertex = skins.attribute("weightspervertex", 0);
		mesh->initialiseSkinData(skinCount, weightsPerVertex);
		for(XML::iterator i=skins.begin(); i!=skins.end(); ++i) {
			if(*i == "group") mesh->setSkinName(k++, i->attribute("name"));
			else if(*i == "weights") parts[0] = &(*i);
			else if(*i == "indices") parts[1] = &(*i);
		}
		if(!parts[0] || !parts[1]) {
			printf("Error: Skin data missing\n");

		} else {
			// Data (floatN,shortN)
			if(weightsPerVertex>4) { delete [] tmp; tmp = new VertexType[weightsPerVertex * count]; }
			size_t values = count * weightsPerVertex;
			parseValues(tmp, values, parts[0]->text());

			IndexType* wi = new IndexType[ values ];
			parseValues(wi, values, parts[1]->text());

			// Create vertex buffer
			size_t wStride = weightsPerVertex * sizeof(VertexType);
			size_t iStride = weightsPerVertex * sizeof(IndexType);
			size_t stride = wStride + iStride;
			char* buffer = new char[ count * stride ];
			for(size_t i=0; i<count; ++i) {
				memcpy(buffer + i*stride, tmp+i*weightsPerVertex, wStride);
				memcpy(buffer + i*stride + wStride, wi+i*weightsPerVertex, iStride);
			}

			AttributeType wType = (AttributeType) (VA_FLOAT1 + weightsPerVertex - 1);
			AttributeType iType = (AttributeType) (VA_SHORT1 + weightsPerVertex - 1);

			HardwareVertexBuffer* sBuffer = new HardwareVertexBuffer();
			sBuffer->setData(buffer, count, stride);
			sBuffer->attributes.add(VA_SKINWEIGHT, wType);
			sBuffer->attributes.add(VA_SKININDEX, iType, wStride);
			mesh->setSkinBuffer(sBuffer);
			delete [] wi;
		}
	}

	delete [] tmp;
	return mesh;
}
// ----------------------------------------------------------------------------------------------------------- //

Skeleton* BMLoader::loadSkeleton(const XMLElement& e) {
	// Create skeleton
	Skeleton* skeleton = new Skeleton();

	// Parse bones
	for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
		if(*i == "bone") addBone(*i, skeleton, 0);
	}

	skeleton->setRestPose();
	return skeleton;
}

void BMLoader::addBone(const XMLElement& e, Skeleton* skeleton, Bone* parent) {
	float matrix[16];
	const XMLElement& m = e.find("matrix");
	if(!m.name()){
		printf("BM Error: bone has no matrix\n");
		return;	// Error
	}
	parseValues(matrix, 16, m.text());

	Bone* bone = skeleton->addBone(parent, e.attribute("name"), matrix, e.attribute("length", 1.0));
	for(XML::iterator i=e.begin(); i!=e.end(); ++i) {
		if(*i == "bone") addBone(*i, skeleton, bone);
	}
}


// ----------------------------------------------------------------------------------------------------------- //

inline void swap(float& a, float& b) {
	float t = a;
	a = b;
	b = t;
}


Animation* BMLoader::loadAnimation(const XMLElement& e) {
	Animation* anim = new Animation();
	const char* name = e.attribute("name");
	anim->setName(strdup(name));
	anim->setSpeed( e.attribute("rate", 10.0) );
	float tmp[4];

	for(XML::iterator i = e.begin(); i!=e.end(); ++i) {
		if(*i == "keyset") {
			const char* target = i->attribute("target");
			int id = anim->addKeySet(target);
			for(XML::iterator t = i->begin(); t!=i->end(); ++t) {
				if(*t == "rotation") {
					for(XML::iterator k=t->begin(); k!=t->end(); ++k) {
						int frame = k->attribute("frame", 0);
						parseValues(tmp, 4, k->attribute("value"));
						anim->addRotationKey(id, frame, tmp);
					}
				}
				else if(*t == "position") {
					for(XML::iterator k=t->begin(); k!=t->end(); ++k) {
						int frame = k->attribute("frame", 0);
						parseValues(tmp, 3, k->attribute("value"));
						anim->addPositionKey(id, frame, tmp);
					}
				}
				else if(*t == "scale") {
					for(XML::iterator k=t->begin(); k!=t->end(); ++k) {
						int frame = k->attribute("frame", 0);
						parseValues(tmp, 3, k->attribute("value"));
						anim->addScaleKey(id, frame, tmp);
					}
				}
			}
		}
	}
	return anim;
}

// ----------------------------------------------------------------------------------------------------------- //

template<int N> bool isStringInList(const char* str, const char* (&list)[N]) {
	for(int i=0; i<N; ++i) if(strcmp(str, list[i])==0) return true;
	return false;
}

ModelLayout* BMLoader::loadLayout(const XMLElement& e) {
	static const char* reserved[] = { "name", "bone", "position", "orientation", "scale", "mesh", "instance", "light", "shape" };
	ModelLayout* layout = new ModelLayout;
	struct Stack { const XMLElement* element; ModelLayout::Node* parent; };
	std::vector<Stack> stack;
	stack.push_back({&e, &layout->root()});
	for(size_t index=0; index<stack.size(); ++index) {
		const XMLElement& i = *stack[index].element;
		ModelLayout::Node* n = new ModelLayout::Node();
		const char* name = i.attribute("name");
		const char* bone = i.attribute("bone");
		const char* object = 0;
		static const char* nope = 0;

		n->scale.set(1,1,1);
		sscanf(i.attribute("position"), "%f %f %f", &n->position.x, &n->position.y, &n->position.z);
		sscanf(i.attribute("orientation"), "%f %f %f %f", &n->orientation.w, &n->orientation.x, &n->orientation.y, &n->orientation.z);
		sscanf(i.attribute("scale"), "%f %f %f", &n->scale.x, &n->scale.y, &n->scale.z);

		if(const char* o = i.attribute("mesh", nope)) n->object = o, n->type = ModelLayout::MESH;
		else if(const char* o = i.attribute("shape", nope)) n->object = o, n->type = ModelLayout::SHAPE;
		else if(const char* o = i.attribute("instance", nope)) n->object = o, n->type = ModelLayout::INSTANCE;
		else if(const char* o = i.attribute("light", nope)) {
			sscanf(o, "%f %f %f", &n->scale.x, &n->scale.y, &n->scale.z);
			n->type = ModelLayout::LIGHT;
		}
		n->name = name[0]? strdup(name): 0;
		n->bone = bone[0]? strdup(bone): 0;
		n->object = object? strdup(object): 0;
		n->parent = stack[index].parent;
		n->parent->children.push_back(n);

		// Custom properties
		for(auto& prop: i.attributes()) {
			if(!isStringInList(prop.key, reserved)) n->properties[prop.key] = strdup(prop.value);
		}


		for(const XMLElement& child: i) stack.push_back({&child, n});
	}
	return layout;
}

