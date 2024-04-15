#include <base/particledef.h>
#include "emitters.h"
#include "affectors.h"
#include "renderers.h"

using namespace particle;
using script::Variable;

base::HashMap<Definition<RenderData>*> particle::s_renderDataFactory;
base::HashMap<Definition<Affector>*> particle::s_affectorFactory;
base::HashMap<Definition<Emitter>*> particle::s_emitterFactory;
base::HashMap<Definition<Event>*> particle::s_eventFactory;

class ParticleSystemLoader {
	enum Type { EMITTER, AFFECTOR, RENDERER, EVENT };
	struct ObjectRef { Variable var; Object* object; int type; };
	std::vector<ObjectRef> m_objects;
	template<class T>
	T* getObject(const Variable& var, Type type) const {
		for(const ObjectRef& o: m_objects) if(o.var==var) return static_cast<T*>(o.object);
		return nullptr;
	}
	template<class T>
	static void loadProperties(T* object, const Definition<T>* def, const Variable& data) {
		while(def) {
			for(auto& i: def->properties) {
				const Variable& value = data.get(i.key);
				if(!value.isNull()) i.set(object, value);
			}
			def = def->parent;
		}
	}

	public:
	System* load(const Variable& data) {
		if(!data.get("emitters")) return nullptr;
		particle::System* system = new particle::System();
		if(data.contains("pool")) system->setPoolSize(data.get("pool"));
		for(const Variable& ev: data.get("emitters")) {
			if(Emitter* e = loadObject<Emitter>(ev, EMITTER, s_emitterFactory)) {
				system->addEmitter(e);
			}
		}
		// Resolve any floating objects
		for(ObjectRef& o: m_objects) {
			if(!o.object->getSystem()) {
				switch(o.type) {
				default: break;
				case EMITTER:
					static_cast<Emitter*>(o.object)->eventOnly = true;
					system->addEmitter(static_cast<Emitter*>(o.object));
					break;
				case EVENT:
					if(!static_cast<Event*>(o.object)->target) delete o.object; // unused
					break;
				}
			}
		}

		m_objects.clear();
		return system;
	}
	
	template<class T> void loadReferences(T* object, const Variable& data) {}
	void loadReferences(Emitter* emitter, const Variable& data) {
		emitter->setRenderer(loadObject<RenderData>(data.get("particle"), RENDERER, s_renderDataFactory));
		for(Variable& var: data.get("affectors")) {
			if(Affector* a = loadObject<Affector>(var, AFFECTOR, s_affectorFactory)) {
				emitter->addAffector(a);
			}
		}
		for(Variable& var: data.get("events")) {
			if(Event* e = loadObject<Event>(var, EVENT, s_eventFactory)) {
				if(e->target) emitter->addEvent(e);
			}
		}
	}
	void loadReferences(Event* event, const Variable& var) {
		const Variable& target = var.get("target");
		const char* targetType = target.get("type");
		if(targetType) {
			if(s_emitterFactory.contains(targetType)) event->target = loadObject<Emitter>(target, EMITTER, s_emitterFactory);
			else if(s_eventFactory.contains(targetType)) event->target = loadObject<Event>(target, EVENT, s_eventFactory);
			else if(s_affectorFactory.contains(targetType)) event->target = loadObject<Affector>(target, AFFECTOR, s_affectorFactory);
		}
		if(!event->target) printf("Particle Error: Event has no target\n");
	}


	template<class T>
	T* loadObject(const Variable& data, Type type, const base::HashMap<Definition<T>*>& factory) {
		if(data.isNull()) return nullptr;
		T* object = getObject<T>(data, type);
		if(!object) {
			const Definition<T>* def = factory.get(data.get("type"), nullptr);
			if(!def || !def->create) {
				constexpr const char* types[] = { "emitter", "affector", "renderer", "event" };
				printf("Particle error: Undefined %s type '%s'\n", types[type], (const char*)data.get("type"));
				return nullptr;
			}
			object = def->create();
			m_objects.push_back({data, object, type});
			loadProperties(object, def, data);
			loadReferences(object, data);
		}
		return object;
	}
};


System* particle::loadSystem(const Variable& data) {
	return ParticleSystemLoader().load(data);
}

Value particle::loadValue(const Variable& var) {
	if(var.isNumber()) return Value(var);
	Value value;
	if(var.isVector()) value.setRandom(var.get("x"), var.get("y"));
	else if(var.isArray() && var.size()==2 && var.get(0u).isNumber() && var.get(1u).isNumber()) value.setMapped(var.get(0u), var.get(1u));
	else if(var.isArray()) {
		// ToDo: Load graph
		Graph g;
		for(auto& i: var) {
			vec2 v = i.value;
			g.add(v.x, v.y);
		}
		value.setGraph(g);
	}
	return value;
}
Variable particle::getValueAsVariable(const Value& value) {
	switch(value.getType()) {
	case Value::VALUE: return value.getRange().x;
	case Value::RANDOM: return value.getRange();
	case Value::MAP:
		{
			Variable r;
			r.makeArray();
			r.set(0u, value.getRange().x);
			r.set(1u, value.getRange().y);
			return r;
		}
	case Value::GRAPH:
		{
			Variable r;
			r.makeArray();
			uint index = 0;
			for(auto& i: value.getGraph().data) r.set(index++, vec2(i.key, i.value));
			return r;
		}
	}
	return Variable();
}

Gradient particle::loadGradient(const Variable& var) {
	Gradient gradient;
	if(var.isNumber()) gradient.add(0, var);
	else if(var.isArray()) {
		int index = 0;
		float end = var.size() - 1;
		for(auto& i: var) {
			if(i.value.isVector()) gradient.add(i.value.get("x"), i.value.get("y"));
			else if(i.value.isArray()) gradient.add(i.value.get(0u), i.value.get(1u));
			else if(i.value.isNumber()) gradient.add(index/end, i.value);
			++index;
		}
	}
	return gradient;
}
Variable particle::getGradientAsVariable(const Gradient& gradient) {
	if(gradient.data.size()<=1) return gradient.getValue(0);
	Variable r;
	r.makeArray();
	if(gradient.data.size()==2 && gradient.data[0].key==0 && gradient.data[1].key==1) {
		r.set(0u, gradient.data[0].value);
		r.set(1u, gradient.data[1].value);
		return r;
	}

	uint index = 0;
	for(auto& i: gradient.data) {
		r.get(index).makeArray();
		r.get(index).set(0u, i.key);
		r.get(index).set(1u, i.value);
		++index;
	}
	return r;
}

Variable particle::getEnumAsVariable(int value, const EnumValues& e) {
	if(value >= 0 && value < (int)e.size()) return e[value];
	else return value;

}
int particle::loadEnum(const Variable& v, const EnumValues& e) {
	if(v.isNumber()) return(int)v;
	if(const char* name = v) {
		for(size_t i=0; i<e.size(); ++i) if(strcmp(name, e[i])==0) return(int)i;
	}
	return 0;
}

// ================================================================== //


void particle::registerInternalStructures() {
	{
	auto def = s_eventFactory["Timer"] = new Definition<Event>{{}, 0, "Timer", [](){return new Event(Event::Type::TIME);}};
	AddBasicProperty(Event, Event, def, "time", time, Float);
	AddBasicProperty(Event, Event, def, "once", once, Bool);
	AddBasicProperty(Event, Event, def, "enabled", startEnabled, Bool);
	AddEnumProperty(Event, Event, def, Event::Effect, effect, "Trigger", "Enable", "Disable", "Toggle");
	s_eventFactory["Spawn"] = new Definition<Event>{{}, 0, "Spawn", [](){return new Event(Event::Type::SPAWN);}};
	s_eventFactory["Death"] = new Definition<Event>{{}, 0, "Death", [](){return new Event(Event::Type::DIE);}};
	}

	// ====================================================================================== //
	{
	auto def = s_emitterFactory["Emitter"] = new Definition<Emitter>{{}, 0, "Emitter"};
	AddBasicProperty(Emitter, Emitter, def, "start", startEnabled, Bool);
	AddBasicProperty(Emitter, Emitter, def, "limit", limit, Int);
	AddBasicProperty(Emitter, Emitter, def, "count", spawnCount, Int);
	AddBasicProperty(Emitter, Emitter, def, "inheritVelocity", inheritVelocity, Float);
	AddValueProperty(Emitter, Emitter, def, rate);
	AddValueProperty(Emitter, Emitter, def, scale);
	AddValueProperty(Emitter, Emitter, def, life);
	AddValueProperty(Emitter, Emitter, def, mass);

	def->properties.push_back({"colour", PropertyType::Gradient,
		[](Emitter* e, const Variable& v) { e->colour = loadGradient(v); },
		[](const Emitter* e) { return getGradientAsVariable(e->colour); }
	});
	}

	{
	auto def = CreateEmitterDefinition(PointEmitter, Emitter);
	AddValueProperty(Emitter, PointEmitter, def, cone);
	AddValueProperty(Emitter, PointEmitter, def, velocity);
	}
	{
	auto def = CreateEmitterDefinition(SphereEmitter, PointEmitter);
	AddValueProperty(Emitter, SphereEmitter, def, radius);
	}
	{
	auto def = CreateEmitterDefinition(SphereSurfaceEmitter, Emitter);
	AddValueProperty(Emitter, SphereSurfaceEmitter, def, radius);
	AddValueProperty(Emitter, SphereSurfaceEmitter, def, velocity);
	}
	{
	auto def = CreateEmitterDefinition(BoxEmitter, Emitter);
	AddBasicProperty(Emitter, BoxEmitter, def, "size", size, Vector);
	AddValueProperty(Emitter, BoxEmitter, def, velocity.x);
	AddValueProperty(Emitter, BoxEmitter, def, velocity.y);
	AddValueProperty(Emitter, BoxEmitter, def, velocity.z);
	}
	{
	auto def = CreateEmitterDefinition(RingEmitter, Emitter);
	AddValueProperty(Emitter, RingEmitter, def, sequence);
	AddValueProperty(Emitter, RingEmitter, def, radius);
	AddValueProperty(Emitter, RingEmitter, def, angle);
	AddValueProperty(Emitter, RingEmitter, def, tangent);
	AddValueProperty(Emitter, RingEmitter, def, velocity);
	}
	//CreateEmitterDefinition(MeshVertexEmitter, Emitter);
	//CreateEmitterDefinition(MeshFaceEmitter, Emitter);

	// ====================================================================================== //
	
	{
	auto def = CreateAffectorDefinition(LinearForce, Affector);
	AddValueProperty(Affector, LinearForce, def, force.x);
	AddValueProperty(Affector, LinearForce, def, force.y);
	AddValueProperty(Affector, LinearForce, def, force.z);
	}

	{
	auto def = CreateAffectorDefinition(SetVelocity, Affector);
	AddValueProperty(Affector, SetVelocity, def, velocity.x);
	AddValueProperty(Affector, SetVelocity, def, velocity.y);
	AddValueProperty(Affector, SetVelocity, def, velocity.z);
	}

	{
	auto def = CreateAffectorDefinition(DragForce, Affector);
	AddValueProperty(Affector, DragForce, def, amount);
	}

	{
	auto def = CreateAffectorDefinition(Vortex, Affector);
	AddValueProperty(Affector, Vortex, def, rotation);
	AddBasicProperty(Affector, Vortex, def, "centre", centre, Vector);
	AddBasicProperty(Affector, Vortex, def, "axis", axis, Vector);
	}

	{
	auto def = CreateAffectorDefinition(Attractor, Affector);
	AddValueProperty(Affector, Attractor, def, strength);
	AddBasicProperty(Affector, Attractor, def, "centre", centre, Vector);
	}

	{
	auto def = CreateAffectorDefinition(UniformScale, Affector);
	AddValueProperty(Affector, UniformScale, def, scale);
	}

	{
	auto def = CreateAffectorDefinition(Rotator2D, Affector);
	AddValueProperty(Affector, Rotator2D, def, amount);
	}

	{
	auto def = CreateAffectorDefinition(SetDirection, Affector);
	AddValueProperty(Affector, SetDirection, def, direction.x);
	AddValueProperty(Affector, SetDirection, def, direction.y);
	AddValueProperty(Affector, SetDirection, def, direction.z);
	}

	CreateAffectorDefinition(OrientToVelocity, Affector);

	{
	auto def = CreateAffectorDefinition(Colourise, Affector);
	def->properties.push_back({"colour", PropertyType::Gradient,
		[](Affector* a, const Variable& v) { static_cast<Colourise*>(a)->colour = loadGradient(v); },
		[](const Affector* a) { return getGradientAsVariable(static_cast<const Colourise*>(a)->colour); }
	});
	//AddEnumProperty(Affector, Colourise, def, Colourise::BlendMode, blend, "Set", "Add", "Multiply", "Alpha");
	}

	// ====================================================================================== //


	{
	auto def = s_renderDataFactory["RenderData"] = new Definition<RenderData>{{}, 0, "RenderData"};
	AddFuncProperty(RenderData, RenderData, def, "material", Material, String);
	}
	
	CreateRenderDataDefinition(SpriteRendererQuads, RenderData);
	CreateRenderDataDefinition(SpriteRenderer, RenderData);
	CreateRenderDataDefinition(QuadRenderer, RenderData);

	{
	auto def = CreateRenderDataDefinition(InstanceRenderer,RenderData);
	AddFuncProperty(RenderData, InstanceRenderer, def, "mesh", InstancedMesh, String);
	}

	CreateRenderDataDefinition(RibbonRenderer,RenderData);
	


}



