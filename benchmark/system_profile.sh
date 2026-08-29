#!/bin/sh
set -eu
echo "HKD_SYSTEM_PROFILE"
echo "date=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "uname=$(uname -a)"
echo "arch=$(uname -m)"
if [ "$(uname -s)" = "Darwin" ]; then
  echo "hw_model=$(sysctl -n hw.model 2>/dev/null || true)"
  echo "cpu_brand=$(sysctl -n machdep.cpu.brand_string 2>/dev/null || true)"
  echo "physical_cpu=$(sysctl -n hw.physicalcpu 2>/dev/null || true)"
  echo "logical_cpu=$(sysctl -n hw.logicalcpu 2>/dev/null || true)"
  echo "memory_bytes=$(sysctl -n hw.memsize 2>/dev/null || true)"
fi
