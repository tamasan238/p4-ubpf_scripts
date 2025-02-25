import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
import glob

# 記録対象のファイルサイズ（共通）
file_sizes = [64, 128, 256, 512, 1024, 1280, 1518, 2048, 4096, 8192, 16384, 32768, 65535]

# TCPとUDPのデータを読み込む
def load_data(protocol):
    data = []
    for size in file_sizes:
        file_name = f"{protocol}_{size}.txt"
        try:
            with open(file_name, "r") as f:
                values = [int(line.strip()) for line in f if line.strip().isdigit()]
                data.append(values)
        except FileNotFoundError:
            data.append([])  # ファイルがない場合は空リスト
    return data

tcp_data = load_data("tcp")
udp_data = load_data("udp")

# ヒートマップ用の2Dヒストグラムデータを作成
def create_histogram_matrix(data):
    max_value = max(max(row, default=0) for row in data)  # 最大値を取得（スケール用）
    matrix = np.zeros((len(data), max_value + 1))  # (パケットサイズ × 値の最大値) の行列
    for i, row in enumerate(data):
        for value in row:
            if value < matrix.shape[1]:  # 範囲外アクセスを防ぐ
                matrix[i, value] += 1
    return matrix

tcp_matrix = create_histogram_matrix(tcp_data)
udp_matrix = create_histogram_matrix(udp_data)

# ヒートマップを描画
def plot_heatmap(matrix, title):
    plt.figure(figsize=(12, 6))
    sns.heatmap(matrix, cmap="coolwarm", xticklabels=1, yticklabels=file_sizes, cbar=True)
    plt.xlabel("Logged Value")
    plt.ylabel("Packet Size (Bytes)")
    plt.title(title)
    plt.show()


plot_heatmap(tcp_matrix, "TCP Heatmap")
plot_heatmap(udp_matrix, "UDP Heatmap")

