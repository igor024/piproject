#include "portaudio.h"
//anonymous struct 
struct stereo{
	float left = 0;
	float right = 0;
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
	float* out = (float*) out;

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
