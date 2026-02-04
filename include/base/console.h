#pragma once

#include <base/math.h>
#include <base/colour.h>
#include <base/hashmap.h>
#include "variable.h"


namespace gui { class Font; class Renderer; }
namespace script {class Function; }

/** A console for executing commands and modifying or listing variables 
 *	Command examples: "exit", "explode 100", "say hello"
 *	List variable:    "system.time", "game.blah.x"
 *	Set variables:    "system.fullscreen = 1"
 *	Vector variables are defined in parentheses "v = (x,y,z)"
 *
 *	Linked variables have fixed types
 *	Objects are also fixed, unless flagged volitile.
 * */
class Console {
	public:
	Console(const gui::Font* font, const Point& size, int history=32);
	~Console();

	void setColours(const Colour& text, const Colour& back);
	void setFont(const gui::Font*, int size=12);

	bool isVisible() const;				/// Is the console visible?
	void setVisible(bool v);			/// Show/hide the console
	void show(); 						/// Show the console 
	void hide(); 						/// Hide the console
	void update();						/// Update - process input
	void draw();						/// Draw console

	/** Execute a string */
	bool execute(const char* s);
	void print(const char* string);

	/** Add a function pointer */
	typedef Delegate<void(const char*)> FunctionDelegate;
	void addFunction(const char* name, const char* description, const FunctionDelegate& func);
	void addFunction(const char* name, script::VariableName&& autocomplete, const char* description, const FunctionDelegate& func);
	void addFunction(const char* name, const char* description, script::Function* func);
	void addFunction(const char* name, script::VariableName&& autocomplete, const char* description, script::Function* func);
	void callFunction(const char* function, const char* data="");
	



	// Variable functions
	script::Variable&         root();
	script::Variable&         get(const char* name);
	template<typename T> void set(const char* name, T value);	// Set/create variable
	template<typename T> bool link(const char* name, T& value, int flags=0);	// Set/Create linked variable
	void unlink(const char* name);								// Unlink a variable
	void erase(const char* name);								// Remove a variable


	Delegate<void(const script::VariableName&)> eventSet; // A variable was set using the console

	private:
	/** Simple fixed size queue for histories */
	template<class T>
	class Queue {
		uint m_capacity;
		uint m_front;
		uint m_size;
		T*   m_data;
		T    m_null;
		public:
		Queue(uint cap)         : m_capacity(cap), m_front(0), m_size(0) { m_data = new T[cap]; }
		~Queue() 				{ delete [] m_data; }
		uint size() const		{ return m_size; }
		void clear() 			{ m_size=0; }
		void push(const T& s)	{ m_data[m_front]=s; ++m_front%=m_capacity; if(m_size<m_capacity) ++m_size; }
		const T& get(uint i)    { return i<m_size? m_data[(m_front-1-i+m_capacity)%m_capacity]: m_null; }
	};

	// Text output
	Queue<script::String> m_out;	// Console output
	Queue<script::String> m_cmd;	// Command history

	// Graphics
	const gui::Font* m_font;	// Font to use
	gui::Renderer* m_renderer;
	Colour m_backColour;
	Colour m_fontColour;
	int    m_fontSize;
	Point  m_size;		// Console size	when open
	int    m_height;	// Current height
	int    m_sliding;	// Slide direction

	// Input
	char   m_buffer[256];	// Current command being entered
	int    m_caret;			// Caret position
	int    m_cix;			// command history index
	bool   m_doubleTab = false;

	// Variables
	script::Variable m_root;
	struct ConsoleFunction {
		script::Function* delegate;
		const char* description;
		script::VariableName autoCompleteSource;
	};
	base::HashMap<ConsoleFunction> m_functions;

	// Line parsing
	enum AutoCompleteState  { NONE=0, PARTIAL=1, FULL=2 };
	static script::Variable  parseValue(const char* s, int& len);
	int                      autoComplete(char*, bool print=false) const;
	static AutoCompleteState autoComplete(char* str, const char* match, int fix, bool&);

	// Preset functions
	void listVariables(const char*);
	void listFunctions(const char*);

	// stdout capture
	int m_pipe[2];

};


//// Inlines ////
inline bool        Console::isVisible() const     { return m_height>0; }
inline void        Console::setVisible(bool v)  { v? show(): hide(); }

inline script::Variable&  Console::root() { return m_root; }
inline script::Variable&  Console::get(const char* name) { return m_root.find(name); }
template<typename T> void Console::set(const char* name, T value) { m_root.set(name, value); }
template<typename T> bool Console::link(const char* name, T& value, int f) {
	return m_root.link(name, value, f);
}
inline void Console::erase(const char* name) { return m_root.erase(name); }


