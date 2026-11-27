import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("pf_stats.csv")
lru = df[df["Strategy"] == "LRU"]
mru = df[df["Strategy"] == "MRU"]

fig, axes = plt.subplots(1, 2, figsize=(12, 5))

axes[0].plot(lru["Mixture"], lru["Total_Physical_IO"], marker="o", label="LRU Physical I/O")
axes[0].plot(mru["Mixture"], mru["Total_Physical_IO"], marker="s", label="MRU Physical I/O")
axes[0].set_title("Total Physical I/O (LRU vs MRU)")
axes[0].set_xlabel("Read/Write Query Mixture")
axes[0].set_ylabel("Physical I/O Count")
axes[0].legend()
axes[0].grid(True)

axes[1].plot(lru["Mixture"], lru["Total_Logical_IO"], marker="o", label="Logical I/O (LRU)")
axes[1].plot(lru["Mixture"], lru["Total_Physical_IO"], marker="^", label="Physical I/O (LRU)")
axes[1].plot(mru["Mixture"], mru["Total_Physical_IO"], marker="s", label="Physical I/O (MRU)")
axes[1].set_title("Logical vs Physical I/O")
axes[1].set_xlabel("Read/Write Query Mixture")
axes[1].set_ylabel("I/O Count")
axes[1].legend()
axes[1].grid(True)

plt.tight_layout()
plt.savefig("pf_buffer_stats.png", dpi=200)
print("Graph saved as pf_buffer_stats.png!")
