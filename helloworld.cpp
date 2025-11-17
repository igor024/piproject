#include <portaudio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include "google/cloud/speech/v1/speech_client.h"
#include <cstring>
using namespace std;
namespace speech = ::google::cloud::speech::v1;
namespace speech_v1 = google::cloud::speech_v1::v2_37;

const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 512;
const int NUM_SECONDS = 4;
std::string ReadFileToString(const std::string& file_path) {
	std::ifstream file(file_path, std::ios::binary);
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

static void checkPaErr(PaError err) {
	if(err != paNoError) {
		cout << "PortAudio error: " << Pa_GetErrorText(err) << endl;
		exit(EXIT_FAILURE);
	}
}


void showDevices() {
	int numDev = Pa_GetDeviceCount();
	cout << "Num devices: " << numDev << endl;
	if(numDev < 0) {
		cout << "Error counting devices" << endl;
		exit(EXIT_FAILURE);
	} else if(numDev == 0) {
		cout << "No devices found" << endl;
		exit(EXIT_SUCCESS);
	}

	const PaDeviceInfo* devInfo;
	for(int i = 0; i < numDev; ++i) {
		devInfo = Pa_GetDeviceInfo(i);
		cout << "Device " << i << ":" << endl
		     << "Name: " << devInfo->name << endl
		     << "defaultSampleRate: " << devInfo->defaultSampleRate << endl
		     << "inp chan: " << devInfo->maxInputChannels << endl
		     << "out chan: " << devInfo->maxOutputChannels << endl;
	}

	int dev;
	cout << "enter dev: ";
	cin.clear();
	cin >> dev;

	cout << "got here" << endl;
	/*
	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	
	cout << "got here 2" << endl;

	memset(&inputParameters, 0, sizeof(inputParameters));
	inputParameters.channelCount = 1;
	inputParameters.device = dev;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowInputLatency;

	cout << "got here 3" << endl;
	
	memset(&outputParameters, 0, sizeof(outputParameters));
	outputParameters.channelCount = 1;
	outputParameters.device = dev;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowInputLatency;
	
	cout << "got here 4" << endl;

	PaStream* stream;
	PaError err = Pa_OpenStream(
		&stream,
		&inputParameters,
		&outputParameters,
	SAMPLE_RATE,
	FRAMES_PER_BUFFER,
	paNoFlag,
	NULL,
	NULL

	);
*/
	
    PaStream *stream;
    short buffer[FRAMES_PER_BUFFER];

    PaError err = Pa_OpenDefaultStream(&stream,
                               2, // input channels
                               0,            // output channels
                               paInt16,
                               SAMPLE_RATE,
                               FRAMES_PER_BUFFER,
                               nullptr,      // no callback
                               nullptr);     // no user data
	checkPaErr(err);

	cout << "starting stream" << endl;
	err = Pa_StartStream(stream);
	checkPaErr(err);

	cout << "commencing recording audio" << endl;
	int totalFrames = SAMPLE_RATE * NUM_SECONDS;
	int numBuffers = totalFrames / FRAMES_PER_BUFFER;
	cout << "Total frames: " << totalFrames << ", numBuffers: " << numBuffers << endl;
	FILE* outFile = fopen("recorded.raw", "wb");
	for (int i = 0; i < numBuffers; ++i) {
		cout << "reading buffer " << i << " of " << numBuffers << endl;
		cout.flush(); // Force output to appear immediately
		err = Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
		if (err != paNoError) {
			cout << "Pa_ReadStream error on iteration " << i << ": " << Pa_GetErrorText(err) << endl;
			break;
		}
		cout << "Got here 5\n";
		fwrite(buffer, sizeof(short), FRAMES_PER_BUFFER, outFile);
		cout << "Got here 6\n";
	}
	fclose(outFile);


	checkPaErr(err);
	Pa_Sleep(10 * 1000);

	err = Pa_StopStream(stream);
	checkPaErr(err);

	Pa_Sleep(10 * 1000);

}

int main() {
	PaError err;
	checkPaErr(Pa_Initialize());
	showDevices();
	checkPaErr(Pa_Terminate());


	auto client = speech_v1::SpeechClient(speech_v1::MakeSpeechConnection());

	speech::RecognitionConfig config;
	config.set_encoding(speech::RecognitionConfig::MP3);
	config.set_sample_rate_hertz(SAMPLE_RATE);
	config.set_language_code("en-US");

	auto* diarization_config = config.mutable_diarization_config();
	diarization_config->set_enable_speaker_diarization(true);
	diarization_config->set_min_speaker_count(2);
	diarization_config->set_max_speaker_count(6);

	speech::RecognitionAudio audio;
	std::string audio_content = ReadFileToString("greetings2.mp3");
	//audio.set_uri("https://dn721802.ca.archive.org/0/items/englishaudio/10-%20Learn%20English%20Speaking%20%20Easy%20English%20Conversations%2002.mp3");
	audio.set_content(audio_content);
	auto response = client.Recognize(config, audio);
	if (!response) {
		std::cerr << "Recognition failed: " << response.status() << "\n";
		return 1;
	}

	for (auto const& result : response->results()) {
		auto const& alt = result.alternatives(0);
		std::cout << "Transcript: " << alt.transcript() << "\n";
		for (auto const& word : alt.words()) {
			std::cout << word.word() << " (Speaker " << word.speaker_tag() << ")\n";
		}
	}

	return 0;
	
}

