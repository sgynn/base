#include <base/gui/skin.h>
#include <base/gui/renderer.h>
#include <base/gui/font.h>
#include <base/opengl.h>
#include <base/texture.h>
#include <base/png.h>
#include <cstdio>
#include <cstring>

using namespace gui;

int IconList::addIcon(const char* name, const Rect& r) {
	m_icons.push_back( Icon() );
	m_icons.back().rect = r;
	strcpy(m_icons.back().name, name);
	return m_icons.size() - 1;
}
int IconList::getIconIndex(const char* name) const {
	for(uint i=0; i<m_icons.size(); ++i)
		if(strcmp(m_icons[i].name, name)==0) return i;
	return -1;
}
int IconList::getImageIndex() const { return m_image; }
int IconList::size() const { return m_icons.size(); }
const Rect& IconList::getIconRect(int i) const { return m_icons.at(i).rect; }
void IconList::setImageIndex(int img) { m_image = img; }
const char* IconList::getIconName(int index) const {
	if(index>=0 && index<(int)m_icons.size()) return m_icons[index].name;
	else return 0;
}
void IconList::setIconRect(int index, const Rect& r) {
	if(index<0 || index>=(int)m_icons.size()) return;
	m_icons[index].rect = r;
}
void IconList::setIconName(int index, const char* name) {
	strcpy(m_icons[index].name, name);
}
void IconList::deleteIcon(int i) {
	if((size_t)i < m_icons.size()) {
		m_icons.erase(m_icons.begin() + i);
	}
}
void IconList::clear() {
	m_icons.clear();
}



// ------------------------------------------------------------------------------- //

void Renderer::setImagePath(const char* path) {
	m_imagePath = path;
}

int Renderer::createTexture(int w, int h, int channels, void* data, bool clamp) {
	base::Texture t = base::Texture::create(w, h, channels, data);
	if(clamp) t.setWrap(base::Texture::CLAMP);
	return t.unit();
}

int Renderer::addImage(const char* file) {
	base::PNG png;
	if(m_imagePath.empty()) png = base::PNG::load(file);
	else png = png.load(m_imagePath + file);
	if(!png.data) return -1;
	return addImage(file, png.width, png.height, png.bpp/8, png.data);
}
int Renderer::addImage(const char* name, int w, int h, int channels, void* data, bool clamp) {
	if(m_atlases.empty()) createAtlas(256, 256);
	int image = addImage(name, w, h, 0);
	if(channels!=4 || !addToAtlas(0, image, data)) {
		m_images[image].texture = createTexture(w, h, channels, data, true);
	}
	return image;
}
int Renderer::addImage(const char* name, int width, int height, int glUnit) {
	m_images.push_back( Image() );
	Image& img = m_images.back();
	img.texture = glUnit;
	img.atlased = 0;
	img.size.set(width, height);
	img.multX = 1.0f / width;
	img.multY = 1.0f / height;
	img.offX = 0;
	img.offY = 0;
	img.ref = 0;
	img.name = name;
	return m_images.size() - 1;
}
bool Renderer::replaceImage(unsigned index, int w, int h, int channels, void* data) {
	if(index >= m_images.size()) return false;
	Image& image = m_images[index];
	image.size.set(w,h);
	if(image.atlased) {
		int atlas = image.texture;
		removeFromAtlas(index);
		if(!addToAtlas(atlas, index, data)) {
			image.texture = createTexture(w, h, channels, data, true);
			image.multX = 1.f / w;
			image.multY = 1.f / h;
		}
	}
	else {
		int format = base::Texture::getInternalFormat((base::Texture::Format)channels);
		glBindTexture(GL_TEXTURE_2D, image.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
		replaceImage(index, w, h, image.texture);
	}
	return true;
}
bool Renderer::replaceImage(unsigned index, int w, int h, int unit) {
	if(index >= m_images.size()) return false;
	removeFromAtlas(index);
	Image& image = m_images[index];
	image.texture = unit;
	image.size.set(w,h);
	image.multX = 1.f / w;
	image.multY = 1.f / h;
	return true;
}

int Renderer::getImage(const char* name) const {
	for(unsigned i=0; i<m_images.size(); ++i) if(m_images[i].name == name) return i;
	return -1;
}



unsigned Renderer::getImageCount() const { return m_images.size(); }
Point Renderer::getImageSize(unsigned index) const {
	if(index<m_images.size()) return m_images[index].size;
	else return Point(0,0);
}
const char* Renderer::getImageName(unsigned index) const {
	return index<m_images.size()? m_images[index].name.str(): "";
}

void Renderer::grabImageReference(unsigned i) {
	if(i<m_images.size()) ++m_images[i].ref;
}
void Renderer::dropImageReference(unsigned i) {
	if(i<m_images.size() && m_images[i].ref>0) --m_images[i].ref;
}

// ========================================================================================= //

int Renderer::createAtlas(int width, int height) {
	base::Texture t = base::Texture::create(width, height, base::Texture::RGBA8, 0);
	t.setWrap(base::Texture::CLAMP);
	m_atlases.push_back(Atlas{t.unit(), width, height});
	return m_atlases.size() - 1;
}

bool Renderer::addToAtlas(int atlasId, int imageId, void* imageData) {
	Atlas& atlas = m_atlases[atlasId];
	Image& image = m_images[imageId];
	AtlasedImage img { imageId, Rect(0, 0, image.size) };
	if(img.rect.width > atlas.width || img.rect.height > atlas.height) return false;
	if(!atlas.images.empty()) {
		// Brute force it
		auto test = [](const Atlas& atlas, const AtlasedImage& img) {
			if(atlas.width < img.rect.right()) return false;
			if(atlas.height < img.rect.bottom()) return false;
			for(const AtlasedImage& i: atlas.images) if(i.rect.intersects(img.rect)) return false;
			return true;
		};
		
		bool success = false;
		for(AtlasedImage& i: atlas.images) {
			img.rect.x = i.rect.right();
			img.rect.y = i.rect.y;
			if(test(atlas, img)) { success=true; break; };
			img.rect.x = i.rect.x;
			img.rect.y = i.rect.bottom();
			if(test(atlas, img)) { success=true; break; };
		}
		if(!success) return false;
	}
	glBindTexture(GL_TEXTURE_2D, atlas.texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, img.rect.x, img.rect.y, img.rect.width, img.rect.height, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

	float mx = 1.f / atlas.width;
	float my = 1.f / atlas.height;
	image.atlased = 1;
	image.texture = atlasId;
	image.offX = img.rect.x * mx;
	image.offY = img.rect.y * my;
	image.multX = mx;
	image.multY = my;
	atlas.images.push_back(img);
	return true;
}

bool Renderer::removeFromAtlas(int imageId) {
	Image& image = m_images[imageId];
	if(image.atlased) {
		Atlas& atlas = m_atlases[image.texture];
		for(size_t i=0; i<atlas.images.size(); ++i) {
			if(atlas.images[i].image == imageId) {
				atlas.images[i] = atlas.images.back();
				atlas.images.pop_back();
				image.atlased = 0;
				image.texture = 0;
				image.offX = image.offY = 0;
				return true;
			}
		}
	}
	return false;
}

// ========================================================================================= //

Renderer::Renderer() : m_references(0) {
	m_shader = createShader();
	unsigned data = 0xffffffff;
	addImage("blank", 1, 1, 4, &data); // index will be 0
}

Renderer::~Renderer() {
	for(RenderBatch& b: m_renderData) {
		glDeleteBuffers(2, &b.ix);
		glDeleteVertexArrays(1, &b.vao);
	}
}

Renderer::Batch* Renderer::getBatch(const Rect& box, int image, float line) {
	if(image<0 || !m_scissor.back().intersects(box)) return 0;

	int texture;
	if(image & 0x10000) texture = image & 0xffff; // Use opengl unit directly
	else {
		const Image& img = m_images[image];
		texture = img.atlased? m_atlases[img.texture].texture: img.texture;
	}

	// Get active batch
	size_t activeIndex = 0;
	for(activeIndex = 0; activeIndex<m_active.size(); ++activeIndex) {
		Batch* b = m_active[activeIndex];
		if(b->texture == texture && b->line == line) break;
	}
	Batch* result = activeIndex<m_active.size()? m_active[activeIndex]: 0;


	// Scissor
	size_t needScissor = 0;
	const Rect& activeScissor = result? result->scissor: m_head? m_head->scissor: m_scissor.back();
	if(activeScissor != m_scissor.back()) {
		if(!activeScissor.contains(box.intersection(m_scissor.back()))) {
			needScissor = m_scissor.size() - 1; //!m_scissor.back().contains(box);
			// May be a better scissor to change to. Ideally farther up stack
			while(needScissor > 1) {
				const Rect& s = m_scissor[needScissor-1];
				if(!m_scissor.back().contains(box.intersection(s))) break;
				--needScissor;
			}
			result = 0;
		}
		else if(!m_scissor.back().contains(box.intersection(activeScissor))) {
			// can we apply this scissor to the current batch
			for(Batch* b = result; b; b=b->next) {
				if(!m_scissor.back().contains(b->bounds)) { result=0; break; }
			}
			if(result) for(Batch* b = result; b; b=b->next) b->scissor = m_scissor.back();
			needScissor = m_scissor.size() - 1;
		}
	}

	// Do we overlap with any later batches
	if(result) for(Batch* b = result->next; b&&result; b=b->next) {
		for(const Rect& r: b->coverage) {
			if(r.intersects(box)) {
				result = 0;
				if(!needScissor) needScissor = ~0u; // // Keep active scissor
				break;
			}
		}
	}

	// Vertex buffer has become too large for uin16 indices
	if(result && result->vertices.size() > 65500) result = 0;

	// new batch
	if(result) result->bounds.include(box);
	else {
		result = new Batch();
		result->texture = texture;
		result->line = line;
		result->next = 0;
		result->bounds = box;
		
		if(!m_head) {
			m_batches = m_head = result;
			result->scissor = m_scissor[needScissor];
			m_active.push_back(result);
		}
		else {
			m_head->next = result;
			if(needScissor == 0) result->scissor = m_head->scissor;
			else if(needScissor == ~0u) result->scissor = activeScissor;
			else result->scissor = m_scissor[needScissor];
			m_head = result;
		}
		if(activeIndex<m_active.size()) m_active[activeIndex] = result;
		else m_active.push_back(result);
	}
	result->coverage.push_back(box.intersection(m_scissor.back()));
	return result;
}

unsigned Renderer::createShader() {
	static unsigned exists = 0;
	if(exists && glIsProgram(exists)) return exists;

	static const char vs_src[] =
		#ifdef EMSCRIPTEN
		"precision highp float;"
		#endif
		"attribute vec2 vertex;"
		"attribute vec2 texcoord;"
		"attribute vec4 colour;"
		"uniform mat4 transform;"
		"varying vec2 uv;"
		"varying vec4 col;"
		"void main() { gl_Position=transform*vec4(vertex.xy,0,1); col=colour.bgra; uv=texcoord; }";
	static const char fs_src[] = 
		#ifdef EMSCRIPTEN
		"precision highp float;"
		#endif
		"varying vec2 uv;"
		"varying vec4 col;"
		"uniform sampler2D image;"
		"void main() { gl_FragColor = col * texture2D(image, uv); }";

	const char* vs = vs_src;
	const char* fs = fs_src;
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vert, 1, &vs, NULL);
	glShaderSource(frag, 1, &fs, NULL);
	glCompileShader(vert);
	glCompileShader(frag);

	GLuint shader = glCreateProgram();
	glAttachShader(shader, vert);
	glAttachShader(shader, frag);
	glBindAttribLocation(shader, 0, "vertex");
	glBindAttribLocation(shader, 3, "texcoord");
	glBindAttribLocation(shader, 4, "colour");
	glLinkProgram(shader);

	char buffer[1024];
	glGetProgramInfoLog(shader, 1024, 0, buffer);
	if(buffer[0]) printf("%s\n", buffer);

	exists = shader;
	return shader;
}

void Renderer::buildRenderBatches() {
	size_t index = 0;
	Batch* previous = m_batches;
	for(Batch* b = m_batches; b; b=b->next) {
		if(b->indices.empty() || b->scissor.width<=0 || b->scissor.height<=0) continue;
		if(m_renderData.size() <= index) m_renderData.emplace_back();
		RenderBatch& r = m_renderData[index++];
		if(!r.vao) {
			glGenVertexArrays(1, &r.vao);
			glGenBuffers(2, &r.vx);
			glBindVertexArray(r.vao);
			glBindBuffer(GL_ARRAY_BUFFER, r.vx);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ix);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(3);
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), 0);
			glVertexAttribPointer(3, 2, GL_FLOAT, false, sizeof(Vertex), (void*)8);
			glVertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex), (void*)16);
		}
		else {
			glBindBuffer(GL_ARRAY_BUFFER, r.vx);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r.ix);
		}
		glBufferData(GL_ARRAY_BUFFER, b->vertices.size() * sizeof(Vertex), &b->vertices[0], GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, b->indices.size()*2, &b->indices[0], GL_STATIC_DRAW);
		r.size = b->indices.size();
		r.texture = b->texture;
		r.line = b->line;

		if(previous!=b && previous->scissor == b->scissor) r.scissor.set(0,0,0,0);
		else r.scissor = b->scissor;
		previous = b;
	}
	for(size_t i=index; i<m_renderData.size(); ++i) m_renderData[i].size = 0;
	m_active.clear();
	m_head = 0;
	while(m_batches) {
		Batch* b = m_batches;
		m_batches = b->next;
		delete b;
	}
}

void Renderer::begin(const Point& root, const Point& viewport) {
	m_scissor.emplace_back(0, 0, root.x, root.y);
	m_viewport = viewport;
}

void Renderer::end() {
	Point size = m_scissor[0].size();
	m_scissor.clear();

	// Create vertex buffers if changed
	buildRenderBatches();

	if(m_renderData.empty() || m_renderData[0].size==0) return;

	Matrix transform;
	transform[0] = 2.f/size.x;
	transform[5] = -2.f/size.y;
	transform[10] = 1;
	transform[12] = -1;
	transform[13] =  1;
	transform[14] = 0;

	int sx = size.x, sy = size.y;
	int wx = m_viewport.x, wy = m_viewport.y;

	// Draw
	GL_CHECK_ERROR;
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);
	glUseProgram(m_shader);
	static int loc = glGetUniformLocation(m_shader, "transform");
	glUniformMatrix4fv(loc, 1, false, transform); // 1 should be the right location. Static though so we could use a buffer
	float lineWidth = 1;
	for(const RenderBatch& b: m_renderData) {
		if(b.size==0) continue;
		if(b.scissor.width) glScissor(b.scissor.x*wx/sx, wy - b.scissor.bottom()*wy/sy, b.scissor.width*wx/sx, b.scissor.height*wy/sy);
		if(b.line && b.line!=lineWidth) lineWidth=b.line, glLineWidth(b.line);
		glBindVertexArray(b.vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.ix);
		glBindTexture(GL_TEXTURE_2D, b.texture);
		glDrawElements(b.line? GL_LINE_STRIP: GL_TRIANGLES, b.size, GL_UNSIGNED_SHORT, 0);
	}
	if(lineWidth!=1) glLineWidth(1);
	glDisable(GL_SCISSOR_TEST);
	glBindVertexArray(0);
	glDepthMask(1);
	glEnable(GL_CULL_FACE);
	GL_CHECK_ERROR;
}


void Renderer::push(const Rect& rect) {
	Rect r = m_scissor.back();
	r.intersect(rect);
	m_scissor.push_back(r);
	// return r.width && r.height; // do we do the push if returning false? probably not.
}
void Renderer::pushNew(const Rect& rect) { m_scissor.push_back(rect); }
void Renderer::pop() { m_scissor.pop_back(); }

// ==================================================== //

void Renderer::drawBox(const Rect& rect, int image, const Rect& src, const unsigned* colour, bool gradient, float angle) {
	if(colour[0]>>24==0 && !gradient) return; // fully transparent
	Batch* batch = getBatch(rect, image);
	if(!batch) return;
	Batch& b = *batch;

	const float ix = m_images[image].multX;
	const float iy = m_images[image].multY;
	const float ox = m_images[image].offX;
	const float oy = m_images[image].offY;

	unsigned short start = b.vertices.size();
	b.vertices.emplace_back(rect.x,       rect.y,        src.x*ix+ox+ix/2,       src.y*iy+oy+iy/2,        colour[0]);
	b.vertices.emplace_back(rect.right(), rect.y,        src.right()*ix+ox-ix/2, src.y*iy+oy+iy/2,        colour[1&gradient]);
	b.vertices.emplace_back(rect.x,       rect.bottom(), src.x*ix+ox+ix/2,       src.bottom()*iy+oy-iy/2, colour[2&gradient]);
	b.vertices.emplace_back(rect.right(), rect.bottom(), src.right()*ix+ox-ix/2, src.bottom()*iy+oy-iy/2, colour[3&gradient]);
	
	if(angle != 0) {
		float sinAngle = sin(angle);
		float cosAngle = cos(angle);
		float cx = rect.width/2.f;
		float cy = rect.height/2.f;
		float xx = sinAngle * cx;
		float xy = cosAngle * cx;
		float yx = cosAngle * cy;
		float yy =-sinAngle * cy;
		cx += rect.x;
		cy += rect.y;
		b.vertices[start  ].x = cx - xx - yx;
		b.vertices[start  ].y = cy - xy - yy;
		b.vertices[start+1].x = cx - xx + yx;
		b.vertices[start+1].y = cy - xy + yy;
		b.vertices[start+2].x = cx + xx - yx;
		b.vertices[start+2].y = cy + xy - yy;
		b.vertices[start+3].x = cx + xx + yx;
		b.vertices[start+3].y = cy + xy + yy;
	}

	b.indices.push_back(start);
	b.indices.push_back(start+2);
	b.indices.push_back(start+1);
	b.indices.push_back(start+1);
	b.indices.push_back(start+2);
	b.indices.push_back(start+3);
}

void Renderer::drawNineSlice(const Rect& rect, int image, const Rect& src, const Skin::Border& border, unsigned colour) {
	if(colour>>24==0) return; // fully transparent
	Batch* batch = getBatch(rect, image);
	if(!batch) return;

	// Collapse centre if borders overlap
	Skin::Border b = border;
	if(rect.width < b.left+b.right)  b.right  = b.right * rect.width / (b.left+b.right),   b.left = rect.width - b.right;
	if(rect.height < b.top+b.bottom) b.bottom = b.bottom * rect.height / (b.top+b.bottom), b.top = rect.height - b.bottom;

	const float ix = m_images[image].multX;
	const float iy = m_images[image].multY;
	const float ox = m_images[image].offX;
	const float oy = m_images[image].offY;

	float dx[4] = { (float)rect.x, (float)rect.x + b.left, (float)rect.right() - b.right, (float)rect.right() };
	float dy[4] = { (float)rect.y, (float)rect.y + b.top, (float)rect.bottom() - b.bottom, (float)rect.bottom() };
	float sx[4] = { src.x*ix+ox, (src.x + b.left)*ix+ox, (src.right() - b.right)*ix+ox,   src.right()*ix+ox };
	float sy[4] = { src.y*iy+oy, (src.y + b.top)*iy+oy,  (src.bottom() - b.bottom)*iy+oy, src.bottom()*iy+oy };

	unsigned short start = batch->vertices.size();
	for(int i=0; i<4; ++i) {
		batch->vertices.emplace_back(dx[0], dy[i], sx[0], sy[i], colour);
		batch->vertices.emplace_back(dx[1], dy[i], sx[1], sy[i], colour);
		batch->vertices.emplace_back(dx[2], dy[i], sx[2], sy[i], colour);
		batch->vertices.emplace_back(dx[3], dy[i], sx[3], sy[i], colour);
	}

	batch->indices.reserve(batch->indices.size() + 54);
	unsigned short indices[] = { 0,4,1,1,4,5,  1,5,2,2,5,6,  2,6,3,3,6,7 };
	for(int j=0; j<3; ++j) {
		for(unsigned short i : indices) batch->indices.push_back(start + i);
		start += 4;
	}
}

Point Renderer::drawText(const Point& pos, const Font* font, int size, unsigned colour, const char* text, uint limit) {
	if(colour>>24==0) return pos; // fully transparent
	const Rect rect(pos, font->getSize(text, size));
	Batch* batch = getBatch(rect, font->getTexture() | 0x10000);
	if(!batch) return Point(rect.right(), pos.y);

	uint len = strlen(text);
	if(limit>0 && limit<len) len = limit;
	float scale = font->getScale(size);
	Point ts = font->getTextureSize();
	float ix = 1.f/ts.x;
	float iy = 1.f/ts.y;
	Rect dst = rect;
	batch->vertices.reserve(batch->vertices.size() + len*4);
	batch->indices.reserve(batch->vertices.size() + len*5);
	for(const char* c=text; *c&&len; ++c, --len) {
		if(*c=='\n') {
			dst.x = pos.x;
			dst.y += font->getLineHeight(size);
			if(dst.y >= m_scissor.back().bottom()) break;
		}
		else {
			const Rect& src = font->getGlyph(*c);
			if(src.width) {
				dst.width = src.width * scale;
				dst.height = src.height * scale;
				if(m_scissor.back().intersects(dst)) {
					batch->vertices.emplace_back(dst.x,       dst.y,        src.x*ix,       src.y*iy,       colour);
					batch->vertices.emplace_back(dst.right(), dst.y,        src.right()*ix, src.y*iy, colour);
					batch->vertices.emplace_back(dst.x,       dst.bottom(), src.x*ix,       src.bottom()*iy,       colour);
					batch->vertices.emplace_back(dst.right(), dst.bottom(), src.right()*ix, src.bottom()*iy, colour);

					unsigned short k = batch->vertices.size();
					batch->indices.push_back(k-4);
					batch->indices.push_back(k-2);
					batch->indices.push_back(k-3);
					batch->indices.push_back(k-3);
					batch->indices.push_back(k-2);
					batch->indices.push_back(k-1);
				}
				dst.x += dst.width;
			}
		}
	}
	return dst.position();
}

void Renderer::drawLineStrip(int count, const Point* line, float width, const Point& offset, unsigned colour) {
	if(count==0) return;
	Rect rect(line[0].x, line[0].y, 0, 0);
	for(int i=1; i<count; ++i) rect.include(line[i]);
	rect.position() += offset;
	Batch* b = getBatch(rect, 0, width);
	if(b) {
		// Split multiple line strips
		if(!b->vertices.empty()) {
			b->vertices.push_back(b->vertices.back());
			b->vertices.back().colour = 0;
			b->vertices.emplace_back((float)line[0].x+offset.x, (float)line[0].y+offset.y, 0, 0, 0);
			b->indices.push_back(b->vertices.size()-2);
			b->indices.push_back(b->vertices.size()-1);
		}

		b->vertices.reserve(b->vertices.size() + count);
		b->indices.reserve(b->indices.size() + count);
		unsigned short start = b->vertices.size();
		for(int i=0; i<count; ++i) {
			b->vertices.emplace_back((float)line[i].x+offset.x, (float)line[i].y+offset.y, 0, 0, colour);
			b->indices.push_back(start+i);
		}
	}
}

void Renderer::drawPolygon(int size, const Point* points, const Point& offset, unsigned colour) {
	if(size<3) return;
	Rect rect(points[0].x, points[0].y, 0, 0);
	for(int i=1; i<size; ++i) rect.include(points[i]);
	rect.position() += offset;
	if(Batch* b = getBatch(rect, 0)) {
		b->vertices.reserve(b->vertices.size() + size);
		b->indices.reserve(b->indices.size() + (size-2) * 3);
		unsigned short start = b->vertices.size();
		for(int i=0; i<size; ++i) b->vertices.emplace_back((float)points[i].x+offset.x, (float)points[i].y+offset.y, 0, 0, colour);
		for(int i=2; i<size; ++i) {
			b->indices.push_back(start);
			b->indices.push_back(start+i-1);
			b->indices.push_back(start+i);
		}
	}
}

void Renderer::drawSkin(const Skin* skin, const Rect& r, unsigned colour, int state, const char* text) {
	if(!skin || r.width<=0 || r.height<=0) return;
	const Skin::State& s = skin->getState(state);
	if(s.border.top || s.border.left || s.border.right || s.border.bottom) {
		drawNineSlice(r, skin->getImage(), s.rect, s.border, colour); 
	}
	else drawBox(r, skin->getImage(), s.rect, &colour);
	if(text && text[0]) drawText(r, text, 0, skin, state);
}

void Renderer::drawRect(const Rect& r, uint colour) {
	drawBox(r, 0, Rect(0,0,1,1), &colour);
}

void Renderer::drawImage(int image, const Rect& r, float angle, uint colour, float alpha) {
	Point s = getImageSize(image);
	colour |= (int)(alpha*0xff) << 24;
	drawBox(r, image, Rect(0,0,s.x,s.y), &colour, false, angle);
}

void Renderer::drawIcon(IconList* list, int index, const Rect& r, float angle, unsigned colour) {
	if(!list || index<0 || index>=list->size()) return;
	const Rect& src = list->getIconRect(index);
	drawBox(r, list->getImageIndex(), src, &colour, false, angle);
}

void Renderer::drawGradient(int image, const Rect& r, unsigned c0, unsigned c1, int axis) {
	unsigned c[4] = {c0,c1,c0,c1};
	if(axis) c[1]=c0, c[2]=c1;
	Point s = getImageSize(image);
	drawBox(r, image, Rect(0,0,s.x,s.y), c, true);
}



Point Renderer::drawText(const Point& p, const char* text, uint len, const Font* font, int size, unsigned colour) {
	return drawText(p, font, size, colour, text, len);
}

Point Renderer::drawText(const Point& p, const char* text, uint len, const Skin* skin, int state) {
	Skin::State& s = skin->getState(state);
	Point r =  drawText(p + s.textPos, skin->getFont(), skin->getFontSize(), s.foreColour, text, len);
	return r - s.textPos;
}

Point Renderer::drawText(const Rect& r, const char* text, uint len, const Skin* skin, int state) {
	Point p = r.position();
	int align = skin->getFontAlign();
	if(align != 0x5) {
		Point s = skin->getFont()->getSize(text, skin->getFontSize(), len? len: -1);
		if((align&0x3)==ALIGN_RIGHT) p.x = r.right() - s.x;
		else if((align&0x3)==ALIGN_CENTER) p.x += r.width/2 - s.x/2;
		if((align&0xc)==ALIGN_BOTTOM) p.y = r.bottom() - s.y;
		else if((align&0xc)==ALIGN_MIDDLE) p.y += r.height/2 - s.y/2;
	}
	return drawText(p + skin->getState(state).textPos, text, len, skin, state);
}









