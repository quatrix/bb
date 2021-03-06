#!/usr/bin/env python

import click
from struct import pack, unpack
from collections import defaultdict
from consts import SEP

import os
import math
import random
import numpy as np



def read_raw(res, input_file, chunk_size):
    step = 3
    chunk_size *= step
    chunk_size = math.floor(chunk_size / step) * step + step

    with open(input_file) as f:
        acc = []
        cls = None

        while True:
            line = f.readline().strip()

            if not line:
                break

            if line == SEP:
                if acc:
                    for c in list(chunk(acc, step, chunk_size=chunk_size)):
                        if len(c) == chunk_size:
                            res[cls].append(c)
                    acc = []
                cls = None
            else:
                raw = [int(i) for i in line.split(',')]
                cls = raw[-1]
                acc += raw[4:-1]


def chunk(data, step, chunk_size):
    res = []

    for i in range(0, len(data), step):
        res += data[i:i+step]

        if len(res) >= chunk_size:
            yield res
            res = []

    return res 

@click.command()
@click.option('-i', '--input-file', required=True, multiple=True)
@click.option('-o', '--output-dir', required=True)
@click.option('-c', '--chunk-size', default=10)
@click.option('-n', '--n-samples', default=30)
def main(input_file, output_dir, chunk_size, n_samples):
    all_samples = defaultdict(list)
    samples = defaultdict(dict)

    for i in input_file:
        read_raw(all_samples, i, chunk_size)

    for cls, s in all_samples.items():
        random.shuffle(s)

        samples['train'][cls] = s[0:n_samples]
        samples['test'][cls] = s[n_samples:]

        print(cls, len(samples['train'][cls]))

    for t, clss in samples.items():
        for cls, tss in clss.items():
            f = open(os.path.join(output_dir, '{}_{}.bin'.format(t, cls)), 'wb')
            f_asc = open(os.path.join(output_dir, '{}_{}.txt'.format(t, cls)), 'w')

            for ts in tss:
                packed = pack('%dh' % len(ts), *ts)
                f.write(packed)
                f_asc.write(','.join([str(i) for i in ts]) + '\n')

            
if __name__ == '__main__':
    main()
