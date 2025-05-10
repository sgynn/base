#pragma once

#include <vector>
#include <string>

namespace base {

enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER, GEOMETRY_SHADER, TESSCONTROL_SHADER, TESSELATION_SHADER };
class ShaderPart {
	friend class Shader;
	public:
	ShaderPart(ShaderType type, const char* source=0, const char* defines=0);
	ShaderPart(const ShaderPart&);
	ShaderPart(ShaderPart&&);
	~ShaderPart();
	ShaderPart& operator=(ShaderPart&);
	int  getLog(char* buffer, int size) const;		// Get shader compile log
	void setSource(const char*);					// Set shader source
	void define(const char* def);					// Set a preprocessor define
	void undef(const char* def);					// Unset a preprocessor define
	bool isDefined(const char* def) const;			// Is a preprocessor defined
	void setDefines(const char* def);				// Set all proprocessor defines separated by commas
	bool compile();									// Compile shader

	ShaderType  getType() const { return m_type; }
	const char* getSource() const { return m_source; }
	protected:
	bool preprocess(const char* code, ShaderType type, int& version, char*& output);
	int  getDefineIndex(const char*) const;
	void dropSource();

	protected:
	ShaderType  m_type;
	unsigned    m_object;
	int         m_compiled;
	bool        m_changed;
	char*       m_source;
	int*        m_sourceRef;
	std::vector<char*> m_defines;
};

enum AttributeMode { SA_FLOAT, SA_NORMALISED, SA_INTEGER, SA_DOUBLE };

/** Simple GLSL Shader class */
class Shader {
	public:

	/** Get supported shader version */
	static int getSupportedVersion();

	/** Currently bound shader */
	static const Shader& current() { return *s_currentShader; }
	static void notifyUnbound() { s_currentShader = &Null; };
	static const Shader Null;

	// Quick shader creation
	static Shader* create(const char* vertexShaderSrc, const char* fragmentShaderSrc, const char* defines=nullptr);

	public:
	Shader();
	~Shader();
	Shader(Shader&& s);
	Shader& operator=(const Shader&) = default;

	Shader* clone() const;	// Create a clone of this shader, with cloned program objects

	void attach(ShaderPart* s);
	void setEntryPoint(ShaderType stage, const char* entry);
	bool isCompiled() const;	// is shader compiled
	bool needsRecompile() const;// does this shader need recompiling
	bool compile();				// compile shader
	void bind() const;			// bind shader

	void setDefines(const char* defines);		// Set defines on all shader objeccts
	void define(const char* define);			// Add a define to all shader obects

	int getLog(char* buffer, int size) const;	// Get compile log

	// Attributes
	void bindAttributeLocation(const char* name, int index);
	int  getAttributeLocation(const char* name) const;
	void setAttributePointer(int loc, int size, int type, size_t stride, AttributeMode mode, const void *pointer) const;

	static void enableAttributeArray(int loc);
	static void disableAttributeArray(int loc);

	// Outputs
	void bindOutput(const char* name, int index);

	// Uniforms
	int getUniformLocation(const char* name) const;
	void setUniform1(int loc, int count, const int* values) const;
	void setUniform2(int loc, int count, const int* values) const;
	void setUniform3(int loc, int count, const int* values) const;
	void setUniform4(int loc, int count, const int* values) const;

	void setUniform1(int loc, int count, const float* values) const;
	void setUniform2(int loc, int count, const float* values) const;
	void setUniform3(int loc, int count, const float* values) const;
	void setUniform4(int loc, int count, const float* values) const;

	void setMatrix2(int loc, int count, const float* matrices) const;
	void setMatrix3(int loc, int count, const float* matrices) const;
	void setMatrix4(int loc, int count, const float* matrices) const;
	void setMatrix3x4(int loc, int count, const float* matrices) const;

	struct UniformInfo { char name[32]; int type; int size; };
	typedef std::vector<UniformInfo> UniformList;
	UniformList getUniforms() const;

	const std::vector<ShaderPart*> getParts() const { return m_shaders; }

	private:
	std::vector<ShaderPart*> m_shaders;
	char*    m_entry[5];
	unsigned m_object;
	int      m_linked;
	bool     m_changed;

	
	static int           s_supported;
	static const Shader* s_currentShader;
};
}

