#ifndef _XML_READER_
#define _XML_READER_

#include <list>

namespace base {
/** Simple XML reader, READ ONLY for now. Also doesn't support body text */
class XML {
	public:
	enum TAGTYPE { TAG, HEADER, COMMENT };
	class Element {
		friend class XML;
		public:
		TAGTYPE type() const { return m_type; }
		const char* name() const { return m_name; }
		const char* operator[](const char* value) const;
		const char* attribute(const char* name, const char* defaultValue="") const;
		float attribute(const char* name, float defaultValue) const;
		double attribute(const char* name, double defaultValue) const;
		int attribute(const char* name, int defaultValue) const;
		bool hasAttribute(const char*) const;
		//child nodes
		std::list<Element>::const_iterator begin() const { return m_children.begin(); }
		std::list<Element>::const_iterator end() const { return m_children.end(); }
		unsigned int size() const { return m_children.size(); }
		private:
		Element();
		TAGTYPE m_type;
		const char* m_name;
		std::list<Element> m_children;
		std::list<const char*> m_attributes;
	};
	typedef std::list<Element>::const_iterator Iterator;

	XML();
	~XML();
	bool loadFile(const char* filename);
	bool loadData(const char* data);
	const Element& getRoot() const { return m_root; }
	
	private:
	int parse();
	Element m_root;
	char* m_data;
	int m_length;
};
};

#endif

