// State Manager v1.2
#ifndef _GAMESTATE_
#define _GAMESTATE_

namespace base {

enum TransitionState { T_NONE, T_IN, T_OUT };
enum StateFlags { NONE=0, OVERLAP=1, PERSISTANT=2 };

class StateManager;

/** Game state base class.
States can have transitions. The transition times are passed to the constructor.
*/
class GameState {
	friend class StateManager;
	public:
		GameState(float tIn=0, float tOut=0, StateFlags flags=NONE);
		virtual ~GameState();
		
		virtual void begin() {}
		virtual void end() {}

		virtual void update() = 0;
		virtual void draw() = 0;
		
	protected:
		/** Current state the gamestate */
		TransitionState state() const { return m_state; }
		float transition() const { return m_transition; }
		
		/** Set the next state to change to and start transitions */
		void changeState(GameState* state);
		/** Get the previous state */
		GameState* lastState() const;
		
		float tIn() const { return m_in; }
		float tOut() const { return m_out; }
	private:
		TransitionState m_state;
		float m_transition;
		StateManager* m_stateManager;
		int updateState();
		float m_in, m_out;
		StateFlags m_flags;
};

/** Game State manager - Handles transitions and control */
class StateManager {
	friend class GameState;
	public:
		StateManager();
		~StateManager();
		
		void update();
		void draw();
		
		/** Change the game state */
		bool change(GameState* next);
		bool state(GameState* next) { return change(next); } //alias for compatibility
		
		bool running() const { return m_running; }
		
		void quit();
		
	private:
		GameState* currentState;
		GameState* nextState;
		GameState* prevState;
		bool m_running;
};
};

#endif

