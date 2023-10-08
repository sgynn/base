#pragma once

#include <base/variable.h>
#include <base/particles.h>

namespace particle {
	extern void    registerInternalStructures();
	extern System* loadSystem(const script::Variable& data);



	using EnumValues = std::vector<const char*>;
	extern Value loadValue(const script::Variable&);
	extern Gradient loadGradient(const script::Variable&);
	extern script::Variable getValueAsVariable(const Value&);
	extern script::Variable getGradientAsVariable(const Gradient&);
	extern script::Variable getEnumAsVariable(int value, const EnumValues& e);
	extern int loadEnum(const script::Variable&, const EnumValues& e);

	enum class PropertyType { Value, Int, Float, Bool, Vector, String, Enum, Gradient };
	template<typename T>
	struct Property {
		script::VariableName key;
		PropertyType type;
		void(*set)(T*,const script::Variable&);
		script::Variable(*get)(const T*);
		EnumValues enumValues;
	};
	template<class T>
	struct Definition {
		std::vector<Property<T>> properties;
		Definition* parent = 0;
		const char* type = 0;
		T*(*create)() = 0;
	};


	extern base::HashMap<Definition<RenderData>*> s_renderDataFactory;
	extern base::HashMap<Definition<Affector>*> s_affectorFactory;
	extern base::HashMap<Definition<Emitter>*> s_emitterFactory;
	extern base::HashMap<Definition<Event>*> s_eventFactory;

	// createAffectorDefinition<Affector, WindAffector>(s_affectorFactory, "WindAffector", "Affector");
	template<class T, class E> Definition<T>* createDefinition(base::HashMap<Definition<T>*>& factory, const char* name, const char* parent=0) {
		Definition<T>* def = factory[name] = new Definition<T>();
		def->parent = parent? factory.get(parent, 0): 0;
		def->type = name;
		def->create = []()->T*{ return new E(); };
		return def;
	}
};

#define CreateAffectorDefinition(Type, Parent) \
	particle::createDefinition<Affector, Type>(s_affectorFactory, #Type, #Parent);
#define CreateEmitterDefinition(Type, Parent) \
	particle::createDefinition<Emitter, Type>(s_emitterFactory, #Type, #Parent);
#define CreateRenderDataDefinition(Type, Parent) \
	particle::createDefinition<RenderData, Type>(s_renderDataFactory, #Type, #Parent);

// Property is a Value type
#define AddValuePropertyN(Base, Type, def, key, var) \
	def->properties.push_back( {	\
		key, PropertyType::Value, \
		[](Base* a, const Variable& v) { static_cast<Type*>(a)->var = particle::loadValue(v); }, \
		[](const Base* a) { return particle::getValueAsVariable(static_cast<const Type*>(a)->var); } \
	});
#define AddValueProperty(Base, Type, def, var) AddValuePropertyN(Base, Type, def, #var, var);

// Property is a basic variable
#define AddBasicProperty(Base, Type, def, key, var, t) \
	def->properties.push_back( Property<Base>{ \
		key, PropertyType::t, \
		[](Base* a, const Variable& v) { static_cast<Type*>(a)->var = v; }, \
		[](const Base* a)->script::Variable { return static_cast<const Type*>(a)->var; } \
	});

// Property used getValue/setValue accessor functions
#define AddFuncProperty(Base, Type, def, key, Func, t) \
	def->properties.push_back( Property<Base>{ \
		key, PropertyType::t, \
		[](Base* a, const Variable& v) { static_cast<Type*>(a)->set##Func(v); }, \
		[](const Base* a)->script::Variable { return static_cast<const Type*>(a)->get##Func(); } \
	});

// Property is an integer set from enum strings
#define AddEnumPropertyN(Base, Type, def, EnumType, key, var, ...) \
	def->properties.push_back( Property<Base>{ \
		key, PropertyType::Enum, \
		[](Base* a, const Variable& v) { static_cast<Type*>(a)->var = (EnumType)loadEnum(v, {__VA_ARGS__}); }, \
		[](const Base* a)->script::Variable { return getEnumAsVariable((int)static_cast<const Type*>(a)->var, {__VA_ARGS__}); }, \
		{ __VA_ARGS__ } \
	});
#define AddEnumProperty(Base, Type, def, EnumType, var, ...) AddEnumPropertyN(Base, Type, def, EnumType, #var, var, __VA_ARGS__)
	


