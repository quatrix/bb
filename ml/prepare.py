#!/usr/bin/env python

import click
import os
import math
from consts import Classifications, axis, Segment, RawLine


def read_segment(raw_data, segment):
    return [l for l in raw_data if l.ts >= segment.start and l.ts <= segment.end]


def parse_line(ts, *axis):
    return RawLine(*[int(ts)] + [float(i) for i in axis])


def read_raw(f):
    return [parse_line(*l.split(',')) for l in open(f)]


def chunk(data, chunk_size):
    prev_data = [RawLine(0, 0, 0, 0, 0, 0, 0) for _ in range(chunk_size)]

    for i in range(0, len(data), chunk_size):
        yield prev_data[math.floor(chunk_size/2):] + data[i:i+chunk_size]
        prev_data = data[i:i+chunk_size]
    

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

        Segment(1500980803, 1500980838, Classifications.STANDING),
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

        res = {
            'train': segment[0:math.floor(len(segment) * ratio)],
            'test': segment[math.floor(len(segment) * ratio):],
        }

        for t, v in res.items():
            for c in chunk(v, chunk_size):
                if len(c) != (chunk_size + math.floor(chunk_size/2)):
                    continue
                    
                for axi in axis:
                    line = ','.join([str(getattr(i, axi)) for i in c]) + '\n'
                    open(os.path.join(output_dir, t, axi), 'a').write(line)
            
                line = '{}\n'.format(s.classification)
                open(os.path.join(output_dir, t, 'classification'), 'a').write(line)
            


if __name__ == '__main__':
    main()
