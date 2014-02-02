/* ALApp
 *
 * Copyright (C) 2003-2004, Alexander Zaprjagaev <frustum@frustum.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include <mad.h>
#include "alapp.h"

#ifdef _WIN32
#pragma comment (lib,"openal32.lib")
#pragma comment (lib,"ogg.lib")
#pragma comment (lib,"vorbis.lib")
#pragma comment (lib,"vorbisfile.lib")
#pragma comment (lib,"libmad.lib")
#endif

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

class SoundFileOgg : public SoundFile {
public:
	SoundFileOgg(const char *name);
	virtual ~SoundFileOgg();
	
	virtual int size();
	virtual int read(char *buf,int size = -1);
	virtual void seek(double time);

protected:
	FILE *file;
	OggVorbis_File vf;
	vorbis_info *vi;
};

SoundFileOgg::SoundFileOgg(const char *name) {
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"SoundFileOgg::SoundFileOgg(): error open \"%s\" file\n",name);
		return;
	}
	if(ov_open(file,&vf,NULL,0) < 0) {
		fprintf(stderr,"SoundFileOgg::SoundFileOgg(): \"%s\" is not ogg bitstream\n",name);
		fclose(file);
		file = NULL;
		return;
	}
	vi = ov_info(&vf,-1);
	channels = vi->channels;
	freq = vi->rate;
}

SoundFileOgg::~SoundFileOgg() {
	if(file) {
		ov_clear(&vf);
		fclose(file);
	}
}

/*
 */
int SoundFileOgg::size() {
	if(!file) return 0;
	return (int)(ov_time_total(&vf,-1) + 0.5) * channels * freq * 2;
}

int SoundFileOgg::read(char *buffer,int size) {
	if(!file) return 0;
	int current_section;
	if(size < 0) size = this->size();
	int read = 0;
	while(read < size) {
		int ret = ov_read(&vf,buffer + read,size - read,0,2,1,&current_section);
		if(ret <= 0) break;
		read += ret;
	}
	return read;
}

void SoundFileOgg::seek(double time) {
	if(!file) return;
	ov_time_seek(&vf,time);
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

class SoundFileMp3 : public SoundFile {
public:
	SoundFileMp3(const char *name);
	virtual ~SoundFileMp3();
	
	virtual int size();
	virtual int read(char *buf,int size = -1);
	virtual void seek(double time);

protected:
	int read_frame();
	inline int scale(mad_fixed_t sample);
	
	enum {
		BUFFER_SIZE = 4096,
	};
	
	FILE *file;
	int file_size;
	unsigned char buffer[BUFFER_SIZE];
	int buffer_length;
	struct mad_synth synth;
	struct mad_stream stream;
	struct mad_frame frame;
	int bitrate;
};

SoundFileMp3::SoundFileMp3(const char *name) {
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"SoundFileMp3::SoundFileMp3(): error open \"%s\" file\n",name);
		return;
	}
	fseek(file,0,SEEK_END);
	file_size = ftell(file);
	fseek(file,0,SEEK_SET);
	buffer_length = 0;
	mad_synth_init(&synth);
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	if(read_frame() == 0) {
		fprintf(stderr,"SoundFileMp3::SoundFileMp3(): can`t find frame\n",name);
		fclose(file);
		file = NULL;
		mad_synth_finish(&synth);
		mad_stream_finish(&stream);
		mad_frame_finish(&frame);
		return;
	}
	channels = (frame.header.mode == MAD_MODE_SINGLE_CHANNEL) ? 1 : 2;
	freq = frame.header.samplerate;
	bitrate = frame.header.bitrate;
}

SoundFileMp3::~SoundFileMp3() {
	if(file) {
		fclose(file);
		mad_synth_finish(&synth);
		mad_stream_finish(&stream);
		mad_frame_finish(&frame);
	}
}

/*
 */
int SoundFileMp3::read_frame() {
	while(1) {
		int ret = fread(&buffer[buffer_length],1,BUFFER_SIZE - buffer_length,file);
		if(ret <= 0) break;
		buffer_length += ret;
		while(1) {
			mad_stream_buffer(&stream,buffer,buffer_length);
			ret = mad_frame_decode(&frame,&stream);
			if(stream.next_frame) {
				int length = buffer + buffer_length - (unsigned char*)stream.next_frame;
				memmove(buffer,stream.next_frame,length);
				buffer_length = length;
			}
			if(ret == 0) return 1;
			if(stream.error == MAD_ERROR_BUFLEN) break;
		}
	}
	return 0;
}

/*
 */
inline int SoundFileMp3::scale(mad_fixed_t sample) {
	sample += (1 << (MAD_F_FRACBITS - 16));
	if(sample >= MAD_F_ONE) sample = MAD_F_ONE - 1;
	else if(sample < -MAD_F_ONE) sample = -MAD_F_ONE;
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 */
int SoundFileMp3::size() {
	if(!file) return 0;
	return file_size * 8 / bitrate * channels * freq * 2;
}

int SoundFileMp3::read(char *buffer,int size) {
	if(!file) return 0;
	if(size < 0) size = this->size();
	int read = 0;
	while(read < size) {
		mad_synth_frame(&synth,&frame);
		struct mad_pcm *pcm = &synth.pcm;
		mad_fixed_t *left = pcm->samples[0];
		mad_fixed_t *right = pcm->samples[1];
		unsigned short *data = (unsigned short*)(buffer + read);
		for(unsigned int length = pcm->length; length > 0; length--) {
			*data++ = scale(*left++);
			if(channels == 2) *data++ = scale(*right++);
		}
		read += pcm->length * channels * 2;
		if(!read_frame()) return read;
	}
	return read;
}

void SoundFileMp3::seek(double time) {
	if(!file) return;
	fseek(file,(unsigned int)((double)bitrate / 8.0 * time),SEEK_SET);
	read_frame();
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

class SoundFileWav : public SoundFile {
public:
	SoundFileWav(const char *name);
	virtual ~SoundFileWav();
	
	virtual int size();
	virtual int read(char *buf,int size = -1);
	virtual void seek(double time);

protected:	
	enum {
		RIFF = 0x46464952,
		WAVE = 0x45564157,
		FMT = 0x20746D66,
		DATA = 0x61746164,
	};
	
	struct Fmt {
		unsigned short encoding;
		unsigned short channels;
		unsigned int frequency;
		unsigned int byterate;
		unsigned short blockalign;
		unsigned short bitspersample;
	};
	
	FILE *file;
	Fmt fmt;
	unsigned int data_offset;
	unsigned int data_length;
};

SoundFileWav::SoundFileWav(const char *name) {
	memset(&fmt,0,sizeof(Fmt));
	file = fopen(name,"rb");
	if(!file) {
		fprintf(stderr,"SoundFileWav::SoundFileWav(): error open \"%s\" file\n",name);
		return;
	}
	unsigned int magic;
	unsigned int length;
	fread(&magic,sizeof(unsigned int),1,file);
	fread(&length,sizeof(unsigned int),1,file);
	if(magic != RIFF) {
		fprintf(stderr,"SoundFileWav::SoundFileWav(): wrong main chunk\n");
		fclose(file);
		file = NULL;
		return;
	}
	fread(&magic,sizeof(unsigned int),1,file);
	if(magic != WAVE) {
		fprintf(stderr,"SoundFileWav::SoundFileWav(): unknown file type\n");
		fclose(file);
		file = NULL;
		return;
	}
	while(1) {
		if(fread(&magic,sizeof(unsigned int),1,file) != 1) break;
		if(fread(&length,sizeof(unsigned int),1,file) != 1) break;
		if(magic == FMT) {
			fread(&fmt,sizeof(Fmt),1,file);
			if(fmt.encoding != 1) {
				fprintf(stderr,"SoundFileWav::SoundFileWav(): can`t open compressed waveform data\n");
				fclose(file);
				file = NULL;
				return;
			}
			if(fmt.bitspersample != 16) {
				fprintf(stderr,"SoundFileWav::SoundFileWav(): can`t open %d bit per sample format\n",fmt.bitspersample);
				fclose(file);
				file = NULL;
				return;
			}
			channels = fmt.channels;
			freq = fmt.frequency;
		} else if(magic == DATA) {
			data_offset = ftell(file);
			data_length = length;
			break;
		} else {
			fseek(file,length,SEEK_CUR);
		}
	}
	if(channels == 0 || freq == 0 || data_offset == 0 || data_length == 0) {
		fprintf(stderr,"SoundFileWav::SoundFileWav(): can`t find FMT or DATA block\n");
		fclose(file);
		file = NULL;
	}
}

SoundFileWav::~SoundFileWav() {
	if(file) fclose(file);
}

/*
 */
int SoundFileWav::size() {
	if(!file) return 0;
	return data_length;
}

int SoundFileWav::read(char *buffer,int size) {
	if(!file) return 0;
	int left = data_length - ftell(file) + data_offset;
	if(size < 0 || left < size) size = left;
	fread(buffer,sizeof(char),size,file);
	return size;
}

void SoundFileWav::seek(double time) {
	if(!file) return;
	fseek(file,data_offset + (int)(time * fmt.channels * fmt.frequency),SEEK_SET);
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

SoundFile *SoundFile::load(const char *name) {
	if(strstr(name,".ogg")) return new SoundFileOgg(name);
	if(strstr(name,".mp3")) return new SoundFileMp3(name);
	if(strstr(name,".wav")) return new SoundFileWav(name);
	fprintf(stderr,"\"%s\" is not supported\n",name);
	return NULL;
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

Sound::Sound(const char *name,int flag) : flag(flag) {
	file = SoundFile::load(name);
	if(!file) return;
	format = file->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	if(file->size() < BUFFER_SIZE * 2) this->flag &= ~STREAM;
	if(this->flag & STREAM) {
		buffer = new char[BUFFER_SIZE * 2];
		alGenBuffers(2,buffers);
		alGenSources(1,&source);
		current_buffer = 0;
	} else {
		buffer = new char[file->size()];
		int size = file->read(buffer);
		alGenBuffers(1,buffers);
		alBufferData(buffers[0],format,buffer,size,file->freq);
		alGenSources(1,&source);
		alSourcei(source,AL_BUFFER,buffers[0]);
		alSourcei(source,AL_LOOPING,flag & LOOP ? AL_TRUE : AL_FALSE);
		delete buffer;
	}
}

Sound::~Sound() {
	if(!file) return;
	delete file;
	if(flag & STREAM) delete buffer;
	alDeleteSources(1,&source);
	if(flag & STREAM) alDeleteBuffers(2,buffers);
	else alDeleteBuffers(1,buffers);
}

/*
 */
void Sound::play() {
	if(!file) return;
	ALint state;
	alGetSourcei(source,AL_SOURCE_STATE,&state);
	if(state == AL_PLAYING) return;
	if(state != AL_PAUSED && flag & STREAM) {
		file->read(buffer,BUFFER_SIZE);
		alBufferData(buffers[0],format,buffer,BUFFER_SIZE,file->freq);
		file->read(buffer,BUFFER_SIZE);
		alBufferData(buffers[1],format,buffer,BUFFER_SIZE,file->freq);
		alSourceQueueBuffers(source,2,buffers);
		ALApp::addStream(this);
	}
	alSourcePlay(source);
}

/*
 */
void Sound::pause() {
	if(!file) return;
	alSourcePause(source);
}

/*
 */
void Sound::stop() {
	if(!file) return;
	alSourceStop(source);
	file->seek(0.0);
	if(flag & STREAM) {
		ALint queued;
		alGetSourcei(source,AL_BUFFERS_QUEUED,&queued);
		if(queued > 0) alSourceUnqueueBuffers(source,2,buffers);
		ALApp::removeStream(this);
		current_buffer = 0;
	}
}

/*
 */
void Sound::update() {
	if(!file) return;
	if(flag & STREAM) {
		ALint processed;
		alGetSourcei(source,AL_BUFFERS_PROCESSED,&processed);
		if(processed == 1) {
			alSourceUnqueueBuffers(source,1,&buffers[current_buffer]);
			int size = file->read(buffer,BUFFER_SIZE);
			if(size > 0 || (size == 0 && flag & LOOP)) {
				alBufferData(buffers[current_buffer],format,buffer,size,file->freq);
				alSourceQueueBuffers(source,1,&buffers[current_buffer]);
				if(size != BUFFER_SIZE && flag & LOOP) file->seek(0.0);
			} else {
				int queued;
				alGetSourcei(source,AL_BUFFERS_QUEUED,&queued);
				if(queued == 0) file->seek(0.0);
			}
			current_buffer = 1 - current_buffer;
		} else if(processed == 2) {
			alSourceUnqueueBuffers(source,2,buffers);
			current_buffer = 0;
			play();
		}
	}
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

std::vector<Sound*> ALApp::streams;

ALApp::ALApp() {
	device = alcOpenDevice(NULL);
	if(!device) {
		fprintf(stderr,"ALApp::ALApp(): invalid device\n");
		return;
	}
	context = alcCreateContext(device,NULL);
	if(!context) {
		fprintf(stderr,"ALApp::ALApp(): invalid context\n");
		return;
	}
	alcMakeContextCurrent(context);
}

ALApp::~ALApp() {
	alcDestroyContext(context);
	alcCloseDevice(device);
}

/*
 */
void ALApp::error() {
	ALenum error;
	while((error = alGetError()) != AL_NO_ERROR) {	
		fprintf(stderr,"ALApp::error(): 0x%04X\n",error);
	}
}

/*
 */
void ALApp::update() {
	for(int i = 0; i < (int)streams.size(); i++) streams[i]->update();
}

/*
 */
void ALApp::addStream(Sound *s) {
	int i = 0;
	for(i = 0; i < (int)streams.size(); i++) if(streams[i] == s) break;
	if(i == (int)streams.size()) streams.push_back(s);
}

/*
 */
void ALApp::removeStream(Sound *s) {
	if((int)streams.size() == 0) return;
	int i = 0;
	for(i = 0; i < (int)streams.size(); i++) if(streams[i] == s) break;
	if(i == (int)streams.size()) streams.erase(streams.begin() + i);
}
