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



System* particle::loadSystem(const Variable& data) {
	if(!data.get("emitters")) return nullptr;
	struct EmitterRef  { Variable var; particle::Emitter* emitter; };
	struct ParticleRef { Variable var; particle::RenderData* renderData; };
	struct AffectorRef { Variable var; particle::Affector* affector; };
	struct EventRef    { Variable var; particle::Event* event; };
	std::vector<EmitterRef> emitters;
	std::vector<ParticleRef> renderers;
	std::vector<AffectorRef> affectors;
	std::vector<EventRef> events;

	particle::System* system = new particle::System();
	if(data.contains("pool")) system->setPoolSize(data.get("pool"));
	for(auto& v: data.get("emitters")) {
		const Variable& emitterVar = v.value;
		particle::Emitter* emitter = 0;
		for(const auto& i: emitters) if(i.var==emitterVar) emitter = i.emitter;
		if(!emitter && emitterVar) {
			Definition<Emitter>* def = s_emitterFactory.get(emitterVar.get("type"), 0);
			if(!def || !def->create) {
				printf("Particle error: Undefined emitter type '%s'\n", (const char*)emitterVar.get("type"));
				continue;
			}
			emitter = def->create();

			// Set emitter propertes
			while(def) {
				for(auto& i: def->properties) {
					const Variable& value = emitterVar.get(i.key);
					if(!value.isNull())
						i.set(emitter, value);
				}
				def = def->parent;
			}

			emitters.push_back({emitterVar, emitter});
			system->addEmitter(emitter);

			// Create renderer
			particle::RenderData* renderData = 0;
			const Variable& particleVar = emitterVar.get("particle");
			for(auto& i: renderers) if(i.var == particleVar) renderData = i.renderData;
			if(!renderData && particleVar) {
				Definition<RenderData>* rdef = s_renderDataFactory.get(particleVar.get("type"), 0);
				if(rdef && rdef->create) {
					renderData = rdef->create();
					renderers.push_back({particleVar, renderData});
					system->addRenderer(renderData);
					// Additional properties
					renderData->setMaterial(particleVar.get("material"));
				}
				else printf("Particle error: Undefined render type '%s'\n", (const char*)particleVar.get("type"));
			}
			emitter->setRenderer(renderData);


			// Create affectors
			for(const auto& a: emitterVar.get("affectors")) {
				const Variable& affectorVar = a.value;
				particle::Affector* affector = 0;
				for(auto& i: affectors) if(i.var == affectorVar) affector = i.affector;
				if(!affector && affectorVar) {
					Definition<Affector>* adef = s_affectorFactory.get(affectorVar.get("type"), 0);
					if(adef && adef->create) {
						affector = adef->create();
						while(adef) {
							for(auto& i: adef->properties) {
								const Variable& value = affectorVar.get(i.key);
								if(!value.isNull()) i.set(affector, value);
							}
							adef = adef->parent;
						}
						affectors.push_back({affectorVar, affector});
					}
					else printf("Particle error: Undefined affector type '%s'\n", (const char*)affectorVar.get("type"));
				}
				if(affector) emitter->addAffector(affector);
			}

			// Create events
			for(const auto& e: emitterVar.get("events")) {
				const Variable& eventVar = e.value;
				particle::Event* event = nullptr;
				for(auto& i: events) if(i.var == eventVar) event = i.event;
				if(!event && eventVar.get("target")) {
					Definition<Event>* edef = s_eventFactory.get(eventVar.get("type"), 0);
					if(edef && edef->create) {
						event = edef->create();
						for(auto& i: edef->properties) {
							const Variable& value = eventVar.get(i.key);
							if(!value.isNull()) i.set(event, value);
						}
						events.push_back({eventVar, event});
					}
					else printf("Particle error: Undefined event type '%s'\n", (const char*)eventVar.get("type"));
				}
				if(event) emitter->addEvent(event);
			}
		}
	}
	
	// Link event targets
	for(auto& event: events) {
		const Variable& target = event.var.get_const("target");
		for(auto& e: emitters)  if(e.var == target) event.event->target = e.emitter;
		for(auto& a: affectors) if(a.var == target) event.event->target = a.affector;
		for(auto& e: events)    if(e.var == target) event.event->target = e.event;
	}


	return system;
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


// ================================================================== //


void particle::registerInternalStructures() {
	{
	auto def = s_eventFactory["Timer"] = new Definition<Event>{{}, 0, "Timer", [](){return new Event(Event::Type::TIME);}};
	AddBasicProperty(Event, Event, def, "time", time, Float);
	AddBasicProperty(Event, Event, def, "once", once, Bool);
	AddBasicProperty(Event, Event, def, "enabled", startEnabled, Bool);
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

	CreateAffectorDefinition(OrientToVelocity, Affector);

	{
	auto def = CreateAffectorDefinition(Colourise, Affector);
	def->properties.push_back({"colour", PropertyType::Gradient,
		[](Affector* a, const Variable& v) { static_cast<Colourise*>(a)->colour = loadGradient(v); },
		[](const Affector* a) { return getGradientAsVariable(static_cast<const Colourise*>(a)->colour); }
	});
	}

	// ====================================================================================== //


	{
	auto def = s_renderDataFactory["RenderData"] = new Definition<RenderData>{{}, 0, "RenderData"};
	AddFuncProperty(RenderData, RenderData, def, "material", Material, String);
	}
	
	CreateRenderDataDefinition(SpriteRenderer,RenderData);

	{
	auto def = CreateRenderDataDefinition(InstanceRenderer,RenderData);
	AddFuncProperty(RenderData, InstanceRenderer, def, "mesh", InstancedMesh, String);
	}

	CreateRenderDataDefinition(RibbonRenderer,RenderData);
	


}



