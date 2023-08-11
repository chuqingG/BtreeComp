with open('/Users/shreyaballijepalli//Downloads/keys_random_20M.txt') as f:
    data = f.readlines()

data = data[:10000000]
print(len(data))
parsed_data = []
for val in data:
    print(len(val))
    break
