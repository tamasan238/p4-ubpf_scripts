import sys
import re

# TCP/UDP Mbps

fname = sys.argv[1]
s = sys.stdin.read()
v = re.search(r'(\d+\.\d+)\s*$', s.strip()).group(1)

with open(fname, 'a') as f:
    f.write(v + '\n')
