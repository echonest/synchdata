# Synchdata with Synchstring

by Tristan Jehan, 06/20/2011

Copyright (c) 2011 The Echo Nest Corporation

Synchdata is some sample code (in C++ and Python) that demonstrates how to accurately synchronize [The Echo Nest analysis data](http://developer.echonest.com/docs/v4/track.html "Track API methods") to a corresponding waveform, regardless of which mp3 decoder was used to generate that waveform. This is done using the Echo Nest "synchstring," a base64 encoding of a zlib compression of an hex-encoded series of ASCII integers, that describe the zero-crossing locations for multiple chunks of audio throughout the file. The decoded list of integers is formatted as follows:

    Fs Nch <Nzs Zi dz_1 ... dz_Nzs>_1 ... <Nzs Zi dz_1 ... dz_Nzs>_Nch

    where,
    Fs: sampling rate (currently 22050)
    Nch: number of chunks (currently 3)
    Nzs: number of zero crossings
    Zi: a zero crossing reference
    dz_n: number of samples to the next zero crossing

## Why is this useful?

All mp3 decoders (e.g. mpg123, ffmpeg, quicktime, lame, and others) have their own approach to decoding and correcting errors (corrupt frames). That leads to slight variations in the output waveform. In particular, the beginning of the waveform may be shifted in time by a small, yet noticeable time offset (e.g. tens of milliseconds). Unfortunately that offset is somewhat signal dependent, and therefore intractable by simply using the decoder name and version.

## How it works

Synchdata first decodes the synchstring into 3 lists of zero-crossing sample locations, as extracted by the Echo Nest analyzer (we use mpg123), i.e. 1 second worth of audio at the beginning, the middle and the end of the file. It then extracts zero-crossings in the same 3 locations from the proposed 1-second chunks of audio: locally decoded mp3, converted to mono and resampled at 22050 Hz. It finally correlates the zero-crossing data as described in the synchstring with that of the proposed waveform, and retains the optimal sample-accurate alignment (a time offset returned in seconds) for each of the chunks.

If the 3 time offsets are identical, then the offset can be trusted throughout the file, and added to any of the timing information provided in the JSON analysis data (e.g. segment onsets, beats, bars). If there's a mismatch between some of the computed offsets, then the analysis data is misaligned with the waveform somewhere, and sample accuracy isn't guarantied. This can occur when the decoder tries to cope with a corrupt mp3 frame by either inserting some silence, some bogus audio, or by discarding the frame, resulting in discontinuities and time misalignments.

## Speed

The synchdata sample code is provided as an example on how to deal with the Echo Nest synchstring and as a result, data synchronization. It is by no means optimized for speed but will be improved in future updates. For instance, the convolution function could be significantly accelerated with the [FFT-based algorithm](https://ccrma.stanford.edu/~jos/mdft/Convolution_Theorem.html "FFT Convolution"). If speed is a concern, or if only a partial waveform is available (e.g. when streaming audio), one can only compute the initial offset, and assume it to be accurate, while others become available. Currently, the maximum retrieved offset can be +/- 500 ms. However, we almost never run into offsets beyond +/- 100 ms. Computation can be reduced by correlating only 200 ms worth of zero crossings.

## C++

Compile the sample program using: make

Test the program with the proposed waveform stored in raw binary format for 3 different decoders:

    $ ./synchdata ../data/billie.mpg123.22050.mono.raw ../data/billie.synchstring.txt 
    Offset = 0.00000 seconds
    
Note that since the synchstring was generated with the same version of mpg123, there's an exact match.

    $ ./synchdata ../data/billie.ffmpeg.22050.mono.raw ../data/billie.synchstring.txt 
    Warning: Mismatch detected!
    Found offsets 0.01197 -0.01415 -0.01415 seconds

In this case, an error occurred in the first section of the file. There will be a misalignment up to 0.01415 + 0.01197 = 0.02612 seconds or ~26 ms.

    $ ./synchdata ../data/billie.quicktime.22050.mono.raw ../data/billie.synchstring.txt 
    Offset = 0.03478 seconds

The offset here is consistent and can be trusted. The client program should add this constant offset to the timing data in the JSON file.

## Python

Assuming numpy, base64, and zlib modules installed, run the test examples like this:

    $ python synchdata.py ../data/billie.mpg123.22050.mono.raw ../data/billie.synchstring.txt
    Offset = 0.00000 seconds

    $ python synchdata.py ../data/billie.ffmpeg.22050.mono.raw ../data/billie.synchstring.txt
    Warning: Mismatch detected!
    Found offsets 0.01197 -0.01415 -0.01415 seconds

    $ python synchdata.py ../data/billie.quicktime.22050.mono.raw ../data/billie.synchstring.txt
    Offset = 0.03478 seconds

See comments in the C++ section.

## FAQ

Q: Can I use this yet?

A: No. The current API doesn't return synchstrings yet.
