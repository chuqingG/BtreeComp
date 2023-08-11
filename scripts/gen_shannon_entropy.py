from pathlib import Path
import string
import random
random.seed(10)


alphabet_size = 8
num_keys = 10000000
ds_folder = '../../datasets/shannon_entropy_datasets'

corpus = [c for c in string.printable[:alphabet_size]]
if '\n' in corpus:
    corpus.remove('\n')
if '\r' in corpus:
    corpus.remove('\r')

bytes_sizes = [8, 12, 20, 28, 36]

for byte_size in bytes_sizes:
    # Using list to preserve order for reproducibility
    dataset = list()
    keys = set()
    while len(dataset) < num_keys:
        chars = [random.choice(corpus) for j in range(byte_size)]
        key = ''.join(chars)
        if key not in keys:
            dataset.append(key)
            keys.add(key)
    print(len(dataset))
    subfolder = ds_folder+'/' + str(alphabet_size)
    Path(subfolder).mkdir(parents=True, exist_ok=True)
    file_name = subfolder + '/key_size_'+str(byte_size)+'.txt'
    with open(file_name, 'w') as f:
        for key in dataset:
            f.write(f"{key}\n")