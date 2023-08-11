import csv
import argparse
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
from collections import defaultdict


def read_file_to_dict(file_name, key, val):
    infile = csv.DictReader(open(file_name))
    key_val_dict = {}
    for row in infile:
        key_val_dict[row[key]] = float(row[val])
    return key_val_dict

def plot_graph(times_dict, sizes_dict, output_file):
    fig, ax = plt.subplots()
    markers = ["." , "o" , "^" , "s" , "+" , "x", "D", "p"]
    colors = ['red','blue','green','black','yellow', 'cyan', 'purple', 'brown']
    for ind, label in enumerate(times_dict):
        print(label)
        x = sizes_dict[label]
        y = times_dict[label]
        print(x,y)
        #7 markers and colors based on 8 algorithms
        ax.plot(x, y, label=label, color=colors[ind], marker=markers[ind])
        ax.set_xlabel("Key size (bytes)", fontsize = 'large')
        ax.set_ylabel("Time (s)", fontsize = 'large')
    
    ax.set_xlim(xmin=0)
    ax.legend(loc = 'upper center', fontsize = 'large', ncol = 2)
    plt.savefig(output_file)
    plt.close()

def plot_bar_graph(sizes_dict, output_file):
    markers = ["." , "o" , "^" , "s" , "+" , "x", "D", "p"]
    colors = ['red','blue','green','black','yellow', 'cyan', 'purple', 'brown']
    x = np.array([10,20,30,50])
    y1 = np.array(sizes_dict['BPTree-std'])
    y2 = np.array(sizes_dict['BPTree-headtail'])
    y3 = np.array(sizes_dict['BPTree-WT'])
    y4 = np.array(sizes_dict['BPTree-MyISAM'])
    # Create the first bar graph
    fig, ax = plt.subplots()
    width = 0.75
    ax.bar(x - width/2, y1, width, align='center', label='BPTree-std')
    ax.bar(x + width/2, y2, width, align='center', label='BPTree-headtail')
    ax.bar(x + width, y3, width, align='center', label='BPTree-WT')
    ax.bar(x + 3*width/2, y4, width, align='center', label='BPTree-MyISAM')

    # Set the x-ticks and labels
    ax.set_xticks(x)
    ax.set_xlabel("Prefix Length (in B)", fontsize = 'large')
    ax.set_ylabel("Key Length (in B)", fontsize = 'large')

    # Set the title and legend
    ax.set_title('Key size Prefix Length increases')
    ax.legend()
    plt.savefig(output_file)
    plt.close()


if __name__ == "__main__":
    argParser = argparse.ArgumentParser()
    argParser.add_argument("-i", "--input", help="Path to input files")
    args = argParser.parse_args()
    input_paths = args.input.split(",")
    sizes_dict_all = defaultdict(list)
    for ind in range(len(input_paths)):
        sizes_dict = read_file_to_dict(input_paths[ind]+"/statistics.txt", "name" , "keysize")
        for key, value in sizes_dict.items():
            sizes_dict_all[key].append(value)
    plot_bar_graph(sizes_dict_all, "bar_graph_key_size.png")