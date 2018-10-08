import numpy as np
from tqdm import tqdm

import os

ROOT_PATH = os.getcwd()
GAME_PATH = os.path.join(ROOT_PATH, 'games')
GAME_PATHS = [
    # os.path.join(ROOT_PATH, 'games'),
    # os.path.join(ROOT_PATH, 'games_10-10'),
    os.path.join(ROOT_PATH, 'games3_10-10'),
]
MODEL_PATH = os.path.join(ROOT_PATH, 'models')
DATA_PATH = os.path.join(ROOT_PATH, 'data')
TEST_RESULT_PATH = os.path.join(ROOT_PATH, 'results')
H5_PATH = os.path.join(DATA_PATH, 'data.h5')

os.makedirs(MODEL_PATH, exist_ok=True)

board_shape = (19, 19, 1)
block_shape = (19, 19, 5)

learning_rate = 1e-4
batch_size = 64
num_epoch = 300

split_ratio = 0.1
