#ifndef _GUI_LAYOUTS_
#define _GUI_LAYOUTS_

#include "gui.h"

namespace gui {

	// Layouts automatically position widgets
	class Layout {
		public:
		Layout(int margin=0, int space=0) : m_margin(margin), m_space(space), ref(0) {}
		virtual ~Layout() {}
		virtual uint type() const = 0;	// Get layout type id
		virtual const char* typeName() const = 0;	// Get layout type string
		virtual void apply(Widget*) const = 0;	// Apply layout to this widget
		int getMargin() const { return m_margin; }
		int getSpacing() const { return m_space; }
		protected:
		void assignSlot(const Rect& slot, class Widget*) const;
		int m_margin, m_space;
		private:
		friend class Widget;
		int ref;
	};

	#define DeclareLayout(Name) class Name : public Layout { \
		public: using Layout::Layout; \
		uint type() const override { return staticType(); } \
		const char* typeName() const override { return staticName(); } \
		void apply(Widget*) const override; \
		static uint staticType() { static uint id=__COUNTER__; return id; } \
		static const char* staticName() { return #Name; } \
	};
	
	DeclareLayout(HorizontalLayout);
	DeclareLayout(VerticalLayout);
	DeclareLayout(FlowLayout);
	DeclareLayout(FixedGridLayout);
	DeclareLayout(DynamicGridLayout);
}


#endif

