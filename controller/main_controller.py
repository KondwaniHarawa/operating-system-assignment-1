import subprocess
import json
import time
import os

C_EXECUTABLE = "./eduos"
PCB_FILE = "../pcb_snapshot.json"

def run_c_simulator():
    process = subprocess.Popen(
        [C_EXECUTABLE],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    for line in process.stdout:
        print("[C]", line.strip())

    process.wait()

def wait_for_pcb_file():
    while not os.path.exists(PCB_FILE):
        time.sleep(1)

    time.sleep(1)

def load_pcb_data():
    with open(PCB_FILE, "r") as f:
        return json.load(f)

def run_python_scheduler():
    subprocess.run([
        "python",
        "../python_scheduler/scheduler_sim.py",
        "--file",
        PCB_FILE
    ])

def main():
    print("Starting EduOS Controller...")

    run_c_simulator()

    wait_for_pcb_file()

    data = load_pcb_data()

    print("\nLoaded PCB Data:")
    print(data)

    run_python_scheduler()

    print("\nSimulation Complete.")

if __name__ == "__main__":
    main()
