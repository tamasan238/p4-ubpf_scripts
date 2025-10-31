"import re
import subprocess
import csv
import sys

def parse_ovs_statistics(output, iface_name):
    stats_match = re.search(r'statistics\s*:\s*\{(.*)\}', output, re.S)
    if not stats_match:
        return {}
    stats_text = stats_match.group(1)
    stats = {}
    for pair in re.findall(r'([^=,{}]+)=([^,{}]+)', stats_text):
        key = pair[0].strip()
        val = pair[1].strip()
        if val.isdigit():
            val = int(val)
        else:
            try:
                val = float(val)
            except ValueError:
                pass
        stats[key] = val
    return {iface_name: stats}

def get_ovs_statistics(interfaces):
    results = {}
    for iface in interfaces:
        cmd = f""ovs-vsctl --column statistics list interface {iface}""
        output = subprocess.getoutput(cmd)
        results.update(parse_ovs_statistics(output, iface))
    return results

def print_csv(data):
    keys = sorted({k for iface in data.values() for k in iface.keys()})
    writer = csv.writer(sys.stdout)
    writer.writerow([""interface""] + keys)
    for iface, stats in data.items():
        row = [iface] + [stats.get(k, """") for k in keys]
        writer.writerow(row)

if __name__ == ""__main__"":
    interfaces = [""dpdk0"", ""vhost1""]
    stats = get_ovs_statistics(interfaces)
    print_csv(stats)"