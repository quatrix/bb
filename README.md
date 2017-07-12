## Blackbox

Use Arduino 101's 6-axis accelerometer/gyro + machine learning to figure out road conditions

### Hardware
* Arduino 101
* SD Card reader/writer
* RealTimeClock
* LCD display
* EM506 GPS Receiver Module
* Arduino Uno (used as GPS proxy)

### installation
```bash
cd ml
virtualenv --system-site-packages -p python3 .pyenv
source ./pyenv/bin/activate
pip install -r requirements.txt
```

### preparing data
```bash
cd rnn
. ./pyenv/bin/activate
mkdir -p processed/{test/train}
python prepare.py -i ../raw_data/new_format/17_6_30.CSV -i ../raw_data/new_format/17_7_1.CSV -i ../raw_data/new_format/17_7_2.CSV -i ../raw_data/new_format/17_7_3.CSV -o processed -c 200 -u 2 -l 100 -r 0.6

```

### training a model
```bash
cd rnn
. ./pyenv/bin/activate
python train_rnn.py processed model_666.ckpf
```

### classifying a csv
```
cd rnn
. ./pyenv/bin/activate
python classify_em.py -i ../raw_data/new_format/17_7_3.CSV -c 200 -u 2 -l 100  -m model_666.ckpf
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
