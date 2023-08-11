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

def flatten(lst):
    return [item for sublist in lst for item in sublist]

def plot_graph(x_list, y_dict, output_file):
    x = x_list
    y = list(y_dict.values())
    #7 markers and colors based on 8 algorithms
    markers = ["." , "o" , "^" , "s" , "+" , "x", "D", "p"]
    colors = ['red','blue','green','yellow','black', 'cyan', 'purple', 'brown']    
    for i , label in enumerate(y_dict):
        plt.plot(x_list, y_dict[label],label=label, color = colors[i],marker=markers[i])
    plt.xticks(x_list)
    plt.xlabel("Node size (bytes)", fontsize = 'large')
    plt.ylabel("Search Time (s)", fontsize = 'large')
    plt.ylim(ymin=0, ymax= max(flatten(y)) + 10)
    plt.xlim(xmin=0, xmax = max(x) + 10)
    plt.legend(bbox_to_anchor=(0.5, 1.3),loc = 'upper center', fontsize = 'large', ncol = 2)
    plt.savefig(output_file, bbox_inches="tight")
    plt.close()


if __name__ == "__main__":
    argParser = argparse.ArgumentParser()
    argParser.add_argument("-i", "--input", help="Path to input files")
    argParser.add_argument("-y", "--col", help="Y-axis column")
    args = argParser.parse_args()
    input_paths = args.input.split(",")
    ycol = args.col
    y_dict = defaultdict(list)
    for ind in range(len(input_paths)):
        curr_dict = read_file_to_dict(input_paths[ind], "name" , ycol)
        for key, value in curr_dict.items():
            y_dict[key].append(value)
    print(y_dict)
    x_list = [10, 20, 30, 50]
    plot_graph(x_list, y_dict, "line_graph.png")