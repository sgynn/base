#ifndef _GIZMO_
#define _GIZMO_

#include <base/math.h>
#include <base/drawable.h>

namespace base { class HardwareVertexBuffer; class Camera; }

namespace editor {

/** Mouse picking data */
class MouseRay {
	public:
	MouseRay(base::Camera* camera, int mx, int my, int sw, int sh);
	public:
	vec3 start, direction;				// Ray in 3d scene
	vec2 screen;						// Screen coordinates
	vec3 project(const vec3&) const;	// Project 3d position to screen coordinates (z=depth)
	base::Camera* camera;
	private:
	Point m_viewport;
};

/// Transformation gizmo

class Gizmo : public base::Drawable {
	public:
	Gizmo();
	~Gizmo();

	void setScale(const vec3&);							/// Set scale
	void setPosition(const vec3&);						/// Set local position
	void setOrientation(const Quaternion&);				/// Set local orientation
	void setBasis(const Matrix&);						/// Set local coordinate space as global transform
	void setBasis(const Matrix&, const Quaternion&);	/// Set local coordinate space as global transform
	void setRelative(const Matrix&);					/// Set edit coordinate space (rotation)
	void setRelative(const Quaternion&);				/// Set edit coordinate space
	void setLocalMode();								/// Set local edit mode - separate as it changes as you edit

	const vec3&       getScale() const;				/// Get local scale
	const vec3&       getPosition() const;			/// Get local position
	const Quaternion& getOrientation() const;		/// Get local orientation

	bool isOver(const MouseRay&, float& t) const;	/// Is the mouse over this gizmo
	bool onMouseDown(const MouseRay&);				/// Handle mouseDown on this gizmo
	bool onMouseMove(const MouseRay&);				/// Handle mouseMove on this gizmo
	void onMouseUp();								/// Handle mouseUp on this gizmo
	bool isHeld() const;							/// Is a handle held


	enum Mode  { POSITION, ORIENTATION, SCALE };
	Mode getMode() const;						/// Get gizmo mode
	void setMode(Mode);							/// Set gizmo mode
	bool isLocal() const;						/// Are we in local mode

	void render(const vec3&, const vec3&) const;
	void draw(base::RenderState&);
	
	private:

	Quaternion mOrientation;	// Local orientation
	vec3       mPosition;		// Local position
	vec3       mScale;			// Local scale
	Matrix     mBasis;			// Coordinate space for translation
	Quaternion mBaseRot;		// Rotational offset basis, if translation basis is different from rotatoinal basis
	Quaternion mRelative;		// Relative rotation mode
	bool       mLocalMode;		// Local transform mode
	
	Mode  mMode;		// Current mode
	int   mHandle;		// Active handle index 0=none
	float mOffset;		// Held offset
	bool  mHeld;		// Is active handle held
	float mSize;		// Size of gizmo

	mutable float mCachedScale;		// Screen space scaling
	Quaternion mCachedOrientation;	// Orientation when grabbed

	int  pickHandle(const MouseRay& m, float threshold, float& offset, float& depth) const;
	void getSpaceQuaternions(const Quaternion& in, Quaternion& base, Quaternion& local) const;
	static float closestPointOnLine(const MouseRay& m, const vec3& a, const vec3& b);
	static bool overLine(const vec2& m, const vec3& a, const vec3& b, float threshold, float& depth, float& offset);
	static bool overRing(const MouseRay& m, const vec3& centre, const Quaternion& orientation, float radius, float threshold, float& depth, float& offset);
	static void decomposeOrientation(const Quaternion& in, Quaternion& y, Quaternion& p, Quaternion& r);

	private:
	struct Geometry {
		base::HardwareVertexBuffer* arrowData = 0;
		base::HardwareVertexBuffer* blockData = 0;
		base::HardwareVertexBuffer* ringData = 0;
		unsigned arrowBinding = 0;
		unsigned blockBinding = 0;
		unsigned ringBinding = 0;
	};
	static Geometry sGeometry;
	void createGeometry();
};

}


#endif

