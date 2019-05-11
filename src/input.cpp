#include "base/input.h"
#include <cstring> 

#include "base/game.h" //Game included to get window position for mouse data
#include "base/window.h"

#include <cstdio>

using namespace base;

int   Input::s_map[256];
char Input::s_ascii[2][128]; 

Input::Input() {
	clear();
	createMap();
}

Input::~Input() {
}

void Input::update() {
	//clear frame data
	memset(m_pressed, 0, 128);
	m_mouseClick = 0;
	m_mouseWheel = 0;
	m_lastKey = 0;
	m_lastChar = 0;
	//Cache mouse position
	m_mousePosition = queryMouse();
	// Joysticks
	updateJoysticks();
}

Point Input::queryMouse() { 
	Point p = Game::window()->queryMouse();
	p.y = Game::window()->getSize().y - p.y;
	return p;
}
void Input::warpMouse(int x, int y) {
	m_mousePosition.set(x, y);
	Game::window()->warpMouse(x, Game::window()->getSize().y - y);
}

void Input::setButton(int code, bool down, const Point& pt) {
	int mask = 1<<(code-1);
	if(down) m_mouseButton |= mask;
	else m_mouseButton &= ~mask;
	//Click
	if(down) {
		m_mouseClick = mask; 
		m_mouseClickPoint.x = pt.x;
		m_mouseClickPoint.y = Game::window()->getSize().y - pt.y;
	}
}

//extern void setTestKey(int code);
void Input::setKey(int code, bool down) {
	m_key[s_map[code]] = down; 
	if(down) {
		m_pressed[s_map[code]] = down;
		m_lastKey = s_map[code]; 
		m_lastChar = s_ascii[ key(KEY_LSHIFT) || key(KEY_RSHIFT)? 1:0 ][ m_lastKey ];
	}
}


void Input::createMap() {
	//create mapping from KeyCode to ASCII characters
	strcpy(s_ascii[0], ".abcdefghijklmnopqrstuvwxyz01234567890123456789.............`-=.\t[]\n;'#\\,./ ........../*-+.\n");
	strcpy(s_ascii[1], ".ABCDEFGHIJKLMNOPQRSTUVWXYZ)!\"£$%^&*(0123456789.............¬_+.\t{}\n:@~|<>? ........../*-+.\n");
	for(int i=0; i<128; i++) {
		if(i>91 || (s_ascii[0][i]=='.' && i!=KEY_STOP)) s_ascii[0][i] = 0;
		if(i>91 || (s_ascii[1][i]=='.' && i!=KEY_STOP)) s_ascii[1][i] = 0;
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
/*0x88*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x90*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0x98*/	0,	0,	0,	0,	KEY_ENTER_PAD,	KEY_RCONTROL,	0,	0,	
/*0xa0*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xa8*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xb0*/	0,	0,	0,	0,	0,	KEY_SLASH_PAD,	0,	0,	
/*0xb8*/	KEY_ALTGR,	0,	0,	0,	0,	0,	0,	0,	
/*0xc0*/	0,	0,	0,	0,	0,	KEY_NUMLOCK,	0,	KEY_HOME,	
/*0xc8*/	KEY_UP,	KEY_PGUP,	0,	KEY_LEFT,	0,	KEY_RIGHT,	0,	KEY_END,	
/*0xd0*/	KEY_DOWN,	KEY_PGDN,	KEY_INSERT,	KEY_DEL,	0,	0,	0,	0,	
/*0xd8*/	0,	0,	0,	KEY_LWIN,	KEY_RWIN,	KEY_MENU,	0,	0,	
/*0xe0*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xe8*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xf0*/	0,	0,	0,	0,	0,	0,	0,	0,	
/*0xf8*/	0,	0,	0,	0,	0,	0,	0,	0,
	};
	memcpy(s_map, map, 256 * sizeof(int));

	#else
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
	memcpy(s_map, map, 256 * sizeof(int));
	#endif
}

void Input::clear() {
	memset(m_key,	  0, 128 * sizeof(bool));
	memset(m_pressed, 0, 128 * sizeof(bool));
	m_lastKey = 0;
	m_lastChar = 0;
	m_mouseClick = 0;
	m_mouseButton = 0;
	m_mousePosition.x = m_mousePosition.y = 0;
}

