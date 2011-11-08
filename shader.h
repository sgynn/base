#ifndef _SHADER_
#define _SHADER_

#include <map>
#include <vector>
#include <string>
namespace base {

class VertexShader {
	friend class Shader;
	unsigned int m_shader;
	bool m_compiled;
	public:
	VertexShader() : m_shader(0), m_compiled(0) {};
	char* log(char* buffer, int size) const;
};

class FragmentShader {
	friend class Shader;
	unsigned int m_shader;
	bool m_compiled;
	public:
	FragmentShader() : m_shader(0), m_compiled(0) {};
	char* log(char* buffer, int size) const;
};

class GeometryShader {
	friend class Shader;
	unsigned int m_shader;
	bool m_compiled;
	public:
	GeometryShader() : m_shader(0), m_compiled(0) {};
	char* log(char* buffer, int size) const;
};

/** Simple GLSL Shader class */
class Shader {
	friend class VertexShader;
	friend class FragmentShader;
	friend class GeometryShader;
	public:
	
	static const VertexShader& getVertexShader(const char* name);
	static const FragmentShader& getFragmentShader(const char* name);
	static Shader& getShader(const char* name);
	static Shader& link( const char* name, const VertexShader& vertex, const FragmentShader& fragment );
	static Shader& link( const VertexShader& vertex, const FragmentShader& fragment );
	static int supported();
	static Shader& current() { return s_currentShader; }
	static Shader Null;
	
	Shader();
	
	void bind();

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
	
	enum VType { UNIFORM, ATTRIBUTE };
	int variable(const char* name, VType type);
	
	private:
	
	static const char* load(const char* filename);
	static unsigned int compile(const char* shader, int type);
	static int queryShader(unsigned int shader, int param);
	static int queryProgram(unsigned int shader, int param);
	
	//store variable pointers
	std::map<std::string, int> m_vars;
	int m_linked;
	const char* m_name;
	const FragmentShader* m_fragment;
	const VertexShader*   m_vertex;
	unsigned int m_program;
	
	//need to store uniform variable values so they can be edited when the shader is not bound ?
	
	static int s_supported;
	static std::map<std::string, VertexShader> s_vertexShaders;
	static std::map<std::string, FragmentShader> s_fragmentShaders;
	static std::vector<Shader> s_shaders; //the linked shader programs
	static Shader s_currentShader;
	static unsigned int s_bound; //currently bound shader
};
};

#endif
