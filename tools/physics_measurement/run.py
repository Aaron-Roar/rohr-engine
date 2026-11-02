#!/usr/bin/env python3
"""Serial, interrupt-forwarding measurement runner; never changes engine settings."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import signal
import subprocess
import tempfile
import time


def fields(line):
    return dict(re.findall(r"(\w+)=([^ ]+)", line))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("output", type=Path, help="New directory; existing evidence is never overwritten")
    parser.add_argument("--repetitions", type=int, default=1)
    parser.add_argument("--profile", action="store_true")
    parser.add_argument("--rendered", action="store_true", help="Retain the real display environment")
    parser.add_argument("--controlled", action="store_true", help="Operator confirms AC power, streaming closed and no concurrent builds")
    parser.add_argument("--arguments", nargs=argparse.REMAINDER, default=[])
    args = parser.parse_args()
    if args.repetitions < 1:
        parser.error("repetitions must be positive")
    if args.rendered and os.environ.get("SDL_VIDEODRIVER") in ("dummy", "offscreen"):
        parser.error("rendered measurements require a real display driver")
    binary = args.binary.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    active = None
    stopped = False

    def interrupt(number, _frame):
        nonlocal stopped
        stopped = True
        if active is not None:
            active.send_signal(number)

    signal.signal(signal.SIGINT, interrupt)
    signal.signal(signal.SIGTERM, interrupt)
    environment = dict(os.environ, ROHR_MEASUREMENT_PROFILE="1" if args.profile else "0",
                       ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1")
    if not args.rendered:
        environment.update(SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", SDL_RENDER_DRIVER="software")
    command = [str(binary), *args.arguments]
    with binary.open("rb") as stream:
        binary_hash = hashlib.file_digest(stream, "sha256").hexdigest()
    document = {"schema_version": 1, "command": command, "binary_sha256": binary_hash,
                "profile": args.profile, "rendered_requested": args.rendered,
                "platform": platform.platform(),
                "display_environment": {key: environment.get(key) for key in ("SDL_VIDEODRIVER", "SDL_RENDER_DRIVER", "WAYLAND_DISPLAY", "DISPLAY")},
                "controlled_operator_confirmation": args.controlled,
                "acceptance": "not_established_by_runner", "runs": []}
    with tempfile.TemporaryDirectory(prefix="rohr_measurement_") as working:
        for repetition in range(1, args.repetitions + 1):
            if stopped:
                break
            log_path = output / f"run_{repetition}.log"
            trace_path = output / f"run_{repetition}.csv"
            environment["ROHR_MEASUREMENT_TRACE"] = str(trace_path)
            power = Path("/sys/class/power_supply/ACAD/online")
            ac = power.read_text().strip() if power.exists() else "unknown"
            if args.controlled and ac == "0":
                raise SystemExit("Controlled measurement refused: AC power disconnected")
            started = time.monotonic()
            with log_path.open("x") as log:
                active = subprocess.Popen(command, cwd=working, env=environment,
                                          stdout=log, stderr=subprocess.STDOUT)
                code = active.wait()
                active = None
            text = log_path.read_text(errors="replace")
            lines = text.splitlines()
            stages = [fields(line) for line in lines if line.startswith("measurement_stage ")]
            totals = [fields(line) for line in lines if line.startswith("measurement_total ")]
            endpoints = [fields(line) for line in lines if line.startswith("completion ")]
            rows = []
            if trace_path.exists():
                with trace_path.open() as stream:
                    rows = list(csv.DictReader(stream))
            # Trace comparison excludes timing; counts, observer duration and trajectories remain exact.
            trajectory = hashlib.sha256()
            for row in rows:
                trajectory.update(json.dumps({k: v for k, v in row.items() if k != "wall_ns"}, sort_keys=True).encode())
            run = {"repetition": repetition, "returncode": code, "elapsed_seconds": time.monotonic()-started,
                   "ac_online_at_start": ac, "ac_online_at_end": power.read_text().strip() if power.exists() else "unknown", "interrupted": stopped,
                   "log": log_path.name, "trace": trace_path.name if trace_path.exists() else None,
                   "trace_updates": len(rows), "trajectory_work_sha256": trajectory.hexdigest() if rows else None,
                   "endpoints": endpoints, "all_declared_endpoints_reached": bool(endpoints) and all(e.get("reached") == "1" for e in endpoints),
                   "passed_cases": sum(line.startswith("PASS ") for line in lines),
                   "failed_cases": sum(line.startswith("FAIL ") for line in lines),
                   "stages": stages, "totals": totals,
                   "summary_lines": [line for line in lines if line.startswith(("benchmark ", "physics_cpu_ms ", "frame_work_ms ", "render_work_ms ", "result ", "worst ", "FAIL ", "schedule_lag_ms ", "verdict="))]}
            run["trace_valid"] = all(r["finite"] == "1" and r["fatal"] == "0" and r["attempted"] == r["completed"] == r["observed"] and r.get("report_loss", "0") == "0" for r in rows) if rows else None
            run["accounting_valid"] = bool(totals) and all(t.get("accounting_valid") == "1" for t in totals)
            run["work_accounting_valid"] = bool(totals) and all(t.get("lost_report_updates", "0") == "0" and
                (t.get("enabled") != "1" or (t.get("measured_candidates") == t.get("candidates") and
                 t.get("measured_contacts") == t.get("contacts"))) for t in totals)
            document["runs"].append(run)
            (output / "results.json").write_text(json.dumps(document, indent=2)+"\n")
            print(json.dumps({k: run[k] for k in ("repetition", "returncode", "elapsed_seconds", "all_declared_endpoints_reached", "trace_updates", "trace_valid", "accounting_valid")}), flush=True)
            # A failed numerical/performance verdict does not cancel later repetitions.
    return 130 if stopped else 0


if __name__ == "__main__":
    raise SystemExit(main())
