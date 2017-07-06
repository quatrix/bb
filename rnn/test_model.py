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
    Y_test_path = os.path.join(test_dir, 'classification')

    X_test = load_X(X_test_signals_paths)
    Y_test = load_Y(Y_test_path)

    n_hidden = 32
    n_classes = 5

    lambda_loss_amount = 0.0015
    learning_rate = 0.0025

    n_steps = len(X_test[0])
    n_input = len(X_test[0][0])

    print('n_steps: {} n_input: {}'.format(n_steps, n_input))
    x = tf.placeholder(tf.float32, [None, n_steps, n_input])
    y = tf.placeholder(tf.float32, [None, n_classes])

    weights = {
        'hidden': tf.Variable(tf.random_normal([n_input, n_hidden])), # Hidden layer weights
        'out': tf.Variable(tf.random_normal([n_hidden, n_classes], mean=1.0))
    }
    biases = {
        'hidden': tf.Variable(tf.random_normal([n_hidden])),
        'out': tf.Variable(tf.random_normal([n_classes]))
    }

    pred = LSTM_RNN(n_input, n_steps, n_hidden, x, weights, biases)
    correct_pred = tf.equal(tf.argmax(pred,1), tf.argmax(y,1))
    accuracy = tf.reduce_mean(tf.cast(correct_pred, tf.float32))

    l2 = lambda_loss_amount * sum(
        tf.nn.l2_loss(tf_var) for tf_var in tf.trainable_variables()
    ) # L2 loss prevents this overkill neural network to overfit the data
    cost = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=y, logits=pred)) + l2 # Softmax loss
    optimizer = tf.train.AdamOptimizer(learning_rate=learning_rate).minimize(cost) # Adam Optimizer

    cost = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(labels=y, logits=pred)) + l2 # Softmax loss
    saver = tf.train.Saver()


    with tf.Session() as sess:
        saver.restore(sess, model)
        print('Model restored')
    
        loss, acc = sess.run(
            [cost, accuracy], 
            feed_dict={
                x: X_test,
                y: one_hot(Y_test)
            }
        )

        print("PERFORMANCE ON TEST SET: " + \
              "Batch Loss = {}".format(loss) + \
              ", Accuracy = {}".format(acc))



if __name__ == '__main__':
    main()
