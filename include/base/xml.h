#ifndef _BASE_XML_
#define _BASE_XML_

#include <list>

namespace base {

/** XML Element class */
class XMLElement {
	friend class XML;
	public:
	/** Get the type of this node (XML::TagType) */
	int type() const { return m_type; }
	/** Get the element name. Returns null if type==COMMENT */
	const char* name() const { return m_name; }
	/** Get the body text or comment text - does not support child elements */
	const char* text() const { return m_text; }
	/** Get an attribute value. Returns 0 if not found */
	const char* operator[](const char* value) const;
	/** Get a string attribute */
	const char* attribute(const char* name, const char* defaultValue="") const;
	/** Get an float attribute */
	float attribute(const char* name, float defaultValue) const;
	/** Get a double attribute */
	double attribute(const char* name, double defaultValue) const;
	/** Get an integer attribute */
	int attribute(const char* name, int defaultValue) const;
	/** Does this element have a named attribute */
	bool hasAttribute(const char*) const;
	/** Child element iterator start */
	std::list<XMLElement>::const_iterator begin() const { return m_children.begin(); }
	/** Child node iterator end */
	std::list<XMLElement>::const_iterator end() const { return m_children.end(); }
	/** Get The number of child elements */
	unsigned int size() const { return m_children.size(); }
	private:
	XMLElement();
	int m_type; // XML::TagType
	const char* m_name;
	const char* m_text;
	std::list<XMLElement> m_children;
	std::list<const char*> m_attributes;
};


/** Simple XML reader, READ ONLY for now. */
class XML {
	public:
	enum TagType { TAG, HEADER, COMMENT };
	typedef XMLElement Element;
	/** XML Element iterator */
	typedef std::list<Element>::const_iterator iterator;

	XML();
	~XML();
	XML(const XML&);
	const XML& operator=(const XML&);

	/** Get the XML root node */
	const Element& getRoot() const { return m_root; }

	/** Load and parse an XML file from disk */
	static XML load(const char* filename);
	/** Parse an XML file from a string */
	static XML parse(const char* string);
	
	private:
	int parse();		// Parse the XML string to create Element objects
	Element m_root;		// Root element
	char* m_data;		// XML data. Elements point to parts of this
	int m_length;		// Length of data string
	int* m_ref;			// Reference counter for data string to fix object copying
};
};

#endif

