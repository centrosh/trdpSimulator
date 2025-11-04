# TRDP Simulator

The TRDP Simulator targets railway communication scenarios using the
TCNopen TRDP stack. This repository now includes C++ scaffolding for the
simulator runtime, continuous integration, and contributor workflows.

## Current Capabilities

- C++ communication wrapper with lifecycle management, PD publish/receive,
  MD send/receive, and structured diagnostics recorded for each action.
- Loopback stack adapter that mimics the TRDP API so the CLI and unit tests
  can exercise callback flows without network access.
- Scenario engine driving ordered PD/MD events and validating message-data
  acknowledgements before advancing.

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
4. Execute the demo CLI and provide a scenario name. You can optionally pass
   a TRDP endpoint and PD/MD events to exercise the wrapper without a network
   connection. Event arguments accept optional COM ID, dataset ID, and payload
   values (ASCII or `0x`-prefixed hex):
   ```bash
   ./build/trdp_sim_cli demo-scenario \
       --endpoint 127.0.0.1 \
       --event pd:doors-close:1001:1001:0x0102 \
       --event md:departure:2001:2001:0x7B
   ```

## Documentation

- [`docs/design.md`](docs/design.md) – architecture overview and approved
  design decisions.
- [`docs/milestones.md`](docs/milestones.md) – incremental milestones and
  user stories.
- [`docs/issue-tracker.md`](docs/issue-tracker.md) – prioritised backlog
  aligned with milestones.
- [`docs/operations-handbook.md`](docs/operations-handbook.md) – distribution,
  release, maintenance, and support procedures.
- [`docs/TCNOpen_TRDP.md`](docs/TCNOpen_TRDP.md) – guidance for adding the
  TCNOpen TRDP stack as a git submodule, building its static libraries, and
  linking them into the simulator.
- [`docs/development-next-steps.md`](docs/development-next-steps.md) – near-term
  roadmap focusing on device XML ingestion, validation, and operator tooling.

## Distribution

The simulator can be consumed either as a Docker image or as a Python package.

- **Docker:** Run `scripts/build_docker.sh <image-tag>` to produce a runtime image based on
  `docker/Dockerfile`, then push it via `scripts/publish_docker.sh <image-tag> <registry>`.
- **Python package:** Use `scripts/build_python_package.sh` to generate wheel and sdist artefacts,
  then publish with `scripts/publish_python_package.sh [repository-url]`.
