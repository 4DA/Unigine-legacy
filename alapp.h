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

#ifndef __ALAPP_H__
#define __ALAPP_H__

#include <vector>
#include <AL/al.h>
#include <AL/alc.h>

/*
 */
class SoundFile {
public:
	SoundFile() : channels(0), freq(0) { }
	virtual ~SoundFile() { }
	
	static SoundFile *load(const char *name);
	
	virtual int size() = 0;
	virtual int read(char *buffer,int size = -1) = 0;
	virtual void seek(double time) = 0;

	int channels;
	int freq;
};

/*
 */
class Sound {
public:
	Sound(const char *name,int flag = 0);
	~Sound();
		
	enum {
		LOOP = 1 << 0,
		STREAM = 1 << 1,
		BUFFER_SIZE = 65536,
	};
	
	void play();
	void pause();
	void stop();
	void update();
	
	int flag;
	SoundFile *file;
	ALuint format;
	char *buffer;
	int current_buffer;
	ALuint buffers[2];
	ALuint source;
};	

/*
 */
class ALApp {
public:
	ALApp();
	virtual ~ALApp();
	
	void error();
	void update();
	
protected:
	
	ALCdevice *device;
	ALCcontext *context;
	
	friend class Sound;
	static void addStream(Sound *s);
	static void removeStream(Sound *s);
	static std::vector<Sound*> streams;
};

#endif /* __ALAPP_H__ */
