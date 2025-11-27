#include <cmath>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <strings.h>
#include "portaudio.h"
#include <math.h>
const double SAMPLE_RATE = 44100;
const float FRAMES_PER_BUFFER = 256.0f;

struct stereo{
	float left;
	float right;
};

unsigned int cycle = 0;

static int playAudio( const void *inputBuffer, void *outputBuffer,
		    unsigned long framesPerBuffer,
		    const PaStreamCallbackTimeInfo* timeInfo,
		    PaStreamCallbackFlags statusFlags,
		    void *userData )
{
	//cast void type pointers
	stereo* data = (stereo*) userData;
	(void) inputBuffer; //ignore inputBuffer

	if (statusFlags & paOutputUnderflow) {
		printf("Underflow!\n");
		return 1;
	}

	if (statusFlags & paOutputOverflow) {
		printf("Overflow!\n");
		return 1;
	}

	float* out = (float*) outputBuffer;
	double left = data->left;
	double right = data->right;
	/* play sawtooth
	for(int i = 0; i < framesPerBuffer; ++i) {
		*out++ = data->left;
		*out++ = data->right;
		data->left += 0.005f;
		data->right += 0.2f;
		if(data->left > 1) {
			data->left = -1;
		}
		if (data->right > 1) {
			data->right = -1;
		} 
	} */  
	//play sine wave
	for(int i = 0; i < framesPerBuffer; ++i) {
		*out++ = sinf(left) / 5.0f;
		*out++ = sinf(right) / 5.0f;
		left += 0.04f;
		right += 0.01f;
		cycle++;
	}

	data->left = left;
	data->right = right;

	return paContinue; //return 1 to quit
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
		stereo data = {0.02f, 0.02f};
		printf("left %f right %f", data.left, data.right);

		const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(dev);

		printf("Device %d Name: %s\n maxOutputChannels: %d\n maxInputChannels: %d\n defaultSampleRate: %f\n\n", dev, devInfo->name, devInfo->maxOutputChannels, devInfo->maxInputChannels, devInfo->defaultSampleRate);

		if(devInfo->maxOutputChannels < 2) {
			printf("Device does not support 2 channels. Aborting.\n");
			throw 1;
		}

		PaStreamParameters outputParameters;

		memset( &outputParameters, 0, sizeof( outputParameters ) ); //make NULL all fields
		outputParameters.channelCount = 2;
		outputParameters.device = dev;
		outputParameters.sampleFormat = paFloat32;
		outputParameters.suggestedLatency = devInfo->defaultLowOutputLatency;

		run(Pa_OpenStream(&stream, NULL, &outputParameters,  SAMPLE_RATE, (unsigned long)FRAMES_PER_BUFFER, paNoFlag, playAudio, &data));
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
