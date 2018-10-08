from keras.models import load_model
from keras.utils import HDF5Matrix

import os

from config import *
import utils
import policy, value
import model


def main():
    init = 0

    use_h5 = True
    do_train = True
    do_load_model = init > 0
    force_create_data = True

    os.makedirs(DATA_PATH, exist_ok=True)
    data_dir = os.path.join(DATA_PATH, 'alphago_data')

    if not os.path.isdir(data_dir) or force_create_data:
        x, p, v = utils.create_dataset_alphago(GAME_PATHS)
        if use_h5:
            utils.data_augment(x, p, v, h5_path=H5_PATH)
        else:
            x, p, v = utils.data_augment(x, p, v)

        os.makedirs(data_dir, exist_ok=True)
        np.save(os.path.join(data_dir, 'board.npy'), x)
        np.save(os.path.join(data_dir, 'policy.npy'), p)
        np.save(os.path.join(data_dir, 'value.npy'), v)
        print('Successfully saved npy')

    if use_h5:
        dataset = H5_PATH
    else:
        x = np.load(os.path.join(data_dir, 'board.npy'))
        p = np.load(os.path.join(data_dir, 'policy.npy'))
        v = np.load(os.path.join(data_dir, 'value.npy'))

        print('\t*** Dataset ***')
        print(x.shape, p.shape, v.shape)

        dataset = (x, p, v)

    model_name = 'complex_1'

    if do_load_model:
        m = load_model(os.path.join(MODEL_PATH, model_name, 'model.h5'))
        print('*** Successfully loaded model. ***')
    else:
        m = model.build_complex()

    if do_train:
        model.train_complex(m, dataset, model_name, init=init)
        # policy.train_policy(p_model, dataset, model_name, init=init)
    else:
        model.test_complex(None, dataset)
        # model.test_complex(model_name, dataset)


if __name__ == '__main__':
    main()
