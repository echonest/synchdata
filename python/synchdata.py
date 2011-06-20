#!/usr/bin/env python
# encoding: utf-8
"""
synchdata.py

Takes a raw mono wavefile at 22050 Hz, and a synchstring.
Computes an offset in number of samples.

Created by T on 2011-06-13.
Copyright (c) 2011 The Echo Nest. All rights reserved.
"""

import sys, os
import numpy as np
import zlib, base64

SYNC_DURATION = 1

# The synchstring format goes as follows:
# Fs Nch <Nzs Zi dz_1 ... dz_Nzs>_1 ... <Nzs Zi dz_1 ... dz_Nzs>_Nch
#
# where,
# Fs: sampling rate (currently 22050)
# Nch: number of chunks (currently 3)
# Nzs: number of zero crossings
# Zi: a zero crossing reference
# dz_n: number of samples to the next zero crossing

def decode_string(s):
    l = [int(i) for i in s.split(' ')]
    Fs = l.pop(0)
    Nch = l.pop(0)
    list = []
    n = 0
    for i in range(Nch):
        N = l[n]
        list.append((l[n+1],[]))
        n += 2
        for j in range(N-1):
            if j == 0: list[i][1].append(l[n])
            else: list[i][1].append(list[i][1][j-1] + l[n])
            n += 1
    return (Fs, list)

def decompress_string(compressed_string):
    compressed_string = compressed_string.encode('utf8')
    if compressed_string == "":
        return ""
    # do the zlib/base64 stuff
    try:
        # this will decode both url-safe and non-url-safe b64 
        actual_string = zlib.decompress(base64.urlsafe_b64decode(compressed_string))
    except (zlib.error, TypeError):
        print "Could not decode base64 zlib string %s" % (compressed_string)
        return None
    return actual_string

def get_zero_crossings(wav):
    zcs = np.zeros((len(wav),), dtype=np.int32)
    for i in range(len(wav)-1):
        if wav[i] < 0 and wav[i+1] >= 0: zcs[i] = 1
    return zcs

def align(ref, snd):
    # correlations are much like convolutions
    cor = np.convolve(ref, snd[::-1], mode="same") # correlate
    return np.argmax(cor) - len(ref)/2

def align_all(zcs, wav):
    Fs = zcs[0]
    offsets = []
    for z in zcs[1]:
        ref = np.zeros((SYNC_DURATION*Fs,), dtype=np.int32)
        for i in z[1]: ref[i] = 1
        snd = wav[1][z[0]:z[0]+int(SYNC_DURATION*wav[0])]
        snd = get_zero_crossings(snd)
        offsets.append( align(snd,ref) )
    return offsets

def analyze_offsets(offset):
    dists = []
    for i,o1 in enumerate(offset):
        for j,o2 in enumerate(offset):
            if j > i:
                dists.append(abs(o1 - o2))
    if np.max(dists) == 0: return int(offset[0])
    if np.max(dists) == 1: return int(round(np.mean(offset)))
    else: return None

def read_synchstring(filepath):
    f = open(filepath, "r")
    line = f.readline()
    return line

def read_raw_audio(filepath, Fs=22050):
    f = open(filepath, "rb")
    audio = np.fromfile(file=f, dtype=np.int16)
    return (Fs, audio)
    
def synch(raw_audio_file, synchstring_file):
    wav = read_raw_audio(raw_audio_file)
    synchstring = read_synchstring(synchstring_file)
    decompressed_string = decompress_string(synchstring)
    if decompressed_string == None: return None
    zcs = decode_string(decompressed_string)
    
    off = align_all(zcs, wav)
    res = analyze_offsets(off)
    
    if res != None:
        print "Offset = %.5f seconds\n" % (res*(wav[0]/zcs[0])/float(wav[0]))
        return int(res*(wav[0]/zcs[0]))
    else:
        print "Warning: Mismatch detected!"
        st = ''.join([('%.5f ' % (off[i]*(wav[0]/zcs[0])/float(wav[0]))) for i in range(len(zcs)+1)])
        print "Found offsets %sseconds\n" % st
        return None

if __name__ == "__main__":
    if len(sys.argv) == 3:
        synch(str(sys.argv[1]), str(sys.argv[2]))
    else:
        print "Usage %s mono_audio_at_22050_Hz.raw synchstring.txt" % sys.argv[0]

