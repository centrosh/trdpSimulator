# Next Development Steps

This guide summarises the immediate priorities for moving the TRDP Simulator
from scaffolding towards an operator-ready tool. Each item references the
existing architecture and milestone planning documents.

## 1. Milestone 1: Communication Core Bootstrap ✅ Completed

Milestone 1 has been implemented. The communication wrapper now exposes
lifecycle management, PD publish/receive, MD send/receive with acknowledgement
status, and diagnostic hooks surfaced through the CLI and engine. Focused unit
tests in `tests/test_wrapper.cpp` and `tests/test_engine.cpp` exercise success
and failure paths so regressions are caught automatically.

While future work will replace the loopback adapter with the real TCNopen TRDP
stack, the current implementation provides a deterministic seam for higher-level
scenarios and validates callback wiring. Subsequent development should build on
this foundation to ingest real device profiles and orchestrate scenarios.

## 2. Milestone 2 – Device & Scenario Orchestration ✅ Completed

The simulator now ingests TRDP XML via `DeviceProfileRepository`, validates
uploads with the bundled schema, and persists metadata for auditing. Scenarios
are parsed through `ScenarioParser`, catalogued by `ScenarioRepository`, and can
be imported, listed, and exported directly from the CLI. Automated tests cover
the repository lifecycle, and documentation has been refreshed so operators can
register devices, manage scenarios, and run loopback simulations end-to-end.

The next phase builds on this foundation to introduce durable storage and
replayable artefacts.

## 3. Milestone 3 – Scenario Repository & Persistence

With the catalogue online, focus shifts to the persistence stories outlined in
[`docs/milestones.md`](milestones.md#milestone-3--scenario-repository--persistence):

1. **Formal scenario schema:** Publish JSON/YAML schemas and integrate them with
   the validation pipeline (CLI flag and CI job) so scenario changes are gated by
   automated checks.
2. **Run artefact persistence:** Extend `SimulationEngine` to record executed
   scenarios, diagnostics, and payload traces to disk. Link the artefacts to the
   scenario manifest so operators can audit historical runs.
3. **Import/export tooling:** Wrap the repository in high-level commands or
   services that bundle scenarios with their referenced device profiles, enabling
   movement between environments.
4. **Automation hooks:** Expose lightweight APIs or CLI commands to fetch the
   manifest, retrieve stored runs, and trigger replays, preparing the ground for
   upcoming REST/UI work.

## 4. Build Editing Surfaces for Device & Scenario Configuration

Parallel to Milestone 3, begin designing REST and UI capabilities (Milestone 4):

- **REST API:** Expose endpoints that list, validate, import, and export both
  device profiles and scenarios, reusing the shared validation pipeline.
- **Web UI integration:** Prototype components that surface repository
  inventories, validation feedback, and historical run artefacts.
- **Version control:** Evaluate lightweight versioning for both XML and scenario
  YAML so operators can diff and roll back changes.

## 5. Align CI/CD and Documentation

- Wire the scenario repository tests and schema validators into CI so
  regressions surface automatically.
- Expand operator documentation to cover artefact export/import workflows and
  historical run retrieval.
- Keep the README and operations handbook aligned with new CLI flags and
  services as they land.

Executing this sequence keeps development focused on delivering durable, auditable
simulation artefacts while preparing the control surfaces required by later
milestones.
