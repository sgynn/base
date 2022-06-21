#include <base/console.h>
#include <base/script.h>
#include <base/input.h>
#include <base/game.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <base/gui/renderer.h>
#include <base/gui/font.h>

/*
// Old stdout pipe code - unix only
#include <fcntl.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
void capture() {
	int fd[2];
	pipe(fd);							// Create pipe
	fcntl(fd[0], F_SETFL, O_NONBLOCK);	// Make read nonbocking
	dup2(fd[1], fileno(stdout));		// Redirect stdout to pipe
	int r = read(fd[0], buffer, sizeof(buffer)); // Read from pipe
	close(fd[0]);	// Shutdown pipe
	close(fd[1]);
}
*/

using namespace base;

char outputBuffer[8192];

Console::Console(const gui::Font* font, const Point& s, int h) : m_out(h), m_cmd(12), m_font(font), m_size(s) {
	m_sliding = 0;
	m_buffer[0] = 0;
	m_caret = 0;
	m_height = 0;
	m_cix = -1;

	m_fontColour = white;
	m_backColour.set(0,0,0,0.3);
	setFont(font, 16);

	m_renderer = new gui::Renderer();

	// Capture stdout
	memset(outputBuffer, 0, 8192);
	#ifndef EMSCRIPTEN
	setvbuf(stdout, outputBuffer, _IOFBF, 8192);
	printf("Buffer initialised\n");
	#else
	print("FIXME: printf -> console redirection");
	#endif

	m_root.makeObject();

	addFunction("help", "Show this help", [this](const char*) {
		printf("Commands:\n");
		for(auto& i: m_functions) printf("  %s - %s\n", i.key, i.value.description);
	});

	addFunction("ls", "List data", [this](const char*) { printf("%s\n", m_root.toString(1).str()); });
}

Console::~Console() {
	setbuf(stdout, NULL);
	delete m_renderer;
	for(auto& f: m_functions) delete f.value.delegate;
}

void Console::setFont(const gui::Font* font, int size) {
	m_font = font;
	m_fontSize = size;
}

void Console::addFunction(const char* name, const char* desc, const FunctionDelegate& func) {
	addFunction(name, desc, script::Function::bind({"x"}, [func](const char* s) { func(s); return script::Variable(); }));
}
void Console::addFunction(const char* name, const char* desc, script::Function* func) {
	delete m_functions.get(name, {0,0}).delegate;
	m_functions[name] = { func, desc };
}

void Console::callFunction(const char* function, const char* param) {
	if(m_functions.contains(function)) {
		m_functions[function].delegate->call(param);
	}
}


// Print a single line to the console and elsewhere
void Console::print(const char* s) {
	// Split into lines
	while(const char* e = strchr(s, '\n')) {
		if(s[0]==27) s = strchr(s, 'm')+1;
		m_out.push(script::String(s, e-s));
		s = e+1;
	}
	if(s[0]==27) s = strchr(s, 'm')+1;
	if(s[0]) m_out.push(script::String(s));
}



void Console::update() {
	// Update buffer
	if(outputBuffer[0]) {
		fflush(stdout);
		print(outputBuffer);
		memset(outputBuffer, 0, strlen(outputBuffer));
	}

	// Slide open/closed
	if(m_sliding) {
		m_height += m_sliding * 24;
		if(m_height<0) { m_height=0; m_sliding=0; }
		if(m_height>m_size.y) { m_height=m_size.y; m_sliding=0; }
	}
	// Toggle console
	if(Game::Pressed(KEY_TICK)) {
		if(m_sliding) m_sliding *= -1;
		else setVisible(!m_height);
	}

	// Does the console get input
	if(!m_height || m_sliding<0) return;
	Input* in  = Game::input();
	int    len = strlen(m_buffer);
	const int limit = sizeof(m_buffer);
	switch(in->lastKey()) {
	case KEY_ENTER:				// Enter - Execute command
		if(m_buffer[0]) {
			execute(m_buffer);
			if(m_cmd.size()==0 || strcmp(m_cmd.get(0),m_buffer)) m_cmd.push(m_buffer);
			m_buffer[0] = 0;
			m_caret = 0;
			m_cix = -1;
		}
		break;
	case KEY_BACKSPACE:			// Backspace - delete
		if(m_caret>0) {
			for(int i=--m_caret; i<len; ++i) m_buffer[i] = m_buffer[i+1];
		}
		break;
	case KEY_DEL:
		if(m_caret<len) {
			for(int i=m_caret--; i<len; ++i) m_buffer[i] = m_buffer[i+1];
		}
		break;
	case KEY_HOME:
		m_caret = 0;
		break;
	case KEY_END:
		m_caret=len;
		break;
	case KEY_TAB:	// ToDo: Autocomplete
		m_caret += autoComplete(m_buffer);
		break;
	case KEY_UP:	// History previous
		if(m_cix<(int)m_cmd.size()-1) strcpy(m_buffer, m_cmd.get(++m_cix));
		m_caret = strlen(m_buffer);
		break;
	case KEY_DOWN:	// History Next
		if(m_cix==0) { m_buffer[0] = 0; --m_cix; }
		else if(m_cix>0) strcpy(m_buffer, m_cmd.get(--m_cix)); 
		m_caret = strlen(m_buffer);
		break;
	case KEY_LEFT:
		if(m_caret>0) --m_caret;
		break;
	case KEY_RIGHT:
		if(m_caret<len) ++m_caret;
		break;
	default:
		if(len >= limit-1) return;
		if(in->lastChar()>=32) {
			if(m_caret<=len) for(int i=len; i>=m_caret; --i) m_buffer[i+1]=m_buffer[i];
			m_buffer[m_caret] = in->lastChar();
			++m_caret;
		}
	}
}


void Console::draw() {
	if(m_height<=0) return;

	m_renderer->begin(Game::getSize(), Game::getSize());
	m_renderer->drawRect(Rect(0, 0, m_size.x, m_height), m_backColour.toARGB());
	unsigned colour = m_fontColour.toARGB();
	
	// Input caret
	int step = m_font->getLineHeight(m_fontSize);
	int promptWidth = m_font->getSize("] ", m_fontSize).x;
	int cx = promptWidth + m_font->getSize(m_buffer, m_fontSize, m_caret).x;
	m_renderer->drawRect(Rect(cx, m_height-step+2, 1, step-2), colour);

	// Text
	Point pos(0, m_height - step);
	m_renderer->drawText(pos, m_font, m_fontSize, colour, "] ");
	m_renderer->drawText(Point(promptWidth, pos.y), m_font, m_fontSize, colour, m_buffer);

	// history
	for(size_t i=0; i<m_out.size() && pos.y>0; ++i) {
		pos.y -= step;
		m_renderer->drawText(pos, m_font, m_fontSize, colour, m_out.get(i));
	}
	
	m_renderer->end();
}


using namespace script;

#define isAlpha(c) ((*c>='a' && *c<='z') || (*c>='A' && *c<='Z') || *c=='_')
#define isDigit(c) (*c>='0' && *c<='9')

int Console::autoComplete(char* str, const char* match, int fix, bool& first) {
	if(fix==0 || strncmp(str, match, fix)==0) {
		if(first) {
			strcpy(str, match);
			first = false;
			return strlen(match)-fix;
		}
		int c = fix;
		while(str[c]==match[c]&&str[c]) ++c;
		str[c] = 0;
		return c-fix;
	} 
	return 0;
}

int Console::autoComplete(char* buffer) const {
	if(!buffer[0]) return 0;
	// read back from end of buffer
	char* e = buffer + strlen(buffer);
	char* c = e-1;
	while(c>buffer && (*c=='.' || isAlpha(c) || isDigit(c))) {
		if(*c=='.' && *e==0) e = c;
		--c;
	}
	char t = *e; *e = 0;
	char tmp[128]; 
	strcpy(tmp, t?e+1:c);
	int fix = strlen(tmp);
	bool first = true;
	// Autocomplete functions
	if(t==0) {
		autoComplete(tmp, "list", fix, first); // Builtin function
		for(const auto& i : m_functions) {
			autoComplete(tmp, i.key, fix, first);
		}
	}
	// Autocomplete variable
	const Variable& var = t? m_root.find(c): m_root;
	if(t) c = e+1;
	for(Variable::iterator i=var.begin(); i!=var.end(); ++i) {
		autoComplete(tmp, i->key, fix, first);
	}

	// Finalise
	*e = t;
	int len = strlen(tmp) - fix;
	if(len>0) strcat(buffer, tmp+fix);
	return len;
}


Variable Console::parseValue(const char* s, int& len) {
	if(strcmp(s, "true")==0) return Variable(true);
	if(strcmp(s, "false")==0) return Variable(false);
	if(isAlpha(s)) {	// Variable
		len=0;
		for(const char* c=s; *c; ++c) {
			if(c>s && (*c-1)=='.' && (*c=='.' || isDigit(c))) return Variable();  // Invalid
			if(*c=='.' || isAlpha(c) || isDigit(c)) ++len;
			else break;
		}
		String name(s, len);
		return Variable(name);
	} else if(*s=='-' || isDigit(s)) {	// Number constant
		char* end;
		double d = strtod(s, &end);
		len = end-s;
		for(int i=0; i<len; ++i) if(s[i]=='.') return Variable(d);
		return Variable((int)d);
	} else if(*s=='"') {
		for(len=1; s[len]; ++len) {
			if(s[len]=='"') {
				++len;
				String str(s+1, len-2);
				return Variable(str);
			}
		}
	} else if(*s=='(') { // Vector
		float vec[4];
		int l, i;
		Variable v;
		const char* c = s+1;
		for(i=0; i<4; ++i) {
			while(*c==' ') ++c;
			v = parseValue(c, l);
			if(v.isNumber()) vec[i] = v;
			else return Variable();
			c+=l; while(*c==' ') ++c;
			if(*c==')') break;
			if(*c==',') ++c;
		}
		len = c-s;
		if(i==1) return Variable( vec2(vec) );
		if(i==2) return Variable( vec3(vec) );

	}
	return Variable();
}

bool Console::execute(const char* s) {
	while(*s==' ') ++s; // Leading whitespace

	// Console functions
	const char* e = s;
	while((*e>='0' && *e<='9') || (*e>='A'&&*e<='Z') || (*e>='a'&&*e<='z') || *e=='_') ++e;
	if(e > s) {
		script::String name(s, e-s);
		if(m_functions.contains(name)) {
			while(*e==' ') ++e;
			m_functions[name].delegate->call(e);
			return true;
		}
	}

	// Evaluate expression
	if(script::Expression* expression = script::Script::parseExpression(s, true)) {
		Variable result = expression->evaluate(script::Context(m_root));
		if(expression->isConst()) {
			printf("%s = %s\n", s, result.toString().str());
		}
		else {
			const char* eq = strchr(s, '=');
			printf("Set %.*s to %s\n", (int)(eq-s), s, result.toString().str());
			// FIXME Variable change callbacks
			// result is a copy, and does not have the function
			// either need to parse the variable, or let evaluate() fire the callbacks.
			// Perhaps a parameter: evaluate(context, fireCallbacks) ?
			// or we can fire them regardless? could be slower having to do extra checks though
		}
		delete expression;
		return true;
	}
	else {
		printf("Invalid expression\n");
		return false;
	}
}


