/* Sonic library
   Copyright 2010
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

/*
The Sonic Library implements a new algorithm invented by Bill Cox for the
specific purpose of speeding up speech by high factors at high quality.  It
generates smooth speech at speed up factors as high as 6X, possibly more.  It is
also capable of slowing down speech, and generates high quality results
regardless of the speed up or slow down factor.  For speeding up speech by 2X or
more, the following equation is used:

    newSamples = period/(speed - 1.0)
    scale = 1.0/newSamples;

where period is the current pitch period, determined using AMDF or any other
pitch estimator, and speed is the speedup factor.  If the current position in
the input stream is pointed to by "samples", and the current output stream
position is pointed to by "out", then newSamples number of samples can be
generated with:

    out[t] = (samples[t]*(newSamples - t) + samples[t + period]*t)/newSamples;

where t = 0 to newSamples - 1.

For speed factors < 2X, the PICOLA algorithm is used.  The above
algorithm is first used to double the speed of one pitch period.  Then, enough
input is directly copied from the input to the output to achieve the desired
speed up factor, where 1.0 < speed < 2.0.  The amount of data copied is derived:

    speed = (2*period + length)/(period + length)
    speed*length + speed*period = 2*period + length
    length(speed - 1) = 2*period - speed*period
    length = period*(2 - speed)/(speed - 1)

For slowing down speech where 0.5 < speed < 1.0, a pitch period is inserted into
the output twice, and length of input is copied from the input to the output
until the output desired speed is reached.  The length of data copied is:

    length = period*(speed - 0.5)/(1 - speed)

For slow down factors below 0.5, no data is copied, and an algorithm
similar to high speed factors is used.
*/

/* Uncomment this to use sin-wav based overlap add which in theory can improve
   sound quality slightly, at the expense of lots of floating point math. */
/* #define SONIC_USE_SIN */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SONIC_INTERNAL
/* The following #define's are used to change the names of the routines defined
 * here so that a new library (sonic2) can reuse these names, and then call
 * the original names.  We do this for two reasons: 1) we don't want to change
 * the original API, and 2) we want to add a shim, using the original names and
 * still call these routines.
 *
 * Original users of this API and the libsonic library need to do nothing.  The
 * original behavior remains.
 *
 * A new user that add some additional functionality above this library (a shim)
 * should #define SONIC_INTERNAL before including this file, undefine all these
 * symbols and call the sonicIntXXX functions directly.
 */
#define sonicCreateStream sonicIntCreateStream
#define sonicDestroyStream sonicIntDestroyStream
#define sonicWriteFloatToStream sonicIntWriteFloatToStream
#define sonicWriteShortToStream sonicIntWriteShortToStream
#define sonicWriteUnsignedCharToStream sonicIntWriteUnsignedCharToStream
#define sonicReadFloatFromStream sonicIntReadFloatFromStream
#define sonicReadShortFromStream sonicIntReadShortFromStream
#define sonicReadUnsignedCharFromStream sonicIntReadUnsignedCharFromStream
#define sonicFlushStream sonicIntFlushStream
#define sonicSamplesAvailable sonicIntSamplesAvailable
#define sonicGetSpeed sonicIntGetSpeed
#define sonicSetSpeed sonicIntSetSpeed
#define sonicGetPitch sonicIntGetPitch
#define sonicSetPitch sonicIntSetPitch
#define sonicGetRate sonicIntGetRate
#define sonicSetRate sonicIntSetRate
#define sonicGetVolume sonicIntGetVolume
#define sonicSetVolume sonicIntSetVolume
#define sonicGetQuality sonicIntGetQuality
#define sonicSetQuality sonicIntSetQuality
#define sonicGetSampleRate sonicIntGetSampleRate
#define sonicSetSampleRate sonicIntSetSampleRate
#define sonicGetNumChannels sonicIntGetNumChannels
#define sonicGetUserData sonicIntGetUserData
#define sonicSetUserData sonicIntSetUserData
#define sonicSetNumChannels sonicIntSetNumChannels
#define sonicChangeFloatSpeed sonicIntChangeFloatSpeed
#define sonicChangeShortSpeed sonicIntChangeShortSpeed
#define sonicEnableNonlinearSpeedup sonicIntEnableNonlinearSpeedup
#define sonicComputeSpectrogram sonicIntComputeSpectrogram
#define sonicGetSpectrogram sonicIntGetSpectrogram

#endif /* SONIC_INTERNAL */

/* This specifies the range of voice pitches we try to match.
   Note that if we go lower than 65, we could overflow in findPitchInRange */
#ifndef SONIC_MIN_PITCH
#define SONIC_MIN_PITCH 65
#endif  /* SONIC_MIN_PITCH */
#ifndef SONIC_MAX_PITCH
#define SONIC_MAX_PITCH 400
#endif  /* SONIC_MAX_PITCH */

/* These are used to down-sample some inputs to improve speed */
#define SONIC_AMDF_FREQ 8000

struct sonicStreamStruct;
typedef struct sonicStreamStruct* sonicStream;

/* For all of the following functions, numChannels is multiplied by numSamples
   to determine the actual number of values read or returned. */

/* Create a sonic stream.  Return NULL only if we are out of memory and cannot
  allocate the stream. Set numChannels to 1 for mono, and 2 for stereo. */
sonicStream sonicCreateStream(int sampleRate, int numChannels);
/* Destroy the sonic stream. */
void sonicDestroyStream(sonicStream stream);
/* Attach user data to the stream. */
void sonicSetUserData(sonicStream stream, void *userData);
/* Retrieve user data attached to the stream. */
void *sonicGetUserData(sonicStream stream);
/* Use this to write floating point data to be speed up or down into the stream.
   Values must be between -1 and 1.  Return 0 if memory realloc failed,
   otherwise 1 */
int sonicWriteFloatToStream(sonicStream stream, const float* samples, int numSamples);
/* Use this to write 16-bit data to be speed up or down into the stream.
   Return 0 if memory realloc failed, otherwise 1 */
int sonicWriteShortToStream(sonicStream stream, const short* samples, int numSamples);
/* Use this to write 8-bit unsigned data to be speed up or down into the stream.
   Return 0 if memory realloc failed, otherwise 1 */
int sonicWriteUnsignedCharToStream(sonicStream stream, const unsigned char* samples,
                                   int numSamples);
/* Use this to read floating point data out of the stream.  Sometimes no data
   will be available, and zero is returned, which is not an error condition. */
int sonicReadFloatFromStream(sonicStream stream, float* samples,
                             int maxSamples);
/* Use this to read 16-bit data out of the stream.  Sometimes no data will
   be available, and zero is returned, which is not an error condition. */
int sonicReadShortFromStream(sonicStream stream, short* samples,
                             int maxSamples);
/* Use this to read 8-bit unsigned data out of the stream.  Sometimes no data
   will be available, and zero is returned, which is not an error condition. */
int sonicReadUnsignedCharFromStream(sonicStream stream, unsigned char* samples,
                                    int maxSamples);
/* Force the sonic stream to generate output using whatever data it currently
   has.  No extra delay will be added to the output, but flushing in the middle
   of words could introduce distortion. */
int sonicFlushStream(sonicStream stream);
/* Return the number of samples in the output buffer */
int sonicSamplesAvailable(sonicStream stream);
/* Get the speed of the stream. */
float sonicGetSpeed(sonicStream stream);
/* Set the speed of the stream. */
void sonicSetSpeed(sonicStream stream, float speed);
/* Get the pitch of the stream. */
float sonicGetPitch(sonicStream stream);
/* Set the pitch of the stream. */
void sonicSetPitch(sonicStream stream, float pitch);
/* Get the rate of the stream. */
float sonicGetRate(sonicStream stream);
/* Set the rate of the stream. */
void sonicSetRate(sonicStream stream, float rate);
/* Get the scaling factor of the stream. */
float sonicGetVolume(sonicStream stream);
/* Set the scaling factor of the stream. */
void sonicSetVolume(sonicStream stream, float volume);
/* Chord pitch is DEPRECATED.  AFAIK, it was never used by anyone.  These
   functions still exist to avoid breaking existing code. */
/* Get the chord pitch setting. */
int sonicGetChordPitch(sonicStream stream);
/* Set chord pitch mode on or off.  Default is off.  See the documentation
   page for a description of this feature. */
void sonicSetChordPitch(sonicStream stream, int useChordPitch);
/* Get the quality setting. */
int sonicGetQuality(sonicStream stream);
/* Set the "quality".  Default 0 is virtually as good as 1, but very much
 * faster. */
void sonicSetQuality(sonicStream stream, int quality);
/* Get the sample rate of the stream. */
int sonicGetSampleRate(sonicStream stream);
/* Set the sample rate of the stream.  This will drop any samples that have not
 * been read. */
void sonicSetSampleRate(sonicStream stream, int sampleRate);
/* Get the number of channels. */
int sonicGetNumChannels(sonicStream stream);
/* Set the number of channels.  This will drop any samples that have not been
 * read. */
void sonicSetNumChannels(sonicStream stream, int numChannels);
/* This is a non-stream oriented interface to just change the speed of a sound
   sample.  It works in-place on the sample array, so there must be at least
   speed*numSamples available space in the array. Returns the new number of
   samples. */
int sonicChangeFloatSpeed(float* samples, int numSamples, float speed,
                          float pitch, float rate, float volume,
                          int useChordPitch, int sampleRate, int numChannels);
/* This is a non-stream oriented interface to just change the speed of a sound
   sample.  It works in-place on the sample array, so there must be at least
   speed*numSamples available space in the array. Returns the new number of
   samples. */
int sonicChangeShortSpeed(short* samples, int numSamples, float speed,
                          float pitch, float rate, float volume,
                          int useChordPitch, int sampleRate, int numChannels);


int get_sonicReadLen(sonicStream stream);

#include "custom_mem.h"

#define sonicCalloc_user      custom_calloc_psram
#define sonicRealloc_user     custom_realloc_psram
#define sonicFree_user        custom_free_psram

#ifdef __cplusplus
}
#endif
