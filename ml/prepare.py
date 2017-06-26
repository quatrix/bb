#!/usr/bin/env python

import click
import os
import math
from consts import Classifications, axis, Segment, RawLine, Sample


def read_segment(raw_data, segment):
    return [enrich(l, segment) for l in raw_data if l.ts >= segment.start and l.ts <= segment.end]


def enrich(l, segment):
    return Sample(l.ax, l.ay, l.az, l.gx, l.gy, l.gz, segment.classification)


def parse_line(ts, *axis):
    return RawLine(*[int(ts)] + [float(i) for i in axis])


def read_raw(f):
    return [parse_line(*l.split(',')) for l in open(f)]


def chunk(data, chunk_size):
    for i in range(0, len(data), chunk_size):
        yield data[i:i+chunk_size]
    

@click.command()
@click.option('-i', '--input-file', required=True)
@click.option('-o', '--output-dir', required=True)
@click.option('-c', '--chunk_size', default=30)
def main(input_file, output_dir, chunk_size):
    segments = [
        Segment(1500980803, 1500980838, Classifications.STANDING),
        Segment(1500984890, 1500984919, Classifications.STANDING),
        Segment(1500980838, 1500981255, Classifications.BAD_ROAD),
        Segment(1500981255, 1500981700, Classifications.GOOD_ROAD),
        Segment(1500981955, 1500982120, Classifications.BAD_ROAD),
        Segment(1500982944, 1500983626, Classifications.DIRT_ROAD),
        Segment(1500983688, 1500983851, Classifications.GOOD_ROAD),
        Segment(1500985370, 1500986266, Classifications.BAD_ROAD),
    ]

    raw_data = read_raw(input_file)


    for t in 'train', 'test':
        for axi in axis:
            os.unlink(os.path.join(output_dir, t, axi))

        os.unlink(os.path.join(output_dir, t, 'classification'))

    for s in segments:
        segment = read_segment(raw_data, s)
        res = {
            'train': segment[0:math.floor(len(segment) * 0.7)],
            'test': segment[math.floor(len(segment) * 0.7):],
        }

        for t, v in res.items():
            for c in chunk(v, chunk_size):
                if len(c) < chunk_size:
                    continue
                    
                for axi in axis:
                    line = ','.join([str(getattr(i, axi)) for i in c]) + '\n'
                    open(os.path.join(output_dir, t, axi), 'a').write(line)
            
                line = '{}\n'.format(s.classification)
                open(os.path.join(output_dir, t, 'classification'), 'a').write(line)
            


if __name__ == '__main__':
    main()
