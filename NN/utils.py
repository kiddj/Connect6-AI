from keras.utils import to_categorical
import matplotlib
matplotlib.use('TkAgg')

import matplotlib.pyplot as plt
import h5py

import collections
import string
import copy
import random

from config import *


letters = string.ascii_lowercase
char_idx = dict([(c, i) for i, c in enumerate(letters)])
to_player = {1: "Black", -1: "White"}


""" Arithmetic functions """


def sigmoid(x):
    return 1 / (1 + np.exp(-x))


def border_function(x):
    x = x * 2 - 1
    return 0.5 - 1 / (np.exp(4 * x) + np.exp(-4 * x))


""" Condition functions """


def is_black(x):
    # Black: 1 / White: -1
    return 1 if x % 4 < 2 else -1


def is_black_turn(board):
    turn = get_board_turn(board)
    return is_black(turn + 1) > 0


""" Getter """


def get_board_turn(board):
    x = collections.Counter(list(board.flat))
    return x[1] + x[-1]


def get_one_hot_from_xy(x, y):
    # return one-hot of (x, y), or empty when one of pos is None.
    board = [[0 for _ in range(19)] for _ in range(19)]
    if x is None or y is None:
        return np.array(board)
    board[x][y] = 1
    return np.array(board)


def get_empty_board():
    board = [[0 for _ in range(19)] for _ in range(19)]
    return np.array(board)


""" Setter """


def to_one_hot(pred_board, reverse=False):
    sign = -1 if reverse else 1
    return to_categorical(np.argmax(sign * pred_board.reshape(-1, 361)), 361).reshape(-1, 19, 19, 1).astype('float32')


""" Create Dataset """


def read_game(filename, is_draw=False):
    board = [[0 for _ in range(19)] for _ in range(19)]
    game = []
    pos = []
    win = 0

    with open(os.path.join(GAME_PATH, filename), 'r') as f:
        lines = f.readlines()

        for idx, line in enumerate(lines):
            line = line.rstrip()
            if line[0] not in letters:
                continue
            c, r = line[0], int(line[1:])

            board[r - 1][char_idx[c]] = is_black(idx)
            game.append(copy.deepcopy(board))
            pos.append((r - 1, char_idx[c]))

        win = 0 if is_draw else is_black(len(game))

    return np.array(game), pos, win


def read_game_alphago(game_path, filename, is_draw=False):
    black = [[0 for _ in range(19)] for _ in range(19)]
    white = [[0 for _ in range(19)] for _ in range(19)]
    black_record = [black] * num_prev_board
    white_record = [white] * num_prev_board
    pos = []
    turn = 0

    with open(os.path.join(game_path, filename), 'r') as f:
        black = [[0 for _ in range(19)] for _ in range(19)]
        white = [[0 for _ in range(19)] for _ in range(19)]

        lines = f.readlines()

        for idx, line in enumerate(lines):
            line = line.rstrip()
            if line[0] not in letters:
                continue
            c, r = line[0], int(line[1:])
            x, y = r - 1, char_idx[c]
            pos.append([x, y])

            if is_black(idx) > 0:
                black[x][y] = 1
                black_record.append(copy.deepcopy(black))
                # white_record.append(copy.deepcopy(white_record[-1]))
            else:
                white[x][y] = 1
                white_record.append(copy.deepcopy(white))
                # black_record.append(copy.deepcopy(black_record[-1]))

            turn += 1

    win = -1 if is_black(turn) else 1
    win = 0 if is_draw else win

    return np.array(black_record), np.array(white_record), pos, win


def create_dataset(mode):

    avail_mode = ['policy', 'value']
    assert mode in avail_mode

    x = []
    y = []
    empty = np.array([get_empty_board()])

    for file in tqdm(os.listdir(GAME_PATH + DRAW_GAME_PATHS)):
        record, pos, win = read_game(file)
        record = np.concatenate((empty, record))
        pos.insert(0, None)

        for idx, board in enumerate(record):
            if mode == 'policy' and idx < len(record) - 1:
                sign = 1
                if not is_black_turn(board):
                    sign = -1
                p = 1 if win * sign > 0 else 0.6
                # invalid_pos = copy.deepcopy(board) - 0.2
                # np.place(invalid_pos, board > 0, -0.6)

                x.append(board * sign)
                y.append(p * get_one_hot_from_xy(*pos[idx + 1]))# + invalid_pos)

            if mode == 'value':
                x.append(board)
                y.append(win)

    x = np.expand_dims(np.array(x), axis=-1)
    y = np.expand_dims(np.array(y), axis=-1)

    return x, y


def create_dataset_alphago(game_paths):
    x = []
    p = []
    v = []

    for path in game_paths:
        draw = path in DRAW_GAME_PATHS

        for file in tqdm(os.listdir(path)):
            black, white, pos, win = read_game_alphago(path, file, is_draw=draw)

            b = 0
            w = 0

            for i in range(len(pos)):
                block = []
                b_turn = is_black(i + 1) > 0
                win_turn = is_black(i)

                block.append(np.ones((1,) + board_shape[:2]) if b_turn else np.zeros((1,) + board_shape[:2]))
                block.append(copy.deepcopy(black[b: b + num_prev_board]))
                block.append(copy.deepcopy(white[w: w + num_prev_board]))

                if b_turn:
                    label = copy.deepcopy(black[b + num_prev_board] - black[b + num_prev_board - 1])
                    b += 1
                else:
                    label = copy.deepcopy(white[w + num_prev_board] - white[w + num_prev_board - 1])
                    w += 1

                x.append(np.concatenate(block).transpose(1, 2, 0))
                p.append(np.array(label))
                if i == 0:
                    v.append(0)
                else:
                    p_win = win_turn * win
                    v.append(p_win)

    return np.array(x), np.array(p), np.array(v)


def data_augment(x, p, v, h5_path=None):

    x_aug = []
    p_aug = []
    v_aug = []

    x_T = np.flip(x, 2)
    p_T = np.flip(p, 2)

    if h5_path is not None:

        data_len = x.shape[0]

        data = h5py.File(h5_path, 'w')
        data.create_dataset('block', (data_len * 8,) + x.shape[1:], dtype='float32')
        data.create_dataset('policy', (data_len * 8,) + p.shape[1:], dtype='float32')
        data.create_dataset('value', (data_len * 8,) + v.shape[1:], dtype='float32')
        data.attrs['len'] = data_len

        for idx in tqdm(range(4)):
            i = 2 * idx
            j = i + 1
            data['block'][i * data_len: (i + 1) * data_len] = np.rot90(x, idx, (1, 2))
            data['block'][j * data_len: (j + 1) * data_len] = np.rot90(x, idx, (1, 2))
            data['policy'][i * data_len: (i + 1) * data_len] = np.rot90(p, idx, (1, 2))
            data['policy'][j * data_len: (j + 1) * data_len] = np.rot90(p, idx, (1, 2))
            data['value'][i * data_len: (i + 1) * data_len] = v
            data['value'][j * data_len: (j + 1) * data_len] = v

        p = np.random.permutation(data_len * 8)
        data['block'] = data['block'][p]
        data['policy'] = data['policy'][p]
        data['value'] = data['value'][p]
        data.close()

    else:
        for i in tqdm(range(4)):
            x_aug.append(np.rot90(x, i, (1, 2)))
            x_aug.append(np.rot90(x_T, i, (1, 2)))
            p_aug.append(np.rot90(p, i, (1, 2)))
            p_aug.append(np.rot90(p_T, i, (1, 2)))
            v_aug.append(v)
            v_aug.append(v)

        x_aug = np.array(x_aug).reshape(-1, *block_shape)
        p_aug = np.array(p_aug).reshape(-1, *board_shape)
        v_aug = np.array(v_aug).reshape(-1, 1)

    return x_aug, p_aug, v_aug


""" Visualization functions """


def print_board(board):
    board = board.reshape(19, 19)
    ans = []
    for i in range(19):
        tmp = []
        for j in range(19):
            if board[i, j] == 1:
                tmp.append('B')
            if board[i, j] == -1:
                tmp.append('W')
            if 0 < board[i, j] < 1:
                tmp.append('X')
            if board[i, j] == 0:
                tmp.append('.')
        ans.append(' '.join(tmp))
    print('\n'.join(ans) + '\n')


def draw_board(ax, board, color):
    board = board.reshape(19, 19)
    piece_list = []
    for i in range(19):
        for j in range(19):
            if board[i, j] > 0:
                if isinstance(color, tuple) and len(color) <= 4:
                    s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor=color, markeredgewidth=2)
                if color == 'black':
                    s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='k', markeredgewidth=2)
                if color == 'white':
                    s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='w', markeredgewidth=2)
                if color == 'grad':
                    mag = board[i, j]
                    s, = ax.plot(j, 18 - i, 'o', markersize=9, markeredgecolor=(0, 0, 0, mag), markerfacecolor=(1-mag, mag, 0, .5), markeredgewidth=2)
                piece_list.append(s)

    return piece_list


def show_board_rate(model, x_data, y_data, fig=None, axes=None, block=False, save_path=None):

    idx = np.random.randint(x_data.shape[0])
    x_data = x_data[idx].reshape(-1, *block_shape)
    y_data = y_data[idx]
    y_pred = model.predict(x_data) if model is not None else np.random.uniform(-1, 1)

    b_turn = np.all(x_data[..., 0] == 1)

    if fig is None or axes is None:
        fig, axes = plt.subplots(figsize=(5, 5))
    fig.patch.set_facecolor((1, 1, 0.8))

    for x in range(19):
        axes.plot([x, x], [0, 18], 'k')
    for y in range(19):
        axes.plot([0, 18], [y, y], 'k')

    axes.set_position([0.05, 0, 0.9, 0.9])
    axes.set_axis_off()
    axes.set_xlim(-1, 19)
    axes.set_ylim(-1, 19)
    axes.set_title('Current: {} & Win: {}\nWin Rate Pred.: {}'.format('Black' if b_turn else 'White', 'Black' if y_data > 0 else 'White', y_pred[0]))

    cur_black = x_data[..., num_prev_board]
    cur_white = x_data[..., 2 * num_prev_board]

    piece_list = []
    piece_list.extend(draw_board(axes, cur_black, color='black'))
    piece_list.extend(draw_board(axes, cur_white, color='white'))

    plt.draw()
    plt.show(block=block)
    plt.pause(2)

    for p in piece_list:
        p.remove()

    return fig, axes


def show_board_alphago(model, x_data, y_data, fig=None, axes=None, block=False, save_path=None):

    # def draw_board(ax, board, color):
    #     board = board.reshape(19, 19)
    #     piece_list = []
    #     for i in range(19):
    #         for j in range(19):
    #             if board[i, j] > 0:
    #                 if isinstance(color, tuple) and len(color) <= 4:
    #                     s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor=color, markeredgewidth=2)
    #                 if color == 'black':
    #                     s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='k', markeredgewidth=2)
    #                 if color == 'white':
    #                     s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='w', markeredgewidth=2)
    #                 if color == 'grad':
    #                     mag = board[i, j]
    #                     s, = ax.plot(j, 18 - i, 'o', markersize=9, markeredgecolor=(0, 0, 0, mag), markerfacecolor=(1-mag, mag, 0, .5), markeredgewidth=2)
    #                 piece_list.append(s)
    #
    #     return piece_list

    idx = np.random.randint(x_data.shape[0])
    x_data = x_data[idx].reshape(-1, *block_shape)
    y_data = y_data[idx].reshape(-1, *board_shape)
    y_pred = model.predict(x_data) if model is not None else np.random.uniform(0, 1, board_shape)

    b_turn = np.all(x_data[..., 0] == 1)

    if fig is None or axes is None:
        fig, axes = plt.subplots(ncols=3, figsize=(12, 5))
    fig.patch.set_facecolor((1, 1, 0.8))

    titles = ['Current', 'Expectation: {}'.format('Black' if b_turn else 'White'),
              'Prediction\n(Green is to win, Red otherwise)']
    for i, (ax, title) in enumerate(zip(axes, titles)):
        for x in range(19):
            ax.plot([x, x], [0, 18], 'k')
        for y in range(19):
            ax.plot([0, 18], [y, y], 'k')

        ax.set_position([i * 0.33, 0, 0.33, 0.84])
        ax.set_axis_off()
        ax.set_xlim(-1, 19)
        ax.set_ylim(-1, 19)
        ax.set_title(title)

    cur_black = x_data[..., num_prev_board]
    cur_white = x_data[..., 2 * num_prev_board]

    piece_list = []
    for ax in axes:
        piece_list.extend(draw_board(ax, cur_black, color='black'))
        piece_list.extend(draw_board(ax, cur_white, color='white'))

    piece_list.extend(draw_board(axes[1], y_data, color=(0, 1, 0, .5)))
    piece_list.extend(draw_board(axes[2], y_pred, color='grad'))

    plt.draw()
    plt.show(block=block)
    plt.pause(2)

    if save_path is not None:
        plt.savefig(save_path, facecolor=(1, 1, 0.8))

    for p in piece_list:
        p.remove()

    return fig, axes


def show_board_all(policy, value, x_data, p_data, v_data, fig=None, axes=None, block=False, save_path=None):

    idx = np.random.randint(x_data.shape[0])
    x_data = x_data[idx].reshape(-1, *block_shape)
    p_data = p_data[idx].reshape(-1, *board_shape)
    p_pred = policy.predict(x_data) if policy is not None else np.random.uniform(0, 1, board_shape)
    v_data = v_data[idx].reshape(-1, 1)
    v_pred = value.predict(x_data) if value is not None else np.random.uniform(-1, 1)

    b_turn = np.all(x_data[..., 0] == 1)

    if fig is None or axes is None:
        fig, axes = plt.subplots(ncols=3, figsize=(12, 5))
    fig.patch.set_facecolor((1, 1, 0.8))

    titles = ['<Current>\n{} turn'.format('BLACK' if b_turn else 'WHITE'),
              '<Expectation>\n{} won the game...'.format('BLACK' if v_data[0, 0] > 0 else 'WHITE'),
              '<Prediction>\n(Green is to win, Red otherwise)\nWin rate: {:.8f}'.format(v_pred[0, 0])]

    for i, (ax, title) in enumerate(zip(axes, titles)):
        for x in range(19):
            ax.plot([x, x], [0, 18], 'k')
        for y in range(19):
            ax.plot([0, 18], [y, y], 'k')

        ax.set_position([i * 0.33, 0, 0.33, 0.84])
        ax.set_axis_off()
        ax.set_xlim(-1, 19)
        ax.set_ylim(-1, 19)
        ax.set_title(title)

    cur_black = x_data[..., num_prev_board]
    cur_white = x_data[..., 2 * num_prev_board]

    piece_list = []
    for ax in axes:
        piece_list.extend(draw_board(ax, cur_black, color='black'))
        piece_list.extend(draw_board(ax, cur_white, color='white'))

    piece_list.extend(draw_board(axes[1], p_data, color=(0, 1, 0, .5)))
    piece_list.extend(draw_board(axes[2], p_pred, color='grad'))

    plt.draw()
    plt.show(block=block)
    plt.pause(2)

    if save_path is not None:
        plt.savefig(save_path, facecolor=(1, 1, 0.8))

    for p in piece_list:
        p.remove()

    return fig, axes


def show_board(model, x_data, y_data, fig=None, axes=None, block=False):

    # if x_data.ndim == 4 and x_data.shape[-1] == 5:
    #     x_data = x_data[..., 2] - x_data[..., 4]

    def place_piece(board, one_hot):
        new_board = copy.deepcopy(board)
        new_board += one_hot
        # np.place(new_board, board == 0, one_hot)
        # if not is_black_turn(board):
        #     new_board *= -1

        return new_board

    def draw_board(ax, board):
        piece_list = []
        for i in range(19):
            for j in range(19):
                if board[0, i, j, 0] >= 1:
                    s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='k', markeredgewidth=2)
                    piece_list.append(s)
                if board[0, i, j, 0] <= -1:
                    s, = ax.plot(j, 18 - i, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='w', markeredgewidth=2)
                    piece_list.append(s)
                # if board[0, i, j, 0] == 0.5:
                #     s, = ax.plot(i, j, 'o', markersize=10, markeredgecolor=(0, 0, 0), markerfacecolor='b', markeredgewidth=2)
                #     piece_list.append(s)
                if 0 < board[0, i, j, 0] < 1:
                    mag = np.clip(2 * np.power(board[0, i, j, 0], 0.1), 0., 1.)
                    s, = ax.plot(j, 18 - i, 'o', markersize=9, markeredgecolor=(0, 0, 0, .2), markerfacecolor=(1-mag, mag, 0, .5), markeredgewidth=2)
                    piece_list.append(s)

        return piece_list

    if fig is None or axes is None:
        fig, axes = plt.subplots(ncols=3, figsize=(12, 5))
    fig.patch.set_facecolor((1, 1, 0.8))

    # idx = np.random.randint(x_data.shape[0])
    idx = np.random.randint(20)
    board = x_data[idx: idx+1]
    b_turn = is_black_turn(board)
    sign = 1 if b_turn else -1

    pred_board = model.predict(board).reshape(-1, *board_shape) if model is not None else board
    next_board = place_piece(sign * board, 0.9 * to_one_hot(y_data[idx: idx+1]))

    # pred_board = place_piece(board, 0.99 * model.predict(board).reshape(-1, 19, 19, 1)) if model is not None else next_board
    # next_board = place_piece(board, 0.5 * to_one_hot(y_data[idx: idx + 1]))
    # pred_board = place_piece(board, 0.5 * to_one_hot(model.predict(board))) if model is not None else next_board

    # if not b_turn:
    #     board *= -1

    assert board.ndim == 4
    assert not np.all(board == next_board)

    titles = ['Current', 'Expectation: {}'.format('Black' if b_turn else 'White'), 'Prediction\n(Green is to win, Red otherwise)']
    for i, (ax, title) in enumerate(zip(axes, titles)):
        for x in range(19):
            ax.plot([x, x], [0, 18], 'k')
        for y in range(19):
            ax.plot([0, 18], [y, y], 'k')

        ax.set_position([i * 0.33, 0, 0.33, 0.84])
        ax.set_axis_off()
        ax.set_xlim(-1, 19)
        ax.set_ylim(-1, 19)
        ax.set_title(title)

    b1 = draw_board(axes[0], sign * board)
    b2 = draw_board(axes[1], next_board)
    b3 = draw_board(axes[2], pred_board)
    b3_1 = draw_board(axes[2], sign * board)

    plt.draw()
    plt.show(block=block)
    plt.pause(1)

    for b in b1+b2+b3+b3_1:
        b.remove()

    return fig, axes


def main():
    # record, pos, win = read_game('dfs_game_1')
    # for board in record:
    #     print(board)
    # print(to_player[win], "wins!")
    # print(get_board_turn(record[4]))

    force_create_data = True

    os.makedirs(DATA_PATH, exist_ok=True)
    data_dir = os.path.join(DATA_PATH, 'alphago_data')

    if not os.path.isdir(data_dir) or force_create_data:
        x, p, v = create_dataset_alphago(GAME_PATHS)

        os.makedirs(data_dir, exist_ok=True)
        np.save(os.path.join(data_dir, 'board.npy'), x)
        np.save(os.path.join(data_dir, 'policy.npy'), p)
        np.save(os.path.join(data_dir, 'value.npy'), v)
        print('Successfully saved npy')
    else:
        x = np.load(os.path.join(data_dir, 'board.npy'))
        p = np.load(os.path.join(data_dir, 'policy.npy'))
        v = np.load(os.path.join(data_dir, 'value.npy'))

    print(x.shape, p.shape, v.shape)

    for i in range(3):
        print('******')
        for j in range(5):
            print_board(x[i, ..., j])
        # print_board(x[i, ..., 0])
        # print_board(x[i, ..., 2])
        # print_board(x[i, ..., 4])
        # print_board(p[i])
        print(v[i])
        print()

    for i in range(4):
        show_board_all(None, None, x[i:i + 1], p[i:i + 1], v[i:i + 1], block=True)

    exit()

    if not os.path.isfile(data_dir):
        x, y = create_dataset('policy')
        print(x.shape, y.shape)

        np.save(data_dir, np.array([x, y]))
        print('Successfully saved npy')
    else:
        x, y = np.load(data_dir)

    x_, y_ = data_augment(x[4:5], y[4:5])
    for i in range(8):
        print_board(x_[i])
        print()
        print_board(y_[i])
        print()

    exit()

    for i in range(5):
        print(x[i].reshape(-1, 19, 19))
        print(y[i].reshape(-1, 19, 19))

        show_board(None, x[i:i+1], y[i:i+1], block=True)


if __name__ == '__main__':
    main()
