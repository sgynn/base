//state manager implementation

#include "base/gamestate.h"
#include "base/game.h"
#include <cstdio>

#define time Game::frameTime()

using namespace base;

GameState::GameState() {
	addComponent(this);
}
GameState::~GameState() {
	for(GameStateComponent* c: m_updateComponents) {
		if(--c->m_references==0 && c!=this) delete c;
	}
	if(m_stateManager->m_prevState==this) m_stateManager->m_prevState = 0;
	if(m_stateManager->m_currentState==this) m_stateManager->m_currentState = 0;
	if(m_stateManager->m_nextState==this) m_stateManager->m_nextState = 0;
}

template<class T> inline void addToList(std::vector<GameStateComponent*>& list, GameStateComponent* c, const T& less) {
	for(auto i=list.begin(); i!=list.end(); ++i) {
		if(less(c, *i)) {
			list.insert(i, c);
			return;
		}
	}
	list.push_back(c);
}


void GameState::addComponent(GameStateComponent* c) {
	++c->m_references;
	addToList(m_updateComponents, c, [](GameStateComponent* a, GameStateComponent* b) { return a->m_updateOrder < b->m_updateOrder; });
	addToList(m_drawComponents, c, [](GameStateComponent* a, GameStateComponent* b) { return a->m_drawOrder < b->m_drawOrder; });
	c->m_gameState = this;
}

void GameState::updateState() {
	m_componentFlags = 0;
	for(GameStateComponent* c: m_updateComponents) c->update();
}

void GameState::drawState() {
	for(GameStateComponent* c: m_drawComponents) c->draw();
}

void GameState::changeState(GameState* state) {
	m_stateManager->changeState(state);
}

GameState* GameState::previousState() const {
	return m_stateManager->m_prevState;
}


// ------------------------------------------------------------------------------------------- //


GameStateManager::GameStateManager() : m_currentState(0), m_nextState(0), m_prevState(0){}
GameStateManager::~GameStateManager() {}

void GameStateManager::update() {
	if(m_nextState != m_currentState) {
		if(m_prevState && m_prevState!=m_currentState && m_prevState!=m_nextState) {
			m_prevState->m_stateManager = 0;
		}
		m_prevState = m_currentState;
		if(m_currentState) m_currentState->end();
		if(m_nextState) {
			if(m_currentState) for(GameStateComponent* c: m_currentState->m_updateComponents) {
				if(c->getMode() == PERSISTENT) m_nextState->addComponent(c);
			}
			m_nextState->begin();
		}
		m_currentState = m_nextState;
	}
	if(m_currentState) m_currentState->updateState();
}

void GameStateManager::draw() {
	if(m_currentState) m_currentState->drawState();
}

void GameStateManager::changeState(GameState* next) {
	if(next) next->m_stateManager = this;
	m_nextState = next;
}

void GameStateManager::quit() {
	m_nextState = 0;
}

