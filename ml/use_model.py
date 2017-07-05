import tensorflow as tf
from utils import load_X, load_Y, LSTM_RNN, one_hot

from consts import axis

import click
import os


@click.command()
@click.option('-m', '--model', required=True)
@click.option('-t', '--test-dir', required=True)
def main(model, test_dir):
    X_test_signals_paths = [os.path.join(test_dir, axi) for axi in axis]
    X_test = load_X(X_test_signals_paths)

    y_test_path = os.path.join(test_dir, 'classification')
    y_test = load_Y(y_test_path)
    
    n_classes = 5 # Total classes (should go up, or should go down)

    with tf.Session() as sess:
        saver = tf.train.import_meta_graph(model + '.meta')
        saver.restore(sess,tf.train.latest_checkpoint('./'))

        graph = tf.get_default_graph()

        x = graph.get_tensor_by_name('x:0')
        y = graph.get_tensor_by_name('y:0')
        pred = graph.get_tensor_by_name('pred:0')

        argmax = tf.argmax(pred,1)
        correct_pred = tf.equal(tf.argmax(pred,1), tf.argmax(y,1))
        accuracy = tf.reduce_mean(tf.cast(correct_pred, tf.float32))

        actual, expected = sess.run(
            [argmax, correct_pred], 
            feed_dict={
                x: X_test,
                y: one_hot(y_test)
            }
        )

        for r in zip(actual, expected):
            print(r)


if __name__ == '__main__':
    main()
