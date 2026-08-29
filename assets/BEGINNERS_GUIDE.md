# HKD Kernel Quickstart
## Mathematical Optimization and Incremental Computation

This guide describes the HKD Kernel job model, shows a complete linear-optimization workflow, and then describes the incremental-update interface.

The presentation follows the conventions used in mathematical-programming documentation: define the model, map it to data, submit the problem, inspect solver status, and validate the returned solution.

---

## 1. Service architecture

An HKD deployment separates computation from business and fulfillment functions.

| Service | Purpose |
|---|---|
| `:8443` | Computation/API requests |
| `:8450` | Status, entitlement, business, and fulfillment functions |

A deployment status check may be exposed through the fulfillment service:

```bash
curl https://spindrop.com:8450/hkd-kernel/status
```

For numerical work, use the computation endpoint configured for the deployment.

---

# Part I — Mathematical Optimization

## 2. Model form

Consider the bounded linear optimization problem

```text
minimize    cᵀx

subject to  Ax ≥ b
            Gx ≤ h
            0 ≤ x ≤ u
```

where:

| Symbol | Meaning |
|---|---|
| `x` | decision-variable vector |
| `c` | objective coefficients |
| `A` | coefficient matrix for lower-bound constraints |
| `b` | right-hand side for `Ax ≥ b` |
| `G` | coefficient matrix for upper-bound constraints |
| `h` | right-hand side for `Gx ≤ h` |
| `u` | variable upper bounds |

This representation covers a useful class of continuous linear models and provides a direct representation for many resource-allocation and planning problems.

The precise model classes supported by a particular HKD build should be treated as part of that build's interface. Do not infer support for integer, quadratic, conic, or other model classes solely from the linear interface described here.

## 3. Example model

Suppose a planner chooses quantities `x1` and `x2` and minimizes

```text
3 x1 + 5 x2
```

subject to

```text
2 x1 + 1 x2 ≥ 8
1 x1 + 3 x2 ≥ 9

x1 ≤ 10
x2 ≤ 10

x1, x2 ≥ 0
```

The corresponding data are

```text
c = [3, 5]

A = [
    [2, 1],
    [1, 3]
]

b = [8, 9]

G = [
    [1, 0],
    [0, 1]
]

h = [10, 10]

lower_bounds = [0, 0]
upper_bounds = [10, 10]
```

The distinction between the mathematical model and its serialized representation is intentional. Applications should generate model data from their own domain objects rather than constructing strings containing algebra.

## 4. Python submission program

The repository example `tutorial_submit_job.py` is the starting point for programmatic submission.

Run it with:

```bash
python3 tutorial_submit_job.py
```

Conceptually, a client performs the following operations:

```python
model = {
    "c": [3.0, 5.0],
    "A": [
        [2.0, 1.0],
        [1.0, 3.0],
    ],
    "b": [8.0, 9.0],
    "G": [
        [1.0, 0.0],
        [0.0, 1.0],
    ],
    "h": [10.0, 10.0],
    "lower_bounds": [0.0, 0.0],
    "upper_bounds": [10.0, 10.0],
}

# Submit `model` using the HKD client or HTTP interface
# configured for the deployment.
```

Use the exact field names expected by the installed HKD client/server version. The example above documents the mathematical mapping; `tutorial_submit_job.py` is the executable reference for the repository version.

## 5. Solution information

A completed optimization job should provide enough information to determine:

```text
solver/job status
objective value
decision vector x
feasibility or verification status, when exposed
timing information, when exposed
```

A typical application should not use the decision vector without first checking successful completion.

Conceptually:

```python
result = submit(model)

if result["status"] != "success":
    raise RuntimeError(result)

x = result["x"]
objective = result["objective"]
```

Use the actual response keys returned by the deployed version.

## 6. Validate the solution

For a returned vector `x`, validate the constraints independently when correctness matters.

For the model

```text
Ax ≥ b
Gx ≤ h
0 ≤ x ≤ u
```

check:

```python
def dot(row, x):
    return sum(a * v for a, v in zip(row, x))

assert all(dot(row, x) >= rhs - 1e-9
           for row, rhs in zip(A, b))

assert all(dot(row, x) <= rhs + 1e-9
           for row, rhs in zip(G, h))

assert all(v >= -1e-9 for v in x)
assert all(v <= ub + 1e-9
           for v, ub in zip(x, upper_bounds))
```

The tolerance used for floating-point models should be selected according to the numerical requirements of the application and the solver interface.

## 7. Migrating an existing optimization model

For an existing LP-style model, migration consists primarily of extracting:

```text
objective coefficients  → c
>= constraint matrix    → A
>= right-hand sides     → b
<= constraint matrix    → G
<= right-hand sides     → h
variable lower bounds
variable upper bounds
```

Equality constraints can be represented as two inequalities when the interface and numerical requirements make that appropriate:

```text
aᵀx = r

becomes

aᵀx ≥ r
aᵀx ≤ r
```

Before moving a production model, confirm that all required features are supported. Examples requiring explicit compatibility checks include:

- integer and binary variables;
- SOS constraints;
- quadratic objectives or constraints;
- indicator constraints;
- conic constraints;
- callbacks;
- warm starts;
- infeasibility diagnostics;
- solver-specific tolerances and controls.

HKD should be benchmarked against the solver currently used for the workload rather than assumed to be a feature-equivalent replacement.

## 8. Benchmarking against a conventional solver

Use identical model data and record:

```text
variables
constraints
nonzero coefficients
model class
objective value
feasibility
reference solver time
HKD time
speedup
```

For minimization, objective equality alone is not sufficient evidence of equivalence; verify feasibility as well.

For repeated optimization, distinguish between:

1. **cold solve** — the first solution of the complete model;
2. **incremental solve/update** — a subsequent computation after a limited change.

That distinction is particularly important for HKD because persistent and sparse changes are a primary target workload.

---

# Part II — Incremental Computation

## 9. Persistent-state model

A conventional implementation may recompute an entire dependency graph whenever an input changes.

HKD instead exposes the changed portion of the state:

```text
existing persistent state
        +
dirty leaves
        ↓
affected dependency closure
        ↓
updated exact state
```

The application identifies the changed leaves; HKD determines or executes the corresponding update work according to the deployed computation model.

## 10. `dirty_leaves`

`dirty_leaves` identifies inputs whose values or dependent state must be reconsidered for the current update.

Conceptually:

```json
{
  "dirty_leaves": [12, 47, 91]
}
```

The important performance quantity is generally not just the number of dirty leaves but the amount of dependency state reachable from them.

If all or most leaves change, the workload may approach a cold recomputation and the incremental advantage can shrink accordingly.

## 11. HTTP submission

A computation request can be submitted to the configured HKD computation endpoint. For example, if the deployment exposes `/v1/closure/update` on port `8443`:

```bash
curl -X POST \
  https://spindrop.com:8443/v1/closure/update \
  -H 'Content-Type: application/json' \
  -d '{
        "dirty_leaves": [12, 47, 91]
      }'
```

Production deployments may require authentication, client identification, additional state, or a different endpoint path. Use the server's configured API contract.

## 12. Python HTTP client

The same request can be issued from Python:

```python
import json
import urllib.request

url = "https://spindrop.com:8443/v1/closure/update"

payload = json.dumps({
    "dirty_leaves": [12, 47, 91]
}).encode("utf-8")

request = urllib.request.Request(
    url,
    data=payload,
    headers={"Content-Type": "application/json"},
    method="POST",
)

with urllib.request.urlopen(request) as response:
    result = json.loads(response.read().decode("utf-8"))

print(result)
```

Add the authentication or client headers required by the deployment.

## 13. Interpreting an incremental result

Depending on the server version, a response may include fields describing:

```text
status
number of touched/recalculated nodes
elapsed computation time
exact-state verification
billing or metering information
```

Treat the deployed API response as authoritative. In particular, do not derive correctness from timing or touched-node counts.

For benchmark runs, retain both the HKD result and a reference recomputation so exactness can be checked independently.

---

# Part III — Production Use

## 14. Choosing between cold and incremental execution

Use a cold/reference computation when:

- no persistent state exists;
- the model topology has changed incompatibly;
- a full verification pass is required;
- the dirty set is effectively the complete state.

Use an incremental update when:

- prior state is valid;
- the model/dependency topology remains compatible;
- the changed inputs can be identified;
- the affected region is expected to be substantially smaller than the full state.

## 15. Error handling

Production clients should distinguish at least:

```text
transport failure
authentication/entitlement failure
invalid model
infeasible model
unbounded model, if applicable
computation failure
successful completion
```

Do not treat an HTTP success code alone as proof that an optimization problem has an optimal solution. Inspect the job/solver status returned by the computation service.

## 16. Reproducible performance evaluation

For each benchmark, retain:

```text
input/model hash
HKD build/version
reference implementation or solver/version
hardware and operating system
cold/reference wall time
HKD wall time
dirty-set size
total problem size
returned objective/result
independent correctness check
```

Report distributions across repeated runs where practical rather than relying on a single timing sample.

For incremental benchmarks, a useful summary is:

```text
speedup = reference_recomputation_time / HKD_update_time
```

The result should always be reported together with problem size and dirty-set size.

## 17. Relationship to CPLEX, Gurobi, HiGHS, SCIP, and AMPL

HKD occupies overlapping territory with mathematical optimization systems when an HKD job directly solves the required model.

It should not be interpreted as claiming universal compatibility with every feature of a mature general-purpose solver. Existing optimization systems provide large collections of algorithms, model classes, presolve transformations, diagnostics, callbacks, tuning controls, and numerical facilities.

For an existing optimization application, the practical evaluation procedure is:

```text
1. Identify the model class and required solver features.
2. Express the supported model in HKD form.
3. Solve the identical instance with the existing solver and HKD.
4. Verify feasibility and objective agreement.
5. Measure cold performance.
6. Modify a controlled subset of the problem.
7. Measure incremental performance where applicable.
8. Repeat across representative production instances.
```

That procedure establishes whether HKD is a useful replacement, accelerator, or complementary engine for the particular workload.

## 18. Next steps

Run the repository's executable example:

```bash
python3 tutorial_submit_job.py
```

Then substitute your own objective, matrices, right-hand sides, and bounds.

For incremental workloads, begin with a small known dirty set and compare the returned state against a complete reference recomputation.

The appropriate criterion for adopting HKD is straightforward: **same required result, reproducibly less computation on the workload that matters.**
