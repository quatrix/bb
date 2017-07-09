from prepare import read_raw
import numpy as np
import tensorflow as tf
import click



@click.command()
@click.option('-i', '--input-file', required=True)
@click.option('-c', '--chunk-size', default=30)
@click.option('-u', '--undersample-by', default=4)
@click.option('-l', '--lowpass-threshold', default=100)
@click.option('-m', '--model', required=True)
def main(input_file, chunk_size, undersample_by, lowpass_threshold, model):
    data = read_raw(input_file, chunk_size, undersample_by, lowpass_threshold)

    with tf.Session() as sess:
        saver = tf.train.import_meta_graph(model + '.meta')
        saver.restore(sess,tf.train.latest_checkpoint('./'))

        graph = tf.get_default_graph()

        x = graph.get_tensor_by_name('x:0')
        y = graph.get_tensor_by_name('y:0')
        pred = graph.get_tensor_by_name('pred:0')

        argmax = tf.argmax(pred,1)
        
        for i in data:
            axis = np.array([np.transpose(i[2], (1, 0))])
            res = sess.run(
                argmax,
                feed_dict={
                    x: axis,
                }
            )

            print('{},{}'.format(i[0], res[0]))


if __name__ == '__main__':
    main()
