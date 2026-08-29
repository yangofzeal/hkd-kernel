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

---

## Step 3 — Understand What You Submit Today

At this point, you have successfully translated a real-world change into an HKD-style job:

```json
{
  "dirty_leaves": [3]
}
```

That is useful because it shows how to *think* about sparse/incremental work.

However, there is an important distinction for new users:

> **The public HKD Kernel service currently documented at `spindrop.com:8450` is the Business onboarding, registration, licensing, and package-fulfillment service.**

Do **not** send `dirty_leaves` JSON to the licensing service.

The old example that used:

```text
https://spindrop.com:8443/v1/closure/update
```

has been removed from this guide because that endpoint is not the public onboarding path documented here.

For a new HKD Kernel user, the safe workflow is:

```text
Understand your problem
        ↓
Identify sparse changes
        ↓
Evaluate whether HKD fits
        ↓
Get HKD Kernel
        ↓
Register/install your licensed production node
        ↓
Integrate the local HKD Kernel library
        ↓
Benchmark your real workload
```

---

# Your First Live Request: Check the HKD Service

Before doing anything complicated, make one harmless request.

Open a terminal and run:

```bash
curl https://spindrop.com:8450/hkd-kernel/status
```

This does **not** buy anything.

It does **not** register a machine.

It does **not** consume a license.

It simply asks:

> "Is the HKD Kernel Business service alive?"

A healthy response is JSON describing the service status.

The exact formatting may change, so your response does not need to look character-for-character identical to a screenshot or example.

The important field is:

```json
{
  "status": "ok"
}
```

If you see `"status": "ok"`, you have successfully communicated with the HKD Kernel service.

Congratulations — that is your first live HKD request.

---

# What Is Running on Port 8450?

The service address is:

```text
https://spindrop.com:8450
```

For beginners, think of it as the **front desk for HKD Kernel Business**.

It handles the server-side parts of onboarding such as:

```text
purchase/subscription verification
        ↓
node registration
        ↓
license issuance
        ↓
package fulfillment
```

It is **not** a generic endpoint where you invent a computation and POST arbitrary `dirty_leaves`.

That distinction matters.

---

# Important 8450 Routes

## Status

```text
GET /hkd-kernel/status
```

Full request:

```bash
curl https://spindrop.com:8450/hkd-kernel/status
```

Use this first.

It is the simplest way to verify that your computer can reach the service.

## Registration

```text
/hkd-kernel/register
```

This is part of the Business registration flow.

Registration is not the same thing as submitting a computation.

A Business customer registers an entitled production node so that HKD can issue the appropriate node-bound license.

## Bootstrap

```text
/hkd-kernel/bootstrap
```

The bootstrap flow is used during licensed installation/onboarding.

Use the bootstrap/registration information issued for your purchase rather than inventing tokens, subscription IDs, or node credentials.

## Webhook

```text
/hkd-kernel/webhook
```

This is for Stripe-to-server fulfillment events.

**Customers should not manually POST test data to this endpoint.**

It is infrastructure used by the payment/fulfillment system.

---

# Example 2: Several Things Changed

The HKD *problem representation* still applies when several things change.

Suppose your application knows that these inputs changed:

```text
node 3 changed
node 17 changed
node 81 changed
```

Conceptually, that changed set is:

```json
{
  "dirty_leaves": [3, 17, 81]
}
```

In plain English:

> Nodes 3, 17, and 81 changed. Work depending on those values may need to be updated.

This example teaches the data model.

It is **not** an instruction to POST this JSON to port 8450.

---

# Example 3: A Real-World Problem

Imagine a shipping company.

It has:

```text
10,000 packages
500 trucks
200 routes
many scheduling calculations
```

The company has already computed today's schedule.

Then:

```text
Truck 127 breaks down.
```

The first question is not:

> "Which API URL do I call?"

The first question is:

> **What part of my computation changed?**

Suppose the application represents Truck 127's availability as:

```text
node 8421
```

Then:

```text
REAL WORLD
Truck 127 broke down
        ↓
APPLICATION STATE
truck 127 availability changed
        ↓
HKD REPRESENTATION
node 8421 changed
```

Conceptually:

```json
{
  "dirty_leaves": [8421]
}
```

That translation is the important part.

HKD Kernel becomes useful when your application can identify a relatively small changed portion of a much larger reusable computation.

---

# How Do I Know Which Node Number to Use?

HKD does not magically know:

```text
8421 = Truck 127
```

Your application establishes its state/dependency representation.

Think of a node ID as a stable identifier for a piece of computation.

For example:

```text
node 1    = customer A balance
node 2    = customer B balance
node 3    = customer C balance

node 100  = warehouse temperature
node 101  = warehouse inventory

node 8421 = truck 127 availability
```

When something changes, your application knows which piece of state changed.

The important progression is:

```text
REAL WORLD
        ↓
YOUR APPLICATION'S STATE
        ↓
CHANGED PART OF THE STATE
        ↓
HKD INCREMENTAL WORK
```

---

# How to Decide Whether Your Problem Fits HKD

Ask four questions.

## Question 1 — Do you repeat a large computation?

If no, HKD may not help much.

If yes, continue.

## Question 2 — Does only a small part usually change?

For example:

```text
1,000,000 values exist

but

only 10 changed
```

That is potentially interesting.

## Question 3 — Can your program identify what changed?

For example:

```text
changed nodes = [7, 18, 52]
```

If your program can identify the changed state, an incremental approach becomes much easier to investigate.

## Question 4 — Can unchanged computation safely be reused?

If yes, the workload may be a good candidate for HKD Kernel.

If almost everything changes every time, the incremental advantage may be small or nonexistent.

---

# A Useful Mental Exercise

Suppose your program currently behaves like this:

```python
while True:
    read_everything()
    calculate_everything()
    produce_result()
```

Ask:

> **What actually changed since the previous iteration?**

If the answer is:

```text
almost everything
```

HKD may not provide much advantage.

If the answer is:

```text
three things out of a million
```

that is the type of workload worth investigating.

The goal is to move from:

```text
something changed
        ↓
recompute everything
```

toward:

```text
something changed
        ↓
identify the changed state
        ↓
update affected computation
        ↓
reuse unaffected state
```

---

# Beginner Onboarding: Go Slowly

If you are completely new to HKD Kernel, do not start by redesigning a production system.

Start here.

## Step 1 — Verify the service

Run:

```bash
curl https://spindrop.com:8450/hkd-kernel/status
```

Confirm that the service reports:

```json
"status": "ok"
```

## Step 2 — Understand your problem

Find one computation your application performs repeatedly.

Do not integrate anything yet.

Write down what the computation does.

## Step 3 — Identify what changes

Ask:

> Between two runs, which inputs actually changed?

Start with one simple case.

For example:

```text
Truck 127 changed from AVAILABLE to UNAVAILABLE.
```

## Step 4 — Identify what can be reused

Ask:

> Which parts of the previous computation are definitely unaffected by that change?

This is where the potential HKD advantage comes from.

## Step 5 — Try HKD locally

Use the HKD Kernel package and examples to experiment with a small, non-production workload.

Verify correctness first.

Then measure performance.

## Step 6 — Benchmark your real workload

Compare:

```text
FULL RECOMPUTATION
```

against:

```text
HKD INCREMENTAL UPDATE
```

Check both:

```text
correctness
performance
```

A speedup without correct output is not a successful optimization.

## Step 7 — Move to production only after verification

For HKD Kernel Business, production machines are licensed per production node.

Use the registration/bootstrap information provided through the Business fulfillment flow to install the licensed package on the entitled node.

Do not copy registration tokens or licenses between unrelated machines.

---

# What Port Should I Remember?

For this guide, remember one port:

```text
8450
```

Specifically:

```text
https://spindrop.com:8450
```

And your first test is:

```bash
curl https://spindrop.com:8450/hkd-kernel/status
```

Do **not** use the old `:8443/v1/closure/update` examples from earlier documentation.

Port `8450` is the currently documented HKD Kernel Business onboarding/fulfillment service.

---

# Your First HKD Exercise

Do these three things in order.

### 1. Check the service

```bash
curl https://spindrop.com:8450/hkd-kernel/status
```

### 2. Pick one real problem

For example:

```text
I recalculate a large dependency graph whenever one record changes.
```

### 3. Write down exactly what changes

For example:

```text
There are 1,000,000 input records.

Usually only 1–10 records change between calculations.
```

You have now done the most important first part of HKD integration:

```text
one problem
        ↓
one changing input
        ↓
identify reusable work
        ↓
test locally
        ↓
verify exactness
        ↓
measure
```

Do not worry about making your first HKD experiment impressive.

Make it **small, understandable, and correct**.

Then add complexity slowly.
