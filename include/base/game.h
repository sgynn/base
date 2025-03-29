#ifndef _BASE_GAME_
#define _BASE_GAME_

#include "math.h"

namespace base {

class Window;
class Input;
class GameState;
class GameStateManager;

/** Game main functions */
class Game {
	public:
	/** Create the game instance. Can only have one */
	static Game* create(int width=800, int height=600, int bpp=32, bool fullscreen=false, int fps=60, uint antialiasing=0);

	/** Depricated - should be private */
	Game(int width=800, int height=600, int bpp=32, bool fullscreen=false, uint antialiasing=0);
	~Game();
	
	/** Set the initial game state */
	void setInitialState(GameState* state);

	/** Set static framerate. 0 = variable */
	void setTargetFPS(uint fps=60);

	/** Run the app */
	void run();

	/** terminate the app */
	static void exit();

	static Point getSize();
	static int width();
	static int height();
	static float aspect() { return (float)width()/height(); }

	//Time 
	static uint64 getTicks();
	static uint64 getTickFrequency();

	/** Time since the game started (seconds) */
	static float gameTime() { return s_inst->m_totalTime; }
	/** Time since tha last frame - use for framerate independance */
	static float frameTime() { return s_inst->m_frameTime; }
	/** Get detailed timings */
	static void debugTime(uint& game, uint& system, uint& update, uint& render);
	static void setFrameTime(float t) { s_inst->m_frameTime=t; }
	static void setGameTime(float time) { s_inst->m_totalTime=time; }

	/// Get the active game state
	static GameState* getState();

	//Input stuff (depricated)
	static bool Key(int k);				// Get the state of a key
	static bool Pressed(int k);			// Was the key pressed
	static uint LastKey();				// Get the last key pressed
	static uint16 LastChar();			// Get the last key pressed as character
	static int Mouse();					// Mouse button state (1=left, 2=centre, 4=right ... )
	static int Mouse(int& x, int&y);	// Mouse button and position
	static int MouseClick();			// Were mouse buttons pressed last frame
	static void WarpMouse(int x, int y);// Move the cursor
	static float MouseWheel();			// Get the mouse wheel delta since last frame

	// Get input class
	static Input* input() { return s_input; }

	static base::Window* window() { return s_window; }
	static void toggleFullScreen(bool borderless=true);

	protected:
	friend class Window;
	GameStateManager* m_state; 

	static Window* s_window;
	static Input* s_input;

	//only allow one instance
	static Game* s_inst;
	static void cleanup();

	//Timing data
	uint m_targetFPS;	// Target framerate when fixed
	uint m_gameTime=0;	// Time taken for a complete game loop (in system ticks)
	uint m_systemTime=0;	// Time taken between game loops (~0 for variable rate)
	uint m_renderTime=0;	// Time to send render commands *NOT ACTUAL REMDER TIME*
	uint m_elapsedTime=0;	// Time since last frame
	uint m_updateTime=0;	// Time spent in update loop
	float m_totalTime=0;	// Total time since game started (seconds)
	float m_frameTime=0;	// Time since last frame (seconds);
};
};

#endif


