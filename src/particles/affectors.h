#pragma once
#include <base/particles.h>

namespace particle {

class LinearForce : public Affector {
	public:
	void update(Instance& i, Particle& p, float time) const override {
		float age = i.getTime() - p.spawnTime;
		p.velocity.x += force.x.getValue(age) * time / p.mass;
		p.velocity.y += force.y.getValue(age) * time / p.mass;
		p.velocity.x += force.z.getValue(age) * time / p.mass;
	}
	public:
	struct { Value x,y,z; } force;
};

class DragForce : public Affector {
	public:
	Value amount = 0.1; // 0-1
	void update(Instance& i, Particle& p, float time) const override {
		p.velocity -= p.velocity * amount.getValue(i.getTime()-p.spawnTime) * time;
	}
};


class SetVelocity : public Affector {
	public:
	void update(Instance& i, Particle& p, float time) const override {
		float age = i.getTime() - p.spawnTime;
		p.velocity.x = velocity.x.getValue(age);
		p.velocity.y = velocity.y.getValue(age);
		p.velocity.x = velocity.z.getValue(age);
	}
	public:
	struct { Value x,y,z; } velocity;
};


class PointAttactor : public Affector {
	public:
	vec3 position;
	Value strength = 1;
	void update(Instance& i, Particle& p, float time) const override {
		vec3 dir = (position - p.position);
		//float dist = dir.normaliseWithLength();
		p.velocity += dir * strength.getValue(i.getTime() - p.spawnTime) * time / p.mass;
	}
};

class Rotator2D : public Affector {
	public:
	Value amount = 0.1;
	void update(Instance& i, Particle& p, float time) const override {
		p.orientation.x += amount.getValue(i.getTime()-p.spawnTime) * time;
	}
};

class Rotator3D : public Affector {
	public:
	bool local = true;
	Value amount = 0.1;
	vec3 axis = vec3(0,1,0);
	void update(Instance& i, Particle& p, float time) const override {
		Quaternion rot(local? p.orientation*axis: axis, amount.getValue(i.getTime()-p.spawnTime) * time);
		p.orientation *= rot;
	}
};

class Attractor : public Affector {
	public:
	vec3 centre;
	Value strength = 1;
	void update(Instance& i, Particle& p, float time) const override {
		vec3 ctr = i.getTransform() * centre;
		float s = strength.getValue(i.getTime() - p.spawnTime);
		float d = ctr.distance(p.position);
		p.velocity += (ctr - p.position) * (s * time / fmax(0.01f,d)) / p.mass;
	}
};

class Vortex : public Affector {
	public:
	vec3 centre;
	vec3 axis = vec3(0,1,0);
	Value rotation = 1; // keyed by distance
	void update(Instance& i, Particle& p, float time) const override {
		vec3 rel = p.position - centre;
		//vec3 c = axis * axis.dot(rel);
		//float amount = rotation.getValue(c.length());
		float amount = rotation.getValue(i.getTime() - p.spawnTime);

		Quaternion r(axis, amount * time); // skip the sqrt ? perhaps a Quaternion::fromAxisN(normalisedAxis, angle)
		p.position = centre + r * rel;
		p.velocity = r * p.velocity;
	}
};

class UniformScale : public Affector {
	public:
	Value scale = 1;
	void update(Instance& i, Particle& p, float time) const override {
		float s = scale.getValue(i.getTime() - p.spawnTime);
		p.scale.set(s,s,s);
	}
};

class Colourise : public Affector {
	public:
	Gradient colour;
	void update(Instance& i, Particle& p, float time) const override {
		p.colour = colour.getValue(i.getTime() - p.spawnTime);
	}
	private:
};

class OrientToVelocity : public Affector {
	public:
	void update(Instance&, Particle& p, float time) const override {
		if(p.velocity.x!=0 && p.velocity.y!=0 && p.velocity.z!=0) {
			vec3 dir = p.velocity.normalised();
			vec3 up(0,1,0);
			vec3 left = up.cross(dir).normalise();
			up = dir.cross(left);
			p.orientation.fromMatrix(Matrix(left, up, dir));
		}
	}
};

class SetDirection : public Affector {
	public:
	struct { Value x,y,z; } direction;
	void update(Instance& i, Particle& p, float time) const override {
		float v = i.getTime() - p.spawnTime;
		vec3 dir(direction.x.getValue(v), direction.y.getValue(v), direction.z.getValue(v));
		if(dir != vec3()) {
			dir.normalise();
			vec3 left = p.orientation.xAxis();
			vec3 up = dir.cross(left).normalise();
			left = up.cross(dir);
			p.orientation.fromMatrix(Matrix(left, up, dir));
		}
	}
};



}


