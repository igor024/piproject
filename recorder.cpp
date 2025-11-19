#include <stdio.h>
#include <iostream>
#include <strings.h>
#include "portaudio.h"

const double SAMPLE_RATE = 44120;
const double FRAMES_PER_BUFFER = 256;

//anonymous struct 
struct stereo{
	float left;
	float right;
};


static int callback( const void *inputBuffer, void *outputBuffer,
		    unsigned long framesPerBuffer,
		    const PaStreamCallbackTimeInfo* timeInfo,
		    PaStreamCallbackFlags statusFlags,
		    void *userData )
{
	//cast void type pointers
	stereo* data = (stereo*) userData;
	(void) inputBuffer; //ignore inputBuffer
	float* out = (float*) outputBuffer;

	for(int i = 0; i < framesPerBuffer; ++i) {
		*out++ = data->left;
		*out++ = data->right;
		data->left = 1.02 * data->left + 0.005;
		data->right = 1.03 * data->right + 0.01;
		if(data->left > 1) {
			data->left = 0;
		}
		else if (data->right > 1) {
			data->right = 0;
		}
	}
	return 0; //return 1 to quit
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
		stereo data = {0, 0};

		PaStreamParameters outputParameters;
		PaStreamParameters inputParameters;

		bzero( &inputParameters, sizeof( inputParameters ) ); //not necessary if you are filling in all the fields
		inputParameters.channelCount = 0;
		inputParameters.device = dev;
		inputParameters.hostApiSpecificStreamInfo = NULL;
		inputParameters.sampleFormat = paFloat32;
		inputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowInputLatency ;
		inputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field


		bzero( &outputParameters, sizeof( outputParameters ) ); //not necessary if you are filling in all the fields
		outputParameters.channelCount = 22;
		outputParameters.device = dev;
		outputParameters.hostApiSpecificStreamInfo = NULL;
		outputParameters.sampleFormat = paFloat32;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowOutputLatency ;
		outputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field


		run(Pa_OpenStream(&stream, &inputParameters, &outputParameters,  SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, callback, &data));
		printf("Starting stream\n");	
		run(Pa_StartStream(stream));
		Pa_Sleep(4000);
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
}
