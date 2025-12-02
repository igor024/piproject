#include <cmath>
#include <cstdint>
#include <memory>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <strings.h>
#include "portaudio.h"
#include <math.h>

#include "google/cloud/speech/v1/speech_client.h"
#include <google/cloud/status_or.h>

using namespace std;

namespace speech_v1 = ::google::cloud::speech::v1;
namespace v2_37 = ::google::cloud::speech_v1::v2_37;
typedef int16_t SAMPLE;

const double SAMPLE_RATE = 44100;
const int NUM_CHANNELS = 2;
const unsigned int FRAMES_PER_BUFFER = 256; // Removed .0f (it's an int)
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
	recording* rec = (recording*) userData;
	(void) outputBuffer; 
	SAMPLE* rptr = (SAMPLE*) inputBuffer;
	SAMPLE* wptr = &rec->data[rec->framesRecorded * rec->numChannels];

	if (statusFlags & paOutputUnderflow) { printf("Underflow!\n"); return 1; }
	if (statusFlags & paOutputOverflow) { printf("Overflow!\n"); return 1; }

	long numFrames = std::min(rec->maxFrames - rec->framesRecorded, framesPerBuffer);

	for(int i = 0; i < numFrames; ++i) {
		for(int j = 0; j < rec->numChannels; ++j) {
			if(inputBuffer == NULL) *wptr++ = 0;
			else *wptr++ = *rptr++;
		}
	}
	rec->framesRecorded += numFrames;
	if(numFrames < framesPerBuffer) return paComplete;
	return paContinue;
}

static int playAudio( const void *inputBuffer, void *outputBuffer,
		     unsigned long framesPerBuffer,
		     const PaStreamCallbackTimeInfo* timeInfo,
		     PaStreamCallbackFlags statusFlags,
		     void *userData )
{
	recording *rec = (recording*) userData;
	SAMPLE* rptr = &rec->data[rec->framesRecorded * rec->numChannels];
	SAMPLE* out = (SAMPLE*) outputBuffer;

	if (statusFlags & paOutputUnderflow) { printf("Underflow!\n"); return 1; }
	if (statusFlags & paOutputOverflow) { printf("Overflow!\n"); return 1; }

	long numFrames = std::min(rec->maxFrames - rec->framesRecorded, framesPerBuffer);

	for(int i = 0; i < numFrames; ++i) {
		for(int j = 0; j < rec->numChannels; ++j) {
			*out++ = *rptr++;
		}
	}
	rec->framesRecorded += numFrames;
	if(numFrames < framesPerBuffer) return paComplete;
	return paContinue;
}

void run(PaError err) {
	if(err != paNoError) throw err;
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
	return dev;
}


int main() {
	// init PortAudio
	try {
		run(Pa_Initialize());
		printf("Running %s\n", Pa_GetVersionText()); 
	}
	catch (PaError err) {
		printf("Pa_Initialize() error: %s\n", Pa_GetErrorText(err));
		return 1;
	}

	//init voice rec
	std::shared_ptr<v2_37::SpeechConnection> connection;
	std::unique_ptr<v2_37::SpeechClient> client;

	try {
		connection = v2_37::MakeSpeechConnection();
		client = std::make_unique<v2_37::SpeechClient>(v2_37::SpeechClient(connection));
	}
	catch(int err) {
		printf("Error connecting to voice recognition: %d\n", err);
	}

	//main logic
	try {
		int dev = findDevice();
		PaStream* stream;
		recording rec;
		rec.numChannels = NUM_CHANNELS;
		rec.maxFrames = NUM_SECONDS * SAMPLE_RATE; // Total frames (time steps)
		rec.framesRecorded = 0;
		rec.data = (SAMPLE*)malloc(rec.maxFrames * rec.numChannels * sizeof(SAMPLE));

		if(rec.data == NULL) {
			printf("malloc failed bruh \n");
			throw 1;
		}
		memset(rec.data, 0, rec.maxFrames * rec.numChannels * sizeof(SAMPLE));

		//conf dev
		const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(dev);
		PaStreamParameters inputParameters;
		memset( &inputParameters, 0, sizeof( inputParameters ) );
		inputParameters.channelCount = 2;
		inputParameters.device = dev;
		inputParameters.sampleFormat = paInt16;
		inputParameters.suggestedLatency = devInfo->defaultLowInputLatency;

		PaStreamParameters outputParameters;
		memset( &outputParameters, 0, sizeof( outputParameters ) );
		outputParameters.channelCount = 2;
		outputParameters.device = dev;
		outputParameters.sampleFormat = paInt16;
		outputParameters.suggestedLatency = devInfo->defaultLowOutputLatency;

		//record
		run(Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, recordAudio, &rec));
		printf("Starting Recording...\n");    
		run(Pa_StartStream(stream));
		Pa_Sleep(1000 * NUM_SECONDS); 
		printf("Stopping Recording.\n");
		run(Pa_StopStream(stream));

		//convert int16 array to byte string
		std::string audio_content(reinterpret_cast<const char*>(rec.data), rec.maxFrames * rec.numChannels * sizeof(int16_t));

		//playback audio
		rec.framesRecorded = 0;
		run(Pa_OpenStream(&stream, NULL, &outputParameters,  SAMPLE_RATE, (unsigned long)FRAMES_PER_BUFFER, paNoFlag, playAudio, &rec));
		printf("Playing back...\n");    
		run(Pa_StartStream(stream));
		Pa_Sleep(1000 * NUM_SECONDS);
		run(Pa_StopStream(stream));

		printf("running voice recognition\n");

		speech_v1::RecognizeRequest request;
		request.mutable_audio()->set_content(std::move(audio_content));

		auto* config = request.mutable_config();
		config->set_language_code("en-US");
		config->set_encoding(speech_v1::RecognitionConfig::LINEAR16);
		config->set_sample_rate_hertz(static_cast<int>(SAMPLE_RATE));
		config->set_audio_channel_count(NUM_CHANNELS);

		//send request
		google::cloud::StatusOr<speech_v1::RecognizeResponse> response_wrapper = client->Recognize(request);

		//handle response
		if (!response_wrapper) {
			printf("voice recognition error: %s\n", response_wrapper.status().message());
			return 1; 
		}

		speech_v1::RecognizeResponse response = *response_wrapper;

		for (int i = 0; i < response.results_size(); ++i) {
			const speech_v1::SpeechRecognitionResult& result = response.results(i);
			for (int j = 0; j < result.alternatives_size(); ++j) {
				const speech_v1::SpeechRecognitionAlternative& alternative = result.alternatives(j);
				printf("Transcript: %s\n", alternative.transcript().c_str());
				printf("Confidence: %f\n", alternative.confidence());
			}
		}

		run(Pa_CloseStream(stream));
		free(rec.data); 

	} catch (PaError err) {
		printf("Main code error: %s\n", Pa_GetErrorText(err));
	} catch (...) {
		printf("Unknown error occurred.\n");
	}

	try {
		run(Pa_Terminate());
	} catch (PaError err) {
		printf("Pa_Terminate error: %s\n", Pa_GetErrorText(err));
	}

	return 0;
}
