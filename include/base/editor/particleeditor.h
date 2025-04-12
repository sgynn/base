#pragma once

#include <base/gui/gui.h>
#include "editor.h"

namespace particle { class Manager; class Object; class System; class Emitter; class Affector; class RenderData; template<class T>struct Definition; }
namespace nodegraph { class NodeEditor; class Node; struct Link; }
namespace gui { class Button; class Popup; }
namespace script { class Variable; }

namespace editor {
class ParticleNode;
class GradientBox;
class ParticleEditor;

class ParticleEditorComponent : public EditorComponent {
	public:
	void initialise() override;
	void setParticleManager(particle::Manager*);
	ParticleEditor* showParticleSystem(particle::System*, const char* name=0);
	particle::Manager* getParticleManager() { return m_manager; }
	void saveAll();
	public:
	void assetCreationActions(AssetCreationBuilder&) override;
	bool assetActions(gui::MenuBuilder& menu, const Asset&) override;
	gui::Widget* openAsset(const Asset&) override;
	protected:
	bool drop(gui::Widget* target, const Point& p, const Asset&, bool) override;
	protected:
	gui::Widget* m_panel = nullptr;
	particle::Manager* m_manager = nullptr;
	std::vector<ParticleEditor*> m_editors;
	friend class ParticleEditor;
};



class ParticleEditor {
	public:
	enum NodeType { SystemNode, EmitterNode, AffectorNode, RenderDataNode, EventNode };
	~ParticleEditor();

	void setParticleManager(particle::Manager*);
	void setParticleSystem(particle::System*, const char* name=0);
	particle::System* getParticleSystem() const { return m_system; }
	gui::Widget* getPanel() { return m_panel; }

	static bool save(particle::System* system, const char* file);

	bool drop(gui::Widget*, const Point& p, const Asset&, bool);
	bool isModified() const { return m_modified; }

	protected:
	friend class ParticleEditorComponent;
	ParticleEditor(ParticleEditorComponent* parent, particle::System* system, gui::Widget* panel, const char* name);
	ParticleEditorComponent* m_parent;
	gui::Widget* m_panel;
	bool m_modified = false;
	particle::Manager* m_manager = nullptr;
	particle::System* m_system = nullptr;
	nodegraph::NodeEditor* m_graph = nullptr;
	gui::Popup* m_modePopup = nullptr;
	gui::Widget* m_gradientEditor = nullptr;
	gui::Widget* m_graphEditor = nullptr;


	void notifyChange();
	void linked(const nodegraph::Link&);
	void unlinked(const nodegraph::Link&);
	void deleteNode(nodegraph::Node*);
	void reinitialise();

	void createOrLinkNode(ParticleNode* from, particle::Object* to, NodeType type, Point& pos);
	void createNodeFromDrag(gui::Widget* w, const Point& pos, int b);
	ParticleNode* createGraphNode(NodeType type, const char* className, particle::Object* data);
	void selectNode(gui::Button*);
	template<class T> void createPropertiesPanel(particle::Definition<T>* def, T* item);
	void propertyModeChanged(gui::Button*);
	void showGradientEditor(editor::GradientBox* target);
	void applyGradient();

	void showGraphEditor(const script::Variable&);

	enum class ValueType { Constant, Random, Mapped, Graph };
	Delegate<void(ValueType)> m_changeTypeDelegate;
	Delegate<void(const script::Variable&)> m_changeValueDelegate;
	static script::Variable convertValueType(ValueType newType, const script::Variable& data);
	static ValueType getValueType(const script::Variable& value);

};
}
