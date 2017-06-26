from collections import namedtuple

labels = [
    "DIRT_ROAD",
    "GOOD_ROAD",
    "BAD_ROAD",
    "STANDING",
]

class Classifications(object):
    DIRT_ROAD = 1
    GOOD_ROAD = 2
    BAD_ROAD = 3
    STANDING = 4

axis = ['ax', 'ay', 'az', 'gx', 'gy', 'gz']
Segment = namedtuple('Segment', ['start', 'end', 'classification'])
RawLine = namedtuple('RawLine', ['ts'] + axis)
Sample = namedtuple('Sample', axis + ['classification'])
