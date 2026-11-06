#!/usr/bin/env python3
# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
"""Serial, bounded experiment runner. Numerical failure is not incompletion."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--workloads', nargs='+', choices=['matrix', 'car', 'pit', 'flies'], default=['matrix'])
parser.add_argument('--modes', nargs='+', choices=['control', 'predict', 'sweep'], default=['control', 'predict', 'sweep'])
parser.add_argument('--runs', type=int, default=1)
parser.add_argument('--audit', action='store_true')
args = parser.parse_args()
active = None
stopped = False

def stop(number, _frame):
    global stopped
    stopped = True
    if active is not None:
        try:
            os.killpg(active.pid, number)
        except ProcessLookupError:
            pass

signal.signal(signal.SIGINT, stop)
signal.signal(signal.SIGTERM, stop)
commands = {'matrix': ['collision_matrix'], 'car': ['car_comparison', '--benchmark=headless', '--frames=600'],
            'pit': ['pit_comparison', '--headless', '--seconds=12'], 'flies': ['flies_comparison']}
args.output.mkdir(parents=True, exist_ok=True)
results = []
for run in range(1, args.runs + 1):
    for workload in args.workloads:
        for mode in args.modes:
            if stopped:
                raise SystemExit(130)
            directory = args.output / f'{workload}_{mode}_{run}'
            directory.mkdir()  # Never overwrite earlier evidence.
            command = commands[workload].copy()
            command[0] = str((args.build / command[0]).resolve())
            environment = dict(os.environ, ROHR_COLLISION_EXPERIMENT=mode,
                               ROHR_COLLISION_AUDIT='1' if args.audit else '0',
                               SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
            start = time.monotonic()
            capped = False
            with (directory / 'stdout.log').open('w') as log:
                active = subprocess.Popen(command, cwd='/tmp', env=environment, stdout=log,
                                          stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    code = active.wait(timeout=59)
                except subprocess.TimeoutExpired:
                    capped = True
                    os.killpg(active.pid, signal.SIGINT)
                    try:
                        code = active.wait(timeout=max(.01, 60-(time.monotonic()-start)))
                    except subprocess.TimeoutExpired:
                        os.killpg(active.pid, signal.SIGKILL)
                        code = active.wait()
            active = None
            output = (directory / 'stdout.log').read_text()
            # The car retains its inherited 60-second acceptance label. This
            # experiment explicitly declares only the ten-second prefix.
            car_prefix = re.search(r'benchmark frames=600 complete=0 .*simulated=([0-9.]+)', output)
            complete = bool(re.search(r'(matrix complete=1|result count=800 frames=720 complete=1|flies complete=1)', output))
            if workload == 'car':
                complete = code == 0 and car_prefix is not None and abs(float(car_prefix[1])-10) < .0001
            reached = complete
            sanitizer_finding = bool(re.search(r'ERROR:.*Sanitizer|runtime error:|LeakSanitizer has encountered', output))
            complete = reached and not capped and not stopped and not sanitizer_finding
            result = dict(workload=workload, mode=mode, run=run, returncode=code,
                          complete=complete, end_condition_reached=reached, capped=capped, interrupted=stopped,
                          sanitizer_finding=sanitizer_finding,
                          elapsed=time.monotonic()-start, command=command, audit=args.audit,
                          endpoint='10 simulated seconds (60-second acceptance incomplete)' if workload=='car' else 'declared full fixture endpoint',
                          executable_sha256=hashlib.sha256(Path(command[0]).read_bytes()).hexdigest())
            (directory / 'result.json').write_text(json.dumps(result, indent=2)+'\n')
            results.append(result)
            print(json.dumps(result), flush=True)
            if not complete:
                raise SystemExit('Incomplete endpoint; stopped sequence. See '+str(directory))
(args.output / 'results.json').write_text(json.dumps(results, indent=2)+'\n')
