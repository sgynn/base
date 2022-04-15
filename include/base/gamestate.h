#pragma once

#include <vector>
class Point;

namespace base {

enum StateMode { TRANSIENT, PERSISTENT };
enum ComponentFlags { BLOCK_KEYS=1, BLOCK_MOUSE=2, BLOCK_WHEEL=4, BLOCK_UPDATE=8, BLOCK_DRAW=0x10, BLOCK_GRAB=0x20 };

class GameStateManager;
class GameState;

/** Game state component base class
 * These are encapsulated update/draw plugins that can be added to the game state
 * Persistent components will automatically be added to the new state
 */
class GameStateComponent {
	friend class GameState;
	public:
	GameStateComponent(int updateOrder=0, int drawOrder=0, StateMode mode=TRANSIENT) : m_mode(mode), m_updateOrder(updateOrder), m_drawOrder(drawOrder) {}
	virtual ~GameStateComponent() {}
	virtual void update() = 0;
	virtual void draw() = 0;
	virtual void resized(const Point& size) {}
	virtual void focusChanged(bool focus) {}
	GameState* getState() const { return m_gameState; }
	StateMode getMode() const { return m_mode; }

	// Component flags are used to pass state information to subsequent components in the game state
	// They are reset at the start of every frame so need to set every update
	// It is up to the component implementation how to handle them
	// Some basic flags are defined in the ComponentFlags enum
	void setComponentFlags(unsigned set);
	bool hasComponentFlags(unsigned flags) const;
	unsigned getComponentFlags() const;

	private:
	GameState* m_gameState;
	StateMode m_mode;
	int m_references = 0;
	int m_updateOrder;
	int m_drawOrder;
};


/** Game state base class.
States can have transitions. The transition times are passed to the constructor.
*/
class GameState : public GameStateComponent {
	friend class GameStateManager;
	friend class GameStateComponent;
	public:
		GameState(StateMode mode=TRANSIENT);
		virtual ~GameState();
		
		virtual void begin() {}
		virtual void end() {}

		void update() {}
		void draw() {}

		void addComponent(GameStateComponent*);
		void removeComponent(GameStateComponent*);

		template<class T> T* getComponent() {
			for(GameStateComponent* c: m_updateComponents) if(T* r = dynamic_cast<T*>(c)) return r;
			return 0;
		}

	//protected:
		/** Set the next state to change to and start transitions */
		void changeState(GameState* state);

		/** Get the previous state */
		GameState* previousState() const;

	private:
		void updateState();
		void drawState();

	private:
		GameStateManager* m_stateManager;
		unsigned m_componentFlags;
		std::vector<GameStateComponent*> m_updateComponents;
		std::vector<GameStateComponent*> m_drawComponents;
		bool m_transient = false;
};

/** Game State manager - Handles transitions and control */
class GameStateManager {
	friend class GameState;
	public:
		GameStateManager();
		~GameStateManager();
		
		void update();
		void draw();
		
		void changeState(GameState* next);
		void onResized(const Point& newSize);
		void onFocusChanged(bool focus);
		
		bool running() const { return m_currentState || m_nextState; }
		
		void quit();
		
	private:
		GameState* m_currentState;
		GameState* m_nextState;
		GameState* m_prevState;
};

// Component flags implemntation - needs to be after GameState class
inline void GameStateComponent::setComponentFlags(unsigned set) { m_gameState->m_componentFlags |= set; }
inline bool GameStateComponent::hasComponentFlags(unsigned flags) const { return m_gameState->m_componentFlags & flags; }
inline unsigned GameStateComponent::getComponentFlags() const { return m_gameState->m_componentFlags; }

};

