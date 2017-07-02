#!/usr/bin/env python

import click
import os
import math
from collections import defaultdict
from scipy.interpolate import interp1d
from consts import IMU_MIN, IMU_MAX, SEP, axis
import random
import numpy as np


def read_raw(input_file, chunk_size):
    res = []

    with open(input_file) as f:
        acc = None
        cls = None

        while True:
            line = f.readline().strip()

            if not line:
                break

            if line == SEP:
                if acc is not None and acc.size:
                    for c in merge(cls, acc, chunk_size):
                        res.append((cls, c))
                    acc = None
                cls = None
            else:
                splitted = line.split(',')
                cls = int(splitted[-1]) + 1
                #m = interp1d([IMU_MIN, IMU_MAX],[0,1])
                #imu = np.array([m(float(i)) for i in splitted[1:-1]], dtype=float)
                imu = np.array([float(i) for i in splitted[1:-1]], dtype=float)
                if acc is not None:
                    acc = np.vstack((acc, imu))
                else:
                    acc = imu

    return res


def merge(cls, acc, chunk_size):
    acc = np.transpose(acc)
    chunked = np.array([list(chunk(i, chunk_size)) for i in acc])
    chunked = np.array([np.array([np.pad(i, (0, chunk_size - len(i)), 'constant') for i in d]) for d in chunked])
    return np.transpose(chunked, (1, 0, 2))
    

def chunk(data, chunk_size):
    half_chunk = math.floor(chunk_size/2)
    prev_data = data[0:chunk_size]

    yield prev_data

    for i in range(chunk_size, len(data), half_chunk):
        chunk = np.append(prev_data[half_chunk:], data[i:i+half_chunk])
        yield chunk
        prev_data = chunk
    

@click.command()
@click.option('-i', '--input-file', required=True, multiple=True)
@click.option('-o', '--output-dir', required=True)
@click.option('-c', '--chunk_size', default=30)
@click.option('-r', '--ratio', default=0.6)
def main(input_file, output_dir, chunk_size, ratio):
    try:
        for t in 'train', 'test':
            for axi in axis:
                os.unlink(os.path.join(output_dir, t, axi))

            os.unlink(os.path.join(output_dir, t, 'classification'))
    except FileNotFoundError:
        pass

    data = []
    for i in input_file:
        data += read_raw(i, chunk_size)

    random.shuffle(data)

    for i in data:
        cls = i[0]

        t = 'train' if random.random() < 0.7 else 'test'

        for axi, ts in zip(axis, i[1]):
            open(os.path.join(output_dir, t, axi), 'a').write(','.join([str(j) for j in ts]) + '\n')
         
        open(os.path.join(output_dir, t, 'classification'), 'a').write(str(cls) + '\n')
            
if __name__ == '__main__':
    main()
