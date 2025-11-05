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
- Device profile catalogue and scenario repository backed by deterministic
  manifests, schema validation, and CLI tooling for importing, listing, and
  exporting simulation assets.
- Scenario exports bundle their referenced device profiles so archived runs can
  be replayed in new environments without manually copying XML assets.
- Automation interfaces expose a REST API, browser dashboard, and Python CLI
  capable of starting, pausing, resuming, validating, and inspecting
  simulations from scripts or operators.

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
4. Register a device XML and run the bundled loopback scenario. The simulator
   copies the XML into `~/.trdp-simulator/devices`, validates it against the
   bundled schema, and persists the scenario definition in the repository for
   future runs:
   ```bash
   ./build/trdp_sim_cli \
       --device-xml resources/trdp/device1.xml \
       --scenario-file resources/trdp/loopback.yaml
   ```
   Subsequent invocations can load the persisted scenario directly by
   referencing its identifier (the CLI stores it under
   `~/.trdp-simulator/scenarios/manifest.db`):
   ```bash
   ./build/trdp_sim_cli loopback-demo
   ```
   To craft ad-hoc sequences without a YAML file, supply explicit events and an
   existing device profile identifier:
   ```bash
   ./build/trdp_sim_cli adhoc --device device1 \
       --event pd:doors-close:1001:1001:0x0102 \
       --event md:departure:2001:2001:0x7B
   ```
   Manage the catalogue without running a simulation using the new CLI
   management flags:
   ```bash
   # Import scenarios without executing them
   ./build/trdp_sim_cli --import-scenario resources/trdp/loopback.yaml --no-run

   # Lint scenario definitions against the YAML schema
   ./build/trdp_sim_cli --validate-scenario resources/trdp/loopback.yaml --no-run

   # Inspect registered scenarios and export a copy
   ./build/trdp_sim_cli --list-scenarios --no-run
   ./build/trdp_sim_cli --export-scenario loopback-demo /tmp/loopback.yaml --no-run
   ```
   Exported bundles place the scenario YAML alongside a `devices/` directory
   containing the referenced XML profiles so the catalogue can be rehydrated on
   another host.

5. Start the automation API server and exercise the automation surfaces:
   ```bash
   # Launch the automation control plane
   trdp-sim-api

   # Start a run with explicit PD messages
   trdp-sim run loopback --message 1001:door-open --message 1002:door-close

   # Pause, resume, and inspect status from separate automation steps
   trdp-sim pause <run-id>
   trdp-sim resume <run-id>
   trdp-sim status <run-id>

   # Browse the HTML dashboard
   open http://127.0.0.1:8000/ui

   # Inspect catalogue assets and validate scenarios via the CLI
   trdp-sim scenarios
   trdp-sim devices
   trdp-sim validate resources/trdp/loopback.yaml
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
- [`docs/milestone-2-plan.md`](docs/milestone-2-plan.md) – detailed plan for the
  upcoming scenario orchestration and device profile ingestion milestone.

## Distribution

The simulator can be consumed either as a Docker image or as a Python package.

- **Docker:** Run `scripts/build_docker.sh <image-tag>` to produce a runtime image based on
  `docker/Dockerfile`, then push it via `scripts/publish_docker.sh <image-tag> <registry>`.
- **Python package:** Use `scripts/build_python_package.sh` to generate wheel and sdist artefacts,
  then publish with `scripts/publish_python_package.sh [repository-url]`.
