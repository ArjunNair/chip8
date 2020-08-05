#include "Chip8Sound.h"
#include <SDL_audio.h>
#include <cstring>

void play_callback(void *userData, unsigned char *audioData, int length)
{
	sdl_audio_ring_buffer *ringBuffer = (sdl_audio_ring_buffer *)userData;
		
	int region1Size = length;
	int region2Size = 0;
	if (ringBuffer->PlayCursor + length > ringBuffer->Size)
	{
		region1Size = ringBuffer->Size - ringBuffer->PlayCursor;
		region2Size = length - region1Size;
	}
	memcpy(audioData, (unsigned char*)(ringBuffer->Data) + ringBuffer->PlayCursor, region1Size);
	memcpy(&audioData[region1Size], ringBuffer->Data, region2Size);
	ringBuffer->PlayCursor = (ringBuffer->PlayCursor + length) % ringBuffer->Size;
	ringBuffer->WriteCursor = (ringBuffer->PlayCursor + 2048) % ringBuffer->Size;
	ringBuffer->Length -= length;
}

void Chip8Sound::init()
{
	SDL_AudioSpec want;
	SDL_memset(&want, 0, sizeof(want));

	want.freq = samplesPerSecond;
	want.format = AUDIO_S16LSB;
	want.channels = 1;
	want.samples =  bufferSize / want.channels;
	want.callback = &play_callback;
	want.userdata = &AudioRingBuffer;

	DeviceID = SDL_OpenAudioDevice(NULL, 0, &want, &audioSpec, 0);

	if (DeviceID == 0) 
	{
		SDL_Log("Failed to open audio: %s", SDL_GetError());
	}
	else 
	{
		if (audioSpec.format != want.format) 
		{
			SDL_Log("Couldn't set Float32 audio format.");
		}
		SDL_PauseAudioDevice(DeviceID, 0);
	}
	soundIsPlaying = false;
	bytesPerSample = sizeof(short) * audioSpec.channels;
	bufferSize = audioSpec.size;
	AudioRingBuffer.Size = bufferSize;
	AudioRingBuffer.Data = malloc(bufferSize);
	AudioRingBuffer.PlayCursor = AudioRingBuffer.WriteCursor = AudioRingBuffer.Length = 0;
}

void Chip8Sound::play_single_buffer(bool playNote) 
{
	int queuedBytes = SDL_GetQueuedAudioSize(DeviceID);
	int bytesToWrite = targetQueueBytes - queuedBytes;

	if (bytesToWrite > 0)
	{
		int numSamples = audioSpec.samples * bytesPerSample;
		int sampleCount = bytesToWrite / bytesPerSample;
		void* soundBuffer = malloc(bytesToWrite);
		short *sampleOut = (short *)soundBuffer;

		for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
		{
			short tone = ((runningSampleIndex++ / halfSquareWavePeriod) % 2) ? toneVolume : audioSpec.silence;
			short sample = playNote ? tone : audioSpec.silence;
			
			*sampleOut++ = sample;
			if (audioSpec.channels == 2)
				*sampleOut++ = sample;	
		}

		SDL_QueueAudio(DeviceID, soundBuffer, bytesToWrite);
		
		if (!soundIsPlaying)
		{
			SDL_PauseAudioDevice(DeviceID, 0);
			soundIsPlaying = true;
		}
		free(soundBuffer);
	}
}

void Chip8Sound::play_ring_buffer(bool playNote) 
{
	int secondaryBufferSize = audioSpec.size;// samplesPerSecond * bytesPerSample;

	SDL_LockAudio();

	int byteToLock = runningSampleIndex * bytesPerSample % secondaryBufferSize;
	int bytesToWrite;

	if (byteToLock == AudioRingBuffer.PlayCursor)
	{
		bytesToWrite = secondaryBufferSize;
	} 
	else if (byteToLock > AudioRingBuffer.PlayCursor)
	{
		bytesToWrite = (secondaryBufferSize - byteToLock);
		bytesToWrite += AudioRingBuffer.PlayCursor;
	}
	else
	{
		bytesToWrite = AudioRingBuffer.PlayCursor - byteToLock;
	}

	void *region1 = (unsigned char *)AudioRingBuffer.Data + byteToLock;

	int region1Size = bytesToWrite;

  	if (region1Size + byteToLock > secondaryBufferSize)
        region1Size = secondaryBufferSize - byteToLock;

	void *region2 = AudioRingBuffer.Data;
	int region2Size = bytesToWrite - region1Size;

	SDL_UnlockAudio();

	int region1SampleCount = region1Size / bytesPerSample;
	short *sampleOut = (short *)region1;

	for (int sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
	{
		short tone = ((runningSampleIndex++ / halfSquareWavePeriod) % 2) ? toneVolume : audioSpec.silence;
		short sample = playNote ? tone : audioSpec.silence;
		*sampleOut++ = sample;

		if (audioSpec.channels == 2)
			*sampleOut++ = sample;
	}

	int region2SampleCount = region2Size / bytesPerSample;
	sampleOut = (short *)region2;

	for (int sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
	{
		short tone = ((runningSampleIndex++ / halfSquareWavePeriod) % 2) ? toneVolume : audioSpec.silence;
		short sample = playNote ? tone : audioSpec.silence;
		*sampleOut++ = sample;

		if (audioSpec.channels == 2)
			*sampleOut++ = sample;
	}

	AudioRingBuffer.Length += bytesToWrite;

	if (!soundIsPlaying)
	{
		SDL_PauseAudioDevice(DeviceID, 0);
		soundIsPlaying = true;
	}
}

Chip8Sound::~Chip8Sound() {}
