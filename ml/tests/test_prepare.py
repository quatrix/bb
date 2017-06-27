import sys, os
myPath = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, myPath + '/../')

from prepare import chunk, read_segment, RawLine, Segment, Classifications, segment_to_train_and_test
from functools import reduce
import random

def test_chunk_empty_list():
    assert list(chunk([], 5)) == []


def test_chunk_size_bigger_than_data():
    assert list(chunk([1, 2, 3], 10)) == [[1,2,3]]


def test_chunk_happy_path():
    data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    expected_results = [
        [1, 2, 3, 4],
        [3, 4, 5, 6],
        [5, 6, 7, 8],
        [7, 8, 9, 10],
    ]

    assert list(chunk(data, 4)) == expected_results


def test_chunk_happy_path_not_even_data():
    data = [1, 2, 3, 4, 5, 6, 7]
    expected_results = [
        [1, 2, 3, 4],
        [3, 4, 5, 6],
        [5, 6, 7],
    ]

    assert list(chunk(data, 4)) == expected_results

def test_chunk_happy_path_not_even_chunk_size():
    data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    expected_results = [
        [1, 2, 3],
        [2, 3, 4],
        [3, 4, 5],
        [4, 5, 6],
        [5, 6, 7],
        [6, 7, 8],
        [7, 8, 9],
        [8, 9, 10],
    ]

    assert list(chunk(data, 3)) == expected_results



def test_read_semgne():
    segments = [
        Segment(666, 667, Classifications.BAD_ROAD),
        Segment(668, 669, Classifications.BAD_ROAD),
        Segment(670, 671, Classifications.BAD_ROAD),
        Segment(672, 673, Classifications.BAD_ROAD),
    ]

    expected = [
        [
            RawLine(666, 0, 0, 0, 0, 0, 0),
            RawLine(666, 0, 0, 0, 0, 0, 0),
            RawLine(667, 0, 0, 0, 0, 0, 0),
            RawLine(667, 0, 0, 0, 0, 0, 0),
        ],
        [
            RawLine(668, 0, 0, 0, 0, 0, 0),
            RawLine(668, 0, 0, 0, 0, 0, 0),
            RawLine(669, 0, 0, 0, 0, 0, 0),
        ],
        [
            RawLine(670, 0, 0, 0, 0, 0, 0),
            RawLine(671, 0, 0, 0, 0, 0, 0),
            RawLine(671, 0, 0, 0, 0, 0, 0),
        ],
        [
            RawLine(672, 0, 0, 0, 0, 0, 0),
            RawLine(672, 0, 0, 0, 0, 0, 0),
            RawLine(673, 0, 0, 0, 0, 0, 0),
        ],
    ]

    def reducer(acc, i):
        acc += i
        return acc

    raw_data = reduce(reducer, expected, [])

    for i, segment in enumerate(segments):
        assert read_segment(raw_data, segment) == expected[i]


def test_segment_to_train_and_test_in_half():
    data = [1,2,3,4,5,6,7,8,9, 10]
    segmented = segment_to_train_and_test(data, 0.5)

    assert segmented['train'] == [1,2,3,4,5]
    assert segmented['test'] == [6,7,8,9,10]


def test_segment_to_train_and_test_30_70():
    data = [1,2,3,4,5,6,7,8,9,10,11]
    segmented = segment_to_train_and_test(data, 0.7)

    assert segmented['train'] == [1,2,3,4,5,6,7,8]
    assert segmented['test'] == [9,10,11]
