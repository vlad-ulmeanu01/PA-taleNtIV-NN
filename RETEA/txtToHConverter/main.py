#python3 main.py file.txt
# => file.h

import sys


fname = sys.argv[1]
print(fname)

fin = open(fname, "r")
fout = open(fname.split(".txt")[0] + ".h", "w")

fout.write("\"")

for line in fin.readlines():
    line = line.rstrip()
    for s in line.split():
        if len(s):
            fout.write(f"{s} ")

fout.write("\"\n")

fin.close()
fout.close()


