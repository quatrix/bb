#!/usr/bin/env python

import click
import os
import math
from scipy.interpolate import interp1d
from consts import Classifications, axis, Segment, RawLine


def read_segment(raw_data, segment):
    return [l for l in raw_data if l.ts >= segment.start and l.ts <= segment.end]


def parse_line(ts, *axis):
    m = interp1d([-35000,35000],[-1,1])
    return RawLine(*[int(ts)] + [m(float(i)) for i in axis])


def read_raw(f):
    return [parse_line(*l.split(',')) for l in open(f)]


def chunk(data, chunk_size):
    if not data:
        return []

    half_chunk = math.floor(chunk_size/2)
    prev_data = data[0:chunk_size]

    yield prev_data

    for i in range(chunk_size, len(data), half_chunk):
        chunk = prev_data[half_chunk:] + data[i:i+half_chunk]
        yield chunk
        prev_data = chunk
    

def segment_to_train_and_test(data, ratio):
    r = math.ceil(len(data) * ratio)

    return {
        'train': data[0:r],
        'test': data[r:],
    }


@click.command()
@click.option('-i', '--input-file', required=True)
@click.option('-o', '--output-dir', required=True)
@click.option('-c', '--chunk_size', default=30)
@click.option('-r', '--ratio', default=0.6)
def main(input_file, output_dir, chunk_size, ratio):
    segments = [
        Segment(1500983202, 1500983626, Classifications.DIRT_ROAD),
        Segment(1500983202, 1500983626, Classifications.DIRT_ROAD),

        Segment(1500981255, 1500981788, Classifications.GOOD_ROAD),
        Segment(1500981255, 1500981788, Classifications.GOOD_ROAD),

        Segment(1500980804, 1500980838, Classifications.STANDING),
        Segment(1500984890, 1500984900, Classifications.STANDING),
        Segment(1500980803, 1500980838, Classifications.STANDING),
        Segment(1500984890, 1500984900, Classifications.STANDING),
        
        Segment(1500980836, 1500981250, Classifications.BAD_ROAD),
        Segment(1500980836, 1500981250, Classifications.BAD_ROAD),
    ]

    raw_data = read_raw(input_file)

    try:
        for t in 'train', 'test':
            for axi in axis:
                os.unlink(os.path.join(output_dir, t, axi))

            os.unlink(os.path.join(output_dir, t, 'classification'))
    except FileNotFoundError:
        pass

    for s in segments:
        segment = read_segment(raw_data, s)
        train_test = segment_to_train_and_test(segment, ratio)
        for t, v in train_test.items():
            for c in chunk(v, chunk_size):
                if len(c) != chunk_size:
                    continue
                    
                for axi in axis:
                    line = ','.join([str(getattr(i, axi)) for i in c]) + '\n'
                    open(os.path.join(output_dir, t, axi), 'a').write(line)
            
                line = '{}\n'.format(s.classification)
                open(os.path.join(output_dir, t, 'classification'), 'a').write(line)
            


if __name__ == '__main__':
    main()
