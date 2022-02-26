#ifndef _BASE_GAMESTATE_
#define _BASE_GAMESTATE_

#include <vector>

namespace base {

enum TransitionState { T_NONE, T_IN, T_OUT };
enum StateFlags { NONE=0, OVERLAP=1, PERSISTANT=2 };
enum ComponentFlags { BLOCK_KEYS=1, BLOCK_MOUSE=2, BLOCK_WHEEL=4, BLOCK_UPDATE=8, BLOCK_DRAW=16 };


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

	// Component flags are used to pass state information to subsequent components in the game state
	// They are reset at the start of every frame so need to set every update
	// It is up to the component implementation how to handle them
	// Some basic flags are defined in the ComponentFlags enum
	void setComponentFlags(unsigned set);
	bool hasComponentFlags(unsigned flags) const;
	unsigned getComponentFlags() const;
		
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
	friend class GameStateComponent;
	public:
		GameState(float tIn=0, float tOut=0, StateFlags flags=NONE);
		virtual ~GameState();
		
		virtual void begin() {}
		virtual void end() {}

		void update() {}
		void draw() {}

		void addComponent(GameStateComponent*);

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

// Component flags implemntation - needs to be after GameState class
inline void GameStateComponent::setComponentFlags(unsigned set) { m_gameState->m_componentFlags |= set; }
inline bool GameStateComponent::hasComponentFlags(unsigned flags) const { return m_gameState->m_componentFlags & flags; }
inline unsigned GameStateComponent::getComponentFlags() const { return m_gameState->m_componentFlags; }

};

#endif

