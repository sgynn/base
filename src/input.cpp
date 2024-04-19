#include "base/input.h"
#include <cstring> 

#include "base/game.h" //Game included to get window position for mouse data
#include "base/window.h"
#include "base/inifile.h"

#include <cstdio>

using namespace base;

int   Input::s_map[256];
char Input::s_ascii[2][128]; 

Input::Input() : mouse(m_mouseState) {
	clear();
	createMap();

	Point mousePos = queryMouse();
	m_mouseState.x = mousePos.x;
	m_mouseState.y = mousePos.y;
}

Input::~Input() {
}

static Point addDelta;

void Input::update() {
	//clear frame data
	memset(m_keyChange, 0, 128);
	m_lastKey = 0;
	m_lastChar = 0;
	// Mouse
	Point mousePos = queryMouse();
	m_mouseState.delta = mousePos - m_mouseState + m_mouseDelta;
	m_mouseState.x = mousePos.x;
	m_mouseState.y = mousePos.y;
	m_mouseState.pressed = 0;
	m_mouseState.released = 0;
	m_mouseState.wheel = 0;
	m_mouseDelta.set(0,0);
	// Joysticks
	for(AxisBinding& b: m_axisBinding) b.last = checkJoystickThreshold(b);
	updateJoysticks();
}

Point Input::queryMouse() { 
	Point p = Game::window()->queryMouse();
	p.y = Game::window()->getSize().y - p.y;
	return p;
}
void Input::warpMouse(int x, int y) {
	m_mouseDelta += queryMouse() - m_mouseState;
	if(Game::window()->warpMouse(x, Game::window()->getSize().y - y)) {
		m_mouseState.x = x;
		m_mouseState.y = y;
	}
	queryMouse();
}

void Input::setButton(int code, bool down, const Point& pt) {
	int mask = 1 << (code - 1);
	if(down) {
		m_mouseState.pressed |= mask;
		m_mouseState.button |= mask;
	}
	else {
		m_mouseState.released |= mask;
		m_mouseState.button &= ~mask;
	}
}

//extern void setTestKey(int code);
void Input::setKey(int code, bool down) {
	int key = s_map[code];
	m_key[key] = down; 
	m_keyChange[key] |= down? 1: 2;
	
	int maskKey = 0;
	if(key == KEY_LCONTROL || key==KEY_RCONTROL) maskKey = MODIFIER_CTRL;
	else if(key == KEY_LSHIFT || key==KEY_RSHIFT) maskKey = MODIFIER_SHIFT;
	else if(key == KEY_ALT) maskKey = MODIFIER_ALT;
	else if(key == KEY_LMETA || key == KEY_RMETA) maskKey = MODIFIER_META;

	if(down) {
		m_keyMask |= maskKey;
		m_lastKey = key;
		m_lastChar = s_ascii[ m_keyMask & MODIFIER_SHIFT? 1: 0 ][ m_lastKey ];
	}
	else m_keyMask &= ~maskKey;
}


void Input::createMap() {
	//create mapping from KeyCode to ASCII characters
	strcpy(s_ascii[0], ".abcdefghijklmnopqrstuvwxyz01234567890123456789.............`-=.\t[]\n;'#\\,./ ........../*-+.\n");
	strcpy(s_ascii[1], ".ABCDEFGHIJKLMNOPQRSTUVWXYZ)!\"£$%^&*(0123456789.............¬_+.\t{}\n:@~|<>? ........../*-+.\n");
	for(int i=0; i<128; i++) {
		if(i==KEY_STOP || i==KEY_DEL_PAD) continue;
		if(i>91 || s_ascii[0][i]=='.') s_ascii[0][i] = 0;
		if(i>91 || s_ascii[1][i]=='.') s_ascii[1][i] = 0;
	}
	
	//mapping from system input
	#ifdef WIN32
	int map[256] = {
/*0x00*/	0,	KEY_ESC,	KEY_1,	KEY_2,	KEY_3,	KEY_4,	KEY_5,	KEY_6,	
/*0x08*/	KEY_7,	KEY_8,	KEY_9,	KEY_0,	KEY_MINUS,	KEY_EQUALS,	KEY_BACKSPACE,	KEY_TAB,	
/*0x10*/	KEY_Q,	KEY_W,	KEY_E,	KEY_R,	KEY_T,	KEY_Y,	KEY_U,	KEY_I,	
/*0x18*/	KEY_O,	KEY_P,	KEY_OPENBRACE,	KEY_CLOSEBRACE,	KEY_ENTER,	KEY_LCONTROL,	KEY_A,	KEY_S,	
/*0x20*/	KEY_D,	KEY_F,	KEY_G,	KEY_H,	KEY_J,	KEY_K,	KEY_L,	KEY_COLON,	
/*0x28*/	KEY_QUOTE,	KEY_TICK,	KEY_LSHIFT,	KEY_HASH,	KEY_Z,	KEY_X,	KEY_C,	KEY_V,	
/*0x30*/	KEY_B,	KEY_N,	KEY_M,	KEY_COMMA,	KEY_STOP,	KEY_SLASH,	KEY_RSHIFT,	KEY_ASTERISK,	
/*0x38*/	0,	KEY_SPACE,	KEY_CAPSLOCK,	KEY_F1,	KEY_F2,	KEY_F3,	KEY_F4,	KEY_F5,	
/*0x40*/	KEY_F6,	KEY_F7,	KEY_F8,	KEY_F9,	0,	KEY_PAUSE,	KEY_SCRLOCK,	KEY_7_PAD,	
/*0x48*/	KEY_8_PAD,	KEY_9_PAD,	KEY_MINUS_PAD,	KEY_4_PAD,	KEY_5_PAD,	KEY_6_PAD,	KEY_PLUS_PAD,	KEY_1_PAD,	
/*0x50*/	KEY_2_PAD,	KEY_3_PAD,	KEY_0_PAD,	KEY_DEL_PAD,	0,	0,	KEY_BACKSLASH,	KEY_F11,	
/*0x58*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x60*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x68*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x70*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x78*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x80*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x88*/	KEY_F12,	0,	0,	0,	0,	0,	0,	0,	
/*0x90*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x98*/	0,	0,	0,	0,	KEY_ENTER_PAD,	KEY_RCONTROL,	0,	0,	
/*0xa0*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xa8*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xb0*/	0,	0,	0,	0,	0,	KEY_SLASH_PAD,	0,	0,	
/*0xb8*/	KEY_ALTGR,	0,	0,	0,	0,	0,	0,	0,	
/*0xc0*/	0,	0,	0,	0,	0,	KEY_NUMLOCK,	0,	KEY_HOME,	
/*0xc8*/	KEY_UP,	KEY_PGUP,	0,	KEY_LEFT,	0,	KEY_RIGHT,	0,	KEY_END,	
/*0xd0*/	KEY_DOWN,	KEY_PGDN,	KEY_INSERT,	KEY_DEL,	0,	0,	0,	0,	
/*0xd8*/	0,	0,	0,	KEY_LMETA,	KEY_RMETA,	KEY_MENU,	0,	0,	
/*0xe0*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xe8*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xf0*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xf8*/	0,	0,	0,	0,	0,	0,	0,	0,
	};
	#endif

	#ifdef LINUX
	int map[256] = {
/*0x00*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x08*/	0,	KEY_ESC,	KEY_1,	KEY_2,	KEY_3,	KEY_4,	KEY_5,	KEY_6,
/*0x10*/	KEY_7,	KEY_8,	KEY_9,	KEY_0,	KEY_MINUS,	KEY_EQUALS,	KEY_BACKSPACE,	KEY_TAB,
/*0x18*/	KEY_Q,	KEY_W,	KEY_E,	KEY_R,	KEY_T,	KEY_Y,	KEY_U,	KEY_I,
/*0x20*/	KEY_O,	KEY_P,	KEY_OPENBRACE,	KEY_CLOSEBRACE,	KEY_ENTER,	KEY_LCONTROL,	KEY_A,	KEY_S,
/*0x28*/	KEY_D,	KEY_F,	KEY_G,	KEY_H,	KEY_J,	KEY_K,	KEY_L,	KEY_COLON,
/*0x30*/	KEY_QUOTE,	KEY_TICK,	KEY_LSHIFT,	KEY_HASH,	KEY_Z,	KEY_X,	KEY_C,	KEY_V,
/*0x38*/	KEY_B,	KEY_N,	KEY_M,	KEY_COMMA,	KEY_STOP,	KEY_SLASH,	KEY_RSHIFT,	KEY_ASTERISK,
/*0x40*/	KEY_ALT,	KEY_SPACE,	KEY_CAPSLOCK,	KEY_F1,	KEY_F2,	KEY_F3,	KEY_F4,	KEY_F5,
/*0x48*/	KEY_F6,	KEY_F7,	KEY_F8,	KEY_F9,	KEY_F10,	KEY_NUMLOCK,	KEY_SCRLOCK,	KEY_7_PAD,
/*0x50*/	KEY_8_PAD,	KEY_9_PAD,	KEY_MINUS_PAD,	KEY_4_PAD,	KEY_5_PAD,	KEY_6_PAD,	KEY_PLUS_PAD,	KEY_1_PAD,
/*0x58*/	KEY_2_PAD,	KEY_3_PAD,	KEY_0_PAD,	KEY_DEL_PAD,	0,	0,	KEY_BACKSLASH,	KEY_F11,
//*0x60* /	KEY_F12,	KEY_HOME,	KEY_UP,	KEY_PGUP,	KEY_LEFT,	0,	KEY_RIGHT,	KEY_END,
//*0x68* /	KEY_DOWN,	KEY_PGDN,	KEY_INSERT,	KEY_DEL,	KEY_ENTER_PAD,	KEY_RCONTROL,	KEY_PAUSE,	KEY_PRTSCR,
//*0x70* /	KEY_SLASH_PAD,	KEY_ALTGR,	0,	KEY_LWIN,	KEY_RWIN,	KEY_MENU,	0,	KEY_DEL,

/*0x60*/	KEY_F12,	0,	0,	0,	0,	0,	0,	0,
/*0x68*/	KEY_ENTER_PAD,	KEY_RCONTROL,	KEY_SLASH_PAD,	0,	KEY_ALTGR,	0,	KEY_HOME,	KEY_UP,
/*0x70*/	KEY_PGUP,	KEY_LEFT,	KEY_RIGHT,	KEY_END,	KEY_DOWN,	KEY_PGDN,	KEY_INSERT,	KEY_DEL,

/*0x78*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x80*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x88*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x90*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x98*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xa0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xa8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xb0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xb8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xc0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xc8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xd0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xd8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xe0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xe8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xf0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xf8*/	0,	0,	0,	0,	0,	0,	0,	0,
	};
	#endif
	
	#ifdef EMSCRIPTEN
	int map[256] = {
/*0x00*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x08*/	KEY_BACKSPACE,	KEY_TAB,	0,	0,	0,	KEY_RETURN,	0,	0,
/*0x10*/	KEY_LSHIFT,	KEY_LCONTROL,	KEY_ALT,	KEY_PAUSE,	KEY_CAPSLOCK,	0,	0,	0,
/*0x18*/	0,	0,	0,	KEY_ESC,	0,	0,	0,	0,
/*0x20*/	KEY_SPACE,	KEY_PGUP,	KEY_PGDN,	KEY_END,	KEY_HOME,	KEY_LEFT,	KEY_UP,	KEY_RIGHT,
/*0x28*/	KEY_DOWN,	0,	0,	0,	0,	KEY_INSERT,	KEY_DEL,	0,
/*0x30*/	KEY_0,	KEY_1,	KEY_2,	KEY_3,	KEY_4,	KEY_5,	KEY_6,	KEY_7,
/*0x38*/	KEY_8,	KEY_9,	0,	KEY_COLON,	0,	KEY_EQUALS,	0,	0,
/*0x40*/	0,	KEY_A,	KEY_B,	KEY_C,	KEY_D,	KEY_E,	KEY_F,	KEY_G,
/*0x48*/	KEY_H,	KEY_I,	KEY_J,	KEY_K,	KEY_L,	KEY_M,	KEY_N,	KEY_O,
/*0x50*/	KEY_P,	KEY_Q,	KEY_R,	KEY_S,	KEY_T,	KEY_U,	KEY_V,	KEY_W,
/*0x58*/	KEY_X,	KEY_Y,	KEY_Z,	KEY_LMETA,	0,	0,	0,	0,
/*0x60*/	KEY_0_PAD,	KEY_1_PAD,	KEY_2_PAD,	KEY_3_PAD,	KEY_4_PAD,	KEY_5_PAD,	KEY_6_PAD,	KEY_7_PAD,
/*0x68*/	KEY_8_PAD,	KEY_9_PAD,	KEY_ASTERISK,	KEY_PLUS_PAD,	0,	KEY_MINUS_PAD,	KEY_DEL_PAD,	KEY_SLASH_PAD,
/*0x70*/	KEY_F1,	KEY_F2,	KEY_F3,	KEY_F4,	KEY_F5,	KEY_F6,	KEY_F7,	KEY_F8,
/*0x78*/	KEY_F9,	KEY_F10,KEY_F11,KEY_F12,0,	0,	0,	0,
/*0x80*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x88*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0x90*/	KEY_NUMLOCK,	KEY_SCRLOCK,	0,	0,	0,	0,	0,	0,
/*0x98*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xa0*/	0,	0,	0,	KEY_HASH,	0,	0,	0,	0,
/*0xa8*/	0,	0,	0,	0,	0,	KEY_MINUS,	0,	0,
/*0xb0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xb8*/	0,	0,	0,	0,	KEY_COMMA,	0,	KEY_STOP,	KEY_SLASH,
/*0xc0*/	KEY_TICK,	0,	0,	0,	0,	0,	0,	0,
/*0xc8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xd0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xd8*/	0,	0,	0,	KEY_OPENBRACE,	KEY_BACKSLASH,	KEY_CLOSEBRACE,	KEY_QUOTE,	0,
/*0xe0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xe8*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xf0*/	0,	0,	0,	0,	0,	0,	0,	0,
/*0xf8*/	0,	0,	0,	0,	0,	0,	0,	0,
	};
	#endif

	memcpy(s_map, map, 256 * sizeof(int));
}

void Input::clear() {
	memset(m_key,	  0, 128 * sizeof(bool));
	memset(m_keyChange, 0, 128);

	m_mouseState.x = m_mouseState.y = 0;
	m_mouseState.button = m_mouseState.pressed = m_mouseState.released = 0;
	m_mouseState.wheel = 0;
	m_mouseState.delta.set(0,0);
	m_lastKey = 0;
	m_lastChar = 0;
	m_keyMask = 0;
}


// ====================================================== //

uint Input::getAction(const char* name) {
	uint key = m_names.get(name, m_names.size());
	if(key==m_names.size()) m_names[name] = key;
	return key;
}

uint Input::setAction(const char* name, uint key) {
	if(m_names.contains(name)) printf("Warning: setAction %s already exists as %s\n", name, getActionName(key));
	m_names[name] = key;
	return key;
}

void Input::bind(uint action, uint keycode) {
	while(m_binding.size() <= action) m_binding.push_back(std::vector<Binding>());
	m_binding[action].push_back(Binding{keycode&0xff, 0, keycode>>8});
}

void Input::bindMouse(uint action, uint button) {
	while(m_binding.size() <= action) m_binding.push_back(std::vector<Binding>());
	m_binding[action].push_back(Binding{button, 1, 0});
}

void Input::bindJoystick(uint action, uint js, uint button) {
	while(m_binding.size() <= action) m_binding.push_back(std::vector<Binding>());
	m_binding[action].push_back(Binding{button, 2, js});
}

void Input::bindJoystick(uint action, uint js, uint axis, float threshold) {
	while(m_binding.size() <= action) m_binding.push_back(std::vector<Binding>());
	m_binding[action].push_back(Binding{(uint)m_axisBinding.size(), 3, js});
	m_axisBinding.push_back({(uint8)js, (uint8)axis, (char)(threshold*127), 0});
}

void Input::bindAxisInternal(uint action, uint8 type, uint8 js, uint index, float multiplier) {
	while(m_valueBinding.size() <= action) m_valueBinding.emplace_back();
	for(ValueBinding& b: m_valueBinding[action]) {
		if(b.type == type && b.js==js && b.index==index) { b.multiplier = multiplier; return; }
	}
	m_valueBinding[action].push_back(ValueBinding{type, js, (uint16)index, multiplier});
}
void Input::bindJoystickValue(uint action, uint joystick, uint axis, float multiplier) {
	bindAxisInternal(action, 0, joystick, axis, multiplier);
}
void Input::bindMouseValue(uint action, uint axis, float multiplier) {
	bindAxisInternal(action, axis<2? 2: 3, 0, axis, multiplier);
}
void Input::bindButtonValue(uint action, uint key, float value) {
	bindAxisInternal(action, 4, 0, key, value);
}
void Input::bindButtonValue(uint action, uint joystick, uint button, float value) {
	bindAxisInternal(action, 1, joystick, button, value);
}



void Input::unbind(uint action) {
	if(action < m_binding.size()) m_binding[action].clear();
	if(action < m_valueBinding.size()) m_valueBinding[action].clear();
}

void Input::unbindAll() {
	for(auto& action: m_binding) action.clear();
	for(auto& action: m_valueBinding) action.clear();
}

inline bool Input::checkJoystickThreshold(const AxisBinding& binding) const {
	char value = joystick(binding.js).axis(binding.axis) * 127;
	if(binding.threshold > 0 && value > binding.threshold) return true;
	if(binding.threshold < 0 && value < binding.threshold) return true;
	return false;
}

bool Input::check(uint action) const {
	if(action >= m_binding.size()) return false;
	for(Binding b: m_binding[action]) {
		switch(b.type) {
		case 0: if(key(b.button) && (b.mask==MODIFIER_ANY || (m_keyMask&b.mask>>8))) return true; break;
		case 1: if(mouse.button & 1<<b.button) return true; break;
		case 2: if(joystick(b.mask).button(b.button)) return true; break;
		case 3: if(checkJoystickThreshold(m_axisBinding[b.button])) return true; break;
		}
	}
	return false;
}

bool Input::pressed(uint action) const {
	if(action >= m_binding.size()) return false;
	for(Binding b: m_binding[action]) {
		switch(b.type) {
		case 0: if(keyPressed(b.button) && (b.mask==MODIFIER_ANY || (m_keyMask&b.mask>>8))) return true; break;
		case 1: if(mouse.pressed & 1<<b.button) return true; break;
		case 2: if(joystick(b.mask).pressed(b.button)) return true; break;
		case 3: if(!m_axisBinding[b.button].last && checkJoystickThreshold(m_axisBinding[b.button])) return true; break;
		}
	}
	return false;
}

bool Input::released(uint action) const {
	if(action >= m_binding.size()) return false;
	for(Binding b: m_binding[action]) {
		switch(b.type) {
		case 0: if(keyReleased(b.button)) return true; break;
		case 1: if(mouse.released & 1<<b.button) return true; break;
		case 2: if(joystick(b.mask).pressed(b.button)) return true; break;
		case 3: if(m_axisBinding[b.button].last && !checkJoystickThreshold(m_axisBinding[b.button])) return true; break;
		}
	}
	return false;
}

float Input::value(uint action) const {
	float value = 0;
	if(action>=m_valueBinding.size()) return 0;
	for(const ValueBinding& b: m_valueBinding[action]) {
		switch(b.type) {
		case 0: // Joystick axis
			value += joystick(b.js).axis(b.index) * b.multiplier;
			break;
		case 1: // Joystick button
			if(joystick(b.js).button(b.index)) value += b.multiplier;
			break;
		case 2: // Mouse axis
			value += mouse.delta[b.index] * b.multiplier;
			break;
		case 3: // Mouse wheel
			value += mouse.wheel * b.multiplier;
			break;
		case 4: // Keyboard value
			if(key(b.index)) value += b.multiplier;
			break;
		}
	}
	return value;
}

const char* Input::getActionName(uint action) const {
	for(auto& i: m_names) if(i.value == action) return i.key;
	return nullptr;
}

void Input::loadActions(const INIFile& file) {
	union { Binding binding; uint value; } key;
	for(const auto& i: file["input"]) {
		uint action = getAction(i.key);
		key.value = i.value;
		m_binding[action].push_back(key.binding);
	}
}

void Input::saveActions(INIFile& file) const {
	union { Binding binding; uint value; } key;
	INIFile::Section section = file["input"];
	for(size_t action=0; action<m_binding.size(); ++action) {
		if(!m_binding[action].empty()) {
			const char* name = getActionName(action);
			for(const Binding& binding: m_binding[action]) {
				key.binding = binding;
				section.add(name, key.value);
			}
		}
	}
}





