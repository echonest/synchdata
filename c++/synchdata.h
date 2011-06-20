//
//  Synchdata.h
//
//  Created by Tristan on 06/15/11.
//  Copyright 2011 The Echo Nest. All rights reserved.
//

#ifndef SYNCHDATA_H
#define SYNCHDATA_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <math.h>
#include <zlib.h>
#include <climits>
#include "Base64.h"

typedef unsigned int uint; 
#define SYNC_DURATION 1.0 // in seconds

const std::string inflate(std::string s);
const std::string decompress_string(const std::string &s);
std::vector<int> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<int> split(const std::string &s, char delim);
std::vector< std::vector<int> > decode(std::vector<int> list_of_numbers, uint* sampling_rate);
int maxvi(const int *aVector, uint aSize, uint * anIndex);
float meanv(const int *aVector, uint aSize);
int align(int *snd, int *ref, uint size);
void convolve(int* pOutput, const int* pInput1, const int* pInput2, uint aSize);
int synch(const float* pcm, uint numSamples, uint numChannels, uint sampleRate, const char *synchstring, std::vector<int> &offsets);

#endif