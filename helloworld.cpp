#include <portaudio.h>
#include "google/cloud/speech/v1/speech_client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include "google/cloud/speech/v1/speech_client.h"
#include <cstring>

namespace speech = ::google::cloud::speech::v1;
namespace speech_v1 = google::cloud::speech_v1::v2_37;


std::string ReadFileToString(const std::string& file_path) {
	std::ifstream file(file_path, std::ios::binary);
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

static void checkPaErr(PaError err) {
	if(err != paNoError) {
		printf("PortAudio error: %s\n", Pa_GetErrorText(err));
		exit(EXIT_FAILURE);
	}
}

void showDevices() {
	int numDev = Pa_GetDeviceCount();
	printf("Num devices: %d\n", numDev);
	if(numDev < 0) {
		printf("Error counting devices");
		exit(EXIT_FAILURE);
	} else if(numDev == 0) {
		printf("No devices found");
		exit(EXIT_SUCCESS);
	}

	const PaDeviceInfo* devInfo;
	for(int i = 0; i < numDev; ++i) {
		devInfo = Pa_GetDeviceInfo(i);
		printf("Device %d:\nName: %s\ndefaultSampleRate: %f\n",
				i,
				devInfo->name,
				devInfo->defaultSampleRate);
	}

	int dev;
	cout << "enter dev: ";
	cin >> dev;

	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;

	memset(&inputParameters, 0, sizeOf(inputParameters);
	inputParameters.channelCount = 2;
	inputParameters.device = dev;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowInputLatency;

	memset(&outputParameters, 0, sizeof(outputParameters));
	outputParameters.channelCount = 2;
	outputParameters.device = dev;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
}

int main() {
	PaError err;
	checkPaErr(Pa_Initialize());
	showDevices();
	checkPaErr(Pa_Terminate());


	auto client = speech_v1::SpeechClient(speech_v1::MakeSpeechConnection());

	speech::RecognitionConfig config;
	config.set_encoding(speech::RecognitionConfig::MP3);
	config.set_sample_rate_hertz(44100);
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

