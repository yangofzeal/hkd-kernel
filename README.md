# ⚡ HKD Kernel — Exact Sparse / Incremental Computation

**~18,000× measured mean speedup on the included sparse/incremental benchmark suite.**

HKD Kernel is an exact computation engine for workloads in which structure persists while a comparatively small part of the state changes. It is designed to reduce unnecessary recomputation while preserving exact results.

The same architecture can also be used for **structured mathematical optimization**: linear objectives, linear constraints, bounds, scheduling/assignment formulations, exact-cover workloads, logistics, and related problems.

> **Benchmark note:** speedup is workload-dependent. The ~18,000× figure is a measured mean for the repository's benchmark workload population; it is not a claim that every program or every optimization model runs 18,000× faster.

## What HKD Kernel targets

HKD is most relevant when one or more of the following are true:

- the problem is sparse;
- most state persists between evaluations;
- only a small dirty set changes;
- repeated recomputation dominates runtime;
- the workload has exploitable dependency structure;
- an optimization model has structured variables and constraints;
- exact agreement with the reference computation is required.

Representative applications include:

- mathematical optimization;
- scheduling and assignment;
- logistics and recovery planning;
- exact cover;
- graph closure and dependency propagation;
- incremental simulation;
- repeated sparse numerical computation;
- persistent-state computation.

## Mathematical optimization

HKD Kernel supports optimization-oriented workflows. A representative linear model is

```text
minimize    cᵀx

subject to  Ax ≥ b
            Gx ≤ h
            0 ≤ x ≤ u
```

The model data consist of an objective vector, constraint matrices and right-hand sides, and variable bounds. A completed job returns the solution vector and objective information.

If you already work with mathematical-programming systems such as CPLEX, Gurobi, HiGHS, SCIP, or modeling environments such as AMPL, the useful mental model is:

**HKD is an additional computation/optimization engine, not a claim of universal feature-for-feature replacement.**

Mature general-purpose solvers support broad families of LP, MIP, quadratic, conic, constraint-programming, presolve, diagnostic, and numerical features. HKD's strongest case is where the submitted problem lies in its supported model class or where sparse/incremental structure permits it to avoid work that a conventional full recomputation would repeat.

See [BEGINNERS_GUIDE.md](assets/BEGINNERS_GUIDE.md) for an optimization-oriented quickstart.

## Incremental computation

For persistent workloads, the central idea is simple:

```text
persistent state
      +
changed inputs / dirty leaves
      ↓
HKD dependency update
      ↓
exact updated result
```

Instead of treating every request as an unrelated cold computation, HKD can use the structure of the existing state and the identity of the changed inputs.

This is the source of the large speedups seen in workloads where the dirty set is much smaller than the total state.

## APIs and service ports

The deployment separates computation from business/fulfillment infrastructure:

- **`:8443`** — computation/API traffic.
- **`:8450`** — business, entitlement, status, and fulfillment infrastructure.

Applications performing computation should use the computation API rather than treating the fulfillment service as the numerical execution endpoint.

## Quickstart

Start with:

**[HKD Kernel Quickstart — Optimization and Incremental Computation](assets/BEGINNERS_GUIDE.md)**

It covers:

- the optimization model and its data;
- objective coefficients, constraints, and bounds;
- submitting and inspecting an optimization job;
- interpreting the returned solution;
- persistent-state and `dirty_leaves` semantics;
- submitting incremental requests with `curl` and Python;
- the distinction between computation and fulfillment services.

## Benchmarking

Performance claims should be evaluated on the workload that matters to you.

For an incremental workload, measure at least:

```text
cold/reference execution time
HKD update execution time
dirty-set size
total-state size
exact-result equality
```

For optimization, additionally record:

```text
model class
number of variables
number of constraints
sparsity
objective value
feasibility
reference-solver result
wall-clock time
```

A speedup is useful only when the compared computations solve the same problem to the required correctness standard.

## Scope

HKD Kernel does **not** replace XNU, modify CPU microcode, disable SIP, or change processor ALU hardware. Its acceleration comes from computation architecture: exploiting structure, sparsity, persistence, and incremental state so that unnecessary work can be avoided.

## Documentation

- [Beginner's Guide / Optimization Quickstart](assets/BEGINNERS_GUIDE.md)
- Repository benchmarks and examples provide the reproducible basis for performance evaluation.

HKD Kernel is intended to be measured, reproduced, and compared against conventional computation on real workloads.
