  ////////////////////////////
  // Noise Generator v1.0   //
  // Authour: Sam Gynn      //
  // Date:    01/10/2009    //
  ////////////////////////////
  //   -Perlin noise        //
  //   -Cell noise          //
  //   -Turbulance          //
  //   -FbM                 //
  //   -Hybred MultiFractal //
  //   -RigidMultiFractal   //
  ////////////////////////////

#ifndef _FRACTAL_
#define _FRACTAL_

#include <cstdlib>
#include <cstring>
#include <cmath>

#define MULT(v, m) v[0]*=m; v[1]*=m; v[2]*=m;
/** Fractal base class. All fractals extend this. Templates for float or double */
template <typename T>
class Fractal {
	public:
	Fractal() {}
	virtual ~Fractal() {}
	
	/** Get the value from a 1, 2 or 3 dimensional vector
	 *  @param v Vector as an array
	 *  @param n Vector dimensions
	 *  @return Procedural value at this point
	 */
	virtual T value(const T* v, int n) = 0;
	/** get a procedural value for a 1d point */
	T value(const T x) { return value(&x, 1); }
	/** get a procedural value for a 2d point */
	T value(const T x, T y) { T v[2] = { x, y }; return value(v, 2); }
	/** get a procedural value for a 3d point */
	T value(const T x, T y, T z) { T v[3] = { x, y, z }; return value(v, 3); }
	
	/** Get a random number between min and max */
	inline static T random(T a, T b) { return ((T)rand() / RAND_MAX) * (b-a) + a; }
	
	protected: //some functions a lot of them use:
	/* generate exponent array. Used by FbM, and the Multi-Fractals */
	static void expArray(T* &array, int size, T H, T lacunarity);
	/* Fill an array with white noise */
	static void noise(T* array, int size, int dimensions=1, T min=-1, T max=1);
	/* fill an array with unique random integers from 0 to size */
	static void index(int* array, int size);
};

/**
*	Perlin noise. Many other fractals are based on this
*/
template <typename T>
class Perlin : public Fractal<T> {
	public:
	Perlin();
	~Perlin();
	T value(const T* v, int n) override;
	
	private:
	static const int B = 0x100;
	static const int BM = 0xff;
	static const int N = 0x1000;
	
	float* g[3];
	int p[B];
	
	T noise1(const T* vec);
	T noise2(const T* vec);
	T noise3(const T* vec);
	void init(int n);
	
	T lerp(T a, T b, T t) const { return a + (b-a)*t; }
	T curve(T v) const { return v*v*(3-2*v); }
};

/**
 * Many functions use perlin noise as a base
 */
template <typename T>
class PerlinDerived : public Fractal<T> {
	private:
	bool ownedPerlin;
	protected:
	Perlin<T>* perlin;
	PerlinDerived(Perlin<T>* p) : perlin(p), ownedPerlin(false) {}
	PerlinDerived(int seed) : perlin(new Perlin<T>(seed)), ownedPerlin(true) {}
	PerlinDerived() : perlin(new Perlin<T>()), ownedPerlin(true) {}
	~PerlinDerived() { if(ownedPerlin) delete perlin; }
};


/**
*	Perlin noise again - perhaps i should merge them
*	This version can have multiple octaves however
*/
template <typename T>
class Perlin2 : public PerlinDerived<T> {
	public:
	Perlin2(Perlin<T>* perlin, int octaves=1) : PerlinDerived<T>(perlin), octaves(octaves) {};
	Perlin2(int seed, int octaves=1) : PerlinDerived<T>(seed), octaves(octaves) {};
	Perlin2(int octaves=1) : octaves(octaves) {};
	T value(const T*v, int n) override;
	private:
	int octaves;
};

/**
*	Turbulance - generated from perlin noise
*	Change frequencies for detail
*/
template <typename T>
class Turbulance : public Fractal<T> {
	public:
	Turbulance(Perlin<T>* perlin, T frequencies=2.3) : PerlinDerived<T>(perlin), freq(frequencies) { }
	Turbulance(int seed, T frequencies=2.3) : PerlinDerived<T>(seed), freq(frequencies) { }
	Turbulance(T frequencies=2.3) : freq(frequencies) { }
	void set(T frequencies) { freq = frequencies; }
	T value(const T* v, int n) override;
	private:
	T freq;
};

/**
*	Cell noise
*	The only one that doesn't use perlin noise
*/
template <typename T>
class CellNoise : public Fractal<T> {
	public:
	CellNoise();
	
	T value(const T* v, int n) override;
	
	private:
	static const int B = 0x100;
	static const int BM = 0xff;
	T g[B];
	int p[B];
	
	void consider(T* cell, int n, T& dist1, T& dist2);
};

/** Base class for FbM and others */
template <typename T>
class ComplexFractal : public PerlinDerived<T> {
	protected:
	ComplexFractal(Perlin<T>* perlin, T H, T lacunarity, T octaves) : PerlinDerived<T>(perlin), H(H), lacunarity(lacunarity), octaves(octaves) { init(); }
	ComplexFractal(int seed, T H, T lacunarity, T octaves) : PerlinDerived<T>(seed), H(H), lacunarity(lacunarity), octaves(octaves) { init(); }
	ComplexFractal(T H, T lacunarity, T octaves) : H(H), lacunarity(lacunarity), octaves(octaves) { init(); }
	~ComplexFractal() { delete [] exponent_array; }
	void init() { Fractal<T>::expArray(exponent_array, (int)octaves+1, H, lacunarity); }

	public:
	void set(T H, T lacunarity, T octaves) {
		this->H = H;
		this->lacunarity = lacunarity;
		this->octaves = octaves;
		init();
	}
	protected:
	T H;
	T lacunarity;
	T octaves;
	T* exponent_array = nullptr;
};


/**
*	FbM (Fractal Brownean Motion)
*/
template <typename T>
class FBM : public ComplexFractal<T> {
	public:
	FBM(Perlin<T>* perlin, T H=0.76, T lacunarity=0.6, T octaves=5.0) : ComplexFractal<T>(perlin, H, lacunarity, octaves) {}
	FBM(int seed, T H=0.76, T lacunarity=0.6, T octaves=5.0) : ComplexFractal<T>(seed, H, lacunarity, octaves) {}
	FBM(T H=0.76, T lacunarity=0.6, T octaves=5.0) : ComplexFractal<T>(H, lacunarity, octaves) {}

	T value(const T* v, int n) override {
		T value, remainder;
		T vc[3] = {0,0,0}; memcpy(vc, v, n*sizeof(T));
		int i;
		value = 0;
		for (i=0; i<this->octaves; i++) {
			value += this->perlin->value(vc, n) * this->exponent_array[i];
			MULT(vc, this->lacunarity);
		}
		
		remainder = this->octaves - (int)this->octaves;
		if (remainder) value += remainder * this->perlin->value( vc, n ) * this->exponent_array[i];
		
		return value / this->octaves;
	}
	using Fractal<T>::value;
};

/**
*	Hybrid Multi-Fractal
*/
template <typename T>
class HybridMultiFractal : public Fractal<T> {
	public:
	HybridMultiFractal(Perlin<T>* perlin, T H=0.5, T lacunarity=0.551, T octaves=3.57, T offset=0.01) : ComplexFractal<T>(perlin, H, lacunarity, octaves), offset(offset) {}
	HybridMultiFractal(int seed, T H=0.5, T lacunarity=0.551, T octaves=3.57, T offset=0.01) : ComplexFractal<T>(seed, H, lacunarity, octaves), offset(offset) {}
	HybridMultiFractal(T H=0.5, T lacunarity=0.551, T octaves=3.57, T offset=0.01) : ComplexFractal<T>(H, lacunarity, octaves), offset(offset) {}

	void set(T H, T lacunarity, T octaves, T offset) {
		set(H, lacunarity, octaves, offset);
		this->offset = offset;
	}
	
	T value(const T* v, int n) override {
		T result, signal, weight, remainder;
		T vc[3] = {0,0,0}; memcpy(vc, v, n*sizeof(T));
		result = (this->perlin->value(vc, n) + offset) * this->exponent_array[0];
		weight = result;
		MULT(vc, this->lacunarity);
		int i;
		for (i=1; i<this->octaves; i++) {
			if(weight > 1.0) weight = 1.0;
			signal = ( this->perlin->value(vc, n) + offset ) * this->exponent_array[i];
			result += weight * signal;
			weight *= signal;			
			MULT(vc, this->lacunarity);
		}
		remainder = this->octaves - (int)this->octaves;
		if (remainder) result += remainder * this->perlin->value(vc, n) * this->exponent_array[i];
		return result / this->octaves;
	}
	
	private:
	T offset;
};

/**
*	Rigid Multi-Fractal
*/
template <typename T>
class RigidMultiFractal : public Fractal<T> {
	public:
	RigidMultiFractal(Perlin<T>* perlin, T H=0.526, T lacunarity=1.0, T octaves=6.85, T offset=0.011, T gain=0.49) : ComplexFractal<T>(perlin, H, lacunarity, octaves), offset(offset), gain(gain) {}
	RigidMultiFractal(int seed, T H=0.526, T lacunarity=1.0, T octaves=6.85, T offset=0.011, T gain=0.49) : ComplexFractal<T>(seed, H, lacunarity, octaves), offset(offset), gain(gain) {}
	RigidMultiFractal(T H=0.526, T lacunarity=1.0, T octaves=6.85, T offset=0.011, T gain=0.49) : ComplexFractal<T>(H, lacunarity, octaves), offset(offset), gain(gain) {}
	
	void set(T H, T lacunarity, T octaves, T offset, T gain) {
		init(H, lacunarity, octaves);
		this->offset = offset;
		this->gain = gain;
	}
	
	T value(const T* v, int n) override {
		double result, signal, weight;
		T vc[3] = {0,0,0}; memcpy(vc, v, n*sizeof(T));
		int i;
		signal = this->perlin->value(vc, n);
		if(signal < 0.0) signal = -signal;
		result = signal = (offset - signal) * signal;
		weight = 1.0;
		for(i=1; i<this->octaves; i++) {
			MULT(vc, this->lacunarity);
			weight = signal * gain;
			if(weight > 1.0) weight = 1.0;
			if(weight < 0.0) weight = 0.0;
			signal = this->perlin->value(vc, n);
			if (signal < 0.0) signal = -signal;
			signal = (offset - signal) * signal * weight;
			result += signal * this->exponent_array[i];
		}
		return result;
	}
	
	private:
	T offset;
	T gain;
};

// ------------------------------------------------------------------------------------------------------------------------------
template <typename T>
void Fractal<T>::expArray(T* &array, int size, T H, T lacunarity) {
	if(array) delete [] array;
	array = new T[ size ];
	T frequency = 1.0;
	for(int i=0; i<size; i++) {
		array[i] = pow( frequency, -H );
		frequency *= lacunarity;
	}	
}
template <typename T>
void Fractal<T>::noise(T* array, int size, int d, T min, T max) {
	int i, j;
	for(i=0 ; i<size ; i++) {
		//random values
		for(j=0; j<d; j++) array[i*d+j] = random(min, max);
		//normalise vector
		if(d>1) {
			T l = 0;
			for(j=0; j<d; j++) l += (array[i*d+j] * array[i*d+j]);
			l = sqrt(l);
			for(j=0; j<d; j++) array[i*d+j] /= l;
		}
	}
}
template <typename T>
void Fractal<T>::index(int* array, int size) {
	for(int i=0; i<size; i++) array[i]=i;
	for(int i=0; i<size; i++) {
		int k = array[i];
		array[i] = rand() % size;
		array[ array[i] ] = k;
	}
}

// ------------------------------------------------------------------------------------------------------------------------------
template <typename T>
Perlin<T>::Perlin() {
	Fractal<T>::index(p, B);
	for(int i=0; i<3; i++) g[i]=0;
	init(2);
}
template <typename T>
Perlin<T>::~Perlin() {
	for(int i=0; i<3; i++) if(g[i]) delete [] g[i];
}

template <typename T>
T Perlin<T>::value(const T* v, int n) {
	switch(n) {
		case 1: return noise1(v);
		case 2: return noise2(v);
		case 3: return noise3(v);
		default: return 0;
	}
}
template <typename T>
void Perlin<T>::init(int n) {
	if(g[n-1]) return;
	g[n-1] = new T[n*B];
	Fractal<T>::noise(g[n-1], B, n);
}

#define setup(x,b0,b1,r0,r1)\
	b0 = ((int)floor(x)) & BM;\
	b1 = (b0+1) & BM;\
	r0 = x - (int)floor(x);\
	r1 = r0 - 1.0f;

template <typename T>
T Perlin<T>::noise1(const T* vec) {
	init(1);
	int bx0, bx1;
	T rx0, rx1, a, b;
	setup(vec[0], bx0, bx1, rx0, rx1);
	T t = curve(rx0);
	a = rx0 * g[0][ p[ bx0 ] ];
	b = rx1 * g[0][ p[ bx1 ] ];
	return lerp(a, b, t);
}
template <typename T>
T Perlin<T>::noise2(const T* vec) {
	init(2);
	int bx0, bx1, by0, by1, b00, b10, b01, b11;
	T rx0, rx1, ry0, ry1, *q, sx, sy, a, b, u, v;
	register int i, j;
	setup(vec[0], bx0,bx1, rx0,rx1);
	setup(vec[1], by0,by1, ry0,ry1);
	//get point info
	i = p[ bx0 ];
	j = p[ bx1 ];
	b00 = p[ (i + by0) & BM ];
	b10 = p[ (j + by0) & BM ];
	b01 = p[ (i + by1) & BM ];
	b11 = p[ (j + by1) & BM ];
	sx = curve(rx0);
	sy = curve(ry0);
	#define at2(rx,ry) ( rx * q[0] + ry * q[1] )
	//bottom two points (y-)
	q = &g[1][ b00*2 ] ; u = at2(rx0,ry0);
	q = &g[1][ b10*2 ] ; v = at2(rx1,ry0);
	a = lerp(u, v, sx);	
	//top two points (y+)
	q = &g[1][ b01*2 ] ; u = at2(rx0,ry1);
	q = &g[1][ b11*2 ] ; v = at2(rx1,ry1);
	b = lerp(u, v, sx);
	#undef at2
	return lerp(a, b, sy);
}
template <typename T>
T Perlin<T>::noise3(const T* vec) {
	int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
	T rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
	register int i, j;
	init(3);
	setup(vec[0], bx0,bx1, rx0,rx1);
	setup(vec[1], by0,by1, ry0,ry1);
	setup(vec[2], bz0,bz1, rz0,rz1);
	i = p[ bx0 ];
	j = p[ bx1 ];
	b00 = p[ (i + by0) & BM ];
	b10 = p[ (j + by0) & BM ];
	b01 = p[ (i + by1) & BM ];
	b11 = p[ (j + by1) & BM ];
	t  = curve(rx0);
	sy = curve(ry0);
	sz = curve(rz0);
	#define at3(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )
	q = &g[2][ (b00 + bz0)&BM * 3 ]; u = at3(rx0,ry0,rz0);
	q = &g[2][ (b10 + bz0)&BM * 3 ]; v = at3(rx1,ry0,rz0);
	a = lerp(u, v, t);
	q = &g[2][ (b01 + bz0)&BM * 3 ]; u = at3(rx0,ry1,rz0);
	q = &g[2][ (b11 + bz0)&BM * 3 ]; v = at3(rx1,ry1,rz0);
	b = lerp(u, v, t);
	c = lerp(a, b, sy);
	q = &g[2][ (b00 + bz1)&BM * 3 ]; u = at3(rx0,ry0,rz1);
	q = &g[2][ (b10 + bz1)&BM * 3 ]; v = at3(rx1,ry0,rz1);
	a = lerp(u, v, t);
	q = &g[2][ (b01 + bz1)&BM * 3 ]; u = at3(rx0,ry1,rz1);
	q = &g[2][ (b11 + bz1)&BM * 3 ]; v = at3(rx1,ry1,rz1);
	b = lerp(u, v, t);
	#undef at3
	d = lerp(a, b, sy);
	return lerp(c, d, sz);
}
#undef setup
// ------------------------------------------------------------------------------------------------------------------------------
template <typename T>
T Perlin2<T>::value(const T* v, int n) {
	T t=0, vc[3];
	for(int i=0; i<octaves; i++) {
		for(int j=0; j<n; j++) vc[j] = v[j] / (1<<i);
		t+= this->perlin->value(vc, n);
	}
	if(octaves) t /= octaves;
	return t;
}
// ------------------------------------------------------------------------------------------------------------------------------
template <typename T>
T Turbulance<T>::value(const T* v, int n) {
	T t, vc[3], f=freq;
	for(t=0; f>=1; f/=2) {
		vc[0] = f * v[0];
		if(n>1) { vc[1] = f * v[1];
		if(n>2) vc[2] = f * v[2]; 
		}
		t += fabs(this->perlin->value(vc, n)) / f;
	}
	return t;
}
// ------------------------------------------------------------------------------------------------------------------------------
template <typename T>
CellNoise<T>::CellNoise() {
	Fractal<T>::noise(g, B, 1, 0, 1);
	Fractal<T>::index(p, B);
}
template <typename T>
T CellNoise<T>::value(const T* v, int n) {
	T vc[3] = {0,0,0}; memcpy(vc, v, n*sizeof(T));
	int ix[3] = { (int)floor(vc[0]), (int)floor(vc[1]), (int)floor(vc[2]) };
	T fx[3] = { vc[0]-ix[0], vc[1]-ix[1], vc[2]-ix[2] };
	
	T dist1 = 9999999.0;
	T dist2 = 9999999.0;
	T cell[3];
	
	int k;
	for(int x=-1; x<=1; x++) {
		if(n>1) for(int y=-1; y<=1; y++) {
			if(n>2) for(int z=-1; z<=1; z++) {
				k = p[(ix[0]+x) & BM] + p[((ix[1]+y)*3) & BM] + p[((ix[2]+z)*7) & BM];
				cell[0] = g[(3*k+0) & BM] + x - fx[0];
				cell[1] = g[(3*k+1) & BM] + y - fx[1];
				cell[2] = g[(3*k+2) & BM] + z - fx[2];
				consider(cell, n, dist1, dist2);
			} else {
				k = p[(ix[0]+x) & BM] + p[((ix[1]+y)*3) & BM];
				cell[0] = g[(2*k+0) & BM] + x - fx[0];
				cell[1] = g[(2*k+1) & BM] + y - fx[1];
				consider(cell, n, dist1, dist2);
			}
		} else {
			k = p[(ix[0]+x) & BM];
			cell[0] = g[(k+0) & BM] + x - fx[0];
			consider(cell, n, dist1, dist2);
		}
	}
	
	//now do the stuff
	dist1 = sqrt(dist1);
	dist2 = sqrt(dist2);
	
	//umm
	T r = dist2-dist1;
	if(r>=0.9999) r=0.9999;
	return r;
}
template <typename T>
void CellNoise<T>::consider(T* cell, int n, T& dist1, T& dist2) {
	T d = 0;
	for(int i=0; i<n; i++) d+= (cell[i]*cell[i]);
	if(d<dist1) {
		dist2=dist1;
		dist1 = d;
	} else if(d<dist2) {
		dist2=d;
	}
}


#undef MULT

#endif
