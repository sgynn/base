#ifndef _GUI_RENDERER_
#define _GUI_RENDERER_

#include "gui.h"
#include "skin.h"

namespace gui {

struct ImageGenerator {
	enum Type { Single, Four, Eight, Glyphs };
	enum Corner { Square, Rounded, Chamfered };
	struct ColourPair { unsigned line=0, back=0; };
	Type type;						// What t generate
	int  radius = 0;				// Radius of corners/chamfer or glyph size
	Corner corner[4] = { Square, Square, Square, Square };	// Corner modes
	ColourPair colours[8];			// Colours for each state. guess if unset
	int colourVariance = 0;			// 3d shading variance
	int lineWidth = 1;				// line width
	Point core = Point(2,2);		// size of centre secton
};


/** Renderer class - Cannot be shared any more */
class Renderer {
	protected:
	struct Image { char name[128]; unsigned texture; float multX, multY; int ref; };
	std::vector<Image> m_images;
	String m_imagePath;

	friend class Root;
	int m_references = 0;

	public:
	Renderer();
	virtual ~Renderer();

	public: // Image data - ToDo extract to path
	virtual Skin* createDefaultSkin(unsigned line=0x0000a0, unsigned back=0x80000060);
	virtual int   generateImage(const char* name, const ImageGenerator& data);
	virtual int   addImage(const char* name);
	virtual int   addImage(const char* name, int width, int height, int channels, void* data, bool clamp=false);
	virtual int   addImage(const char* name, int width, int height, int glUnit);
	virtual int   getImage(const char* name) const;
	virtual bool  replaceImage(unsigned index, int width, int height, int glUnit);

	Point         getImageSize(unsigned index) const;
	const char*   getImageName(unsigned index) const;
	unsigned      getImageCount() const;

	void          grabImageReference(unsigned);
	void          dropImageReference(unsigned);

	void          setImagePath(const char* path);

	public:
	virtual void  begin(int w, int h);
	virtual void  end();

	virtual void  drawSkin(const Skin*, const Rect& r, unsigned colour=-1, int state=0, const char* text=0);
	virtual Point drawText(const Point&, const char* text, unsigned len, const Font*, int size, unsigned colour=0xffffffff);
	virtual Point drawText(const Point& p, const char* text, unsigned len, const Skin* skin, int state);
	virtual Point drawText(const Rect& r, const char* text, unsigned len, const Skin* skin, int state);
	virtual void  drawRect(const Rect&, unsigned colour=0xffffffff);
	virtual void  drawIcon(IconList* list, int index, const Rect& dest, float angle=0, unsigned colour=-1);
	virtual void  drawImage(int image, const Rect& rect, float angle=0, unsigned colour=0xffffff, float alpha=1.0);
	virtual void  drawLineStrip(int count, const Point* line, float width, const Point& offset, unsigned colour=-1);

	// Clipping
	void  push(const Rect& rect);
	void  pushNew(const Rect& rect);
	void  pop();

	protected:
	struct Vertex {
		float x, y, u, v; unsigned colour; 
		Vertex(float x, float y, float u, float v, unsigned c) : x(x), y(y), u(u), v(v), colour(c) {}
	};
	// Transient data fro building vertex buffers
	struct Batch {
		Batch* next = 0;
		std::vector<Rect> coverage;
		std::vector<Vertex> vertices;
		std::vector<unsigned short> indices;
		Rect scissor;
		Rect bounds;
		int image = 0;
		float line = 0;
	};
	// OpenGL render structure - can be peristant
	struct RenderBatch {
		Rect scissor;
		float line = 0;
		unsigned texture = 0;
		unsigned vao = 0;
		unsigned vx = 0;
		unsigned ix = 0;
		int size = 0;
	};

	unsigned m_shader = 0;
	std::vector<RenderBatch> m_renderData;
	std::vector<Rect> m_scissor;
	std::vector<Batch*> m_active;
	Batch* m_batches = 0;
	Batch* m_head = 0;
	Batch* getBatch(const Rect& rect, int image, float line=0);
	unsigned createShader();
	void buildRenderBatches();

	void  drawBox(const Rect& rect, int image, const Rect& src, unsigned colour, float angle=0);
	void  drawNineSlice(const Rect& rect, int image, const Rect& src, const Skin::Border& border, unsigned colour);

	public:
	Point drawText(const Point& pos, const Font* font, int size, unsigned colour, const char* text, unsigned len=0);
};

}

#endif

