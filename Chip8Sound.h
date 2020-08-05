/** Uses a ring buffer to stream audio into a SDL buffer **/

#pragma once
#include<SDL.h>

struct sdl_audio_ring_buffer
{
	int Size;
	int WriteCursor;
	int PlayCursor;
	int Length;
	void *Data;
};

class Chip8Sound
{
private:
	static constexpr int samplesPerSecond = 48000;
	static constexpr short toneVolume = 3000;

	//See: https://www.seventhstring.com/resources/notefrequencies.html
	static constexpr int toneHz = 440; //A4 (Standard tuning)
	
	unsigned int runningSampleIndex = 0;
	static constexpr int squareWavePeriod = samplesPerSecond / toneHz;
	static constexpr int halfSquareWavePeriod = squareWavePeriod / 2;
	int bytesPerSample = sizeof(short) * 1; //num channels = 1
	int bufferSize = (samplesPerSecond / 60);
	
	SDL_AudioSpec audioSpec;
	int targetQueueBytes = samplesPerSecond * bytesPerSample;
	bool soundIsPlaying;

public:
	sdl_audio_ring_buffer AudioRingBuffer;
	SDL_AudioDeviceID DeviceID = 0;

	void init();
	void play_single_buffer(bool playNote);
	void play_ring_buffer(bool playNote);
	~Chip8Sound();
};

