import numpy as np
import matplotlib.pyplot as plt

# ファイルからデータを読み込む
with open("tcp.txt", "r") as f:
    data = [int(line.strip()) for line in f]

# ヒストグラムを作成
plt.hist(data, bins=np.arange(min(data), max(data) + 2) - 0.5, edgecolor="black")

# ラベルとタイトル
plt.xlabel("Value")
plt.ylabel("Frequency")
plt.title("Histogram of tcp.txt")

# グラフを表示
plt.show()

