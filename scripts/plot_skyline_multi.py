import csv
import argparse
import matplotlib.pyplot as plt
import matplotlib.font_manager
import matplotlib.cm as cm
import numpy as np

def read_file_to_dict(file_name, key, val):
    infile = csv.DictReader(open(file_name))
    key_val_dict = {}
    for row in infile:
        key_val_dict[row[key]] = float(row[val])
    return key_val_dict

def plot_graph(search_times_dict_list, range_times_dict_list, sizes_dict_list, output_file):
    nrows = len(search_times_dict_list)
    fig, ax = plt.subplots(nrows=nrows, ncols=2)
    fig.tight_layout(h_pad=4, w_pad=3)
    if nrows == 1: 
        fig.set_figwidth(5.5)
        fig.set_figheight(2)
    
    search_titles = ['Search Query']
    range_titles = ['Range Query']

    for row in range(nrows):
        search_times_dict = search_times_dict_list[row]
        range_times_dict = range_times_dict_list[row]
        sizes_dict = sizes_dict_list[row]
        x = list(sizes_dict.values())
        y1 = list(search_times_dict.values())
        y2 = list(range_times_dict.values())
        if nrows == 1:
            plt1 = ax[0]
            plt2 = ax[1]
        else:
            plt1 = ax[row, 0]
            plt2 = ax[row , 1]

        #7 markers and colors based on 8 algorithms
        markers = ["." , "o" , "^" , "s" , "+" , "x", "D", "p"]
        colors = ['red','blue','green','yellow','black', 'cyan', 'purple', 'brown']
        for i , label in enumerate(search_times_dict):
            plt1.scatter(x[i], y1[i], label=label, marker = markers[i], color = colors[i])
            plt2.scatter(x[i], y2[i], label=label, marker = markers[i], color = colors[i])
        plt1.set_xlabel("Key size (bytes)", fontsize = 'large')
        plt1.set_ylabel("Time (s)", fontsize = 'large')
        plt1.set_xlim(xmin=0, xmax = max(x) + 5)
        plt1.set_ylim(ymin=0, ymax= max(y1) + 5)
        plt1.set_title(search_titles[row], fontname="Times New Roman", fontweight="bold")
        
        plt2.set_xlabel("Key size (bytes)", fontsize = 'large')
        plt2.set_ylabel("Time (s)", fontsize = 'large')
        plt2.set_xlim(xmin=0, xmax = max(x) + 5)
        plt2.set_ylim(ymin=0, ymax= max(y2) + 5)
        plt2.set_title(range_titles[row], fontname="Times New Roman", fontweight="bold")

    if nrows == 1: 
        plt.legend(bbox_to_anchor=(-0.25, 1.85), loc='upper center', fontsize = 'large', ncol = 2)
    else:
        plt.legend(bbox_to_anchor=(-0.25, 3.5), loc='upper center', fontsize = 'large', ncol = 2)

    plt.savefig(output_file,  bbox_inches="tight")

    plt.close()


if __name__ == "__main__":
    argParser = argparse.ArgumentParser()
    argParser.add_argument("-i", "--input", help="Path to input files")
    args = argParser.parse_args()
    files_path = args.input
    times_search_dict_list = []
    times_range_dict_list = []
    sizes_dict_list = []
    for path in files_path.split(","):
        times_search_dict = read_file_to_dict(path+"/performance_search.txt", "name" , "avg")
        times_range_dict = read_file_to_dict(path+"/performance_range.txt", "name" , "avg")
        sizes_dict = read_file_to_dict(path+"/statistics.txt", "name" , "keysize")
        times_search_dict_list.append(times_search_dict)
        times_range_dict_list.append(times_range_dict)
        sizes_dict_list.append(sizes_dict)

    plot_graph(times_search_dict_list, times_range_dict_list,  sizes_dict_list, "skyline_search_range.png")