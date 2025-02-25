import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt

# 記録対象のファイルサイズ（共通）
file_sizes = [64, 128, 256, 512, 1024, 1280, 1518, 2048, 4096, 8192, 16384, 32768, 65535]

# データを読み込む関数
def load_data(protocol):
    data = []
    for size in file_sizes:
        file_name = f"{protocol}_{size}.txt"
        try:
            with open(file_name, "r") as f:
                values = [int(line.strip()) for line in f if line.strip().isdigit()]
                data.append(values)
        except FileNotFoundError:
            data.append([])
    return data

tcp_data = load_data("tcp")
udp_data = load_data("udp")

# 各パケットサイズごとにヒストグラムを作成し、独立した色で描画
def plot_independent_bars(data, title):
    plt.figure(figsize=(12, 6))
    for i, (size, values) in enumerate(zip(file_sizes, data)):
        if values:
            sns.histplot(values, bins=30, kde=False, element="bars", label=f"{size}B", alpha=0.7)

    plt.xlabel("Logged Value")
    plt.ylabel("Frequency")
    plt.title(title)
    plt.legend(title="Packet Size (Bytes)", loc="upper right", fontsize=8)
    plt.show()

plot_independent_bars(tcp_data, "TCP Packet Distribution")
plot_independent_bars(udp_data, "UDP Packet Distribution")

