import matplotlib.pyplot as plt

def draw_gantt(schedule, title="Gantt Chart"):
    fig, gnt = plt.subplots()

    gnt.set_title(title)
    gnt.set_xlabel("Time")
    gnt.set_ylabel("Processes")

    yticks = []
    ylabels = []

    for i, item in enumerate(schedule):
        pid, start, end = item

        gnt.barh(pid, end - start, left=start)

        yticks.append(pid)
        ylabels.append(f"P{pid}")

    gnt.set_yticks(yticks)
    gnt.set_yticklabels(ylabels)

    plt.tight_layout()
    plt.show()
