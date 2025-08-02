#include "base/gamestate.h"
#include "base/game.h"
#include <cstdio>

using namespace base;


GameState::GameState(StateMode mode) : m_transient(mode==TRANSIENT) {
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
		if(*i == c) return; // Already added
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

void GameState::removeComponent(GameStateComponent* c) {
	for(auto i=m_drawComponents.begin(); i!=m_drawComponents.end(); ++i) {
		if(*i ==c) { m_drawComponents.erase(i); break; }
	}
	for(auto i=m_updateComponents.begin(); i!=m_updateComponents.end(); ++i) {
		if(*i ==c) {
			m_updateComponents.erase(i);
			if(--c->m_references==0 && c!=this) delete c;
			break;
		}
	}
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
		if(m_currentState) {
			for(GameStateComponent* c: m_currentState->m_updateComponents) c->end();
		}
		if(m_nextState) {
			if(m_currentState) for(GameStateComponent* c: m_currentState->m_updateComponents) {
				if(c->getMode() == GameStateComponent::PERSISTENT) m_nextState->addComponent(c);
			}
			for(GameStateComponent* c: m_nextState->m_updateComponents) c->begin();
		}
		m_currentState = m_nextState;

		if(m_prevState && m_prevState->m_transient) {
			delete m_prevState;
		}
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

void GameStateManager::onResized(const Point& s) {
	if(m_currentState) for(GameStateComponent* c: m_currentState->m_updateComponents) c->resized(s);
}
void GameStateManager::onFocusChanged(bool focus) {
	if(m_currentState) for(GameStateComponent* c: m_currentState->m_updateComponents) c->focusChanged(focus);
}

void GameStateManager::quit() {
	m_nextState = 0;
}

