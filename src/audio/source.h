#pragma once

namespace audio {
	enum Format { MONO_8, STEREO_8, MONO_16, STEREO_16 };

	/** Audio source data base class */
	class Source {
		friend class Data;
		friend class Object;
		unsigned buffers[2];	// Sreams use two buffers
		protected:
		TimeStamp length;		// Sound clip duration
		public:
		Source();
		virtual ~Source();
		virtual bool load(const char* file) = 0;
		virtual bool openStream(const char* file);
		virtual void closeStream() {}
		virtual void updateStream(int size) {}
		void setData(int size, Format format, int bitrate, void* data);
		bool createBuffer();
	};

	/** Basic uncompressed wav files */
	class WaveSource : public Source {
		public:
		virtual bool load(const char* file);
	};

	/** Ogg format */
	class OggSource : public Source {
		public:
		virtual bool load(const char* file);
	};

}

