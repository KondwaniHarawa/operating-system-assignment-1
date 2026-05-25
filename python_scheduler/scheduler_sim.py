import csv
import argparse
from gantt import draw_gantt

def load_processes(file_path):
    processes = []

    with open(file_path, "r") as file:
        reader = csv.DictReader(file)

        for row in reader:
            processes.append({
                "pid": int(row["pid"]),
                "arrival": int(row["arrival"]),
                "burst": int(row["burst"]),
                "priority": int(row["priority"])
            })

    return processes


def fcfs(processes):
    processes.sort(key=lambda x: (x["arrival"], x["pid"]))

    time = 0
    schedule = []

    for p in processes:
        if time < p["arrival"]:
            time = p["arrival"]

        start = time
        end = time + p["burst"]

        schedule.append((p["pid"], start, end))
        time = end

    return schedule


def sjf(processes):
    processes = sorted(processes, key=lambda x: (x["arrival"], x["burst"]))

    time = 0
    completed = []
    schedule = []

    while processes:
        available = [p for p in processes if p["arrival"] <= time]

        if not available:
            time += 1
            continue

        p = min(available, key=lambda x: x["burst"])

        start = time
        end = time + p["burst"]

        schedule.append((p["pid"], start, end))

        time = end
        processes.remove(p)
        completed.append(p)

    return schedule


def priority_scheduling(processes):
    time = 0
    schedule = []

    while processes:
        available = [p for p in processes if p["arrival"] <= time]

        if not available:
            time += 1
            continue

        for p in available:
            if (time - p["arrival"]) % 3 == 0 and time != p["arrival"]:
                p["priority"] = max(0, p["priority"] - 1)

        p = min(available, key=lambda x: x["priority"])

        start = time
        end = time + p["burst"]

        schedule.append((p["pid"], start, end))

        time = end
        processes.remove(p)

    return schedule


def round_robin(processes, quantum=2):
    queue = sorted(processes, key=lambda x: x["arrival"])

    time = 0
    schedule = []

    remaining = {p["pid"]: p["burst"] for p in processes}

    while queue:
        p = queue.pop(0)

        if time < p["arrival"]:
            time = p["arrival"]

        exec_time = min(quantum, remaining[p["pid"]])

        start = time
        time += exec_time
        end = time

        schedule.append((p["pid"], start, end))

        remaining[p["pid"]] -= exec_time

        if remaining[p["pid"]] > 0:
            queue.append(p)

    return schedule


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", required=True)
    parser.add_argument("--algo", default="fcfs")
    parser.add_argument("--quantum", type=int, default=2)

    args = parser.parse_args()

    processes = load_processes(args.file)

    if args.algo == "fcfs":
        schedule = fcfs(processes)
    elif args.algo == "sjf":
        schedule = sjf(processes)
    elif args.algo == "priority":
        schedule = priority_scheduling(processes)
    elif args.algo == "rr":
        schedule = round_robin(processes, args.quantum)
    else:
        print("Unknown algorithm")
        return

    print("\nSchedule:")
    for s in schedule:
        print(s)

    draw_gantt(schedule, args.algo.upper())


if __name__ == "__main__":
    main()
