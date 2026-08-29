# Examples: From a Real Problem to an HKD Job

This section is deliberately written for beginners.

You do **not** need to understand HKD internals before using the service.

The basic workflow is:

```text
1. Start with a real problem
        ↓
2. Figure out what changed
        ↓
3. Represent the changed pieces as HKD nodes
        ↓
4. Put those node numbers into JSON
        ↓
5. POST the JSON to HKD
        ↓
6. Read the result
```

The most important idea is:

> **Tell HKD what changed instead of asking your program to recompute everything.**

---

## Example 1: One Value Changed

Imagine that your application maintains a calculation built from thousands of pieces of data.

For a very small example, pretend we have this:

```text
                 FINAL RESULT
                      |
              +-------+-------+
              |               |
           TOTAL A          TOTAL B
          /      \          /      \
       item 1   item 2   item 3   item 4
```

Suppose the program has already calculated everything.

Now:

```text
item 3 changes
```

A simple program might calculate the entire tree again:

```text
item 1  ─┐
item 2  ─┼── recalculate everything
item 3  ─┤
item 4  ─┘
```

But only `item 3` changed.

HKD's job is to exploit that fact.

Conceptually:

```text
item 3 changed
     |
     v
TOTAL B may have changed
     |
     v
FINAL RESULT may have changed
```

Meanwhile:

```text
item 1 = unchanged
item 2 = unchanged
TOTAL A = unchanged
```

HKD can reuse that unchanged state.

---

## Step 1 — Identify the Thing That Changed

Assume our application has assigned these node IDs:

```text
item 1 = node 1
item 2 = node 2
item 3 = node 3
item 4 = node 4
```

Our application just learned:

```text
item 3 changed
```

Therefore the dirty node is:

```text
3
```

In HKD terminology, this is a **dirty leaf**.

"Dirty" does not mean something is broken.

It simply means:

> **This value changed, so anything depending on it may need to be updated.**

---

## Step 2 — Translate the Change into an HKD Job

HKD accepts JSON.

For this example, our JSON job is:

```json
{
  "dirty_leaves": [3]
}
```

Read that as plain English:

```text
dirty_leaves = the input nodes that changed

[3] = node number 3 changed
```

That is the complete idea.

We are **not** saying:

```text
recompute everything
```

We are saying:

```text
node 3 changed;
update the state affected by node 3
```

---

## Step 3 — Submit the Job

Using `curl`:

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H "Content-Type: application/json" \
  -d '{
    "dirty_leaves": [3]
  }'
```

Let's break that command down.

This:

```bash
curl -X POST
```

means:

```text
send data to a server
```

This:

```text
https://spindrop.com:8443/v1/closure/update
```

is the HKD computation endpoint.

This:

```bash
-H "Content-Type: application/json"
```

tells the server:

```text
I am sending JSON.
```

And this:

```bash
-d '{
  "dirty_leaves": [3]
}'
```

is the actual HKD job.

---

## Step 4 — Read the Result

A successful response looks conceptually like:

```json
{
  "status": "success",
  "touched_nodes": 3,
  "elapsed_ms": 0.012,
  "exact_state_match": true
}
```

Here is what those fields mean.

### `status`

```json
"status": "success"
```

The HKD request completed successfully.

### `touched_nodes`

```json
"touched_nodes": 3
```

This reports how much state the update path touched.

The important idea is that HKD attempts to work on the affected part of the state instead of blindly recomputing the entire problem.

### `elapsed_ms`

```json
"elapsed_ms": 0.012
```

This is the server-side elapsed time for the operation, in milliseconds.

Do not assume your own result will be exactly this number.

It depends on the machine, workload, state size, and update.

### `exact_state_match`

```json
"exact_state_match": true
```

This is especially important.

It means the incremental result agrees with the expected exact state check used by the service.

HKD is intended to save unnecessary work without intentionally trading correctness for approximation.

---

# Example 2: Several Things Changed

Now suppose three inputs changed:

```text
node 3 changed
node 17 changed
node 81 changed
```

Do not send three separate requests unless your application specifically needs to.

You can describe the changed set together:

```json
{
  "dirty_leaves": [3, 17, 81]
}
```

Submit it:

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H "Content-Type: application/json" \
  -d '{
    "dirty_leaves": [3, 17, 81]
  }'
```

In plain English, we just told HKD:

> Nodes 3, 17, and 81 changed. Update the computation affected by those changes.

That is an HKD job.

---

# Example 3: A Beginner-Friendly Real-World Problem

Imagine a shipping company.

It has:

```text
10,000 packages
500 trucks
200 routes
many scheduling calculations
```

The company has already computed today's schedule.

Then one thing happens:

```text
Truck 127 breaks down.
```

Without incremental computation, an application might rebuild a large scheduling state from scratch.

With an HKD-style representation, the application first asks:

> **Which input represents Truck 127's availability?**

Suppose that input is represented internally as:

```text
node 8421
```

Then the problem:

```text
Truck 127 broke down
```

becomes the HKD change:

```text
dirty node = 8421
```

which becomes JSON:

```json
{
  "dirty_leaves": [8421]
}
```

and finally becomes an API call:

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H "Content-Type: application/json" \
  -d '{
    "dirty_leaves": [8421]
  }'
```

Notice what happened.

We started with something a human understands:

```text
Truck 127 broke down.
```

Then translated it into something the computation understands:

```text
truck availability state changed
```

Then into the application's node representation:

```text
node 8421 changed
```

Then into an HKD submission:

```json
{
  "dirty_leaves": [8421]
}
```

That translation step is the central skill when integrating HKD.

---

# Example 4: Calling HKD from Python

You usually will not type `curl` manually from a production application.

Your program will make the request.

Here is the same one-node example using only Python's standard library:

```python
import json
import urllib.request

url = "https://spindrop.com:8443/v1/closure/update"

job = {
    "dirty_leaves": [3]
}

body = json.dumps(job).encode("utf-8")

request = urllib.request.Request(
    url,
    data=body,
    headers={
        "Content-Type": "application/json"
    }
)

response = urllib.request.urlopen(request)

result = json.loads(
    response.read().decode("utf-8")
)

print(result)
```

The important part is only:

```python
job = {
    "dirty_leaves": [3]
}
```

Everything else is ordinary HTTP code.

---

# Example 5: Build the Job from Your Program

A real application normally already knows what changed.

For example:

```python
changed_items = [3, 17, 81]
```

Building an HKD job is simply:

```python
job = {
    "dirty_leaves": changed_items
}
```

Now serialize it:

```python
body = json.dumps(job).encode("utf-8")
```

and submit it.

A complete small example:

```python
import json
import urllib.request

changed_items = [3, 17, 81]

job = {
    "dirty_leaves": changed_items
}

request = urllib.request.Request(
    "https://spindrop.com:8443/v1/closure/update",
    data=json.dumps(job).encode("utf-8"),
    headers={
        "Content-Type": "application/json"
    }
)

with urllib.request.urlopen(request) as response:
    result = json.loads(
        response.read().decode("utf-8")
    )

print("HKD status:", result.get("status"))
print("Touched nodes:", result.get("touched_nodes"))
print("Elapsed milliseconds:", result.get("elapsed_ms"))
print("Exact:", result.get("exact_state_match"))
```

Possible output:

```text
HKD status: success
Touched nodes: 9
Elapsed milliseconds: 0.021
Exact: True
```

Again, the numbers shown here are examples.

Measure the real behavior of your own workload.

---

# How Do I Know Which Node Number to Send?

This is the part that often confuses new users.

HKD does not magically know that:

```text
8421 = Truck 127
```

Your application establishes that mapping.

Think of a node ID as a stable address for a piece of computation.

For example:

```text
node 1    = customer A balance
node 2    = customer B balance
node 3    = customer C balance

node 100  = warehouse temperature
node 101  = warehouse inventory

node 8421 = truck 127 availability
```

When your application notices a change, it sends the corresponding node ID to HKD.

So integration looks like:

```text
REAL WORLD
   |
   | "Truck 127 broke down"
   v
YOUR APPLICATION
   |
   | truck 127 -> node 8421
   v
HKD JOB
   |
   | {"dirty_leaves":[8421]}
   v
HKD
```

---

# How to Decide Whether a Problem Fits HKD

Ask these questions.

### Question 1

Is there a large computation that gets performed repeatedly?

If no, HKD may not help much.

If yes, continue.

### Question 2

Between one calculation and the next, does only a small part of the input usually change?

For example:

```text
1,000,000 values exist
but
only 10 changed
```

That is potentially interesting.

### Question 3

Can you identify which part changed?

HKD needs some way to represent the changed state.

For example:

```text
changed nodes = [7, 18, 52]
```

### Question 4

Can unchanged computation safely be reused?

If yes, the workload may be a good candidate for incremental computation.

---

# A Useful Mental Exercise

Suppose your program currently does this:

```python
while True:
    read_everything()
    calculate_everything()
    produce_result()
```

Ask:

> What actually changed since the previous iteration?

If the answer is:

```text
almost everything
```

HKD may not provide much advantage.

If the answer is:

```text
three things out of a million
```

then you have found the kind of workload HKD is designed to investigate.

Instead of:

```text
something changed
        ↓
recompute everything
```

you want to move toward:

```text
something changed
        ↓
identify changed nodes
        ↓
submit changed nodes
        ↓
update affected computation
        ↓
reuse unaffected state
```

---

# Important: Two Different HKD Services

Do not confuse the computation API with the Business fulfillment service.

### Computation API

Used to submit computational update jobs:

```text
https://spindrop.com:8443/
```

Example route:

```text
POST /v1/closure/update
```

### HKD Kernel Business fulfillment service

Used for Business licensing, registration, and package fulfillment:

```text
https://spindrop.com:8450/
```

For example:

```text
/hkd-kernel/register
/hkd-kernel/bootstrap
/hkd-kernel/status
```

**Do not submit computation jobs to the licensing server.**

Port `8443` and port `8450` have different jobs.

A simple way to remember them is:

```text
8443 = computation

8450 = Business licensing / fulfillment
```

---

# Your First HKD Experiment

If you are completely new, start with something tiny.

Pretend these nodes changed:

```text
1
5
9
```

Your first job is:

```json
{
  "dirty_leaves": [1, 5, 9]
}
```

Submit:

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H "Content-Type: application/json" \
  -d '{"dirty_leaves":[1,5,9]}'
```

Then inspect the JSON response.

Once that makes sense, move on to mapping real application state to stable node IDs.

You do not need to redesign your entire application on day one.

Start with:

```text
one problem
one changing input
one node
one HKD request
```

Verify that.

Then add complexity slowly.

