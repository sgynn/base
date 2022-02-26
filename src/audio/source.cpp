#include "audioclass.h"
#include "source.h"
#include <cstdio>

#include <AL/al.h>

#ifndef NO_OGG
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#endif

namespace audio {

	Source::Source() : length(0) {}
	Source::~Source() {}
	bool Source::openStream(const char*) { printf("Streaming not supported by format\n"); return false; }
	bool Source::createBuffer() {
		alGenBuffers(1, buffers); // streams need 2 buffers
		return alGetError() != AL_NO_ERROR;
	}
	void Source::setData(int size, Format f, int rate, void* data) {
		static const int formats[] = { AL_FORMAT_MONO8, AL_FORMAT_STEREO8, AL_FORMAT_MONO16, AL_FORMAT_STEREO16 };
		alBufferData(buffers[0], formats[f], data, size, rate);
		ALenum err = alGetError();
		if(err != AL_NO_ERROR) printf("OpenAL Error %x\n", err);
	}

	bool Data::loadSource(Sound* s) {
		if(!s || !s->file[0]) return false;
		if(s->source) return true;
		printf("Load %s\n", s->file);
		char* ext = s->file + strlen(s->file)-4;
		if(strcmp(ext, ".wav")==0) s->source = new WaveSource();
		#ifndef NO_OGG
		else if(strcmp(ext, ".ogg")==0) s->source = new OggSource();
		#endif
		else { printf("No plugin for source '%s'\n", s->file); return false; }
		// Load data (or validate stream)
		if(s->source->load(s->file)) return true;
		else {
			printf("Failed to load sound '%s'\n", s->file);
			delete s->source; s->source = 0;
			return false;
		}
	}
	void Data::unloadSource(Sound* s) {
		if(!s) return;
		if(s->source) {
			alDeleteBuffers(1, s->source->buffers);
			delete s->source;
		}
		s->source = 0;
	}

	


	//// //// Wave format ////
	bool WaveSource::load(const char* file) {
		FILE* fp = fopen(file, "rb");
		if(!fp) return false;
		char type[4];
		fread(type, 1,4,fp);
		if(memcmp(type, "RIFF", 4)!=0) { fclose(fp); return false; }
		int size=0;
		fread(&size, sizeof(int), 1, fp);
		fread(type, 4, 1, fp);
		if(memcmp(type, "WAVE", 4)!=0) { fclose(fp); return false; }
		// Desc
		short tag, align, channels, bits;
		int bps, rate;
		fread(type,      1, 4, fp);		// Format: "fmt"
		fread(&size,     4, 1, fp);		
		fread(&tag,      2, 1, fp);		// 1=PCM, ...=compressed
		fread(&channels, 2, 1, fp);		// 1=mono, 2=stereo
		fread(&rate,     4, 1, fp);		// sample rate
		fread(&bps,      4, 1, fp);		// bytes per second (average)
		fread(&align,    2, 1, fp);		// Block alignment
		fread(&bits,     2, 1, fp);		// Bits per sample (8,16)

		if((bits!=8 && bits!=16) || (channels!=1 && channels!=2) || tag!=1) {
			if(bits>16) printf("%d bit wav files unsupported\n", bits);
			fclose(fp); return false;
		}
		Format format = (Format) ((bits/16)*2 + channels-1);

		// Data
		fread(type,  1, 4, fp);		// "DATA"
		fread(&size, 4, 1, fp);	// Bytes
		char* data = new char[size];
		fread(data, 1, size, fp);
		createBuffer();
		setData( size, format, rate, data);
		length = size * 1000000ull / (channels*bits/8) / rate;
		delete [] data;
		fclose(fp);
		return true;
	}


	#ifndef NO_OGG
	bool OggSource::load(const char* file) {
		FILE* fp = fopen(file, "rb");
		if(!fp) return false;

		OggVorbis_File ogg;
		int r = ov_open(fp, &ogg , 0, 0);
		if(r<0) { fclose(fp);  return false; }

		vorbis_info* info = ov_info(&ogg, -1);
		Format format = info->channels==1? MONO_16: STEREO_16;

		int section;
		int size = ov_pcm_total(&ogg, -1) * info->channels * 2;
		char* data = new char[size];
		int dp = 0;
		while(dp < size) {
			r = ov_read(&ogg, data+dp, size, 0,2,1, &section);
			if(r > 0) dp += r;
			else break;
		}
		if(dp < size) printf("Warning: truncated\n");
		if(dp > 0) {
			createBuffer();
			setData(dp, format, info->rate, data);
			length = size * 1000000ull / (info->channels*2) / info->rate;
		}
		delete [] data;
		ov_clear(&ogg);		// ov_clear closes the file
		return dp > 0;
	}
	#endif

}

