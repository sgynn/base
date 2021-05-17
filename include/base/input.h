#ifndef _BASE_INPUT_
#define _BASE_INPUT_

#include "math.h"
#include <vector>

namespace base {
	enum KeyCode {
		KEY_A		= 1,
		KEY_B		= 2,
		KEY_C		= 3,
		KEY_D		= 4,
		KEY_E		= 5,
		KEY_F		= 6,
		KEY_G		= 7,
		KEY_H		= 8,
		KEY_I		= 9,
		KEY_J		= 10,
		KEY_K		= 11,
		KEY_L		= 12,
		KEY_M		= 13,
		KEY_N		= 14,
		KEY_O		= 15,
		KEY_P		= 16,
		KEY_Q		= 17,
		KEY_R		= 18,
		KEY_S		= 19,
		KEY_T		= 20,
		KEY_U		= 21,
		KEY_V		= 22,
		KEY_W		= 23,
		KEY_X		= 24,
		KEY_Y		= 25,
		KEY_Z		= 26,
		KEY_0		= 27,
		KEY_1		= 28,
		KEY_2		= 29,
		KEY_3		= 30,
		KEY_4		= 31,
		KEY_5		= 32,
		KEY_6		= 33,
		KEY_7		= 34,
		KEY_8		= 35,
		KEY_9		= 36,
		KEY_0_PAD	= 37,
		KEY_1_PAD	= 38,
		KEY_2_PAD	= 39,
		KEY_3_PAD	= 40,
		KEY_4_PAD	= 41,
		KEY_5_PAD	= 42,
		KEY_6_PAD	= 43,
		KEY_7_PAD	= 44,
		KEY_8_PAD	= 45,
		KEY_9_PAD	= 46,
		KEY_F1		= 47,
		KEY_F2		= 48,
		KEY_F3		= 49,
		KEY_F4		= 50,
		KEY_F5		= 51,
		KEY_F6		= 52,
		KEY_F7		= 53,
		KEY_F8		= 54,
		KEY_F9		= 55,
		KEY_F10		= 56,
		KEY_F11		= 57,
		KEY_F12		= 58,

		KEY_ESC		= 59,	KEY_ESCAPE = 59,
		KEY_TICK	= 60,
		KEY_MINUS	= 61,
		KEY_EQUALS	= 62,
		KEY_BACKSPACE	= 63,
		KEY_TAB		= 64,
		KEY_OPENBRACE	= 65,
		KEY_CLOSEBRACE	= 66,
		KEY_ENTER	= 67,	KEY_RETURN = 67,
		KEY_COLON	= 68,
		KEY_QUOTE	= 69,
		KEY_HASH	= 70,
		KEY_BACKSLASH	= 71,
		KEY_COMMA	= 72,
		KEY_STOP	= 73,
		KEY_SLASH	= 74,
		KEY_SPACE	= 75,
		KEY_INSERT	= 76,
		KEY_DEL		= 77,
		KEY_HOME	= 78,
		KEY_END		= 79,
		KEY_PGUP	= 80,
		KEY_PGDN	= 81,
		KEY_LEFT	= 82,
		KEY_RIGHT	= 83,
		KEY_UP		= 84,
		KEY_DOWN	= 85,
		KEY_SLASH_PAD	= 86,
		KEY_ASTERISK	= 87,
		KEY_MINUS_PAD	= 88,
		KEY_PLUS_PAD	= 89,
		KEY_DEL_PAD	= 90,
		KEY_ENTER_PAD	= 91,
		KEY_PRTSCR	= 92,
		KEY_PAUSE	= 93,
		
		KEY_ABNT_C1 	= 94,
		KEY_YEN		= 95,
		KEY_KANA	= 96,
		KEY_CONVERT	= 97,
		KEY_NOCONVERT	= 98,
		KEY_AT		= 99,
		KEY_CIRCUMFLEX	= 100,
		KEY_COLON2	= 101,
		KEY_KANJI	= 102,
		KEY_EQUALS_PAD	= 103,  /* MacOS X */
		KEY_BACKQUOTE	= 104,  /* MacOS X */
		KEY_SEMICOLON	= 105,  /* MacOS X */
		KEY_COMMAND	= 106,  /* MacOS X */
		KEY_UNKNOWN1	= 107,
		KEY_UNKNOWN2	= 108,
		KEY_UNKNOWN3	= 109,
		KEY_UNKNOWN4	= 110,
		KEY_UNKNOWN5	= 111,
		KEY_UNKNOWN6	= 112,
		KEY_UNKNOWN7	= 113,
		KEY_UNKNOWN8	= 114,

		KEY_LSHIFT	= 115,
		KEY_RSHIFT	= 116,
		KEY_LCONTROL	= 117,
		KEY_LCTRL	= 117,
		KEY_RCONTROL	= 118,
		KEY_RCTRL	= 118,
		KEY_ALT		= 119,
		KEY_ALTGR	= 120,
		KEY_LWIN	= 121,
		KEY_RWIN	= 122,
		KEY_MENU      	= 123,
		KEY_SCRLOCK	= 124,
		KEY_NUMLOCK	= 125,
		KEY_CAPSLOCK	= 126,
	};

	/// Joystick class --------------------------------------------------------------
	class Joystick { 
		friend class Input;
		Joystick(int b, int a);
		public:
		~Joystick();
		bool  button(uint) const;					/// Get state of a button
		bool  pressed(uint) const;					/// Was this button pressed this frame
		bool  released(uint) const;					/// Was this button released this frame
		float axis(uint) const;						/// Get normalised axis value
		int   axisRaw(uint) const;					/// Get raw axis value
		const Point& hat() const;					/// Get POV hat state
		void  setDeadzone(float);					/// Set axis deadzone
		void  getCalibration(uint, int*) const;	/// Get axis calibration
		void  setCalibration(uint, const int*);			/// Set axis calibration
		private:
		bool update();
		private:
		int  m_index;
		char m_name[128];
		// State
		uint   m_numAxes;
		uint   m_numButtons;
		struct Range { int min, max; } *m_range;
		float  m_dead;
		int*   m_axis;
		int    m_buttons;
		int    m_changed;
		Point  m_hat;
		private: // linux stuff
		int    m_file;
		uint8* m_keyMap;
		uint8* m_absMap;
		bool   m_created;
	};
	
	// Global input class -------------------------------------------------------------
	class Input {
		friend class Window;
		friend class X11Window;
		friend class Win32Window;
		public:
		Input();
		~Input();

		/** Get the state of a key */
 		bool key(int k) const { return m_key[k]; }
		/** Was a key pressed in this frame */
		bool pressed(int k) const { return m_pressed[k]; }
		/** The code of the last key pressed */
		int lastKey() const { return m_lastKey; }
		/** The ascii character of the last key pressed. */
		char lastChar() const { return m_lastChar; }
		
		/** Get the mouse buttons that are pressed (bit1:left, bit2:middle, bit3:right etc) */
		int  mouse() const { return m_mouseButton; }
		/** Get the mouse position from bottom left of the screen. @returns mouse button state */
		int  mouse(int&x, int& y) const { x = m_mousePosition.x; y=m_mousePosition.y; return m_mouseButton; }
		/** Get the buttons that were pressed in this frame */
		int  mouseClick() const { return m_mouseClick; }
		/** Get the position of the last click */
		int  mouseClick(int& x, int& y) const{ x = m_mouseClickPoint.x; y = m_mouseClickPoint.y; return m_mouseClick; } 
		/** Get the mouse wheel delta of this frame */
		float mouseWheel() const { return m_mouseWheel; }
		/** Warp the mouse pointer to a location. Origin botom left of the window */
		void warpMouse(int x, int y);
		/** Directly query the mouse location */
		Point queryMouse();
		
		/** Get a joystick state */
		Joystick& joystick(uint id=0); 
		/** Initialise joysticks - returns number found */
		int initialiseJoysticks();
		
		/** Update must be called once per frame BEFORE window events are processed */
		void update();
		private:
		
		bool m_key[128];		//keys that are currently pressed
		bool m_pressed[128];	//keys that were pressed duting tha last frame
		int  m_lastKey;			//the last key that was pressed
		char m_lastChar;		//Ascii character of last key
		
		static int   s_map[256];	//system keycode to keycode map
		static char s_ascii[2][128];	//library keycode to ascii map
		
		//Mouse stuff 
		int   m_mouseButton;
		Point m_mousePosition;
		int   m_mouseClick;
		Point m_mouseClickPoint;
		float m_mouseWheel;

		// Joysticks
		std::vector<Joystick*> m_joysticks;
		void updateJoysticks();
		
		//set method from system events - needs converting to wgd keycodes
		static void createMap();
		void setKey(int code, bool down);	//Set a key state (used by system)
		void setButton(int code, bool down, const Point&); //Set mouse button state (used by system)
		void clear();	//set all keys to 0
	};
};

#endif

