/** Header for math utility functions used only by model - perhaps use SIMD? */

// Macro for resizing C arrays
#define ResizeArray(type, array, oldSize, newSize)	{ type* tmp=array; array=new type[newSize]; if(oldSize) { memcpy(array, tmp, oldSize*sizeof(type)); delete [] tmp; } }

namespace base {
namespace model {
	class Math {
		public:

		/** Multiply matrices a and b */
		template<typename T>
		static void multiplyMatrix(T* out, const T* a, const T* b) {
			out[0]  = a[0]*b[0]  + a[4]*b[1]  + a[8]*b[2]   + a[12]*b[3];
			out[1]  = a[1]*b[0]  + a[5]*b[1]  + a[9]*b[2]   + a[13]*b[3];
			out[2]  = a[2]*b[0]  + a[6]*b[1]  + a[10]*b[2]  + a[14]*b[3];
			out[3]  = a[3]*b[0]  + a[7]*b[1]  + a[11]*b[2]  + a[15]*b[3];
			out[4]  = a[0]*b[4]  + a[4]*b[5]  + a[8]*b[6]   + a[12]*b[7];
			out[5]  = a[1]*b[4]  + a[5]*b[5]  + a[9]*b[6]   + a[13]*b[7];
			out[6]  = a[2]*b[4]  + a[6]*b[5]  + a[10]*b[6]  + a[14]*b[7];
			out[7]  = a[3]*b[4]  + a[7]*b[5]  + a[11]*b[6]  + a[15]*b[7];
			out[8]  = a[0]*b[8]  + a[4]*b[9]  + a[8]*b[10]  + a[12]*b[11];
			out[9]  = a[1]*b[8]  + a[5]*b[9]  + a[9]*b[10]  + a[13]*b[11];
			out[10] = a[2]*b[8]  + a[6]*b[9]  + a[10]*b[10] + a[14]*b[11];
			out[11] = a[3]*b[8]  + a[7]*b[9]  + a[11]*b[10] + a[15]*b[11];
			out[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14]  + a[12]*b[15];
			out[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14]  + a[13]*b[15];
			out[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
			out[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
		}

		/** Multiply affine matrices a and b (slight optimisation) */
		template<typename T>
		static void multiplyAffineMatrix(T* out, const T* a, const T* b) {
			out[0]  = a[0]*b[0]  + a[4]*b[1]  + a[8]*b[2];
			out[1]  = a[1]*b[0]  + a[5]*b[1]  + a[9]*b[2];
			out[2]  = a[2]*b[0]  + a[6]*b[1]  + a[10]*b[2];
			out[3]  = 0;
			out[4]  = a[0]*b[4]  + a[4]*b[5]  + a[8]*b[6];
			out[5]  = a[1]*b[4]  + a[5]*b[5]  + a[9]*b[6];
			out[6]  = a[2]*b[4]  + a[6]*b[5]  + a[10]*b[6];
			out[7]  = 0;
			out[8]  = a[0]*b[8]  + a[4]*b[9]  + a[8]*b[10];
			out[9]  = a[1]*b[8]  + a[5]*b[9]  + a[9]*b[10];
			out[10] = a[2]*b[8]  + a[6]*b[9]  + a[10]*b[10];
			out[11] = 0;
			out[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14]  + a[12];
			out[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14]  + a[13];
			out[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14];
			out[15] = 1;
		}

		/** Create a matrix from quaternion, scale and translation */
		template<typename T>
		static void matrix(T* out, T* q, T* scale, T* pos) {
			out[0]  = (1.0f - 2.0f * ( q[1] * q[1] + q[2] * q[2] )) * scale[0]; 
			out[1]  = 2.0f * (q[0] * q[1] + q[2] * q[3]);
			out[2]  = 2.0f * (q[0] * q[2] - q[1] * q[3]);
			out[3]  = 0.0f;
			out[4]  = 2.0f * ( q[0] * q[1] - q[2] * q[3] );  
			out[5]  = (1.0f - 2.0f * ( q[0] * q[0] + q[2] * q[2] )) * scale[1]; 
			out[6]  = 2.0f * (q[2] * q[1] + q[0] * q[3] );  
			out[7]  = 0.0f;
			out[8]  = 2.0f * ( q[0] * q[2] + q[1] * q[3] );
			out[9]  = 2.0f * ( q[1] * q[2] - q[0] * q[3] );
			out[10] = (1.0f - 2.0f * ( q[0] * q[0] + q[1] * q[1] )) * scale[2];  
			out[11] = 0.0f;  
			out[12] = pos[0];
			out[13] = pos[1];
			out[14] = pos[2];
			out[15] = 1.0f;
		}

		/** Linear Interpolation of a vector */
		template<typename T>
		static void lerp(T* out, const T* a, const T* b, T v) {
			out[0]=a[0]+(b[0]-a[0])*v;
			out[1]=a[1]+(b[1]-a[1])*v;
			out[2]=a[2]+(b[2]-a[2])*v;
		}

		/** Spherical linear interpolation */
		template<typename T>
		static void slerp(T* out, const T* a, const T* b, T v) {
			T w1, w2;
			T dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
			T theta = acos(dot);
			T sintheta = sin(theta);
			if (sintheta > 0.001) {
				w1=sin((1.0f-v)*theta) / sintheta;
				w2 = sin(v*theta) / sintheta;
			} else {
				w1 = 1.0f - v;
				w2 = v;
			}
			out[0] = a[0] * w1 + b[0] * w2;
			out[1] = a[1] * w1 + b[1] * w2;
			out[2] = a[2] * w1 + b[2] * w2;
			out[3]  =a[3] * w1 + b[3] * w2;
		}
	};
};
};


