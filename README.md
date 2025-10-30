# TRDP Simulator

The TRDP Simulator targets railway communication scenarios using the
TCNopen TRDP stack. This repository now includes scaffolding for the
simulator runtime, continuous integration, and contributor workflows.

## Project Structure

```
├── src/trdp_simulator/      # Python package implementing simulator modules
├── tests/                   # Pytest-based automated tests
├── docs/                    # Architecture, milestones, and backlog documentation
├── .github/workflows/       # Continuous integration definitions
├── CONTRIBUTING.md          # Coding standards and contribution workflow
└── pyproject.toml           # Build system and dependency metadata
```

## Getting Started

1. Ensure Python 3.11+ is installed.
2. Install development dependencies:
   ```bash
   make install
   ```
3. Run quality checks locally:
   ```bash
   make check
   ```
4. Execute the demo CLI (placeholder implementation):
   ```bash
   python -m trdp_simulator.cli.main demo-scenario
   ```

## Documentation

- [`docs/design.md`](docs/design.md) – architecture overview and approved
  design decisions.
- [`docs/milestones.md`](docs/milestones.md) – incremental milestones and
  user stories.
- [`docs/issue-tracker.md`](docs/issue-tracker.md) – prioritised backlog
  aligned with milestones.
