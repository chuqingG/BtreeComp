from pathlib import Path
import string
import random
random.seed(10)

ds_folder = '../../datasets/random_prefix_data/key_sizes'
prefix_count = 100
key_group_count = 10000

def generate_dataset(prefix_size, suffix_min_size ,suffix_max_size):
    prefixind = -1
    for ind in range(prefix_count * key_group_count):
        if ind % key_group_count == 0:
            prefixind += 1
        suffix_size = random.randint(suffix_min_size, suffix_max_size)
        chars = [random.choice(corpus) for i in range(suffix_size)]
        dataset.append(prefixes[prefixind] + ''.join(chars))
    size = sum(map(len, dataset))/float(len(dataset))
    random.shuffle(dataset)
    print(len(dataset))
    file_name = ds_folder + '/key_size_'+str(int(size))+'.txt'
    with open(file_name, 'w') as f:
        for key in dataset:
            f.write(f"{key}\n")


if __name__ == "__main__":
    corpus = list(string.ascii_lowercase)

    prefix_sizes = [10, 20, 30, 50]  
    suffix_min_size= 20
    suffix_max_size= 30
    for prefix_size in prefix_sizes:
        prefixes = list()
        dataset = list()
        for ind in range(prefix_count):
            chars = [random.choice(corpus) for i in range(prefix_size)]
            prefixes.append(''.join(chars))
        print(prefixes)
        generate_dataset(prefix_size, suffix_min_size, suffix_max_size)