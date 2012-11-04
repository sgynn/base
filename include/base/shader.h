#ifndef _BASE_SHADER_
#define _BASE_SHADER_

#include <map>
#include <vector>
#include <string>
namespace base {

class ShaderBase {
	protected:
	friend class Shader;
	unsigned int m_shader;
	bool m_compiled;
	int m_type;
	ShaderBase(int type): m_shader(0), m_compiled(0), m_type(type) {}
	public:
	char* log(char* buffer, int size) const;
};

class VertexShader : public ShaderBase {
	public:
	VertexShader() : ShaderBase(1) {};
};

class FragmentShader : public ShaderBase {
	public:
	FragmentShader() : ShaderBase(2) {};
};

class GeometryShader : public ShaderBase {
	public:
	GeometryShader() : ShaderBase(3) {};
};

/** Simple GLSL Shader class */
class Shader {
	friend class ShaderBase;
	public:
	
	/** Load and compile a GLSL shader from file. Must include both vertex and fragment programs */
	static Shader load(const char* filename);
	/** Load and compile a GLSL shader from seperate files for each part */
	static Shader load(const char* vertFile, const char* fragFile);
	/** Load and compile a GLSL shader from seperate files for each part */
	static Shader load(const char* vertFile, const char* geomFile, const char* fragFile);
	
	/** Link a shader containing specified pre-made parts */
	static Shader link(const VertexShader&, const FragmentShader&); 
	/** Link a shader containing specified pre-made parts */
	static Shader link(const VertexShader&, const GeometryShader&, const FragmentShader&);

	/** Load a vertex shader from file */
	static VertexShader   loadVertex(const char* filename);
	/** Load a fragment shader from file */
	static FragmentShader loadFragment(const char* filename);
	/** Load a geometry shader from file */
	static GeometryShader loadGeometry(const char* filename);

	/** Create a vertex shader from a string array */
	static VertexShader   createVertex(const char** code, int c=1);
	/** Create a fragment shader from a string array */
	static FragmentShader createFragment(const char** code, int c=1);
	/** Create a geometry shader from a string array */
	static GeometryShader createGeometry(const char** code, int c=1);

	// Backwards compatibiliy
	static Shader getShader(const char* f) { return load(f); }


	/** Does this computer support shaders? */
	static int supported();
	/** Get the currently bound shader. */
	static Shader& current() { return s_currentShader; }
	/** A null shader constant */
	static const Shader Null;
	
	/** Default constructor - creates null shader */
	Shader();
	
	/** Bind the shader program. Bind null shader to unbind */
	void bind() const;

	/** Is the shader ready to be bound? */
	bool ready() const { return m_program; }
	
	/** Get the linker log for this shader */
	char* log(char* buffer, int size) const;

	void Texture(const char* name, int index) { Uniform1i(name, index); }
	
	void Uniform1i(const char* name, int v1);
	void Uniform2i(const char* name, int v1, int v2);
	void Uniform3i(const char* name, int v1, int v2, int v3);
	void Uniform4i(const char* name, int v1, int v2, int v3, int v4);
	void Uniformiv(const char* name, int size, const int *v);
	
	void Uniform1f(const char* name, float v1);
	void Uniform2f(const char* name, float v1, float v2);
	void Uniform3f(const char* name, float v1, float v2, float v3);
	void Uniform4f(const char* name, float v1, float v2, float v3, float v4);
	void Uniformfv(const char* name, int size, const float *v);
	
	//There are a hell of a lot more where these came from: short1-4, double1-4, many others types in array form 1-4
	//If ou want them, #include glext.h, and use Shader.variable(name, type) to get the variable's index.
	void Attribute1f(const char* name, float v1);
	void Attribute2f(const char* name, float v1, float v2);
	void Attribute3f(const char* name, float v1, float v2, float v3);
	void Attribute4f(const char* name, float v1, float v2, float v3, float v4);
	
	void EnableAttributeArray(const char* name);
	void DisableAttributeArray(const char* name);
	void AttributePointer(const char *name, int size, int type, bool normalised, size_t stride, const void *pointer);
	
	private:
	enum VType { UNIFORM, ATTRIBUTE };
	int variable(const char* name, VType type);
	
	static const char* loadFile(const char* filename);
	static bool compile(ShaderBase& shader, const char** code, int l);
	static int queryShader(unsigned int shader, int param);
	static int queryProgram(unsigned int shader, int param);
	
	//store variable pointers
	std::map<std::string, int> m_vars;
	int m_linked;
	FragmentShader m_fragment;
	GeometryShader m_geometry;
	VertexShader   m_vertex;
	unsigned int m_program;
	
	//need to store uniform variable values so they can be edited when the shader is not bound ?
	//that is a job for a material class though
	
	static int s_supported;
	static Shader s_currentShader;
	static unsigned int s_bound; //currently bound shader
};
};

#endif

