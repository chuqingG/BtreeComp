import sqlite3

con = sqlite3.connect("/Users/shreyaballijepalli/Downloads/database.sqlite")
cur = con.cursor()
rows = cur.execute('SELECT * FROM links;')
links = []
for row in rows:
	links.append(row[1])

print(len(links))

con.close()

