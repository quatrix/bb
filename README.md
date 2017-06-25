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

### Prior arts

#### DTW+kNN approach:

http://nbviewer.jupyter.org/github/markdregan/K-Nearest-Neighbors-with-Dynamic-Time-Warping/blob/master/K_Nearest_Neighbor_Dynamic_Time_Warping.ipynb

#### RNN approach:

https://github.com/guillaume-chevalier/LSTM-Human-Activity-Recognition/blob/master/LSTM.ipynb
