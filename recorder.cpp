#include <cmath>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <strings.h>
#include "portaudio.h"
#include <math.h>

typedef float SAMPLE;

const double SAMPLE_RATE = 44100;
const unsigned int FRAMES_PER_BUFFER = 256.0f;
const unsigned int NUM_SECONDS = 6;
struct recording{
	SAMPLE* data;
	unsigned long framesRecorded;
	unsigned long maxFrames;
	int numChannels;
};

static int recordAudio( const void *inputBuffer, void *outputBuffer,
		       unsigned long framesPerBuffer,
		       const PaStreamCallbackTimeInfo* timeInfo,
		       PaStreamCallbackFlags statusFlags,
		       void *userData )
{
	//cast void type pointers
	recording* rec = (recording*) userData;
	(void) outputBuffer; //ignore outputBuffer
	SAMPLE* rptr = (SAMPLE*) inputBuffer;
	SAMPLE* wptr = &rec->data[rec->framesRecorded * rec->numChannels];


	if (statusFlags & paOutputUnderflow) {
		printf("Underflow!\n");
		return 1;
	}

	if (statusFlags & paOutputOverflow) {
		printf("Overflow!\n");
		return 1;
	}

	long numFrames = std::min(rec->maxFrames - rec->framesRecorded, framesPerBuffer);

	//record
	for(int i = 0; i < numFrames; ++i) {
		for(int j = 0; j < rec->numChannels; ++j) {
			if(inputBuffer == NULL) { 
				*wptr++ = 0.0f;
			} else {
				*wptr++ = *rptr++;
			}
		}
	}

	rec->framesRecorded += numFrames;

	if(numFrames < framesPerBuffer) { //reached end
		return paComplete;
	}
	return paContinue;
}


static int playAudio( const void *inputBuffer, void *outputBuffer,
		       unsigned long framesPerBuffer,
		       const PaStreamCallbackTimeInfo* timeInfo,
		       PaStreamCallbackFlags statusFlags,
		       void *userData )
{
	//cast void type pointers
	recording* rec = (recording*) userData;
	SAMPLE* out = (SAMPLE*) outputBuffer;

	if (statusFlags & paOutputUnderflow) {
		printf("Underflow!\n");
		return 1;
	}

	if (statusFlags & paOutputOverflow) {
		printf("Overflow!\n");
		return 1;
	}

	long numFrames = std::min(rec->maxFrames - rec->framesRecorded, framesPerBuffer);

	//record
	for(int i = 0; i < numFrames; ++i) {
		for(int j = 0; j < rec->numChannels; ++j) {
			out+=

		}
	}

	rec->framesRecorded += numFrames;

	if(numFrames < framesPerBuffer) { //reached end
		return paComplete;
	}
	return paContinue;
}







void run(PaError err) {
	if(err != paNoError) {
		throw err;
	}
}

int findDevice() {
	const int numDev = Pa_GetDeviceCount();
	if(numDev < 0) {
		printf("Error fetching device count!\n");
		throw(numDev);
	}
	for(int i = 0; i < numDev; ++i) {
		const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(i);
		printf("Device %d Name: %s\n maxOutputChannels: %d\n defaultSampleRate: %f\n\n", i, devInfo->name, devInfo->maxOutputChannels, devInfo->defaultSampleRate);
	}
	printf("Enter device number: ");
	int dev;
	std::cin >> dev;
	printf("\n Chose device %d\n", dev);
	return dev;
}

int main() {
	//start PortAudio
	try {
		run(Pa_Initialize());
		printf("Running %s\n", Pa_GetVersionText()); 
	}
	catch (PaError err) {
		printf("Pa_Initialize() error: %s\n", Pa_GetErrorText(err));
	}

	//main code
	try {
		int dev = findDevice();
		PaStream* stream;
		recording rec;
		rec.numChannels = 2;
		rec.maxFrames = NUM_SECONDS * SAMPLE_RATE;
		rec.framesRecorded = 0;
		rec.data = (SAMPLE*)malloc(rec.maxFrames * rec.numChannels * sizeof(SAMPLE));

		if(rec.data == NULL) {
			printf("Failed to allocate memory for recording\n");
			throw 1;
		}

		for( int i = 0; i < rec.maxFrames * rec.numChannels; ++i) {
			rec.data[i] = 0;
		}

		const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(dev);

		printf("Device %d Name: %s\n maxOutputChannels: %d\n maxInputChannels: %d\n defaultSampleRate: %f\n\n", dev, devInfo->name, devInfo->maxOutputChannels, devInfo->maxInputChannels, devInfo->defaultSampleRate);

		if(devInfo->maxInputChannels < 2) {
			printf("Device does not support 2 channels. Aborting.\n");
			throw 1;
		}

		PaStreamParameters inputParameters;

		memset( &inputParameters, 0, sizeof( inputParameters ) ); //make NULL all fields
		inputParameters.channelCount = 2;
		inputParameters.device = dev;
		inputParameters.sampleFormat = paFloat32;
		inputParameters.suggestedLatency = devInfo->defaultLowInputLatency;
		run(Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, recordAudio, &rec));
		printf("Starting stream\n");	
		run(Pa_StartStream(stream));
		Pa_Sleep(10000);
		printf("Closing stream\n");
		run(Pa_StopStream(stream));
		run(Pa_CloseStream(stream));
	}
	catch (PaError err) {
		printf("Main code error: %s\n", Pa_GetErrorText(err));
	}

	//end portAudio
	try {
		run(Pa_Terminate());
	}
	catch (PaError err) {
		printf("Pa_Initialize() error: %s\n", Pa_GetErrorText(err));
	}

	return 0;
}
