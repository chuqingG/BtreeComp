import csv
import argparse
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

def read_file_to_dict(file_name, key, val):
    infile = csv.DictReader(open(file_name))
    key_val_dict = {}
    for row in infile:
        key_val_dict[row[key]] = float(row[val])
    return key_val_dict

def plot_graph(times_dict, sizes_dict, output_file):
    x = list(sizes_dict.values())
    y = list(times_dict.values())
    #7 markers and colors based on 8 algorithms
    markers = ["." , "o" , "^" , "s" , "+" , "x", "D", "p"]
    colors = ['red','blue','green','yellow','black', 'cyan', 'purple', 'brown']
    for i , label in enumerate(times_dict):
        plt.scatter(x[i], y[i], label=label, marker = markers[i], color = colors[i])
    plt.xlabel("Key size (bytes)", fontsize = 'large')
    plt.ylabel("Time (s)", fontsize = 'large')
    plt.ylim(ymin=0, ymax= max(y) + 5)
    plt.xlim(xmin=0, xmax = max(x) + 5)
    plt.legend(loc = 'upper center', fontsize = 'large', ncol = 2)
    plt.savefig(output_file)
    plt.close()


if __name__ == "__main__":
    argParser = argparse.ArgumentParser()
    argParser.add_argument("-i", "--input", help="Path to input files")
    args = argParser.parse_args()
    files_path = args.input
    times_search_dict = read_file_to_dict(files_path+"/performance_search.txt", "name" , "avg")
    times_range_dict = read_file_to_dict(files_path+"/performance_range.txt", "name" , "avg")
    sizes_dict = read_file_to_dict(files_path+"/statistics.txt", "name" , "keysize")
    print(times_search_dict)
    print(times_range_dict)
    print(sizes_dict)
    plot_graph(times_search_dict, sizes_dict, "skyline_search.png")
    plot_graph(times_range_dict, sizes_dict, "skyline_range.png")
