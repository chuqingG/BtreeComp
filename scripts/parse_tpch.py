import pandas as pd

df = pd.read_table('../../../datasets/tpch/customer.tbl',delimiter='|', header=None)
# combined_df = df.iloc[:, 1:].apply(lambda x: "|".join(x.astype(str)), axis=1)
combined_df = df.iloc[:, 1:]
combined_df.to_csv('../../../datasets/tpch/customer_from1_300.txt', header=None, index=None, sep='|', mode='a')

