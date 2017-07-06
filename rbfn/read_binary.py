import click
from struct import unpack

@click.command()
@click.option('-i', '--input-file', required=True)
@click.option('-c', '--chunk-size', required=True)
def main(input_file, chunk_size):
    res = []

    with open(input_file, 'rb') as f:
        acc = []

        while True:
            data = f.read(6)

            if not data:
                res.append(acc)
                break

            acc.append(unpack('hhh', data))

            if len(acc) > chunk_size:
                res.append(acc)
                acc = []

    for i in res:
        print(len(i))

            
if __name__ == '__main__':
    main()
