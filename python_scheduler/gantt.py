import json
import os
import matplotlib
matplotlib.use("Agg")  # non-interactive backend for WSL
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ═══════════════════════════════════════════════
# COLOUR PALETTE
# ═══════════════════════════════════════════════

COLOURS = [
    "#4C72B0", "#DD8452", "#55A868", "#C44E52",
    "#8172B3", "#937860", "#DA8BC3", "#8C8C8C",
    "#CCB974", "#64B5CD"
]
IDLE_COLOUR = "#D3D3D3"

OUTPUT_DIR = "../docs/screenshots"


def ensure_output_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def get_colour_map(schedule):
    pids = sorted(set(pid for pid, _, _ in schedule))
    return {pid: COLOURS[i % len(COLOURS)] for i, pid in enumerate(pids)}


# ═══════════════════════════════════════════════
# GANTT CHART (per algorithm)
# ═══════════════════════════════════════════════

def draw_gantt(schedule, title="Gantt Chart", save=True):
    ensure_output_dir()
    colour_map = get_colour_map(schedule)

    fig, ax = plt.subplots(figsize=(14, 4))
    ax.set_title(title, fontsize=14, fontweight="bold", pad=12)
    ax.set_xlabel("Time Units", fontsize=11)
    ax.set_ylabel("Process", fontsize=11)

    # Draw idle gaps in grey
    all_times = sorted(set(t for _, s, e in schedule for t in (s, e)))
    if all_times:
        start_time = all_times[0]
        end_time   = all_times[-1]
        prev_end   = start_time

        for pid, start, end in sorted(schedule, key=lambda x: x[1]):
            if start > prev_end:
                # Idle gap
                ax.barh(0, start - prev_end, left=prev_end,
                        height=0.5, color=IDLE_COLOUR,
                        edgecolor="black", linewidth=0.5)
                ax.text(prev_end + (start - prev_end) / 2, 0,
                        "IDLE", ha="center", va="center",
                        fontsize=7, color="grey")
            prev_end = max(prev_end, end)

    # Draw process bars
    for pid, start, end in schedule:
        ax.barh(pid, end - start, left=start,
                height=0.5, color=colour_map[pid],
                edgecolor="black", linewidth=0.5)
        ax.text(start + (end - start) / 2, pid,
                f"P{pid}", ha="center", va="center",
                fontsize=8, color="white", fontweight="bold")

    # X-axis ticks at every time unit
    max_time = max(end for _, _, end in schedule)
    ax.set_xticks(range(0, max_time + 1))
    ax.set_xticklabels(range(0, max_time + 1), fontsize=7)

    # Y-axis
    pids = sorted(set(pid for pid, _, _ in schedule))
    ax.set_yticks(pids)
    ax.set_yticklabels([f"P{p}" for p in pids], fontsize=9)

    # Legend
    patches = [mpatches.Patch(color=colour_map[p], label=f"P{p}") for p in pids]
    patches.append(mpatches.Patch(color=IDLE_COLOUR, label="IDLE"))
    ax.legend(handles=patches, loc="upper right",
              fontsize=8, ncol=len(patches))

    ax.grid(axis="x", linestyle="--", alpha=0.4)
    plt.tight_layout()

    if save:
        fname = os.path.join(OUTPUT_DIR,
                             f"gantt_{title.replace(' ', '_').replace('(','').replace(')','').replace('=','')}.png")
        plt.savefig(fname, dpi=150, bbox_inches="tight")
        print(f"[GANTT] Saved: {fname}")
        plt.close()
    else:
        plt.show()


# ═══════════════════════════════════════════════
# COMPARISON BAR CHARTS
# ═══════════════════════════════════════════════

def draw_comparison(results):
    """Side-by-side bar charts for Avg WT, Avg TAT, CPU Utilisation."""
    ensure_output_dir()

    algos   = list(results.keys())
    avg_wt  = [results[a]["avg_wt"]   for a in algos]
    avg_tat = [results[a]["avg_tat"]  for a in algos]
    cpu     = [results[a]["cpu_util"] for a in algos]

    x     = np.arange(len(algos))
    width = 0.25

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.set_title("Algorithm Comparison", fontsize=14, fontweight="bold")

    bars1 = ax.bar(x - width, avg_wt,  width, label="Avg WT",  color="#4C72B0")
    bars2 = ax.bar(x,         avg_tat, width, label="Avg TAT", color="#DD8452")
    bars3 = ax.bar(x + width, cpu,     width, label="CPU %",   color="#55A868")

    # Value labels on bars
    for bars in (bars1, bars2, bars3):
        for bar in bars:
            h = bar.get_height()
            ax.text(bar.get_x() + bar.get_width() / 2, h + 0.1,
                    f"{h:.1f}", ha="center", va="bottom", fontsize=8)

    ax.set_xticks(x)
    ax.set_xticklabels(algos, fontsize=10)
    ax.set_ylabel("Value", fontsize=11)
    ax.legend(fontsize=10)
    ax.grid(axis="y", linestyle="--", alpha=0.4)

    plt.tight_layout()
    fname = os.path.join(OUTPUT_DIR, "comparison_chart.png")
    plt.savefig(fname, dpi=150, bbox_inches="tight")
    print(f"[CHART] Saved: {fname}")
    plt.close()


# ═══════════════════════════════════════════════
# MAIN — reads schedules.json produced by scheduler_sim.py
# ═══════════════════════════════════════════════

def main():
    if not os.path.exists("schedules.json"):
        print("[ERROR] schedules.json not found.")
        print("        Run scheduler_sim.py first.")
        return

    with open("schedules.json") as f:
        data = json.load(f)

    # Also load summaries if available
    summaries = {}
    if os.path.exists("summaries.json"):
        with open("summaries.json") as f:
            summaries = json.load(f)

    for algo_name, schedule in data.items():
        # Convert list-of-lists back to list-of-tuples
        schedule_tuples = [(s[0], s[1], s[2]) for s in schedule]
        draw_gantt(schedule_tuples, title=algo_name)

    if summaries:
        draw_comparison(summaries)
        print("[INFO] Comparison chart saved.")
    else:
        print("[INFO] No summaries.json found — skipping comparison chart.")
        print("       Run scheduler_sim.py --algo all first.")

    print("\n[INFO] All charts saved to docs/screenshots/")


if __name__ == "__main__":
    main()
