#ifndef _GAME_APP_
#define _GAME_APP_

#include "math.h"

namespace base {

class Window;
class Input;
class GameState;
class StateManager;

/** Game main functions */
class Game {
	public:
	Game(int width=800, int height=600, int bpp=32, bool fullscreen=false, uint antialiasing=0);
	~Game();
	
	/** Set title */
	void setTitle(const char* title);
	
	/** Set the initial game state */
	void setInitialState(GameState* state);

	/** Run the app */
	void run();
	/** terminate the app */
	void exit();

	/** Resize the window - Nore: may need to reload glContext (textures, shaders, buffers)*/
	void resize(int width, int height);
	void resize(bool fullscreen);
	void resize(int width, int height, bool fullscreen);

	static Point getSize();

	//Time 
	static uint64 getTicks();
	static uint64 getTickFrequency();

	//Input stuff (depricated)
	static bool Key(int k);				// Get the state of a key
	static bool Pressed(int k);			// Was the key pressed
	static uint LastKey();				// Get the last key pressed
	static uint16 LastChar();			// Get the last key pressed as character
	static int Mouse();					// Mouse button state (1=left, 2=centre, 4=right ... )
	static int Mouse(int& x, int&y);	// Mouse button and position
	static int MouseClick();			// Were mouse buttons pressed last frame
	static void WarpMouse(int x, int y);// Move the cursor
	static int MouseWheel();			// Get the mouse wheel delta since last frame

	// Get input class
	static Input* input() { return s_input; }

	static base::Window* window() { return s_window; }

	//Set clear bits
	static void clear(int bits) { s_clearBits = bits; }

	protected:
	StateManager* m_state; 
	static int s_clearBits;

	static Window* s_window;
	static Input* s_input;

	//only allow one instance
	static Game* s_inst;
	static void cleanup();
};
};

#endif


