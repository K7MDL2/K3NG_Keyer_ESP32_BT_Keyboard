#ifndef GOERTZEL_H
#define GOERTZEL_H


/*


  Goertzel formula audio detector

  This code comes from http://www.skovholm.com/cwdecoder , http://www.skovholm.com/decoder11.ino

  Hjalmar skovholm Hansen, OZ1JHM <hjh@skovholm.com>


	Notes from the original code author, OZ1JHM (with edits from Goody K3NG)

	GOERTZ_SAMPLING_FREQ will be 8928 on a 16 mhz without any prescaler etc.
	because we need the tone in the center of the bins
	you can set GOERTZ_TARGET_FREQ (the tone) to 496, 558, 744 or 992
	then GOERTZ_SAMPLES_INT the number of samples which give the bandwidth
	which can be (8928 / GOERTZ_TARGET_FREQ) * 1 or 2 or 3 or 4 etc           
	init is 8928/558 = 16 * 4 = 64 samples                 

	try to take GOERTZ_SAMPLES = 96 or 128 ;o)    

	48 will give you a bandwidth around 186 hz            
	64 will give you a bandwidth around 140 hz            
	96 will give you a bandwidth around 94 hz            
	128 will give you a bandwidth around 70 hz  

	BUT remember that a high GOERTZ_SAMPLES will take a lot of time so you have to find a compromise  

*/

#define GOERTZ_TARGET_FREQ 600  // target = sample rate/N.  N does not need to be a power of 2.

//#if defined(_VARIANT_ARDUINO_DUE_X_)
  // Arduino Due (84 Mhz clock)
//  #define GOERTZ_SAMPLING_FREQ 46872.0
//  #define GOERTZ_SAMPLES 252 //168 //84
//#if defined(HARDWARE_ESP32_DEV)
  // ESP32-WROOM32 (40 Mhz clock)
  #define CPU_FREQ 240 // MHz CPU main clock frequency
  //  Default analogRead() Sample Rate Typical speed: ~6–7 kSamples/sec (about 140–160 µs per reading) in Arduino mode.
  //#define GOERTZ_SAMPLING_FREQ  ( CPU_FREQ * GOERTZ_TARGET_FREQ )     // 240*558=133,920
  #define GOERTZ_SAMPLING_FREQ  ( 7200 )     //48KHz - min value must be at least twice the target frequency or higher.
  #define GOERTZ_SAMPLES (GOERTZ_SAMPLING_FREQ / GOERTZ_TARGET_FREQ)  //  N = samplerate/target.  Integer result perferred
//#else
  // Arduino Uno, Mega (16 Mhz clock)
//  #define GOERTZ_SAMPLING_FREQ 8928.0
//  #define GOERTZ_SAMPLES 48 //48 //64
//#endif

#define GOERTZ_NOISE_BLANKER_INITIAL_MS 6

#define GOERTZ_MAGNITUDE_LIMIT_LOW 500 //100
#define GOERTZ_MAGNITUDE_THRESHOLD	0.6 //0.6
#define GOERTZ_MOVING_AVERAGE_FILTER 6

class Goertzdetector { 

  public:
    Goertzdetector();
    void init(int _audioInPin);
    int detecttone();
    int testData[GOERTZ_SAMPLES];
    float magnitude; 
    int magnitudelimit;
	int magnitudelimit_low;

  private:  

	float coeff;
	float Q1;
	float Q2;
	float sine;
	float cosine;  
	int audioInPin;
	int realstate;
	int realstatebefore;
	int filteredstate;
	unsigned long laststarttime;

};

#endif //GOERTZEL_H
