#include <base/particles.h>

#ifndef EMSCRIPTEN
#define assert(x) if(!(x)) asm("int $3\nnop");
#else
#define assert(x)
#endif

using namespace particle;

template<class T>
inline void eraseShift(std::vector<T>& list, T& item) {
	for(size_t i=0; i<list.size(); ++i) {
		if(list[i]==item) {
			list.erase(list.begin()+i);
		}
	}
}
template<class T>
inline void eraseFast(std::vector<T>& list, T& item) {
	for(size_t i=0; i<list.size(); ++i) {
		if(list[i]==item) {
			list[i]=list.back();
			list.pop_back();
		}
	}
}


// ================================================================================= //

#ifndef EMSCRIPTEN
template<> constexpr float ValueGraph<float>::getDefault() { return 0.f; }
template<> constexpr uint ValueGraph<uint>::getDefault() { return 0xffffffffu; }
#else
template<> float ValueGraph<float>::getDefault() { return 0.f; }
template<> uint ValueGraph<uint>::getDefault() { return 0xffffffffu; }
#endif

template<>
inline float ValueGraph<float>::lerp(float a, float b, float t) { return a*(1-t) + b*t; }
template<>
inline uint ValueGraph<uint>::lerp(uint a, uint b, float t) {
	union { uint v; unsigned char c[4]; } ta, tb;
	ta.v = a;
	tb.v = b;
	float s = 1 - t;
	ta.c[0] = s*ta.c[0] + t*tb.c[0];
	ta.c[1] = s*ta.c[1] + t*tb.c[1];
	ta.c[2] = s*ta.c[2] + t*tb.c[2];
	ta.c[3] = s*ta.c[3] + t*tb.c[3];
	return ta.v;
}


template<class T>
int ValueGraph<T>::add(float key, T value) {
	int r = data.size();
	if(data.empty() || key > data.back().key) data.push_back(Pair{key, value});
	else for(size_t i=0; i<data.size(); ++i) {
		if(data[i].key < key) continue;
		if(data[i].key==key) data[i].value = value;
		else data.insert(data.begin()+i, Pair{key, value});
		return i;
	}
	return r;
}

template<class T>
T ValueGraph<T>::getValue(float key) const {
	if(data.empty()) return getDefault();
	if(data.size()==1) return data[0].value;
	if(key <= data[0].key) return data[0].value;
	if(key >= data.back().key) return data.back().value;
	size_t a = 0, b = data.size() - 1;
	while(b > a + 1) {
		size_t c = (a + b) / 2;
		if(data[c].key > key) b = c;
		else a = c;
	}
	float t = (key - data[a].key) / (data[a + 1].key-data[a].key);
	return lerp(data[a].value, data[a+1].value, t);
}

namespace particle {
	template class ValueGraph<float>;
	template class ValueGraph<uint>;
}

// ================================================================================= //

Emitter::Emitter() : startEnabled(true), limit(1000), inheritVelocity(1), spawnCount(1)
	, rate(10), scale(1), life(1)
	, m_renderer(0) {
}
void Emitter::setRenderer(RenderData* r) {
	if(r == m_renderer) return;
	if(getSystem()) {
		if(m_renderer) getSystem()->removeRenderer(m_renderer);
		if(r) getSystem()->addRenderer(r);
	}
	m_renderer = r;
}

void Emitter::update(Instance* instance, float time) const {
	assert((uint)getDataIndex() < instance->m_emitters.size());
	Instance::EmitterInstance& data = instance->m_emitters[getDataIndex()];
	data.accumulator += time;
	float currentRate = rate.getValue(instance->m_time);
	if(currentRate<=0) return;
	int num = data.accumulator * currentRate;
	data.accumulator -= num/currentRate;
	for(int i=0; i<num; ++i) {
		float start = instance->m_time - i/currentRate;
		for(int j=0; j<spawnCount; ++j) {
			Particle& n = instance->allocate(this);
			n.position = vec3(&instance->getTransform()[12]);
			n.orientation.fromMatrix(instance->getTransform());
			n.velocity = instance->getVelocity();
			n.scale = vec3(scale.getValue(instance->m_time));
			n.spawnTime = start;
			n.dieTime = n.spawnTime + life.getValue(instance->m_time);
			n.colour = 0xffffff;
			n.affectorMask = m_initialAffectorMask;
			spawnParticle(n, instance->getTransform(), instance->m_time);
			for(Event* event: m_events[(int)Event::Type::SPAWN]) fireEvent(instance, event, n);
		}
	}
}

void Emitter::updateT(int thread, int count, Instance* instance, float time) const {
	assert((uint)getDataIndex() < instance->m_emitters.size());
	std::vector<uint>& data = instance->m_emitters[getDataIndex()].particles;
	for(size_t i=thread; i<data.size(); i+=count) {
		Particle& particle = instance->m_pool[ data[i] ];
		for(const Affector* a: m_affectors) {
			if(particle.affectorMask & a->getDataIndex())
				a->update(particle, instance->m_time, time);
		}
		particle.position += particle.velocity * time;
		if(particle.dieTime < instance->m_time) instance->m_destroy[thread].push_back( Instance::DestroyMessage{this, &particle, i} );

		for(const Event* event: m_events[(int)Event::Type::TIME]) {
			float t = instance->m_time - particle.spawnTime;
			if(!event->once) t -= floor(t/event->time)*event->time;
			if(t < event->time && t+time >= event->time) fireEvent(instance, event, particle);
		}
	}
}

void Emitter::addAffector(Affector* e) {
	m_affectors.push_back(e);
	allocateAffectorMasks();
}
void Emitter::removeAffector(Affector* e) {
	eraseShift(m_affectors, e);
}
void Emitter::addEvent(Event* e) {
	m_events[(int)e->m_type].push_back(e);
	m_allEvents.push_back(e);
	allocateAffectorMasks();
}
void Emitter::removeEvent(Event* e) {
	eraseShift(m_events[(int)e->m_type], e);
	eraseShift(m_allEvents, e);
}
void Emitter::allocateAffectorMasks() {
	m_initialAffectorMask = 0;
	uint used = 0;
	struct Item { Object* object; bool enabled; };
	std::vector<Item> toAllocate;
	for(Affector* a: m_affectors) {
		if(a->startEnabled) m_initialAffectorMask |= a->getDataIndex();
		if(a->getDataIndex()) used |= a->getDataIndex();
		else toAllocate.push_back({a, a->startEnabled});
	}
	for(Event* e: m_allEvents) {
		if(e->startEnabled) m_initialAffectorMask |= e->getDataIndex();
		if(e->getDataIndex()) used |= e->getDataIndex();
		else toAllocate.push_back({e, e->startEnabled});
	}
	for(Item& e: toAllocate) {
		for(int i=0; i<32; ++i) {
			if(~used & 1<<i) {
				e.object->m_index = (1<<i) | 0x80000000u;
				if(e.enabled) m_initialAffectorMask |= 1<<i;
				break;
			}
		}
	}
}

void Emitter::fireEvent(Instance* instance, const Event* e, Particle& p) const {
	switch(e->effect) {
	case Event::Effect::TRIGGER:
		e->target->trigger(instance, p);
		break;
	case Event::Effect::ENABLE:
		if(e->target->getDataIndex() & 0x80000000) p.affectorMask |= e->target->getDataIndex();
		else instance->m_emitters[e->target->getDataIndex()].enabled = true;
		break;

	case Event::Effect::DISABLE:
		if(e->target->getDataIndex() & 0x80000000) p.affectorMask &= ~e->target->getDataIndex();
		else instance->m_emitters[e->target->getDataIndex()].enabled = false;
		break;
	case Event::Effect::TOGGLE:
		if(e->target->getDataIndex() & 0x80000000) p.affectorMask ^= e->target->getDataIndex();
		else instance->m_emitters[e->target->getDataIndex()].enabled ^= true;
		break;
	}
}

void Emitter::fireEventT(Instance* instance, const Event* e, Particle& p, int thread) const {
	instance->m_triggered[thread].push_back(Instance::TriggeredEvent{e, this, &p});
}

void Emitter::trigger(Instance* instance, Particle& p) const {
	// Spawn a particle here
	Matrix m;
	p.orientation.toMatrix(m);
	m.setTranslation(p.position);
	for(int i=0; i<spawnCount; ++i) {
		Particle& n = instance->allocate(this);
		n.position = p.position;
		n.orientation = p.orientation;
		n.velocity.set(0,0,0);
		n.scale = vec3(scale.getValue(0)); // ToDo - what input ?
		n.spawnTime = instance->m_time;
		n.dieTime = n.spawnTime + life.getValue(0);
		n.colour = 0xffffff;
		n.affectorMask = m_initialAffectorMask;
		spawnParticle(n, m, instance->m_time);
		n.velocity += p.velocity * inheritVelocity;
	}
}

void Affector::trigger(Instance* inst, Particle& p) const {
	update(p, inst->getTime(), 1.f);
}

// ===================================================================================== //

RenderData::RenderData(Type type) : m_type(type), m_material(0) {
	static const int vpp[] = { 6,4,2,1,1 };
	m_verticesPerParticle = vpp[type];
}

RenderData::~RenderData() {
	free(m_material);
}

void RenderData::setMaterial(const char* m) {
	m_material = strdup(m);
}

void RenderData::setCount(Instance* instance, size_t count, int threads) const {
	assert((uint)getDataIndex() < instance->m_renderers.size());
	Instance::RenderInstance& data = instance->m_renderers[getDataIndex()];
	size_t particleSize = m_attributes.getStride() * m_verticesPerParticle;
	if(count * particleSize > data.capacity) {
		delete [] data.data;
		data.capacity = particleSize * (count + 16);
		data.data = new char[data.capacity];
	}
	for(int i=0; i<threads; ++i) data.head[i] = data.data + particleSize * i;
	data.count = count;
}

void RenderData::updateT(int threadIndex, int threadCount, Instance* instance, int emitter, const Matrix& view) const {
	const std::vector<uint>& indices = instance->m_emitters[emitter].particles;
	Instance::RenderInstance& data = instance->m_renderers[getDataIndex()];
	size_t step = m_attributes.getStride() * m_verticesPerParticle * threadCount;
	char* ptr = (char*)data.head[threadIndex];
	for(size_t i=threadIndex; i<indices.size(); i+=threadCount) {
		assert((size_t)ptr - (size_t)data.data < data.capacity);
		setParticleVertices(ptr, instance->m_pool[indices[i]], view);
		ptr += step;
	}
	data.head[threadIndex] = ptr;
}




// ===================================================================================== //

System::System() : m_poolSize(500) {
}
System::~System() {
	for(Emitter* e: m_emitters) delete e;
}
void System::setPoolSize(int size) {
	m_poolSize = size;
}
void System::addEmitter(Emitter* e) {
	assert(!e->m_system);
	e->m_index = m_emitters.size();
	e->m_system = this;
	m_emitters.push_back(e);
	if(e->getRenderer() && e->getRenderer()->m_index<0) addRenderer(e->getRenderer());
}
void System::removeEmitter(Emitter* e) {
	assert(e->m_system == this);
	eraseShift(m_emitters, e);
	for(size_t i=0; i<m_emitters.size(); ++i) m_emitters[i]->m_index = i;
	e->m_index = -1;
	e->m_system = 0;
	if(e->getRenderer()) removeRenderer(e->getRenderer());
}

void System::addRenderer(RenderData* r) {
	assert(!r->m_system || r->m_system==this);
	if(r->m_system != this) {
		r->m_index = m_renderers.size();
		r->m_system = this;
		m_renderers.push_back(r);
	}
}

void System::removeRenderer(RenderData* r) {
	assert(r->m_system==this);
	for(Emitter* e: m_emitters) if(e->getRenderer()==r) return; // Still referenced
	eraseFast(m_renderers, r);
	for(size_t i=0; i<m_renderers.size(); ++i) m_renderers[i]->m_index = i;
	r->m_index = -1;
	r->m_system = 0;
	for(Emitter* e: m_emitters) if(e->getRenderer()==r) e->setRenderer(0);
}

// ===================================================================================== //


Instance::Instance(System* sys)
	: m_manager(0), m_system(sys)
	, m_count(0), m_head(0), m_time(0), m_enabled(false)
	, m_triggered(0), m_destroy(0)
{
}

Instance::~Instance() {
	if(m_manager) m_manager->remove(this);
	for(RenderInstance& r: m_renderers) delete [] r.head;
	delete [] m_triggered;
	delete [] m_destroy;
}

void Instance::initialise() {
	if(!m_system) return;
	Particle tmp { vec3(), vec3(), vec3(1,1,1), Quaternion(), 0, 0, 0xffffffff };
	m_pool.resize(m_system->getPoolSize(), tmp);
	if(m_count) for(Particle& p: m_pool) p.spawnTime=0;
	m_count = 0;
	m_head = 0;
	m_active.clear();
	m_emitters.clear();
	m_emitters.reserve(m_system->emitters().size());
	for(Emitter* e: m_system->emitters()) {
		m_emitters.push_back(EmitterInstance{e, 0.f, e->startEnabled});
		if(e->startEnabled) m_active.push_back(e->m_index);
	}
	for(RenderInstance& r: m_renderers) delete [] r.head;
	m_renderers.clear();
	for(RenderData* r: m_system->renderers()) {
		m_renderers.push_back(RenderInstance{r, 0, 0, 0, 0});
		if(m_threads) m_renderers.back().head = new void*[m_threads];
	}
	for(int i=0; i<m_threads; ++i) {
		m_triggered[i].clear();
		m_destroy[i].clear();
	}
}

void Instance::trigger() {
	if(!m_enabled) return;
	Particle n;
	n.position = vec3(&getTransform()[12]);
	n.orientation.fromMatrix(getTransform());
	n.velocity = getVelocity();
	for(EmitterInstance& e: m_emitters) e.emitter->trigger(this, n);
}

void Instance::update(float time) {
	if(!m_enabled) return;
	// Need to reset head pointers if paused
	if(time==0) {
		for(RenderInstance& r: m_renderers) r.render->setCount(this, r.count, m_threads);
		return;
	}

	m_time += time;

	// Update spawns
	for(EmitterInstance& e: m_emitters) {
		if(e.enabled) e.emitter->update(this, time);
	}

	// Fire threaded events
	for(int i=0; i<m_threads; ++i) {
		for(const TriggeredEvent& e: m_triggered[i]) {
			e.emitter->fireEvent(this, e.event, *e.particle);
		}
		m_triggered[i].clear();

		for(const DestroyMessage& d: m_destroy[i]) {
			EmitterInstance& e = m_emitters[d.emitter->m_index];
			if(d.index < e.particles.size()) {
				for(const Event* event: e.emitter->m_events[(int)Event::Type::DIE]) e.emitter->fireEvent(this, event, *d.particle);
				freeParticle(m_pool[e.particles[d.index]]);
				e.particles[d.index] = e.particles.back();
				e.particles.pop_back();
			}
		}
		m_destroy[i].clear();
	}


	// Update renderer particle counts
	for(RenderInstance& r: m_renderers) r.count = 0;
	for(EmitterInstance& e: m_emitters) {
		if(e.emitter->getRenderer()) m_renderers[e.emitter->getRenderer()->m_index].count += e.particles.size();
	}
	for(RenderInstance& r: m_renderers) r.render->setCount(this, r.count, m_threads);
}

void Instance::updateT(int thread, int count, float time, const Matrix& view) {
	if(time>0) for(Emitter* e: m_system->emitters()) e->updateT(thread, count, this, time);
	// Update geometry
	for(Emitter* e: m_system->emitters()) {
		if(e->getRenderer()) e->getRenderer()->updateT(thread, count, this, e->getDataIndex(), view);
	}
}

Particle& Instance::allocate(const Emitter* emitter) {
	if(m_count == m_pool.size()) return m_pool[0];
	EmitterInstance& data = m_emitters[emitter->m_index];

	if((int)data.particles.size() >= emitter->limit) return m_pool[0];
	
	++m_head%=m_pool.size();
	if(m_pool[m_head].spawnTime == 0) {
		data.particles.push_back(m_head);
		++m_count;
		return m_pool[m_head];
	}

	/* // this allocater is slow
	while(++m_head < m_pool.size()) {
		if(m_pool[m_head].spawnTime == 0) {
			data.particles.push_back(m_head);
			++m_count;
			return m_pool[m_head];
		}
	}
	m_head = 0;
	while(++m_head < m_pool.size()) {
		if(m_pool[m_head].spawnTime == 0) {
			data.particles.push_back(m_head);
			++m_count;
			return m_pool[m_head];
		}
	}
	*/
	return m_pool[0]; // should never get here
}

void Instance::freeParticle(Particle& p) {
	p.spawnTime = p.dieTime = 0;
	--m_count;
}

void Instance::initialiseThreadData(int threads) {
	if(threads == m_threads) return;
	delete [] m_triggered;
	delete [] m_destroy;
	m_triggered = new std::vector<TriggeredEvent>[threads];
	m_destroy = new std::vector<DestroyMessage>[threads];
	for(RenderInstance& r: m_renderers) {
		delete [] r.head;
		r.head = new void*[threads];
	}
	m_threads = threads;
}

void Instance::setEnabled(bool e) {
	m_enabled = e;
}



// ================================================================ //

Manager::Manager() : m_threadsRunning(false), m_timeStep(0) {
}

Manager::~Manager() {
	stopThreads();
}

void Manager::startThreads(int count) {
	for(Instance* i: m_instances) i->initialiseThreadData(count);
	m_threadsRunning = true;
	m_threads.resize(count);
	m_barrier.initialise(count+1);
	for(int i=0; i<count; ++i) m_threads[i].begin(this, &Manager::threadFunc, i);
}
		
void Manager::stopThreads() {
	if(m_threadsRunning) {
		m_threadsRunning = false;
		for(base::Thread& t: m_threads) t.join();
	}
}

void Manager::threadFunc(int index) {
	while(m_threadsRunning) {
		m_barrier.sync();
		for(Instance* inst: m_instances) {
			inst->updateT(index, m_threads.size(), m_timeStep, m_viewMatrix);
		}
		m_barrier.sync();
	}
}

void Manager::update(float time, const Matrix& viewMatrix) {
	// Main thread updates (spawn/destroy particles)
	for(Instance* inst: m_instances) {
		inst->update(time);
	}

	// Trigger threaded updates (move)
	if(m_threadsRunning) {
		m_barrier.sync();
		m_viewMatrix = viewMatrix;
		m_timeStep = time;
		m_barrier.sync();
	}
	else { // Main thread update mode
		for(Instance* inst: m_instances) {
			inst->updateT(0, 1, time, viewMatrix);
		}
	}

	// Update main thread drawables
	for(Instance* inst : m_instances) inst->updateGeometry();
}

void Manager::add(Instance* instance, bool enabled) {
	instance->initialise();
	instance->initialiseThreadData(m_threadsRunning? getThreads(): 1);
	m_instances.push_back(instance);
	instance->setEnabled(enabled);
	instance->m_manager = this;
}

void Manager::remove(Instance* instance) {
	eraseFast(m_instances, instance);
	instance->m_manager = 0;
}
size_t Manager::getParticleCount() const {
	size_t count = 0;
	for(Instance* i: m_instances) count += i->getParticleCount();
	return count;
}

