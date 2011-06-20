//
//  Synchdata.cxx
//
//  Created by Tristan on 06/15/11.
//  Copyright 2011 The Echo Nest. All rights reserved.
//

#include "synchdata.h"

/* 
    Compute the sample offset given a waveform and the Echo Nest synchstring.
    Assumes filename.raw to be a single channel waveform at 22050 Hz.
    Compile this by doing: make
    
    Fs Nch <Nzs Zi dz_1 ... dz_Nzs>_1 ... <Nzs Zi dz_1 ... dz_Nzs>_Nch

    where,
    Fs: sampling rate (currently 22050)
    Nch: number of chunks (currently 3)
    Nzs: number of zero crossings
    Zi: a zero crossing reference
    dz_n: number of samples to the next zero crossing
*/

const std::string inflate(std::string s)
{
    int factor = 10; // arbitrary expansion factor
    z_stream stream;
    
    while(true)
    {
        char *out = new char[factor*s.length()];
        
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = s.length();
        stream.next_in = (Bytef*)s.c_str();
        stream.avail_out = s.length()*factor;
        stream.next_out = (Bytef*)out;
        
        inflateInit(&stream);
        inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        
        if (stream.total_out >= factor*s.length())
        {
            // we didn't allocate enough space to inflate... Let's double this.
            delete[] out;
            factor *= 2;
            continue;
        }
        
        std::string result;
        for (unsigned long i = 0; i < stream.total_out; i++)
            result += out[i];
            
        delete[] out;
        return result;
    }
}

const std::string decompress_string(const std::string &s)
{
    std::string s_decoded = base64_decode(s, true); // this is base64_url
    std::string s_inflated = inflate(s_decoded);
    return s_inflated;
}

std::vector<int> &split(const std::string &s, char delim, std::vector<int> &elems)
{
    std::stringstream ss(s);
    std::string item;
    
    while ( std::getline(ss, item, delim) ) 
        elems.push_back( atoi(item.c_str()) );
    
    return elems;
}

std::vector<int> split(const std::string &s, char delim) 
{
    std::vector<int> elems;
    return split(s, delim, elems);
}

std::vector< std::vector<int> > decode(std::vector<int> list_of_numbers, uint* sample_rate)
{
    std::vector< std::vector<int> > zcs;
    std::vector<int> zc;
    
    uint c = 0;
    *sample_rate = list_of_numbers[c++];
    uint number_offsets = list_of_numbers[c++];
    
    for (uint i=0; i<number_offsets; i++)
    {
        uint counter = list_of_numbers[c++];
        uint sample = list_of_numbers[c++];
        
        zc.clear();
        zc.reserve(counter);
        zc.push_back(sample);
        
        for (uint j=0; j<counter-1; j++)
            zc.push_back( zc.back() + list_of_numbers[c++] );
        
        zcs.push_back(zc);
    }
    
    return zcs;
}

int align(int *snd, int *ref, uint size)
{
    int *xcorr = new int[size];
    
    // reverse snd
    int *snd_rev = new int[size];
    for (uint i=0; i<size; i++)
        snd_rev[i] = snd[size-i-1];
    
    convolve(xcorr, snd_rev, ref, size);
    
    uint max_index = 0;
    maxvi(xcorr, size, &max_index);
    
    delete [] xcorr;
    return (size/2) - max_index;
}

// return the max value in a vector, and its corresponding index
int maxvi(const int *aVector, uint aSize, uint *anIndex)
{
    int val = -INT_MAX;
    uint index = 0;
    
    for (uint i=0;i<aSize;i++)
        if(aVector[i] > val) { val = aVector[i]; index = i; }
    
    memcpy(anIndex, (uint*)&index, sizeof(uint));
    return val;
}

// returns the mean of a vector
float meanv(const int *aVector, uint aSize) 
{
    float val = 0;
    
    for (uint i=0;i<aSize;i++) 
        val = val + (float)aVector[i];
    
    val = val / (float)aSize;
    return val;
}

// convolution function where output size == input size
// this function is slow, but not optimized
void convolve(int* pOutput, const int* pInput1, const int* pInput2, uint aSize)
{
    int workingSize = aSize+aSize-1;
    int *pWorking = new int[workingSize];
    
    for (int i=0; i<workingSize; i++)
    {
        int sum = 0;
        for (int j=0; j < (int)aSize; j++)
            if (i-j >= 0 && i-j < (int)aSize)
                // convolve: multiply and accumulate
                sum += pInput1[i-j] * pInput2[j];
        pWorking[i] = sum;
    }
    
    // Copy the results from pWorking to pOutput.
    const int* pResult = pWorking + aSize/2 - 1;
    std::copy(pResult, pResult + aSize, pOutput);
}

int analyze_offsets(std::vector<int> offsets, int *offset)
{
    int size = (offsets.size() * offsets.size() - offsets.size()) / 2;
    int *dists = new int[size];
    
    uint c = 0;
    for (uint i=0; i<offsets.size(); i++)
        for (uint j=0; j<offsets.size(); j++)
            if (j>i)
                dists[c++] = abs(offsets[i]-offsets[j]);
    
    uint index;
    int max_dist = maxvi(dists, size, &index);
    
    if (max_dist > 1) 
    {
        offset = 0;
        delete [] dists;
        return -1;
    }
    else if (max_dist == 0) *offset = offsets[0];
    else if (max_dist == 1) *offset = (int)roundf(meanv(&offsets[0], offsets.size()));
         
    delete [] dists;
    return 0;
}

int synch(const float* pcm, uint numSamples, uint numChannels, uint sampleRate, const char *synchstring, std::vector<int> &offsets)
{
    std::string decompressed_string = decompress_string( std::string(synchstring) );
    
    if (decompressed_string == "") 
        return -2; // error decoding synchstring
    
    std::vector<int> list_of_numbers = split(decompressed_string,' ');
    
    uint analysis_sample_rate = 0;
    std::vector< std::vector<int> > zcs = decode(list_of_numbers, &analysis_sample_rate);
    
    offsets.reserve((int)zcs.size());
    uint size = (int)(SYNC_DURATION * sampleRate);
    
    int *ref = new int[size];
    int *snd = new int[size];
    
    // we're analyzing all chunks here
    // a quick and dirty approach would only consider the first chunk
    // and perhaps a smaller maximum offset (currently +/- 500 ms)
    
    for (uint i=0; i<zcs.size(); i++)
    {
        std::vector<int> zcl = zcs[i];
        
        memset(ref, 0, size*sizeof(int));
        memset(snd, 0, size*sizeof(int));
        
        // build zero-crossing reference vector
        for (uint j=0; j<(uint)zcl.size(); j++)
            ref[zcl[j]-zcl[0]] = 1;
        
        if (zcl[0] >= (int)numSamples)
            return -3;
        
        // find zero crossings in pcm
        for (uint j=zcl[0]; j<zcl[0]+size-1; j++)
        {
            if (pcm[j] >= numSamples) break;
            if (pcm[j] < 0 && pcm[j+1] >= 0) snd[j-zcl[0]] = 1;
        }
        
        int offset = align(snd, ref, size);
        
        offsets.push_back( offset );
    }
    
    delete [] ref;
    delete [] snd;
    
    return 0;
}

int main(int argc, char** argv) 
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s mono_audio_at_22050_Hz.raw synchstring.txt\n", argv[0]);
        exit(-1);
    }
    
    char *filename = argv[1];
    char *synchname = argv[2];
    
    //  Open the raw file, which was created for example by doing
    //  ffmpeg -i billie_jean.mp3 -ac 1 -ar 22050 -f s16le - > billie_jean.ffmpeg.raw
    
    FILE *pFile = fopen(filename, "rb");
    if (pFile == NULL) { fprintf(stderr, "\nError: Cannot open file!\n\n"); return -4; }

    std::vector<short*> vChunks;
    
    uint _NumberSamples = 0;
    uint nSamplesPerChunk = (uint) 22050*5;
    uint samplesRead = 0;
    
    do {
        short* pChunk = new short[nSamplesPerChunk];
        samplesRead = fread(pChunk, sizeof(short), nSamplesPerChunk, pFile);
        _NumberSamples += samplesRead;
        vChunks.push_back(pChunk);
    } while (samplesRead > 0);
    
    //  Convert from shorts to 16-bit floats and copy into sample buffer.
    
    uint sampleCounter = 0;
    float *_pSamples = new float[_NumberSamples];
    uint samplesLeft = _NumberSamples;
    
    for (uint i=0; i < vChunks.size(); i++) 
    {
        short* pChunk = vChunks[i];
        uint numSamples = samplesLeft < nSamplesPerChunk ? samplesLeft : nSamplesPerChunk;
        for (uint j = 0; j < numSamples; j++)
            _pSamples[sampleCounter++] = (float)pChunk[j]/32768.0f;
        samplesLeft -= numSamples;
        delete [] pChunk, vChunks[i] = NULL;
    }
    
    fclose(pFile);
    
    pFile = fopen(synchname, "r+");
    if (pFile == NULL) { fprintf(stderr, "\nError: Cannot open file!\n\n"); return -4; }
    
    char ch;
    std::string synchstring = std::string();
    
    while (1)
    {
        ch = fgetc(pFile);
        if (ch == EOF || ch == '\n') break;
        else synchstring.push_back(ch);
    }

    fclose(pFile);
    
    //  OK, now do the matching. Single call, takes: pcm buffer (must be floats @ 22050Hz, mono), 
    //  the number of samples, the synchstring, and the offset container
    
    std::vector<int> offsets;
    int result = synch(_pSamples, _NumberSamples, 1, 22050, synchstring.c_str(), offsets);
    //int result = synch(_pSamples, _NumberSamples, 1, 22050, buffer, offsets);
    
    int offset;
    if (result == 0)
        if (offsets.size() > 1) result = analyze_offsets(offsets, &offset);
        else offset = offsets[0];
        
    switch (result)
    {
        case  0: { fprintf(stderr, "Offset = %.5f seconds\n\n", offset/22050.0f); break; }
        case -1: { fprintf(stderr, "Warning: Mismatch detected!\nFound offsets "); 
                    for (uint i=0; i<offsets.size(); i++) fprintf(stderr, "%.5f ", offsets[i]/22050.0f);
                    fprintf(stderr, "seconds\n\n"); break; }
        case -2: { fprintf(stderr, "Error: Decoding synchstring!\n\n"); break; }
        case -3: { fprintf(stderr, "Error: Not enough audio!\n\n"); break; }
        return result;
    }
}
