#!/usr/bin/env python

import click
import os
import math
from collections import defaultdict
from scipy.interpolate import interp1d
from consts import IMU_MIN, IMU_MAX, SEP, axis
import random
import numpy as np


def read_raw(lines, chunk_size, undersample_by, lowpass_threshold):
    res = []

    acc = None
    cls = None
    ts = None

    for line in lines:
        line = line.strip()

        if not line:
            break

        if line == SEP:
            if acc is not None and acc.size:
                for c in merge(cls, acc, chunk_size, undersample_by, lowpass_threshold):
                    res.append((ts, cls, c))
                acc = None

            cls = None
            ts = None
        else:
            splitted = line.split(',')

            if cls is None:
                ts = int(splitted[0])
                cls = int(splitted[-1]) + 1
            
            if cls > 3:
                continue

            imu = np.array([float(i) for i in splitted[4:-1]], dtype=float)

            if acc is not None:
                acc = np.vstack((acc, imu))
            else:
                acc = imu

    return res


def low_pass_filter(d, threshold):
    for i in d:
        if abs(i) > threshold:
            yield i
        else:
            yield 0


def merge(cls, acc, chunk_size, undersample_by, lowpass_threshold):
    chunk_size = math.floor(chunk_size / undersample_by)
    acc = np.transpose(acc)
    filtered = [list(low_pass_filter(undersample(i, undersample_by), lowpass_threshold)) for i in acc]
    chunked = np.array([list(chunk(i, chunk_size)) for i in filtered])
    chunked = np.array([np.array([np.pad(i, (0, chunk_size - len(i)), 'constant') for i in d]) for d in chunked])

    return np.transpose(chunked, (1, 0, 2))
    

def chunk(data, chunk_size):
    half_chunk = math.floor(chunk_size/2)
    prev_data = data[0:chunk_size]

    if not len(prev_data):
        return np.array([])

    prev_data = np.array(prev_data)
    yield prev_data

    for i in range(chunk_size, len(data), half_chunk):
        chunk = np.append(prev_data[half_chunk:], data[i:i+half_chunk])
        yield chunk
        prev_data = chunk
    

def undersample(data, size):
    res = []

    for i in range(0, len(data), size):  
        res.append(np.average(data[i:i+size]))

    return res

@click.command()
@click.option('-i', '--input-file', required=True, multiple=True)
@click.option('-o', '--output-dir', required=True)
@click.option('-c', '--chunk-size', default=30)
@click.option('-r', '--ratio', default=0.7)
@click.option('-u', '--undersample-by', default=4)
@click.option('-l', '--lowpass-threshold', default=100)
def main(input_file, output_dir, chunk_size, ratio, undersample_by, lowpass_threshold):
    try:
        for t in 'train', 'test':
            for axi in axis:
                os.unlink(os.path.join(output_dir, t, axi))

            os.unlink(os.path.join(output_dir, t, 'classification'))
    except FileNotFoundError:
        pass

    data = {
        'train': [],
        'test': [],
    }

    for i in input_file:
        a = open(i).readlines()
        data['train'] += read_raw(a[0:math.floor(len(a) * ratio)], chunk_size, undersample_by, lowpass_threshold)
        data['test'] += read_raw(a[math.floor(len(a) * ratio):], chunk_size, undersample_by, lowpass_threshold)

    for t, data in data.items():
        for i in data:
            cls = i[1]

            for axi, ts in zip(axis, i[2]):
                open(os.path.join(output_dir, t, axi), 'a').write(','.join([str(j) for j in ts]) + '\n')
             
            open(os.path.join(output_dir, t, 'classification'), 'a').write(str(cls) + '\n')
            
if __name__ == '__main__':
    main()
