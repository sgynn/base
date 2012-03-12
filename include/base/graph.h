#ifndef _GRAPH_
#define _GRAPH

#define MAX_GRAPH_LINES 12

#include "font.h"
#include "opengl.h"
#include <cstring>
#include <cstdio>

/** A simple opengl graph for debug use */
namespace base {
class Graph {
	public:
	Graph() : m_font(0), m_title(0), m_x(0), m_y(0), m_width(200), m_height(100), m_min(0), m_max(1), m_step(1) {
		memset(m_plot, 0, MAX_GRAPH_LINES*sizeof(Plot));
		resize();
	}
	~Graph() { 
		for(int i=0; m_plot[i].name && i<MAX_GRAPH_LINES; i++) {
			delete m_plot[i].name;
			if(m_plot[i].caption) delete m_plot[i].caption;
			delete m_plot[i].values;
		}
	}

	void setTitle(const char* title) 		 { m_title =  title; }
	void setRange(float min=0.0f, float max=1.0f)	 { m_min = min; m_max=max; }
	void setPosition(int x, int y)			 { m_x = x; m_y = y; }
	void setSize(int width, int height, float step=1){ m_width = width; m_height = height; m_step = step>0?step:1; resize(); }
	void setFont(Font* font, float scale=1)		 { m_font = font; m_fontScale = scale; }

	void setCaption(const char* name, const char* caption=0) {
		Plot* p = getPlot(name);
		if(p) {
			if(p->caption) delete p->caption;
			if(caption) p->caption = strdup(caption);
			else { p->caption = new char[32]; sprintf((char*)p->caption, "%s: %%1.3f", p->name); }
		}
	}
	void setStyle(const char* name, uint colour=0xffffff, float lineWidth=1, float fill=false) {
		Plot* p = getPlot(name);
		if(p) {
			p->colour = colour;
			p->lineWidth = lineWidth;
			p->filled = fill;
		}
	}
	void push(const char* name, float value) {
		Plot* p = getPlot(name);
		if(p) {
			++p->front %= m_length;
			p->values[p->front] = value;
			if(p->back == p->front) ++p->back %= m_length;
		}
	}
	void set(const char* name, float value) {
		Plot* p = getPlot(name);
		if(p) p->values[ p->front] = value;
	}
	float get(const char* name) {
		Plot* p = getPlot(name);
		return p? p->values[ p->front ]: 0;
	}
	
	/** Draw the graph */
	void draw() {
		glDisable(GL_TEXTURE_2D);
		//Background
		glBegin(GL_QUADS);
		glColor4f(1,1,1,0.6);
		glVertex2f(m_x,m_y);
		glVertex2f(m_x+m_width,m_y);
		glVertex2f(m_x+m_width,m_y+m_height);
		glVertex2f(m_x,m_y+m_height);
		glEnd();
		//Lines
		float lw = 1;
		for(int i=0; m_plot[i].name && i<MAX_GRAPH_LINES; i++) {
			Plot& p = m_plot[i];
			int count = p.front>p.back? p.front-p.back: p.front-p.back+m_length;
			if(p.filled) {
				glColor4f(((p.colour>>16)&0xff)/255.0f, ((p.colour>>8)&0xff)/255.0f, (p.colour&0xff)/255.0f, 0.4f);
				glBegin(GL_TRIANGLE_STRIP);
				for(int j=0; j<count; j++) {
					float value = (p.values[(p.front-j+m_length)%m_length] - m_min) / (m_max-m_min);
					if(value<0) value=0; else if(value>1) value=1; //clamp
					glVertex2f(m_x+m_width-j*m_step, m_y);
					glVertex2f(m_x+m_width-j*m_step, m_y+value*m_height);
				}
				glEnd();
			}
			if(p.lineWidth!=lw) glLineWidth(p.lineWidth);
			glColor4f(((p.colour>>16)&0xff)/255.0f, ((p.colour>>8)&0xff)/255.0f, (p.colour&0xff)/255.0f, 1.0f);
			glBegin(GL_LINE_STRIP);
			for(int j=0; j<count; j++) {
				float value = (p.values[(p.front-j+m_length)%m_length] - m_min) / (m_max-m_min);
				if(value<0) value=0; else if(value>1) value=1; //clamp
				glVertex2f(m_x+m_width-j*m_step, m_y+value*m_height);
			}
			glEnd();
		}
		//Border
		if(lw!=1) glLineWidth(1);
		glBegin(GL_LINE_LOOP);
		glColor4f(0,0,0,1);
		glVertex2f(m_x,m_y);
		glVertex2f(m_x+m_width,m_y);
		glVertex2f(m_x+m_width,m_y+m_height);
		glVertex2f(m_x,m_y+m_height);
		glEnd();
		//Any text?
		if(m_font) {
			glEnable(GL_TEXTURE_2D);
			int step = m_font->textHeight("A") * m_fontScale;
			int py = m_y + m_height - step-2;
			if(m_title) { m_font->print(m_x+2,py,m_fontScale, m_title); py-=step; }
			for(int i=0; m_plot[i].name && i<MAX_GRAPH_LINES; i++) {
				Plot& p = m_plot[i];
				if(p.caption) {
					glColor3f(((p.colour>>16)&0xff)/255.0f, ((p.colour>>8)&0xff)/255.0f, (p.colour&0xff)/255.0f);
					m_font->printf(m_x+5, py, m_fontScale, p.caption, p.values[ p.front ] );
					py -= step;
				}
			}
		}
	}

	protected:
	Font* m_font;
	float m_fontScale;
	const char* m_title;
	int m_x, m_y, m_width, m_height;
	float m_min, m_max, m_step;
	uint m_length;

	struct Plot {
		const char* name;
		const char* caption;
		float* values;
		unsigned int front;
		unsigned int back;
		unsigned int colour;
		float lineWidth;
		bool filled;
	} m_plot[MAX_GRAPH_LINES];
	Plot* getPlot(const char* name) {
		int i;
		for(i=0; m_plot[i].name && i<MAX_GRAPH_LINES; i++) if(strcmp(name, m_plot[i].name)==0) return &m_plot[i];
		if(i<MAX_GRAPH_LINES) {
			m_plot[i].name = strdup(name);
			m_plot[i].caption = new char[32]; sprintf((char*)m_plot[i].caption, "%s: %%1.3f", name);
			m_plot[i].values = new float[ m_length ];
			m_plot[i].front = m_plot[i].back = 0;
			m_plot[i].values[0] = 0;
			m_plot[i].colour = 0xffffff;
			m_plot[i].lineWidth = 1;
			m_plot[i].filled = false;
			return &m_plot[i];
		} else return 0;
	}
	void resize() {
		int newSize = m_width / m_step + 2;
		for(int i=0; m_plot[i].name && i<MAX_GRAPH_LINES; i++) {
			float* tmp = new float[newSize];
			int count = m_plot[i].front - m_plot[i].back;
			if(count>newSize) m_plot[i].back = (m_plot[i].back + (count-newSize)) % m_length;
			if(count>=0) {
				memcpy(tmp, m_plot[i].values+m_plot[i].back, count*sizeof(float));
			} else {
				count += m_length;
				int r = m_length-m_plot[i].back;
				memcpy(tmp, m_plot[i].values+m_plot[i].back, r*sizeof(float));
				memcpy(tmp+r, m_plot[i].values, (count-r)*sizeof(float));
			}
			m_plot[i].back=0;
			m_plot[i].front = count;
		}
		m_length = newSize;
	}
};
};

#endif

