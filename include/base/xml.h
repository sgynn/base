#ifndef _BASE_XML_
#define _BASE_XML_

#include <vector>
#include <base/hashmap.h>

namespace base {

/** Reference counted string */
class RefString { 
	char* s;
	int* ref; 
	void drop();
	public:
	RefString(const char* s=0);
	RefString(const RefString&);
	~RefString();
	const RefString& operator=(const RefString&);
	const RefString& operator=(const char*);
	operator const char*() const { return s; }
	RefString substr(size_t start, size_t len=~0ull);
};


/** XML Element class */
class XMLElement {
	friend class XML;
	public:
	XMLElement(int type=0);
	XMLElement(const char* tag);
	/** Get the type of this node (XML::TagType) */
	int type() const { return m_type; }
	/** Get the element name. Returns null if type==COMMENT */
	const char* name() const { return m_name; }
	/** Get the body text or comment text - does not support child elements */
	const char* text() const { return m_text; }
	/** Get a string attribute */
	const char* attribute(const char* name, const char* defaultValue="") const;
	/** Get an float attribute */
	float attribute(const char* name, float defaultValue) const;
	/** Get a double attribute */
	double attribute(const char* name, double defaultValue) const;
	/** Get an integer attribute */
	int attribute(const char* name, int defaultValue) const;
	/** Get an unsigned integer attribute */
	unsigned attribute(const char* name, unsigned defaultValue) const;
	/** Does this element have a named attribute */
	bool hasAttribute(const char*) const;
	/** Set attribute value */
	void setAttribute(const char* name, const char* value);
	void setAttribute(const char* name, double value);
	void setAttribute(const char* name, float value);
	void setAttribute(const char* name, int value, bool hex=false);
	/** Attribute iteration */
	base::HashMap<RefString>::const_iterator attributesBegin() const { return m_attributes.begin(); }
	base::HashMap<RefString>::const_iterator attributesEnd() const { return m_attributes.end(); }
	/** Set internal text */
	void setText(const char* text);
	/** Add a child element */
	XMLElement& add(const XMLElement& child);
	/** Add child element */
	XMLElement& add(const char* tag);
	/** Find a child element */
	XMLElement& find(const char* tag, int index=0);
	/** Find a child element */
	const XMLElement& find(const char* tag, int index=0) const;
	/** Child element iterator start */
	std::vector<XMLElement>::const_iterator begin() const { return m_children.begin(); }
	/** Child node iterator end */
	std::vector<XMLElement>::const_iterator end() const { return m_children.end(); }
	/** Get The number of child elements */
	unsigned int size() const { return m_children.size(); }
	const XMLElement& child(unsigned int i) const { return m_children.at(i); }
	XMLElement&       child(unsigned int i)       { return m_children.at(i); }
	/** String comparitor */
	bool operator==(const char* s) const;
	bool operator!=(const char* s) const;
	private:
	int m_type; // XML::TagType
	RefString m_name;
	RefString m_text;
	std::vector<XMLElement>  m_children;	// Child nodes
	base::HashMap<RefString> m_attributes;	// Attribute map
};


/** Simple XML reader, READ ONLY for now. */
class XML {
	public:
	enum TagType { TAG, HEADER, COMMENT };
	typedef XMLElement Element;
	/** XML Element iterator */
	typedef std::vector<Element>::const_iterator iterator;
	typedef base::HashMap<RefString>::const_iterator AttributeIterator;

	XML();
	XML(const char* root);

	/** Get the XML root node */
	const Element& getRoot() const { return m_root; }
	Element&       getRoot() { return m_root; }
	Element&       setRoot(const Element& e) { m_root=e; return m_root; }

	/** Load and parse an XML file from disk */
	static XML load(const char* filename);
	/** Parse an XML file from a string */
	static XML parse(const char* string);

	/** Save to file */
	int save(const char* filename) const;
	/** Write to string */
	const char* toString() const;
	
	private:
	int parseInternal(char* s);	// Parse the XML string to create Element objects
	Element m_root;				// Root element
};
};

#endif

