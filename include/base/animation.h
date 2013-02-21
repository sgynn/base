#ifndef _BASE_ANIMATION_
#define _BASE_ANIMATION_

#include "math.h"
#include "quaternion.h"
#include <vector>

namespace base {
namespace model {


	/** Model animation class. */
	class Animation {
		public:
		Animation();										/**< Default constructor */
		~Animation();										/**< Destructor */

		Animation*  grab();									/** Grab reference */
		int         drop();									/** Drop reference */

		Animation*  extract(int start, int end) const;		/**< Create an animation from a subset of keyframes */
		void        optimise();								/**< Remove redundant keyframes */
		int         getLength() const;						/**< Get the animation length in frames */
		void        setSpeed(float fps);					/**< Set animation speed in frames per second */
		float       getSpeed() const;						/**< Get animation speed */
		void        setLoop(bool loop);                     /**< Set wherher this animation loops */
		bool        isLoop() const;                         /**< Is this animation looped */
		const char* getName() const;						/**< Get animation name */
		void        setName(const char* name);				/**< Set animation name */

		void addPositionKey(int bone, int frame, const vec3& position);	/**< Add a position keyframe */
		void addRotationKey(int bone, int frame, const Quaternion& q);	/**< Add a rotation keyframe */
		void addScaleKey   (int bone, int frame, const vec3& scale);	/**< Add a scale keyframe */

		bool removePositionKey(int bone, int frame);					/**< Delete a position keyframe */
		bool removeRotationKey(int bone, int frame);					/**< Delete a rotation keyframe */
		bool removeScaleKey(int bone, int frame);						/**< Delete a scale keyframe */

		int getPosition(int bone, float frame, int hint, vec3& position) const;			/**< Get the interpolated position at a frame */
		int getRotation(int bone, float frame, int hint, Quaternion& rotation) const;	/**< Get the interpolated rotation at a frame */
		int getScale   (int bone, float frame, int hint, vec3& scale) const;			/**< Get the interpolated scale at a frame */

		private:
		// Keyframe structure
		template<int N> struct Keyframe { int frame; float data[N]; };
		// Each bone has a keyframe list
		struct KeyframeList {
			std::vector< Keyframe<4> > rotation;
			std::vector< Keyframe<3> > position;
			std::vector< Keyframe<3> > scale;
		};

		KeyframeList* m_animations;		// Array of bone animations
		const         char* m_name;		// Animation name
		int           m_size;			// Number of bone animations
		int           m_length;			// Animation length in frames
		float         m_fps;			// Animation speed
		bool          m_loop;			// Does animation loop
		int           m_ref;			// Reference counter

		KeyframeList& addList(int bone);	// Get keyframe list from a bone, add if not there.
		template<int N> void addKey(    std::vector< Keyframe<N> >&, int frame, const float* value);
		template<int N> bool removeKey( std::vector< Keyframe<N> >&, int frame);
		template<int N, class P> int getValue( std::vector< Keyframe<N> >& keys, float frame, int hint, float* value, P interpolate) const;
	};

	// Interpolation functors
	struct LerpFunc {
		inline void operator()( float* r, const float* a, const float* b, float t) {
			r[0] = a[0] + (b[0]-a[0])*t;
			r[1] = a[1] + (b[1]-a[1])*t;
			r[2] = a[2] + (b[2]-a[2])*t;
		}
	};
	struct SlerpFunc {
		inline void operator() (float* out, const float* a, const float* b, float t) {
			float w1, w2;
			float dot = a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
			float theta = acos(dot);
			float sintheta = sin(theta);
			if (sintheta > 0.001) {
				w1 = sin((1.0f-t)*theta) / sintheta;
				w2 = sin(t*theta) / sintheta;
			} else {
				w1 = 1.0f - t;
				w2 = t;
			}
			out[0] = a[0] * w1 + b[0] * w2;
			out[1] = a[1] * w1 + b[1] * w2;
			out[2] = a[2] * w1 + b[2] * w2;
			out[3] = a[3] * w1 + b[3] * w2;
		}
	};

	// Inline implementation
	
	inline Animation*  Animation::grab()                 { ++m_ref; return this; }
	inline int         Animation::drop()                 { if(--m_ref>0) return m_ref; delete this; return 0; }
	inline int         Animation::getLength() const      { return m_length; }
	inline float       Animation::getSpeed() const       { return m_fps; }
	inline void        Animation::setSpeed(float fps)    { m_fps = fps; }
	inline void        Animation::setLoop(bool loop)     { m_loop = loop; }
	inline bool        Animation::isLoop() const         { return m_loop; }
	inline const char* Animation::getName() const        { return m_name; }
	inline void        Animation::setName(const char* n) { m_name = n; }

	
};
};

#endif

