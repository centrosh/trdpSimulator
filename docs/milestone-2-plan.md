# Milestone 2 Implementation Plan – Scenario & Device Profile Orchestration

> **Status:** Delivered – device XML ingestion, schema validation, scenario
> parser/repository, and CLI management tooling have landed in the codebase.

Milestone 2 moves the simulator from a loopback proof of concept to a tool that
can ingest customer-supplied TRDP device profiles and execute deterministic
scenarios against the communication layer delivered in Milestone 1. This plan
breaks down the work into focused streams with clear deliverables and exit
criteria so engineering, QA, and documentation efforts stay aligned.

## 1. Goals & Scope

1. Accept TRDP device XML files from operators via CLI and service APIs.
2. Validate uploaded XML against the upstream schema and diagnostic tools
   distributed with the TCNOpen TRDP stack.
3. Persist approved device definitions and reference them from simulation
   scenarios.
4. Introduce a scenario loader capable of combining topology/device metadata
   with ordered PD/MD events for the simulation engine.

> **Out of scope:** UI form editing, full persistence service, and live TRDP
> integration remain Milestone 3+ concerns. Milestone 2 focuses on backend
> ingestion, validation, and orchestration primitives that unblock those later
> milestones.

## 2. Workstreams

### 2.1 Device XML Ingestion & Cataloguing

- **CLI entry point:** Add `--device-xml` to the simulator CLI so operators can
  register device profiles from disk. The flag should copy the XML into the
  managed configuration directory (default `~/.trdp-simulator/devices`).
- **Service API seam:** Define a service-layer interface (e.g.,
  `DeviceProfileRepository`) that abstracts storage. The initial implementation
  persists to the filesystem; later milestones can substitute a database.
- **Metadata tracking:** Maintain a manifest (YAML/JSON) containing device name,
  source path, checksum, and last validation timestamp. This enables audits and
  replay.
- **Tests:** Exercise the CLI and repository with a temporary directory,
  confirming the manifest, file copy, and duplicate handling work as expected.

### 2.2 XML Validation Pipeline

- **Schema validation:** Bundle the `trdp-config.xsd` schema or reference it
  from the submodule and validate uploads using an XML library (e.g., `libxml2`).
- **Binary harness:** Wrap the `trdp-xmlprint-test` utility (documented in
  `docs/trdp_xml_examples.md`) so operators can run a deep-dive validation. On
  failure, propagate stderr/stdout into structured diagnostics for the user.
- **CI integration:** Add tests that intentionally break XML attributes to prove
  validation catches errors. Wire these tests into the existing CTest suite.
- **Diagnostics:** Extend the telemetry design so validation results are exposed
  via CLI and logs, providing actionable feedback without digging into the
  filesystem.

### 2.3 Scenario Loader & Engine Integration

- **Scenario schema:** Draft an initial YAML schema that references device
  profile IDs, PD/MD events, and timing. Keep the format simple (ordered list of
  events with delay + telegram metadata) to minimise implementation risk.
- **Loader implementation:** Implement `ScenarioLoader` in C++ to resolve
  referenced device profiles, validate their existence, and hydrate the
  `SimulationEngine` with events and TRDP endpoints.
- **Engine updates:** Update the engine to request validation before each run,
  fail fast if a device profile is missing or invalid, and expose status via the
  existing diagnostic channel.
- **Regression coverage:** Add integration-style tests that stitch together the
  repository, loader, engine, and communication wrapper using loopback events to
  confirm end-to-end behaviour.

### 2.4 Documentation & Operations

- Extend `docs/operations-handbook.md` with procedures for uploading and
  validating device XML files, including troubleshooting for schema failures.
- Update `README.md` with a quick-start section that demonstrates registering a
  device and running a scenario referencing it.
- Capture open questions (e.g., credential management for future REST uploads)
  so they are visible before Milestone 3 planning.

## 3. Exit Criteria

Milestone 2 is considered complete when:

1. Operators can register TRDP device XML via CLI, the files are stored in the
   managed catalogue, and metadata is tracked.
2. Uploaded XML passes automatic validation with clear diagnostics on failure.
3. Scenarios can reference stored device profiles and run through the simulation
   engine/communication wrapper loopback path without manual intervention.
4. Automated tests and documentation cover the ingestion and validation workflow
   end to end.

## 4. Dependencies & Risks

- **Dependency:** Access to the TCNOpen TRDP utilities (e.g.,
  `trdp-xmlprint-test`) in development and CI environments.
- **Risk:** XML schema drift between upstream releases. Mitigate by vendoring
  the schema version used for validation and documenting upgrade procedures.
- **Risk:** Storage abstraction may need to evolve quickly. Mitigate by keeping
  the repository interface narrow and writing adapter-specific tests.

## 5. Follow-Up Opportunities

- Build REST endpoints and UI editing tools on top of the repository layer.
- Replace the loopback adapter with the real TRDP stack once device profiles are
  validated end-to-end.
- Introduce scenario versioning and change review workflows (Milestone 3).

This plan should be reviewed with stakeholders before implementation to confirm
priorities and refine estimates.
