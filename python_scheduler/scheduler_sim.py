import csv
import json
import argparse
import random
from tabulate import tabulate

# ═══════════════════════════════════════════════
# PROCESS LOADING
# ═══════════════════════════════════════════════

def load_from_csv(file_path):
    processes = []
    with open(file_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            processes.append({
                "pid":      int(row["pid"]),
                "arrival":  int(row["arrival"]),
                "burst":    int(row["burst"]),
                "priority": int(row["priority"])
            })
    return processes


def load_from_json(file_path):
    with open(file_path, "r") as f:
        data = json.load(f)
    processes = []
    for p in data:
        if p.get("state") == "TERMINATED":
            processes.append({
                "pid":      p["pid"],
                "arrival":  p.get("arrival_time", 0),
                "burst":    p.get("burst_time", 1),
                "priority": p.get("priority", 0)
            })
    return processes


def generate_random(n, seed=None):
    if seed is not None:
        random.seed(seed)
    processes = []
    for i in range(1, n + 1):
        processes.append({
            "pid":      i,
            "arrival":  random.randint(0, n),
            "burst":    random.randint(1, 15),
            "priority": random.randint(1, 5)
        })
    return processes


# ═══════════════════════════════════════════════
# METRICS CALCULATION
# ═══════════════════════════════════════════════

def compute_metrics(processes, schedule):
    rows = []
    total_time = max(end for _, _, end in schedule)
    busy_time  = sum(end - start for _, start, end in schedule)

    first_run  = {}
    comp_time  = {}
    for pid, start, end in schedule:
        if pid not in first_run:
            first_run[pid] = start
        comp_time[pid] = end

    for p in processes:
        pid = p["pid"]
        ct  = comp_time.get(pid, 0)
        at  = p["arrival"]
        bt  = p["burst"]
        tat = ct - at
        wt  = tat - bt
        rt  = first_run.get(pid, 0) - at
        rows.append({
            "PID":      pid,
            "Arrival":  at,
            "Burst":    bt,
            "Finish":   ct,
            "TAT":      tat,
            "WT":       wt,
            "RT":       rt
        })

    avg_wt  = sum(r["WT"]  for r in rows) / len(rows)
    avg_tat = sum(r["TAT"] for r in rows) / len(rows)
    avg_rt  = sum(r["RT"]  for r in rows) / len(rows)
    cpu_util = (busy_time / total_time * 100) if total_time > 0 else 0
    throughput = len(processes) / total_time if total_time > 0 else 0

    return rows, {
        "avg_wt":    round(avg_wt,    2),
        "avg_tat":   round(avg_tat,   2),
        "avg_rt":    round(avg_rt,    2),
        "cpu_util":  round(cpu_util,  2),
        "throughput":round(throughput,4)
    }


def print_metrics(algo_name, rows, summary):
    print(f"\n{'━'*60}")
    print(f"  {algo_name} Results")
    print(f"{'━'*60}")
    table = [[r["PID"], r["Arrival"], r["Burst"],
              r["Finish"], r["TAT"], r["WT"], r["RT"]] for r in rows]
    print(tabulate(table,
                   headers=["PID","Arrival","Burst",
                             "Finish","TAT","WT","RT"],
                   tablefmt="rounded_grid"))
    print(f"\n  Avg WT={summary['avg_wt']}  "
          f"Avg TAT={summary['avg_tat']}  "
          f"Avg RT={summary['avg_rt']}  "
          f"CPU={summary['cpu_util']}%  "
          f"Throughput={summary['throughput']} proc/unit\n")


# ═══════════════════════════════════════════════
# SCHEDULING ALGORITHMS
# ═══════════════════════════════════════════════

def fcfs(processes):
    """First Come First Served — non-preemptive.
    Ties in arrival_time broken by lower PID."""
    procs = sorted(processes, key=lambda x: (x["arrival"], x["pid"]))
    time, schedule = 0, []
    for p in procs:
        if time < p["arrival"]:
            time = p["arrival"]
        schedule.append((p["pid"], time, time + p["burst"]))
        time += p["burst"]
    return schedule


def sjf(processes):
    """Shortest Job First — non-preemptive.
    Equal burst_time falls back to FCFS order."""
    procs     = sorted(processes, key=lambda x: (x["arrival"], x["pid"]))
    remaining = list(procs)
    time, schedule = 0, []
    while remaining:
        available = [p for p in remaining if p["arrival"] <= time]
        if not available:
            time = min(p["arrival"] for p in remaining)
            continue
        p = min(available, key=lambda x: (x["burst"], x["arrival"], x["pid"]))
        schedule.append((p["pid"], time, time + p["burst"]))
        time += p["burst"]
        remaining.remove(p)
    return schedule


def priority_scheduling(processes):
    """Priority Scheduling — non-preemptive.
    Lower number = higher urgency.
    Ageing: every 3 time units a waiting process gains +1 priority
    (priority number decreases by 1) to prevent starvation."""
    remaining = [dict(p) for p in processes]
    time, schedule = 0, []
    while remaining:
        available = [p for p in remaining if p["arrival"] <= time]
        if not available:
            time = min(p["arrival"] for p in remaining)
            continue

        # Ageing: boost priority of waiting processes
        for p in available:
            wait = time - p["arrival"]
            if wait > 0 and wait % 3 == 0:
                p["priority"] = max(0, p["priority"] - 1)

        p = min(available, key=lambda x: (x["priority"], x["arrival"], x["pid"]))
        schedule.append((p["pid"], time, time + p["burst"]))
        time += p["burst"]
        remaining.remove(p)
    return schedule


def round_robin(processes, quantum=2):
    """Round Robin — preemptive with user-defined time quantum.
    Maintains a proper ready queue ordered by arrival time."""
    procs     = sorted(processes, key=lambda x: (x["arrival"], x["pid"]))
    remaining = {p["pid"]: p["burst"] for p in procs}
    arrived   = {p["pid"]: p["arrival"] for p in procs}
    queue     = []
    schedule  = []
    time      = 0
    done      = set()
    i         = 0   # index into sorted procs

    while len(done) < len(procs):
        # Enqueue newly arrived processes
        while i < len(procs) and procs[i]["arrival"] <= time:
            queue.append(procs[i]["pid"])
            i += 1

        if not queue:
            time = procs[i]["arrival"]
            continue

        pid      = queue.pop(0)
        run_time = min(quantum, remaining[pid])
        start    = time
        time    += run_time
        remaining[pid] -= run_time
        schedule.append((pid, start, time))

        # Enqueue any new arrivals during this slice
        while i < len(procs) and procs[i]["arrival"] <= time:
            queue.append(procs[i]["pid"])
            i += 1

        if remaining[pid] > 0:
            queue.append(pid)
        else:
            done.add(pid)

    return schedule


# ═══════════════════════════════════════════════
# THREAD MODE
# ═══════════════════════════════════════════════

def run_thread_mode(processes, quantum):
    """Thread scheduling mode — each process represents a thread.
    Threads from the same parent PID are grouped together.
    Context-switch overhead of 1 unit is added between thread groups."""
    print("\n[THREAD MODE] Scheduling threads with context-switch overhead\n")
    schedule = round_robin(processes, quantum)

    # Add context switch cost of 1 unit when PID group changes
    adjusted = []
    prev_pid  = None
    overhead  = 0
    for pid, start, end in schedule:
        if prev_pid is not None and pid != prev_pid:
            overhead += 1
            start    += overhead
            end      += overhead
        adjusted.append((pid, start, end))
        prev_pid = pid

    print("Thread schedule (with 1-unit context-switch overhead):")
    for pid, start, end in adjusted:
        print(f"  Thread PID={pid}  [{start} → {end}]")
    return adjusted


# ═══════════════════════════════════════════════
# COMPARISON TABLE
# ═══════════════════════════════════════════════

def print_comparison(results):
    print(f"\n{'━'*70}")
    print("  Algorithm Comparison")
    print(f"{'━'*70}")
    table = []
    for name, summary in results.items():
        table.append([
            name,
            summary["avg_wt"],
            summary["avg_tat"],
            summary["avg_rt"],
            f"{summary['cpu_util']}%",
            summary["throughput"]
        ])
    print(tabulate(table,
                   headers=["Algorithm","Avg WT","Avg TAT",
                             "Avg RT","CPU%","Throughput"],
                   tablefmt="rounded_grid"))
    best = min(results, key=lambda k: results[k]["avg_wt"])
    print(f"\n  Best Avg Waiting Time: {best} "
          f"({results[best]['avg_wt']} units)\n")


# ═══════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="EduOS Scheduling Simulator")
    parser.add_argument("--file",   help="CSV or JSON input file")
    parser.add_argument("--random", type=int, metavar="N",
                        help="Generate N random processes")
    parser.add_argument("--seed",   type=int, default=42,
                        help="Random seed for reproducibility (default=42)")
    parser.add_argument("--algo",   default="all",
                        choices=["fcfs","sjf","priority","rr","all"],
                        help="Algorithm to run (default=all)")
    parser.add_argument("--quantum",type=int, default=2,
                        help="Time quantum for Round Robin (default=2)")
    parser.add_argument("--mode",   default="process",
                        choices=["process","thread"],
                        help="Scheduling mode (default=process)")
    args = parser.parse_args()

    # Load processes
    if args.random:
        processes = generate_random(args.random, args.seed)
        print(f"[INFO] Generated {args.random} random processes (seed={args.seed})")
    elif args.file:
        if args.file.endswith(".json"):
            processes = load_from_json(args.file)
            print(f"[INFO] Loaded {len(processes)} processes from {args.file}")
        else:
            processes = load_from_csv(args.file)
            print(f"[INFO] Loaded {len(processes)} processes from {args.file}")
    else:
        parser.print_help()
        return

    if not processes:
        print("[ERROR] No processes loaded.")
        return

    # Thread mode
    if args.mode == "thread":
        run_thread_mode(processes, args.quantum)
        return

    # Run algorithms
    algos = {
        "FCFS":     lambda p: fcfs(p),
        "SJF":      lambda p: sjf(p),
        "Priority": lambda p: priority_scheduling(p),
        f"RR(q={args.quantum})": lambda p: round_robin(p, args.quantum)
    }

    if args.algo != "all":
        key_map = {
            "fcfs":     "FCFS",
            "sjf":      "SJF",
            "priority": "Priority",
            "rr":       f"RR(q={args.quantum})"
        }
        selected = key_map[args.algo]
        algos = {selected: algos[selected]}

    results = {}
    schedules = {}
    for name, fn in algos.items():
        schedule = fn(list(processes))
        rows, summary = compute_metrics(processes, schedule)
        print_metrics(name, rows, summary)
        results[name]    = summary
        schedules[name]  = schedule

    if len(results) > 1:
        print_comparison(results)

    # Save schedules for Gantt chart
    with open("schedules.json", "w") as f:
        json.dump({k: v for k, v in schedules.items()}, f, indent=2)
    with open("summaries.json", "w") as f:
        json.dump(results, f, indent=2)
    print("[INFO] Schedules saved to schedules.json")
    print("[INFO] Run gantt.py to generate charts")


if __name__ == "__main__":
    main()
