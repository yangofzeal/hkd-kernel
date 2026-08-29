# HKD Kernel Quickstart

This tutorial solves one optimization problem from start to finish.

## 1. Check the service

curl https://spindrop.com:8450/hkd-kernel/status

## 2. Download the example

tutorial_submit_job.py

## 3. The model

minimize c^T x

subject to:
Ax >= b
Gx <= h
0 <= x <= u

## 4. Run it

python3 tutorial_submit_job.py

## 5. Expected output

...

## 6. What the script does

1. Constructs c, A, b, G, h and bounds.
2. Creates an HKD job.
3. Submits it.
4. Waits for completion.
5. Prints the optimal objective and x.

## 7. Use your own model

Replace c, A, b, G, h and bounds with your data.
