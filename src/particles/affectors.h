#pragma once
#include <base/particles.h>

namespace particle {

class LinearForce : public Affector {
	public:
	void update(Particle& p, float st, float time) const override {
		float age = st - p.spawnTime;
		p.velocity.x += force.x.getValue(age) * time;
		p.velocity.y += force.y.getValue(age) * time;
		p.velocity.x += force.z.getValue(age) * time;
	}
	public:
	struct { Value x,y,z; } force;
};

class DragForce : public Affector {
	public:
	Value amount = 0.1; // 0-1
	void update(Particle& p, float st, float time) const override {
		p.velocity -= p.velocity * amount.getValue(st-p.spawnTime) * time;
	}
};


class PointAttactor : public Affector {
	public:
	vec3 position;
	Value strength = 1;
	void update(Particle& p, float st, float time) const override {
		vec3 dir = (position - p.position);
		//float dist = dir.normaliseWithLength();
		p.velocity += dir * strength.getValue(st - p.spawnTime) * time;
	}
};

class Rotator2D : public Affector {
	public:
	Value amount = 0.1;
	void update(Particle& p, float st, float time) const override {
		p.orientation.x += amount.getValue(st-p.spawnTime) * time;
	}
};

class Attractor : public Affector {
	public:
	vec3 centre;
	Value strength = 1;
	void update(Particle& p, float st, float time) const override {
		float s = strength.getValue(st - p.spawnTime);
		float d = centre.distance(p.position);
		p.velocity += (centre - p.position) * (s * time / fmax(0.01f,d));
	}
};

class Vortex : public Affector {
	public:
	vec3 centre;
	vec3 axis = vec3(0,1,0);
	Value rotation = 1; // keyed by distance
	void update(Particle& p, float st, float time) const override {
		vec3 rel = p.position - centre;
		//vec3 c = axis * axis.dot(rel);
		//float amount = rotation.getValue(c.length());
		float amount = rotation.getValue(st - p.spawnTime);

		Quaternion r(axis, amount * time); // skip the sqrt ? perhaps a Quaternion::fromAxisN(normalisedAxis, angle)
		p.position = centre + r * rel;
		p.velocity = r * p.velocity;
	}
};

class UniformScale : public Affector {
	public:
	Value scale = 1;
	void update(Particle& p, float st, float time) const override {
		float s = scale.getValue(st - p.spawnTime);
		p.scale.set(s,s,s);
	}
};

class Colourise : public Affector {
	public:
	Gradient colour;
	void update(Particle& p, float st, float time) const override {
		p.colour = colour.getValue(st - p.spawnTime);
	}
};

class OrientToVelocity : public Affector {
	public:
	void update(Particle& p, float st, float time) const override {
		if(p.velocity.x!=0 && p.velocity.y!=0 && p.velocity.z!=0) {
			vec3 dir = p.velocity.normalised();
			vec3 up(0,1,0);
			vec3 left = up.cross(dir).normalise();
			up = dir.cross(left);
			p.orientation.fromMatrix(Matrix(left, up, dir));
		}
	}
};


}


