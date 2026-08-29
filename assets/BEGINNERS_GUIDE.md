# HKD Kernel Beginner's Guide

## From a Real Problem to an HKD Job

HKD Kernel is an exact sparse/incremental computation engine. It is designed for workloads in which a large state or model already exists and only a relatively small part changes between computations.

The central idea is simple:

> **Do not recompute unchanged work. Identify the changed leaves, submit those changes, and let HKD update the affected computation.**

This guide explains the execution model, the API boundary, `dirty_leaves`, request submission with `curl` and Python, response interpretation, and how to decide whether a workload is a good fit for HKD.

---

## 1. When to use HKD Kernel

HKD is most useful when all three of the following are true:

1. You have a computation over a persistent state, graph, tree, dependency structure, optimization model, or similar structured workload.
2. Successive computations differ in only part of that state.
3. You need the updated result to remain consistent with the corresponding full computation.

Typical examples include:

- incremental graph closure and dependency propagation
- optimization models with changing coefficients, bounds, constraints, or active data
- scheduling and assignment systems
- exact-cover and combinatorial search workloads
- risk, pricing, and portfolio calculations where only some inputs change
- persistent scientific or engineering computations
- build/dependency systems and other sparse-update workloads

HKD is not a claim that every arbitrary program becomes faster. The advantage comes from exploiting **incrementality and sparsity** when the workload actually contains them.

---

## 2. The computation model

Suppose a full computation depends on a set of leaves:

```text
L0  L1  L2  L3  L4  L5  ...  Ln
 \  /     \  /     \       /
 intermediate dependency state
              |
           result
```

A conventional implementation may rebuild a large portion of this computation after an input changes.

HKD instead maintains persistent computation state. If only `L2` and `L5` changed, the update can be expressed as:

```json
{
  "dirty_leaves": [2, 5]
}
```

HKD then updates the dependency paths affected by those leaves.

The important quantity is therefore not merely the total problem size. It is also the size of the **changed set**.

---

## 3. `dirty_leaves`

`dirty_leaves` identifies the leaves whose values or state changed since the previous materialized computation.

For example:

```json
{
  "dirty_leaves": [17]
}
```

means leaf 17 changed.

Likewise:

```json
{
  "dirty_leaves": [17, 44, 91]
}
```

means three leaves changed.

A leaf should be marked dirty when the underlying input represented by that leaf has changed and dependent state must be updated.

Do not mark every leaf dirty unless every leaf really changed. Doing so removes the sparse-update advantage.

### Mapping application data to leaves

Your application needs a stable mapping between application objects and HKD leaf identifiers.

For example:

```text
Customer 1001     -> leaf 0
Customer 1002     -> leaf 1
Warehouse A       -> leaf 2
Warehouse B       -> leaf 3
Route 44          -> leaf 4
...
```

If `Warehouse B` changes, your application submits leaf `3` as dirty.

The same principle applies to variables, constraints, graph nodes, records, physical objects, or any other application-specific state.

---

## 4. API ports

The public services have different responsibilities.

### Computation API — port 8443

Use port `8443` for HKD computation requests.

Example endpoint:

```text
https://spindrop.com:8443/v1/closure/update
```

### Business fulfillment — port 8450

Port `8450` is used for business/fulfillment functions associated with HKD Kernel.

It should not be treated as the primary computation endpoint.

In short:

```text
8443 = computation
8450 = business fulfillment
```

---

## 5. First request with `curl`

A minimal sparse-update request can be submitted with `curl`:

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H 'Content-Type: application/json' \
  -d '{"dirty_leaves":[1,4,9]}'
```

A successful response is JSON and may contain fields such as:

```json
{
  "status": "success",
  "touched_nodes": 9,
  "elapsed_ms": 0.021,
  "exact_state_match": true
}
```

The exact response fields can vary with the service/version, but the important concepts are:

- `status` — whether the request completed successfully
- `touched_nodes` — how much dependency state was affected
- `elapsed_ms` — server-side elapsed time reported for the operation
- `exact_state_match` — when supplied by the endpoint, whether the updated state matched the reference state used for verification

Do not interpret `elapsed_ms` as Internet round-trip latency. It is a server-side measurement; total client wall time also includes networking, TLS, proxying, and serialization.

---

## 6. The same request from Python

HKD does not require a specialized client library for a basic HTTP request.

Using the Python standard library:

```python
import json
import urllib.request

url = "https://spindrop.com:8443/v1/closure/update"

payload = {
    "dirty_leaves": [1, 4, 9]
}

body = json.dumps(payload).encode("utf-8")

request = urllib.request.Request(
    url,
    data=body,
    headers={"Content-Type": "application/json"},
    method="POST",
)

with urllib.request.urlopen(request, timeout=30) as response:
    result = json.loads(response.read().decode("utf-8"))

print(result)
```

Applications can wrap this operation in their own state-management layer so that changes to application objects automatically produce the corresponding `dirty_leaves`.

---

## 7. A concrete example

Assume a dependency calculation contains 100,000 logical leaves.

During one update, only these records change:

```text
record_172
record_819
record_44102
```

Your application maps them to:

```text
record_172   -> leaf 172
record_819   -> leaf 819
record_44102 -> leaf 44102
```

The request becomes:

```json
{
  "dirty_leaves": [172, 819, 44102]
}
```

The key point is that the application describes **what changed**, rather than asking the system to blindly rebuild all 100,000 logical inputs.

This is the sparse/incremental execution pattern that HKD is intended to exploit.

---

## 8. Using HKD with optimization models

Optimization users can think about HKD as an incremental computation layer rather than as a requirement to abandon familiar mathematical modeling concepts.

A conventional model might be written as:

```text
minimize     c^T x

subject to   A x >= b
             G x <= h
             0 <= x <= u
```

Between solves, perhaps only a small number of quantities change:

```text
b[17] changed
h[203] changed
u[51] changed
```

A conventional workflow may reconstruct or reconsider substantial model state.

An HKD-oriented workflow instead asks:

```text
Which persistent model objects changed?
Which leaves represent those objects?
Which dependent computation must be refreshed?
```

For example:

```text
constraint 17 -> leaf 417
constraint 203 -> leaf 603
bound 51 -> leaf 1051
```

and therefore:

```json
{
  "dirty_leaves": [417, 603, 1051]
}
```

HKD's sparse-update mechanism and an optimization solver are conceptually separate layers. Whether HKD replaces, accelerates, or complements a conventional solver depends on the specific workload and integration. Benchmark that combination against the full baseline that matters to your application.

---

## 9. Persistent state matters

Incremental computation only works if the system knows what state the new update is relative to.

A useful mental model is:

```text
initial state
     |
materialize
     |
persistent HKD state
     |
small change
     |
dirty leaves
     |
incremental update
     |
new persistent HKD state
```

If an application discards all persistent state and starts an unrelated computation from scratch every time, it should not expect the same benefit as a workload with repeated sparse updates.

---

## 10. Benchmarking correctly

When evaluating HKD, compare equivalent results.

A useful benchmark should record at least:

```text
problem size
number of dirty leaves
baseline full-computation time
HKD update time
result equality / verification status
number of repetitions
hardware and software environment
```

Then compute:

```text
speedup = baseline_time / hkd_time
```

For example:

```text
baseline: 1.800 s
HKD:      0.000100 s

speedup = 18,000x
```

That number is meaningful only for the workload and benchmark methodology that produced it.

The repository's approximately `18,000x` headline is a benchmark summary, not a guarantee that every application will receive an `18,000x` speedup.

For production evaluation, benchmark your own workload.

---

## 11. Good and poor fits

### Strong candidates

HKD is worth evaluating when:

```text
large persistent state
+ relatively small updates
+ repeated recomputation
+ dependency locality
+ exact or verifiable results
```

### Poor candidates

HKD may provide little benefit when:

```text
everything changes every time
or
there is no reusable state
or
the computation is already trivial
or
the workload cannot expose changed dependencies
```

This distinction is important. HKD targets unnecessary recomputation; it does not eliminate computation that genuinely must be performed.

---

## 12. Integration pattern

A production application will commonly use a structure like this:

```text
application
    |
    +-- maintains domain state
    |
    +-- detects changed objects
    |
    +-- maps objects -> HKD leaves
    |
    +-- submits dirty_leaves
    |
HKD computation API :8443
    |
    +-- updates affected dependency state
    |
    +-- returns result/status
    |
application consumes updated result
```

Keep the object-to-leaf mapping stable and explicit. That makes sparse updates reproducible and debuggable.

---

## 13. Error handling

Production clients should handle at least:

- connection failures
- request timeouts
- non-2xx HTTP responses
- malformed JSON
- invalid leaf identifiers
- service-side error responses
- loss or reset of persistent state

A simple Python client should therefore catch network and decoding exceptions rather than assuming every request succeeds.

Also distinguish:

```text
server computation time
```

from:

```text
end-to-end client latency
```

when measuring performance.

---

## 14. Verification

For exact workloads, performance is useful only if the updated answer is correct.

When developing an integration, periodically compare the HKD incremental result with a known full/reference computation.

A validation loop can look like:

```text
1. Materialize reference state.
2. Apply a controlled change.
3. Submit the corresponding dirty leaves.
4. Compute the full reference answer independently.
5. Compare results.
6. Record both correctness and timing.
```

This is also the recommended way to establish a defensible benchmark for a new workload.

---

## 15. Moving an existing application to HKD

A practical migration sequence is:

1. Identify the expensive computation.
2. Determine whether successive executions share substantial state.
3. Identify exactly what changes between executions.
4. Assign stable leaf identifiers to those changing objects.
5. Establish a full/reference implementation for correctness checks.
6. Materialize the initial HKD state.
7. Submit subsequent changes as `dirty_leaves`.
8. Verify incremental results against the reference.
9. Measure end-to-end and server-side performance separately.
10. Optimize the integration only after correctness is established.

This process works whether the surrounding application is an optimizer, scheduler, graph system, scientific model, risk engine, database-related workload, or another incremental computation.

---

## 16. Quick reference

### Computation endpoint

```text
https://spindrop.com:8443/v1/closure/update
```

### Minimal request

```json
{
  "dirty_leaves": [1, 4, 9]
}
```

### `curl`

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H 'Content-Type: application/json' \
  -d '{"dirty_leaves":[1,4,9]}'
```

### Core rule

```text
Tell HKD what changed.
Do not recompute what did not change.
Verify the updated result against the appropriate reference.
```

---

## 17. Where to go next

Once the basic update works:

- map real application objects to stable leaf identifiers
- automate dirty-set generation
- validate against your existing full computation
- benchmark representative update sizes
- measure both sparse-update speed and end-to-end latency
- integrate the API into the application's normal update path

HKD Kernel is easiest to evaluate with a real repeated computation and a clearly defined set of changing inputs. Start with one representative workload, establish correctness, and then measure the benefit of sparse incremental execution.
