#pragma once

class RNG {
	unsigned m_seed;
	public:
	RNG(unsigned seed) : m_seed(seed) {}
	unsigned rand() { m_seed = m_seed * 1103515245 + 12345; return m_seed&0x7fffffff; }
	float    randf() { return (float)rand() / (float)0x7fffffff; }
	float    randf(float min, float max) { return min + randf() * (max-min); }
	float    randf(const float* range) { return randf(range[0], range[1]); }
	float    randn(float mean, float sigma) {
		// box-muller transformation
		float u1 = randf(), u2 = randf();
		while(u1==0) u1 = randf(); // u1 must be greater than zero
		float mag = sigma * sqrt(-2.0 * log(u1));
		return mag * cos(TWOPI * u2) + mean; // sin produces the other one
	}
};
