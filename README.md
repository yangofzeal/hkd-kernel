# ⚡ Exact Sparse/Incremental Computation — ~18,000× Mean Speedup

## 🚀 Easiest Way to Use HKD Kernel

You do **not** need to understand the internals of HKD Kernel before trying it.

There are two ways to use HKD:

1. **Use the hosted HKD service** running on `spindrop.com`.
2. **Install HKD Kernel locally** and call the native C library directly.

If you are new to HKD, start with the hosted service.

---

## 🌐 Using the Hosted Service

The hosted HKD service lets your program send work to an HKD server over HTTPS.

Think of it like this:

```text
Your program
     |
     | HTTPS request
     v
spindrop.com
     |
     v
HKD
     |
     | JSON response
     v
Your program
```

You send a request.

HKD performs the operation.

You get a response.

That's it.

### Server

```text
https://spindrop.com
```

The HKD service uses HTTPS, so requests and responses are encrypted in transit.

---

## 🧠 What Does HKD Actually Do?

Suppose you have a large computation.

Normally, if one small thing changes, a program may calculate the whole thing again.

HKD is designed for workloads where that is unnecessary.

Instead:

```text
BIG COMPUTATION
      |
      +---- unchanged
      |
      +---- unchanged
      |
      +---- CHANGED  <--- recompute this part
      |
      +---- unchanged
```

HKD keeps reusable state and updates the affected portion of the computation.

The result is still checked for exactness.

This is why sparse-update workloads can be dramatically faster than repeatedly performing a full computation.

The large benchmark speedups reported in this repository apply to the benchmarked sparse/incremental workloads. They do **not** mean that arbitrary software automatically becomes that many times faster.

---

## 🔌 Calling HKD from an API Client

An API request has three basic pieces:

```text
URL
headers
JSON data
```

For example, a request conceptually looks like:

```bash
curl -X POST \
  https://spindrop.com/<HKD-ENDPOINT> \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <YOUR-API-CREDENTIAL>" \
  -d '{
    "your_input": "your_value"
  }'
```

Do not literally use `<HKD-ENDPOINT>` or `<YOUR-API-CREDENTIAL>`.

Those are placeholders.

Your actual HKD endpoint and credentials depend on the HKD service/product you are using.

---

## 🐍 Calling the API from Python

The same idea in Python is:

```python
import json
import urllib.request

url = "https://spindrop.com/<HKD-ENDPOINT>"

payload = {
    "your_input": "your_value"
}

data = json.dumps(payload).encode("utf-8")

request = urllib.request.Request(
    url,
    data=data,
    headers={
        "Content-Type": "application/json",
        "Authorization": "Bearer <YOUR-API-CREDENTIAL>"
    }
)

response = urllib.request.urlopen(request)

print(response.read().decode("utf-8"))
```

Again, replace the placeholders with the endpoint and credential issued for your service.

---

## 💳 HKD Kernel Business

HKD Kernel Business is licensed **per production node**.

The current Business price is:

```text
$999 / month / production node
```

For example:

```text
1 production node  = 1 license
3 production nodes = 3 licenses
10 production nodes = 10 licenses
```

After a successful Business purchase, the fulfillment service verifies the subscription and handles registration for the licensed node.

A node identifies itself to the licensing system, and HKD issues a signed license for that node.

The private HKD signing key stays on the licensing server.

Customers receive only the information required to verify their license.

---

## 🪪 What Is a Node?

Don't let the word "node" make this complicated.

For licensing purposes, think:

```text
one production machine = one node
```

If your company runs HKD on one production server, you need one production-node license.

If you deploy it on five production servers, you need five.

---

## 🛠️ New User Checklist

If this is your first time using HKD, go slowly:

**Step 1:** Read the examples in this repository.

**Step 2:** Run the included benchmark locally.

**Step 3:** Confirm that the exactness/correctness checks pass.

**Step 4:** Experiment with HKD on a small non-production workload.

**Step 5:** Measure whether your workload actually has the sparse/incremental structure that HKD can exploit.

**Step 6:** Only then consider integrating it into a production application.

HKD can produce very large improvements when a computation contains substantial reusable state.

It is not expected to accelerate every possible workload.

---

## 🔬 Reproduce Before You Believe

You should not have to trust a performance number in a README.

Run the benchmarks yourself.

Try different machines.

Change the workload.

Increase the number of dirty elements.

Try to find the point where incremental computation stops being advantageous.

That is why HKD Kernel includes reproducible benchmarks and exactness checks.

The goal is not:

> "Trust us. It's fast."

The goal is:

> **Run it yourself and measure it.**

---

## 🆘 Something Doesn't Make Sense?

Start small.

You do not need to understand the entire HKD architecture to experiment with it.

A useful mental model is simply:

```text
FULL RECOMPUTATION:

change 1 thing
     ↓
calculate EVERYTHING again


HKD:

change 1 thing
     ↓
find what depends on it
     ↓
update the affected state
     ↓
reuse everything else
```

That is the core idea.

## One-command macOS benchmark

Requires Xcode Command Line Tools (`clang`/`make`).

```sh
./run_all_benchmarks.sh
```

The runner builds from source and writes a timestamped result file under `benchmark/`.

## Three benchmark classes

1. **Dense arithmetic control (`bench_dense`)** — both implementations perform the same full-array summation. A huge speedup is *not* expected. This is the control against misleading CPU-speed claims.
2. **Sparse incremental aggregate (`bench_native`)** — baseline rescans N values after every mutation; HKD updates a materialized exact aggregate from the changed leaf only.
3. **Application-style portfolio refresh (`bench_app`)** — baseline fully revalues all instrument exposures after each price change; HKD changes only the affected weighted contribution. A fresh full revaluation verifies the final answer exactly.

All integer arithmetic is verified for exact final-state equality.

## Tunable sizes

```sh
HKD_DENSE_N=8000000 \
HKD_DENSE_REPS=40 \
HKD_SPARSE_N=4000000 \
HKD_SPARSE_UPDATES=200 \
HKD_APP_N=2000000 \
HKD_APP_UPDATES=300 \
./run_all_benchmarks.sh
```

For a heavier Apple Silicon run, increase N and update counts while keeping memory pressure reasonable.

## Individual commands

```sh
make
./bench_dense 8000000 40
./bench_native 4000000 200
./bench_app 2000000 300
```

## Interpretation

A large sparse/application speedup demonstrates reduced logical work for workloads that support incremental state. It does not mean arbitrary instructions or arbitrary Mac applications execute by that multiplier. The dense control is specifically included to make that distinction testable.

`TESTED_HOST_RESULTS.txt` contains the pre-packaging validation from the build environment. Those measurements are Linux/x86-64 and are **not** presented as Apple Silicon measurements. Run `./run_all_benchmarks.sh` on the Mac to obtain the Apple Silicon numbers and hardware profile.
