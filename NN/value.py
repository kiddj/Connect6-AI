from keras.layers import Input, Conv2D, Dense, Activation, LeakyReLU, Reshape, Flatten, BatchNormalization, Dropout
from keras.optimizers import Adam
from keras.callbacks import ModelCheckpoint, TensorBoard, LambdaCallback
from keras.metrics import top_k_categorical_accuracy
from keras import Model
from keras.models import load_model
from keras.losses import binary_crossentropy, categorical_crossentropy
import keras.backend as K

import os

from config import *
import model
import utils


def build_value_head(outer_tensor=None):

    channel = 1
    kern = (1, 1)

    if outer_tensor is None:
        x = Input(shape=block_shape)
        out = model.build_resnet(x)
    else:
        x, out = outer_tensor

    out = Conv2D(channel, kern, padding='same')(out)
    out = BatchNormalization()(out)
    out = LeakyReLU()(out)

    out = Dropout(0.4)(out)

    out = Flatten()(out)
    out = Dense(256)(out)
    out = LeakyReLU()(out)

    out = Dense(1, activation='tanh')(out)

    m = Model(x, out, name='value')
    # m.summary()

    return m


def train_value(value, dataset, model_name, init=0):

    assert isinstance(value, Model)

    x, _, y = dataset
    p = np.random.permutation(len(x))
    x = x[p]
    y = y[p]

    y = np.reshape(y, (-1, 1))

    pos = int(split_ratio * len(x))
    x_train, x_test = x[:-pos], x[-pos:]
    y_train, y_test = y[:-pos], y[-pos:]

    print('Train Data: {} - {}'.format(x_train.shape, y_train.shape))
    print('Test Data: {} - {}'.format(x_test.shape, y_test.shape))

    model_dir = os.path.join(MODEL_PATH, model_name)
    log_dir = os.path.join(model_dir, 'logs')
    os.makedirs(model_dir, exist_ok=True)
    os.makedirs(log_dir, exist_ok=True)

    value.compile(optimizer=Adam(lr=learning_rate), loss='mse', metrics=['accuracy', 'mae'])

    fig, axes = utils.show_board_rate(value, x_test, y_test)

    callback_list = [
        ModelCheckpoint(os.path.join(model_dir, 'model.h5'), period=5),
        TensorBoard(log_dir=log_dir, batch_size=batch_size, histogram_freq=5),
        LambdaCallback(on_epoch_end=lambda epoch, logs: utils.show_board_rate(value, x_test, y_test, fig, axes)),
    ]

    value.fit(x_train, y_train, batch_size=batch_size, epochs=num_epoch, callbacks=callback_list,
               validation_data=(x_test, y_test), initial_epoch=init)


def main():
    pass

if __name__ == '__main__':
    main()
