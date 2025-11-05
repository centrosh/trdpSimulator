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

## 3. Milestone 3 – Scenario Repository & Persistence ✅ Completed

The repository layer now satisfies the persistence stories outlined in
[`docs/milestones.md`](milestones.md#milestone-3--scenario-repository--persistence):

1. **Formal scenario schema:** The YAML schema is published under
   `resources/scenarios/scenario.schema.yaml`, the CLI exposes
   `--validate-scenario`, and automated tests exercise the validator against
   both valid and invalid documents.
2. **Run artefact persistence:** `SimulationEngine` continues to capture
   executed scenarios, diagnostics, and payload traces and records them in the
   repository manifest for replay.
3. **Import/export tooling:** Scenario exports now bundle the referenced device
   profiles under a `devices/` directory so catalogues can migrate between
   environments without missing assets.
4. **Automation hooks:** Listing and replay commands surface through the CLI,
   making historical runs auditable and ready for downstream automation.

## 4. Milestone 4 – UI & Automation Interfaces (In Flight)

Automation work has started by delivering the REST API and CLI control
surfaces. The remaining focus items centre on rich visualisation:

- **REST API extensions:** Build listing and validation endpoints for device
  and scenario catalogues on top of the existing FastAPI app.
- **Web UI integration:** Prototype components that surface repository
  inventories, validation feedback, and historical run artefacts.
- **Version control:** Evaluate lightweight versioning for both XML and
  scenario YAML so operators can diff and roll back changes.

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
