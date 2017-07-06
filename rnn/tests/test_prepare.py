import sys, os
import numpy as np
myPath = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, myPath + '/../')

from prepare import chunk, undersample, low_pass_filter
from functools import reduce
import random

def test_chunk_empty_list():
    assert list(chunk([], 5)) == []


def test_chunk_size_bigger_than_data():
    assert (np.array(list(chunk([1, 2, 3], 10))) == [[1,2,3]]).all()


def test_chunk_happy_path():
    data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    expected_results = [
        [1, 2, 3, 4],
        [3, 4, 5, 6],
        [5, 6, 7, 8],
        [7, 8, 9, 10],
    ]

    assert (np.array(list(chunk(data, 4))) == expected_results).all()



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

    assert (np.array(list(chunk(data, 3))) == expected_results).all()


def test_chunk_happy_path_not_even_data():
    data = [1, 2, 3, 4, 5, 6, 7]
    expected_results = [
        [1, 2, 3, 4],
        [3, 4, 5, 6],
        [5, 6, 7]
    ]

    assert list(map(list, list(chunk(data, 4)))) == expected_results


def test_understample():
    data = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 500]
    expected = [(10+20+30)/3, (40+50+60)/3, (70+80+90)/3, (100+500)/2]
    assert undersample(data, 3) == expected


def test_lowpass():
    data = [1,2,3,4,5,6,7,8,9, -20, -40]
    expected = [5,6,7,8,9, -20, -40]
    assert (low_pass_filter(data, 4) == expected).all()
