## Blackbox

Use Arduino 101's 6-axis accelerometer/gyro + machine learning to figure out road conditions

### Hardware
* Arduino 101
* SD Card reader/writer
* RealTimeClock
* LCD display

### installation
```bash
cd ml
virtualenv --system-site-packages -p python3 .pyenv
source ./pyenv/bin/activate
pip install -r requirements.txt
```

### preparing data
```bash
cd ml
. ./pyenv/bin/activate
mkdir -p processed/{test/train}
python prepare.py -i ../raw_data/17_7_25.CSV -o processed
```

### building a testing a model
```bash
cd ml
. ./pyenv/bin/activate
python main.py processed
```

### running tests
```bash
cd ml
. ./pyenv/bin/activate
pytest -vv
```

### Prior arts

#### DTW+kNN approach:

http://nbviewer.jupyter.org/github/markdregan/K-Nearest-Neighbors-with-Dynamic-Time-Warping/blob/master/K_Nearest_Neighbor_Dynamic_Time_Warping.ipynb

#### RNN approach:

https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition/blob/master/LSTM.ipynb
