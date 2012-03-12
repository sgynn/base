//state manager implementation

#include "base/state.h"

#include "base/game.h"
#define time Game::frameTime()

using namespace base;

GameState::GameState(float in, float out, StateFlags flags) : m_state(T_IN), m_transition(0), m_stateManager(0), m_in(in), m_out(out), m_flags(flags) {}
GameState::~GameState() {}
int GameState::updateState() {
	//update transition
	if(m_state==T_IN) {
		if(m_in==0) m_transition = 1;
		else m_transition += time/m_in;
		if(m_transition>=1.0f){ 
			m_state = T_NONE;
			m_transition = 1.0f;
		}
	} else if(m_state==T_OUT) {
		if(m_out==0) m_transition=0;
		else m_transition -= time/m_out;
		if(m_transition<=0.0f) return 0;
	}
	
	//call virtual update function
	update();
	
	return 1;
}
void GameState::changeState(GameState* state) {
	m_stateManager->change(state);
}
GameState* GameState::lastState() const {
	return m_stateManager->prevState;
}

StateManager::StateManager() : currentState(0), nextState(0), prevState(0), m_running(true) {}
StateManager::~StateManager() {}

void StateManager::update() {
	if(!currentState) m_running = false;
	else {
		if(nextState && (nextState->m_flags&OVERLAP)) nextState->updateState(); //update the other state if set to overlap
		if(currentState->updateState()==0) {
			currentState->end();
			prevState = 0;
			if(~currentState->m_flags&PERSISTANT) delete currentState;
			else prevState = currentState;
			currentState = nextState;
			nextState = 0;
			if(currentState) {
				currentState->m_state = T_IN;
				currentState->begin();
			}
		}
	}
}

void StateManager::draw() {
	if(nextState && (nextState->m_flags&OVERLAP)) nextState->draw(); //Draw the other state if set to overlap
	if(currentState) currentState->draw();
}

/** Change the game state */
bool StateManager::change(GameState* next) {
	if(next) next->m_stateManager = this;
	if(currentState==0) {
		currentState = next;
		if(currentState) currentState->begin();
	} else {
		currentState->m_state = T_OUT; //signal current state to end
		nextState = next;
	}
	return true;
}

void StateManager::quit() {
	m_running = false;
}
