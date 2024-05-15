#pragma once

#include <base/hardwarebuffer.h>	// For vertex attribute setup
#include <base/thread.h>
#include <base/math.h>
#include <vector>


namespace particle {

class System;
class Instance;
class Manager;

struct Particle {
	vec3 position;
	vec3 velocity;
	vec3 scale = vec3(1,1,1);
	Quaternion orientation;
	float spawnTime = 0;
	float dieTime = 0;
	float mass = 1;
	uint colour = 0xffffffff;
	uint affectorMask = 0xffffffff;
};


class Object {
	public:
	Object() : m_system(0), m_index(-1) {}
	virtual ~Object() {}
	virtual void trigger(Instance*, Particle&) const {}
	int getDataIndex() const { return m_index; }
	System* getSystem() { return m_system; }
	static inline float random() { return (float)rand()/(float)RAND_MAX; }
	protected:
	private:
	friend class System;
	friend class Instance;
	friend class Emitter;
	System* m_system;
	int m_index;
};

template<typename T>
class ValueGraph {
	public:
	int add(float key, T value);
	T getValue(float key) const;
	static T lerp(T a, T b, float t);
	static constexpr T getDefault();
	struct Pair { float key; T value; };
	std::vector<Pair> data;
	enum { Linear, Stepped, Spline } interpolation = Linear;;
	enum { CONSTANT, CONTINUE, REPEAT, REFLECT } before=CONTINUE, after=CONSTANT;
};
struct LerpColour { float operator()(uint a, uint b, float t) const { return a*(1-t) + b*t; }};
using Graph = ValueGraph<float>;
using Gradient = ValueGraph<uint>;

class Value {
	public:
	enum Type { VALUE, RANDOM, MAP, GRAPH } type;
	private:
	union {
		struct { float min, max; };
		Graph* graph;
	};
	void delGraph() { if(type==GRAPH) delete graph; }
	public:
	float getValue(float key=0) const {
		switch(type) {
		case VALUE:  return max;
		case RANDOM: return Object::random() * (max-min) + min;
		case MAP:    return key*(max-min)+min;
		case GRAPH:  return graph->getValue(key);
		default: return 0;
		}
	}
	Value(float v=0) : type(VALUE), min(v), max(v) {}
	Value(float a, float b) : type(RANDOM), min(a), max(b) {}
	void setValue(float v) { delGraph(); type=VALUE; min=max=v; }
	void setRandom(float v) { delGraph(); type=RANDOM; min=0; max=v; }
	void setRandom(float a, float b) { delGraph(); type=RANDOM; min=a; max=b; }
	void setMapped(float low, float high) { delGraph(); type=MAP; min=low; max=high; }
	void setGraph(Graph&& g) { delGraph(); type=GRAPH; graph=new Graph(std::forward<Graph>(g)); }
	void setGraph(const Graph& g) { delGraph(); type=GRAPH; graph=new Graph(g); }
	const Graph& getGraph() const { if(type==GRAPH) return *graph; static Graph g; return g; }
	vec2 getRange() const { return vec2(min, max); }
	Type getType() const { return type; }
};

class RenderData : public Object {
	public:
	enum Type { TRIANGLES, QUADS, STRIP, POINTS, INSTANCE };
	RenderData(Type type);
	virtual ~RenderData();
	virtual const char* getMaterial() const { return m_material; }
	virtual const char* getInstancedMesh() const { return 0; }
	void setMaterial(const char* name);
	Type getDataType() const { return m_type; }
	int  getVerticesPerParticle() const { return m_verticesPerParticle; }
	const base::VertexAttributes& getVertexAttributes() const { return m_attributes; }

	private:
	friend class Instance;
	void setCount(Instance*, size_t count, int threadCount) const;
	void updateT(int threadIndex, int threadCount, Instance*, int emitterIndex, const Matrix& view) const;
	virtual void setParticleVertices(void* output, const Particle& particle, const Matrix& view) const = 0;

	protected:
	base::VertexAttributes m_attributes;

	private:
	Type m_type;
	int m_verticesPerParticle;
	char* m_material;
};

// Affector base class - modifies particles over time
class Affector : public Object {
	public:
	virtual void update(Particle&, float systemTime, float deltaTime) const = 0;
	void trigger(Instance*, Particle&) const;
	bool startEnabled = true;
};

// Event info - attached to emitters
class Event : public Object{
	public:
	enum class Type { SPAWN, DIE, COLLIDE, TRIGGER, TIME };
	enum class Effect { TRIGGER, ENABLE, DISABLE, TOGGLE };
	Event(Type type) : m_type(type) {}
	Event(Type type, Effect effect, Object* target) : target(target), effect(effect), m_type(type) {}
	Event(float time, Effect effect, Object* target, bool once=false) : target(target), effect(effect), time(time), once(once), m_type(Type::TIME) {}
	Type getType() const { return m_type; }
	public:
	Object* target = nullptr;
	Effect effect = Effect::TRIGGER;
	float time = 1;
	bool once = false;
	bool startEnabled = true;
	protected:
	Type m_type;
	friend class Emitter;
};

// Emitters spawn and update their particles
class Emitter : public Object {
	public:
	Emitter();
	virtual void spawnParticle(Particle&, const Matrix& m, float key) const = 0;
	void update(Instance*, float) const;	// update spawns
	void updateT(int threadIndex, int threadCount, Instance*, float) const;	// update affectors

	void trigger(Instance*, Particle&) const override;	// Spawn from another particle
	void setEnabled(bool);				// start/stop spawning

	void addAffector(Affector*);
	void removeAffector(Affector*);
	const std::vector<Affector*>& affectors() const { return m_affectors; };
	void setRenderer(RenderData* r);
	RenderData* getRenderer() const { return m_renderer; }

	void addEvent(Event*);
	void removeEvent(Event*);
	const std::vector<Event*>& events() const { return m_allEvents; }

	void fireEvent(Instance*, const Event*, Particle&) const;
	void fireEventT(Instance*, const Event*, Particle&, int threadIndex) const;
	void allocateAffectorMasks();

	private:
	Particle& spawnParticle(Instance* instance, const vec3& pos, const Quaternion& orientation, const vec3& velocity, float key, float timeOffset) const;

	public:
	bool   eventOnly=false;	// This emitter can only be triggered by internal events
	bool   startEnabled;	// Start spawning patricles. set to false for triggered particles
	int    limit;			// Limit patricle count from this emitter
	float  inheritVelocity;	// Multiplier to inherit parent particle velocity
	int    spawnCount;		// Particles to spawn at once
	Value  rate;			// Spawn rate
	Value  scale;			// Particle scale
	Value  life;			// Particle life time
	Value  mass;			// Mass affects force affectors
	Gradient colour;		// Initial colour

	protected:
	friend class Instance;
	std::vector<Affector*> m_affectors;
	std::vector<Event*> m_allEvents;
	std::vector<Event*> m_events[5];
	uint m_initialAffectorMask;

	protected:
	RenderData* m_renderer;
};


// Particle system data
class System : public Object {
	public:
	System();
	~System();
	void update();
	void setPoolSize(int);
	int  getPoolSize() const { return m_poolSize; }

	void addEmitter(Emitter*);
	void removeEmitter(Emitter*);
	const std::vector<Emitter*>& emitters() const { return m_emitters; }

	void addRenderer(RenderData*);
	void removeRenderer(RenderData*);
	const std::vector<RenderData*>& renderers() const { return m_renderers; }

	protected:
	std::vector<Emitter*> m_emitters;	// All emitters
	std::vector<RenderData*> m_renderers;	// All Render data

	size_t m_poolSize;
};

// Instance of a particle system
class Instance {
	public:
	Instance(System*);
	virtual ~Instance();
	virtual void initialise();
	virtual void createMaterial(const RenderData*) {}

	void setEnabled(bool);
	bool isEnabled() const { return m_enabled; }
	size_t getParticleCount() const { return m_count; }
	System* getSystem() const { return m_system; }
	float getTime() const { return m_time; }
	void trigger();
	void reset();
	void shift(const vec3&);

	protected:
	void initialiseThreadData(int m_threads);
	Particle& allocate(const Emitter*);
	void freeParticle(Particle&);
	void update(float time);
	void updateT(int threadIndex, int threadCount, float time, const Matrix& view);

	virtual void updateGeometry() = 0;
	virtual const Matrix& getTransform() const = 0;
	virtual vec3 getVelocity() const { return vec3(); }

	protected:
	friend class Emitter;
	friend class RenderData;
	friend class Manager;

	Manager* m_manager;			// System update manager
	System*  m_system;			// Source system data
	size_t   m_count;			// Active particle count
	size_t   m_head;			// Particle pool head
	float    m_time;			// Current time in seconds
	bool     m_enabled;			// Is system enabled

	struct EmitterInstance {
		const Emitter* emitter;
		float accumulator;
		bool enabled;
		std::vector<uint> particles;
	};
	struct RenderInstance {
		const RenderData* render;
		char*  data;		// Raw data
		size_t count;		// Number of particles
		size_t capacity;	// buffer capacity in bytes
		void** head;		// per thread data pointers
		void*  drawable;	// Custom drawable data
	};

	std::vector<RenderInstance>  m_renderers;	// Renderer instance data
	std::vector<EmitterInstance> m_emitters;	// Emitter instance data
	std::vector<int>             m_active;		// List of active emitters
	std::vector<Particle>        m_pool;		// Particle pool

	struct TriggeredEvent { const Event* event; const Emitter* emitter; Particle* particle; };
	struct DestroyMessage { const Emitter* emitter; Particle* particle; size_t index; };
	std::vector<TriggeredEvent>* m_triggered = 0;
	std::vector<DestroyMessage>* m_destroy = 0;
	int m_threads = 0;
};

class Manager {
	public:
	Manager();
	~Manager();
	void startThreads(int count=4);
	void stopThreads();
	int getThreads() const { return m_threads.size(); }
	void update(float time, const Matrix& viewMatrix);
	void add(Instance*, bool enabled=true);
	void remove(Instance*);
	size_t getParticleCount() const;

	std::vector<Instance*>::const_iterator begin() const { return m_instances.begin(); }
	std::vector<Instance*>::const_iterator end() const { return m_instances.end(); }

	protected:
	void threadFunc(int index);
	std::vector<base::Thread> m_threads;
	base::Barrier m_barrier;
	bool m_threadsRunning;
	float m_timeStep;
	Matrix m_viewMatrix;

	std::vector<Instance*> m_instances;
};


}


