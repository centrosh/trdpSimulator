# Next Development Steps

This guide summarises the immediate priorities for moving the TRDP Simulator
from scaffolding towards an operator-ready tool. Each item references the
existing architecture and milestone planning documents.

## 1. Finish Milestone 1: Communication Core Bootstrap

Focus the next sprints on completing the P0 stories in
[`docs/milestones.md`](milestones.md) and the matching backlog entries in
[`docs/issue-tracker.md`](issue-tracker.md): bring the C++ communication layer to
feature parity with the TRDP stack by implementing lifecycle management,
process-data publishing, message-data send/receive, and diagnostic hooks. This
establishes a reliable runtime foundation for higher-level features and is a
prerequisite for integrating external device configurations.

Key tasks:

- Wire the C++ wrappers in `src/communication/` to the TCNOpen TRDP APIs and
  exercise them via unit tests in `tests/`.
- Provide minimal CLI commands in `src/cli/` (or the existing Python CLI) to
  exercise PD/MD flows against a loopback scenario so the integration can be
  regression-tested.
- Capture TRDP errors and surface them through structured diagnostics as outlined
  in [`docs/design.md`](design.md).

## 2. Accept and Validate Device XML Profiles

With the communication core in place, enable operators to supply device-under-
test profiles:

1. **Ingestion path:** Extend the scenario loader (see
   [`docs/design.md`](design.md#14-ui--cli-layer)) to accept raw TRDP XML files.
   Provide a CLI flag (e.g., `--device-xml /path/to/device.xml`) and REST upload
   endpoint so users can register device profiles on the simulator host.
2. **Validation:** Integrate the upstream helpers documented in
   [`docs/trdp_xml_examples.md`](trdp_xml_examples.md) by wrapping the
   `trdp-xmlprint-test` utility or invoking an XSD validator. Reject malformed
   XML before it reaches the runtime and return actionable error reports.
3. **Configuration catalogue:** Persist approved XML files under a managed
   directory (for example `~/.trdp-simulator/devices/`) and reference them by ID
   in scenarios.

Deliverables include automated tests that feed known-good and intentionally
broken XML samples through the validation pipeline and check for the expected
responses.

## 3. Build Editing Surfaces for Device Configuration

Once ingestion and validation exist, expose editing capabilities aligned with
Milestone 4 (UI & Automation Interfaces):

- **REST API:** Add endpoints that fetch, validate, and update stored XML
  profiles. The API should reuse the validation layer from step 2 so that
  rejected edits return the same diagnostics.
- **Web UI integration:** Implement UI components that present the XML (or a
  structured form) to the operator. Provide syntax highlighting, schema-aware
  hints, and validation feedback prior to saving changes.
- **Version control:** Back the storage with simple versioning (Git repository
  or timestamped snapshots) so operators can roll back to a previous
  configuration when needed.

This work can progress in parallel with the remaining Milestone 2 scenario
orchestration stories, because validated XML profiles become a primary input for
scenario definitions.

## 4. Align CI/CD and Documentation

- Update `docs/operations-handbook.md` with the validation workflow once the
  pipeline is implemented.
- Add CI jobs that execute the XML validation tests and run the TRDP wrappers
  against sample configurations.
- Document operator workflows in the README and user guides so teams know how to
  register devices, validate XML, and launch simulations with custom profiles.

Executing this sequence keeps development focused on delivering a usable end-to-
end path: from accepting a customer-supplied TRDP configuration, through
validation, to running simulations that mirror the device under test.
