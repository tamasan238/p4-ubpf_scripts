import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

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

# 3Dヒストグラムを描画する関数
def plot_3d_histogram(data, title):
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')

    for i, (size, values) in enumerate(zip(file_sizes, data)):
        if values:
            hist, bins = np.histogram(values, bins=20)  # データをヒストグラム化
            xpos = (bins[:-1] + bins[1:]) / 2  # X座標（ビンの中央）
            ypos = np.full_like(xpos, i)  # Y座標（パケットサイズのインデックス）
            zpos = np.zeros_like(xpos)  # Z軸の開始位置

            dx = (bins[1] - bins[0]) * np.ones_like(xpos)  # X軸方向の幅
            dy = np.full_like(xpos, 0.8)  # Y軸方向の幅（間隔を調整）
            dz = hist  # Z軸の高さ（頻度）

            ax.bar3d(xpos, ypos, zpos, dx, dy, dz, alpha=0.7)

    # 軸ラベルとタイトルを設定
    ax.set_xlabel("Logged Value")
    ax.set_ylabel("Packet Size Index")
    ax.set_zlabel("Frequency")
    ax.set_title(title)
    
    # Y軸のインデックスをパケットサイズに変換
    ax.set_yticks(range(len(file_sizes)))
    ax.set_yticklabels(file_sizes)

    plt.show()

# TCPとUDPの3Dヒストグラムを描画
plot_3d_histogram(tcp_data, "TCP Packet Distribution (3D)")
plot_3d_histogram(udp_data, "UDP Packet Distribution (3D)")

