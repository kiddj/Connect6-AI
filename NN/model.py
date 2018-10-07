from keras.layers import Input, Conv2D, Dense, Activation, LeakyReLU, Reshape, Flatten, BatchNormalization, Dropout, Concatenate
from keras.optimizers import Adam
from keras.callbacks import ModelCheckpoint, TensorBoard, LambdaCallback
from keras.metrics import top_k_categorical_accuracy
from keras import Model
from keras.models import load_model
from keras.losses import binary_crossentropy, categorical_crossentropy
import keras.backend as K

import numpy as np
import h5py

from config import *
import utils
import policy, value


def build_resnet(x):

    channel = 32
    kern = (3, 3)

    def _conv_block(_x):
        out = _x
        out = Conv2D(channel, kern, padding='same')(out)
        out = BatchNormalization()(out)
        out = LeakyReLU()(out)

        return out

    def _res_block(_x):
        out = _x
        out = Conv2D(channel, kern, padding='same')(out)
        out = BatchNormalization()(out)
        out = LeakyReLU()(out)
        out = Conv2D(channel, kern, padding='same')(out)
        out = BatchNormalization()(out)
        out = Concatenate()([x, out])
        out = LeakyReLU()(out)

        return out

    # x = Input(shape=board_shape)
    out = x

    out = _conv_block(out)

    for _ in range(10):
        out = _res_block(out)

    return out


def build_complex():

    x = Input(shape=block_shape, name='input_block')

    res = build_resnet(x)

    p = policy.build_policy_head([x, res])
    v = value.build_value_head([x, res])

    m = Model(x, [p(x), v(x)], name='policy_value')
    m.summary()

    return m


def train_complex(model, dataset, model_name, init=0):

    assert isinstance(model, Model)

    policy = model.layers[1]
    value = model.layers[2]

    assert isinstance(policy, Model)
    assert isinstance(value, Model)

    x, p, v = dataset
    # permu = np.random.permutation(len(x))
    # x = x[permu]
    # p = p[permu]
    # v = v[permu]

    p = np.reshape(p, (-1, 361))

    pos = int(split_ratio * len(x))
    x_train, x_test = x[:-pos], x[-pos:]
    p_train, p_test = p[:-pos], p[-pos:]
    v_train, v_test = v[:-pos], v[-pos:]

    print('Train Data: {} - {} - {}'.format(x_train.shape, p_train.shape, v_train.shape))
    print('Test Data: {} - {} - {}'.format(x_test.shape, p_test.shape, v_test.shape))
    # exit()

    model_dir = os.path.join(MODEL_PATH, model_name)
    log_dir = os.path.join(model_dir, 'logs')
    os.makedirs(model_dir, exist_ok=True)
    os.makedirs(log_dir, exist_ok=True)

    model.compile(optimizer=Adam(lr=learning_rate), loss=['categorical_crossentropy', 'mse'], metrics=['accuracy'])

    fig, axes = utils.show_board_all(policy, value, x_test, p_test, v_test)

    def batch_generator(x_data, p_data, v_data):
        while True:
            permu = np.random.permutation(x_data.shape[0])
            x_data = x_data[permu]
            p_data = p_data[permu]
            v_data = v_data[permu]
            for i in range(x_data.shape[0] // batch_size):
                x_batch = x_data[i*batch_size: (i+1)*batch_size]
                p_batch = p_data[i*batch_size: (i+1)*batch_size]
                v_batch = v_data[i*batch_size: (i+1)*batch_size]

                yield x_batch, [p_batch, v_batch]

    callback_list = [
        ModelCheckpoint(os.path.join(model_dir, 'model.h5'), period=5),
        TensorBoard(log_dir=log_dir, batch_size=batch_size),
        LambdaCallback(on_epoch_end=lambda epoch, logs: utils.show_board_all(policy, value, x_test, p_test, v_test, fig, axes)),
    ]
    model.fit_generator(batch_generator(x_train, p_train, v_train), steps_per_epoch=x_train.shape[0] // batch_size,
                        epochs=num_epoch, callbacks=callback_list, initial_epoch=init,
                        validation_data=batch_generator(x_test, p_test, v_test), validation_steps=1)
    # policy.fit(x_train, y_train, batch_size=batch_size, epochs=num_epoch, callbacks=callback_list,
    #            validation_data=(x_test, y_test), initial_epoch=init)


def main():
    pass


if __name__ == '__main__':
    main()
