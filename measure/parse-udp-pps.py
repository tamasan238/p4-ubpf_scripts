import sys
import re

# UDP pps

fname = sys.argv[1]
s = sys.stdin.read()
m = re.findall(r'^\s*\d+\s+\d*\s+\d+\.\d+\s+(\d+)', s, flags=re.M)

v = str(round(int(m[1]) / 10))

with open(fname, 'a') as f:
    f.write(v + '\n')
