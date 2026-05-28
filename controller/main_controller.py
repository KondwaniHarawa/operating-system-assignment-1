import subprocess
import json
import time
import os
import sys
from datetime import datetime

# ═══════════════════════════════════════════════
# PATHS
# ═══════════════════════════════════════════════

BASE_DIR       = os.path.dirname(os.path.abspath(__file__))
C_BINARY       = os.path.join(BASE_DIR, "../c_core/eduos")
PCB_FILE       = os.path.join(BASE_DIR, "../c_core/pcb_snapshot.json")
C_WORK_DIR     = os.path.join(BASE_DIR, "../c_core")
SCHEDULER      = os.path.join(BASE_DIR, "../python_scheduler/scheduler_sim.py")
REPORT_FILE    = os.path.join(BASE_DIR, "../docs/simulation_report.json")
SUMMARIES_FILE = os.path.join(BASE_DIR, "../python_scheduler/summaries.json")


# ═══════════════════════════════════════════════
# STEP 1: Launch C Simulator
# ═══════════════════════════════════════════════

def run_c_simulator():
    print("\n" + "═"*55)
    print("  STEP 1: Launching C Simulator")
    print("═"*55)

    if not os.path.exists(C_BINARY):
        print(f"[ERROR] C binary not found at {C_BINARY}")
        print("        Run 'make all' inside c_core/ first.")
        sys.exit(1)

    process = subprocess.Popen(
        [C_BINARY],
        cwd=C_WORK_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    # Capture stdout in real-time
    for line in process.stdout:
        print(f"  [C] {line.rstrip()}")

    process.wait()

    if process.returncode != 0:
        err = process.stderr.read()
        print(f"[ERROR] C simulator exited with code {process.returncode}")
        print(err)
        sys.exit(1)

    print("\n[OK] C simulator completed successfully")


# ═══════════════════════════════════════════════
# STEP 2: Wait for PCB Snapshot
# ═══════════════════════════════════════════════

def wait_for_pcb(timeout=30):
    print("\n" + "═"*55)
    print("  STEP 2: Waiting for pcb_snapshot.json")
    print("═"*55)

    start = time.time()
    while not os.path.exists(PCB_FILE):
        if time.time() - start > timeout:
            print(f"[ERROR] Timed out waiting for {PCB_FILE}")
            sys.exit(1)
        print("  [WAIT] pcb_snapshot.json not ready yet...")
        time.sleep(1)

    print(f"[OK] Found {PCB_FILE}")


# ═══════════════════════════════════════════════
# STEP 3: Load and Validate PCB Data
# ═══════════════════════════════════════════════

def load_pcb_data():
    print("\n" + "═"*55)
    print("  STEP 3: Loading PCB Snapshot")
    print("═"*55)

    with open(PCB_FILE, "r") as f:
        data = json.load(f)

    terminated = [p for p in data if p.get("state") == "TERMINATED"]
    print(f"  Total processes : {len(data)}")
    print(f"  Terminated      : {len(terminated)}")

    if len(terminated) == 0:
        print("[WARN] No terminated processes found in snapshot.")

    for p in data:
        print(f"  PID={p['pid']:3d}  Name={p['name']:<20s}  "
              f"State={p['state']:<12s}  Burst={p['burst_time']}")

    print("\n[OK] PCB data loaded successfully")
    return data


# ═══════════════════════════════════════════════
# STEP 4: Run Python Scheduler
# ═══════════════════════════════════════════════

def run_python_scheduler():
    print("\n" + "═"*55)
    print("  STEP 4: Running Python Scheduler")
    print("═"*55)

    if not os.path.exists(SCHEDULER):
        print(f"[ERROR] Scheduler not found at {SCHEDULER}")
        sys.exit(1)

    result = subprocess.run(
        [sys.executable, SCHEDULER,
         "--file", PCB_FILE,
         "--algo", "all"],
        capture_output=True,
        text=True
    )

    for line in result.stdout.splitlines():
        print(f"  [PY] {line}")

    if result.returncode != 0:
        print(f"[ERROR] Scheduler failed:\n{result.stderr}")
        sys.exit(1)

    print("\n[OK] Python scheduler completed")


# ═══════════════════════════════════════════════
# STEP 5: Generate Summary Report
# ═══════════════════════════════════════════════

def generate_report(pcb_data):
    print("\n" + "═"*55)
    print("  STEP 5: Generating Simulation Report")
    print("═"*55)

    summaries = {}
    if os.path.exists(SUMMARIES_FILE):
        with open(SUMMARIES_FILE, "r") as f:
            summaries = json.load(f)

    report = {
        "timestamp":       datetime.now().isoformat(),
        "simulation_date": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "total_processes": len(pcb_data),
        "terminated":      len([p for p in pcb_data if p.get("state") == "TERMINATED"]),
        "processes":       pcb_data,
        "scheduling_results": summaries,
        "best_algorithm":  min(summaries, key=lambda k: summaries[k]["avg_wt"])
                           if summaries else "N/A"
    }

    os.makedirs(os.path.dirname(REPORT_FILE), exist_ok=True)
    with open(REPORT_FILE, "w") as f:
        json.dump(report, f, indent=2)

    print(f"  Timestamp       : {report['timestamp']}")
    print(f"  Total processes : {report['total_processes']}")
    print(f"  Best algorithm  : {report['best_algorithm']}")
    print(f"\n[OK] Report saved to {REPORT_FILE}")
    return report


# ═══════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════

def main():
    print("\n╔══════════════════════════════════════════════════════╗")
    print("║         EduOS Main Controller Starting              ║")
    print("╚══════════════════════════════════════════════════════╝")

    start_time = time.time()

    run_c_simulator()
    wait_for_pcb()
    pcb_data = load_pcb_data()
    run_python_scheduler()
    report   = generate_report(pcb_data)

    elapsed = round(time.time() - start_time, 2)

    print("\n╔══════════════════════════════════════════════════════╗")
    print("║         EduOS Simulation Complete                   ║")
    print(f"║         Total time: {elapsed}s{' '*(31-len(str(elapsed)))}║")
    print("╚══════════════════════════════════════════════════════╝\n")


if __name__ == "__main__":
    main()
