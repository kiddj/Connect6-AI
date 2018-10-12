from keras.layers import Input, Conv2D, Dense, Activation, LeakyReLU, Reshape, Flatten, BatchNormalization, Dropout
from keras.optimizers import Adam
from keras.callbacks import ModelCheckpoint, TensorBoard, LambdaCallback
from keras.metrics import top_k_categorical_accuracy
from keras import Model
from keras.models import load_model
from keras.losses import binary_crossentropy, categorical_crossentropy
import keras.backend as K

import matplotlib.pyplot as plt
import numpy as np
import h5py

# import datetime

from config import *
import utils
import model


def build_policy():

    channels = [16, 32, 64, 64]
    kern = (3, 3)

    x = Input(shape=block_shape)
    out = x

    for channel in channels:
        out = Conv2D(channel, kern, strides=(2, 2), padding='same')(out)
        out = Conv2D(channel, kern, padding='same')(out)
        out = BatchNormalization()(out)
        out = LeakyReLU()(out)

    # out = Dropout(0.3)(out)
    out = Flatten()(out)
    out = Dense(256)(out)
    out = LeakyReLU()(out)
    out = Dense(361, activation='softmax')(out)

    # out = Conv2D(1, kern, padding='same', activation='softmax')(out)
    # out = Reshape((361,))(out)

    model = Model(x, out, name='policy')
    # model.summary()

    return model


def build_policy_head(outer_tensor=None):

    channel = 2
    kern = (1, 1)

    if outer_tensor is None:
        x = Input(shape=block_shape)
        out = model.build_resnet(x)
    else:
        x, out = outer_tensor

    out = Conv2D(channel, kern, padding='same')(out)
    out = BatchNormalization()(out)
    out = LeakyReLU()(out)

    out = Flatten()(out)
    out = Dense(361, activation='softmax')(out)

    m = Model(x, out, name='policy')
    m.summary()

    return m


def train_policy(policy, dataset, model_name, init=0):

    assert isinstance(policy, Model)

    x, y, _ = dataset
    p = np.random.permutation(len(x))
    x = x[p]
    y = y[p]

    y = np.reshape(y, (-1, 361))

    pos = int(split_ratio * len(x))
    x_train, x_test = x[:-pos], x[-pos:]
    y_train, y_test = y[:-pos], y[-pos:]

    print('Train Data: {} - {}'.format(x_train.shape, y_train.shape))
    print('Test Data: {} - {}'.format(x_test.shape, y_test.shape))

    model_dir = os.path.join(MODEL_PATH, model_name)
    log_dir = os.path.join(model_dir, 'logs')
    os.makedirs(model_dir, exist_ok=True)
    os.makedirs(log_dir, exist_ok=True)

    def complex_crossentropy(y_true, y_pred):
        def one_crossentropy(y_true, y_pred):
            return -1 * K.mean((1 - y_true) * K.log(1 - y_pred))
        return 0.5 * (one_crossentropy(y_true, y_pred) + categorical_crossentropy(y_true, y_pred))

    policy.compile(optimizer=Adam(lr=learning_rate), loss='categorical_crossentropy', metrics=['accuracy', 'categorical_crossentropy', top_k_categorical_accuracy])

    fig, axes = utils.show_board_alphago(policy, x_test, y_test)

    callback_list = [
        ModelCheckpoint(os.path.join(model_dir, 'model.h5'), period=5),
        TensorBoard(log_dir=log_dir, batch_size=batch_size, histogram_freq=5),
        LambdaCallback(on_epoch_end=lambda epoch, logs: model.save_model(policy, model_dir, epoch)),
        LambdaCallback(on_epoch_end=lambda epoch, logs: utils.show_board_alphago(policy, x_test, y_test, fig, axes)),
    ]

    policy.fit(x_train, y_train, batch_size=batch_size, epochs=num_epoch, callbacks=callback_list,
               validation_data=(x_test, y_test), initial_epoch=init)


def test_policy(model_name, dataset):

    os.makedirs(TEST_RESULT_PATH, exist_ok=True)

    x, y, _ = dataset
    p = np.random.permutation(len(x))
    x = x[p]
    y = y[p]

    y = np.reshape(y, (-1, 361))

    pos = int(split_ratio * len(x))
    x_train, x_test = x[:-pos], x[-pos:]
    y_train, y_test = y[:-pos], y[-pos:]

    print('Test Data: {} - {}'.format(x_test.shape, y_test.shape))

    policy = load_model(os.path.join(MODEL_PATH, model_name, 'model.h5'))

    print('*** Successfully loaded model. ***')

    fig = axes = None
    i = 0
    while True:
        idx = np.random.choice(np.arange(x_test.shape[0]), 10)
        fig, axes = utils.show_board_alphago(policy, x_test[idx], y_test[idx], fig, axes,
                                             save_path=os.path.join(TEST_RESULT_PATH, '{:04}.png'.format(i)))
        i += 1


def main():

    do_train = True

    policy = build_policy_head()

    os.makedirs(DATA_PATH, exist_ok=True)
    data_dir = os.path.join(DATA_PATH, 'data_cached.npy')

    if not os.path.isfile(data_dir):
        dataset = utils.create_dataset('policy')
        dataset = utils.data_augment(*dataset)

        np.save(data_dir, np.array(dataset))
        print('*** Successfully saved dataset. ***')
    else:
        dataset = np.load(data_dir)

    print('\t*** Dataset ***')

    model_name = 'model_3'
    if do_train:
        train_policy(policy, dataset, model_name)
    else:
        test_policy(model_name, dataset)


if __name__ == '__main__':
    main()
