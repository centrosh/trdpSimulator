# TRDP Simulator

The TRDP Simulator targets railway communication scenarios using the
TCNopen TRDP stack. This repository now includes C++ scaffolding for the
simulator runtime, continuous integration, and contributor workflows.

## Project Structure

```
├── include/trdp_simulator/   # Public C++ headers for communication and simulation modules
├── src/                      # Library sources and CLI entry point
├── tests/                    # CTest-driven smoke tests
├── docs/                     # Architecture, milestones, and backlog documentation
├── .github/workflows/        # Continuous integration definitions
├── CONTRIBUTING.md           # Coding standards and contribution workflow
└── CMakeLists.txt            # CMake build configuration
```

## Getting Started

1. Ensure a C++20 capable toolchain and CMake 3.16+ are installed.
2. Configure and build the project:
   ```bash
   make build
   ```
3. Run the smoke tests:
   ```bash
   make test
   ```
4. Execute the demo CLI (placeholder implementation):
   ```bash
   ./build/trdp_sim_cli demo-scenario
   ```

## Documentation

- [`docs/design.md`](docs/design.md) – architecture overview and approved
  design decisions.
- [`docs/milestones.md`](docs/milestones.md) – incremental milestones and
  user stories.
- [`docs/issue-tracker.md`](docs/issue-tracker.md) – prioritised backlog
  aligned with milestones.
