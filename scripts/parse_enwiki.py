import pandas as pd

if __name__ == "__main__":
    df = pd.read_csv('enwiki-latest-all-titles',sep='\t')
    titles = df.page_title.tolist()
    titles = [str(x) for x in titles]
    with open('enwiki-latest-all-titles-processed.txt', 'w') as f:
        for title in titles:
            f.write(f"{title}\n")

