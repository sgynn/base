// State Manager v1.2
#ifndef _BASE_GAMESTATE_
#define _BASE_GAMESTATE_

#include <vector>

namespace base {

enum TransitionState { T_NONE, T_IN, T_OUT };
enum StateFlags { NONE=0, OVERLAP=1, PERSISTANT=2 };

class StateManager;
class GameState;

/** Game state component base class
 * These are encapsulated update/draw plugins that can be added to the game state
 */
class GameStateComponent {
	friend class GameState;
	public:
	GameStateComponent(int updateOrder=0, int drawOrder=0) : m_updateOrder(updateOrder), m_drawOrder(drawOrder) {}
	virtual ~GameStateComponent() {}
	virtual void update() = 0;
	virtual void draw() = 0;
	GameState* getState() const { return m_gameState; }
	private:
	GameState* m_gameState;
	int m_references = 0;
	int m_updateOrder;
	int m_drawOrder;
};


/** Game state base class.
States can have transitions. The transition times are passed to the constructor.
*/
class GameState : public GameStateComponent {
	friend class StateManager;
	public:
		GameState(float tIn=0, float tOut=0, StateFlags flags=NONE);
		virtual ~GameState();
		
		virtual void begin() {}
		virtual void end() {}

		virtual void update() = 0;
		virtual void draw() = 0;

		void addComponent(GameStateComponent*);
		unsigned getComponentFlags() const { return m_componentFlags; }
		void setComponentFlags(unsigned set) { m_componentFlags |= set; }
		
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
		int updateState();
		void drawState();

	private:
		TransitionState m_state;
		float m_transition;
		StateManager* m_stateManager;
		float m_in, m_out;
		StateFlags m_flags;
		unsigned m_componentFlags;
		std::vector<GameStateComponent*> m_updateComponents;
		std::vector<GameStateComponent*> m_drawComponents;
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

