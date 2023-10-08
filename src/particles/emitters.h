#pragma once
#include <base/particles.h>

namespace particle {


class PointEmitter : public Emitter {
	public:
	Value cone;
	Value velocity;
	void spawnParticle(Particle& p, const Matrix&, float key) const override {
		float speed = velocity.getValue(key);
		if(speed != 0) {
			const vec3 direction = p.orientation.yAxis();
			float variance = cone.getValue(key) * PI/180;
			if(variance == 0) p.velocity += direction * velocity.getValue(key);
			else {
				vec3 normal = p.orientation.xAxis();
				normal = Quaternion(direction, random() * TWOPI) * normal; // random perpendicular
				vec3 d = Quaternion(normal, variance) * direction;
				p.velocity += d * velocity.getValue(key);
			}
		}
	}
};

class SphereEmitter : public PointEmitter {
	public:
	Value radius = 1;
	void spawnParticle(Particle& p, const Matrix&m, float key) const override {
		PointEmitter::spawnParticle(p, m, key);
		if(float r = radius.getValue(key)) {
			// Tests suggest this is faster than square roots
			while(true) {
				vec3 o(random()*2-1, random()*2-1, random()*2-1);
				if(o.length2() > 1) continue;
				p.position += o * r;
				break;
			}
		}
	}
};

class SphereSurfaceEmitter : public Emitter {
	public:
	Value radius = 1;
	Value velocity = 0;
	void spawnParticle(Particle& p, const Matrix& m, float key) const override {
		float r = radius.getValue(key);
		while(true) {
			vec3 o(random()*2-1, random()*2-1, random()*2-1);
			if(o.length2() > 1) continue;
			o.normalise();
			p.position += o * r;
			p.velocity += velocity.getValue(key);
			break;
		}
	}
};

class BoxEmitter : public Emitter {
	public:
	vec3 size = vec3(1,1,1);
	struct { Value x,y,z; } velocity;	// Initial velocity
	void spawnParticle(Particle& p, const Matrix&, float key) const override {
		vec3 local;
		local.x = random() * size.x - size.x * 0.5;
		local.y = random() * size.y - size.y * 0.5;
		local.z = random() * size.z - size.z * 0.5;
		p.position += p.orientation * local;

		local.x = velocity.x.getValue(key);
		local.y = velocity.y.getValue(key);
		local.z = velocity.z.getValue(key);
		p.velocity += p.orientation * local;
	}
};

class RingEmitter : public Emitter {
	public:
	Value radius = 1;	// ring radius
	Value angle = 0;	// direction normal to ring. 0 = outwards
	Value tangent = 0;	// tangent component of velocity
	Value velocity = 0;	// initial speed
	Value sequence = Value(-3.14, 3.14);
	void spawnParticle(Particle& p, const Matrix& m, float key) const override {
		float r = radius.getValue(key);
		float a = sequence.getValue(key);
		vec2 pos(sin(a), cos(a));
		p.position += p.orientation * pos.xzy() * r;

		float s = velocity.getValue(key);
		if(s != 0) {
			float t = tangent.getValue(key);
			vec3 localVelocity = p.orientation * vec3(pos.y * s, 0, -pos.x * s);
			if(fabs(t) < 1) {
				float r = angle.getValue(key) * PI/180;
				Quaternion q(vec3(pos.y,0,-pos.x), r);
				localVelocity = localVelocity * t + q * pos.xzy() * (1-t) * s;
			}
			p.velocity += p.orientation * localVelocity;
		}
	}
};

class MeshVertexEmitter : public Emitter {
};

class MeshFaceEmitter : public Emitter {
};


}



