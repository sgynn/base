#pragma once

#include <vector>
#include <cstring>

namespace base {
	
	/** Hardware buffer base class */
	class HardwareBuffer {
		public:
		enum Usage {
			STATIC_DRAW, STATIC_READ, STATIC_COPY,
			DYNAMIC_DRAW, DYNAMIC_READ, DYNAMIC_STREAM,
			STREAM_DRAW, STREAM_READ, STREAM_COPY
		};

		HardwareBuffer(Usage usage, bool keepLocalCopy);
		~HardwareBuffer();

		HardwareBuffer* addReference();
		int             dropReference();

		const char* bind() const;	/// Bind buffer
		bool  isBuffer() const;	/// Is this a hardware buffer object
		void  createBuffer();	/// Create hardware buffer
		void  destroyBuffer();	/// Destroy hardware buffer
		void  writeBuffer();	/// Write data to hardware buffer
		void  readBuffer();		/// Read data from hardware buffer

		bool setKeepLocal(bool keepLocalCopy);	/// Keep a local copy of data when buffer created

		template<typename T>
		void setData(T* data, size_t size, bool owned=false) {
			clearLocalData();
			m_ownsData = owned;
			m_data = data;
			m_size = size * sizeof(T);
			if(isBuffer()) writeBuffer();
		}
		template<typename T>
		void copyData(const T* data, size_t size) {
			clearLocalData();
			m_size = size * sizeof(T);
			m_data = new T[size];
			m_ownsData = true;
			if(data) memcpy(m_data, data, m_size);
			if(isBuffer()) writeBuffer();
		}
		template<typename T>
		T* getData() {
			return (T*)m_data;
		}
		template<typename T>
		const T* getData() const {
			return (T*)m_data;
		}

		template<typename T>
		size_t getSize() const { return m_size / sizeof(T); }

		void clearLocalData() {
			if(m_data && m_ownsData) delete [] (char*) m_data;
			m_data = 0;
		}

		protected:
		int      m_ref;
		unsigned m_buffer;
		unsigned m_target;
		unsigned m_usage;
		size_t   m_size;
		void*    m_data;
		bool     m_keepCopy;
		bool     m_ownsData;
	};



	/** Vertex buffer attribute semantics */
	enum AttributeSemantic {
		VA_VERTEX, VA_NORMAL, VA_TEXCOORD, VA_COLOUR, VA_TANGENT, VA_SKININDEX, VA_SKINWEIGHT, VA_CUSTOM
	};
	enum AttributeType {
		VA_INVALID,
		VA_FLOAT1, VA_FLOAT2, VA_FLOAT3, VA_FLOAT4,
		VA_SHORT1, VA_SHORT2, VA_SHORT3, VA_SHORT4,
		VA_INT1,   VA_INT2,   VA_INT3,   VA_INT4,
		VA_UBYTE1, VA_UBYTE2, VA_UBYTE3, VA_UBYTE4,
		VA_ARGB, VA_RGB
	};
	struct Attribute {
		size_t offset;
		AttributeSemantic semantic;
		AttributeType type;
		const char* name;
		unsigned divisor;
		inline operator bool() const { return type!=VA_INVALID; }
	};
	
	class VertexAttributes {
		public:
		Attribute& add(AttributeSemantic, AttributeType, const char* name=0, unsigned divisor=0);
		Attribute& add(AttributeSemantic, AttributeType, unsigned offset, const char* name=0, unsigned divisor=0);
		Attribute& get(AttributeSemantic, int index=0);
		Attribute& get(unsigned index);
		const Attribute& get(unsigned index) const;
		int hasAttrribute(AttributeSemantic) const;
		int remove(AttributeSemantic, bool compact);
		unsigned compact();
		unsigned calculateStride() const;
		unsigned getStride() const;
		unsigned size() const;
		VertexAttributes& operator=(const VertexAttributes& o);
		bool operator==(const VertexAttributes& o) const;
		bool operator!=(const VertexAttributes& o) const;
		std::vector<Attribute>::const_iterator begin() const { return m_attributes.begin(); }
		std::vector<Attribute>::const_iterator end() const { return m_attributes.end(); }
		private:
		std::vector<Attribute> m_attributes;
		unsigned m_stride;
	};



	/** Vertex data buffer */
	class HardwareVertexBuffer : public HardwareBuffer {
		public:
		HardwareVertexBuffer(Usage usage = STATIC_DRAW);

		void setInstanceStep(int step=0);

		void setData(void* data, size_t count, size_t stride, bool owned=false) {
			HardwareBuffer::setData((char*)data, stride*count, owned);
			m_vertexSize = stride;
			m_vertexCount = count;
		}
		void copyData(const void* data, size_t count, size_t stride) {
			HardwareBuffer::copyData((char*)data, stride*count);
			m_vertexSize = stride;
			m_vertexCount = count;
		}

		float* getVertex(int index) { 
			return (float*) (getData<char>() + index * m_vertexSize);
		}
		const float* getVertex(int index) const { 
			return (const float*) (getData<char>() + index * m_vertexSize);
		}

		template<typename T>
		const T& getVertexData(const Attribute& elem, size_t index) const {
			return *reinterpret_cast<const T*>(getData<char>() + index * m_vertexSize + elem.offset);
		}

		size_t getVertexCount() const { return m_vertexCount; }
		size_t getVertexSize() const { return m_vertexSize / sizeof(float); }
		size_t getStride() const { return m_vertexSize; }
		
		// Vertex attributes
		VertexAttributes attributes;

		// Get openGL constants from attribute type
		static unsigned getDataType(AttributeType);		// OpenGL constant for this type
		static int      getDataCount(AttributeType);	// Number of elements in type
		static int      getDataSize(AttributeType);		// Type size in bytes
		
		protected:
		size_t m_instanceStep =0;
		size_t m_vertexCount =0;
		size_t m_vertexSize =0;
		std::vector<Attribute> m_attributes;
	};

	enum class IndexSize : char { I8, I16, I32 };

	/** Index buffer data */
	class HardwareIndexBuffer : public HardwareBuffer {
		public:
		HardwareIndexBuffer(IndexSize size=IndexSize::I16);
		void setIndexSize(IndexSize size) { m_type = size; }
		IndexSize getIndexSize() const { return m_type; }
		unsigned int getIndex(int i) const {
			switch(m_type) {
			case IndexSize::I8: return getData<unsigned char>()[i]; 
			case IndexSize::I16: return getData<unsigned short>()[i]; 
			case IndexSize::I32: return getData<unsigned int>()[i]; 
			default: return 0;
			}
		}
		int getIndexCount() const {
			static const int bytes[] = { 1, 2, 4 };
			return m_size / bytes[(int)m_type];
		}
		unsigned getDataType() const;
		protected:
		IndexSize m_type;
	};
}

