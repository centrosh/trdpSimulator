# Testing and Validation Strategy

This document details the verification approach for the TRDP Simulator.
It derives concrete test cases, automation tooling, and release
acceptance criteria from the architecture and milestone requirements.

## 1. Test Case Matrix by Requirement Level

### 1.1 Unit Tests
| Requirement Source | Component | Objective | Representative Test Cases |
| --- | --- | --- | --- |
| Design §4.1 Communication Wrapper | `TrdpContext`, `TrdpSession` | Validate TRDP wrapper behaviour without network IO by mocking TCNopen calls. | Construct context with valid/invalid `TrdpConfig`; verify `start()/stop()` invoke open/close; ensure `publishPd()` serialises dataset metadata; confirm `sendMd()` propagates acknowledgement callbacks; raise `TrdpError` on simulated stack failures. |
| Milestone 1 Story 4 | Telemetry hooks | Ensure diagnostics propagate to observers. | Subscribe stub observer and emit simulated link errors; assert observer receives structured payload. |
| Design §4.2 Simulation Engine | `SimulationEngine`, `ScenarioLoader` | Guarantee deterministic scheduling logic. | Simulate timeline with stubbed TRDP interactions; verify triggers fire at expected ticks; confirm `applyFault()` transitions state machine. |
| Design §4.2 Session Manager | `TrdpSessionManager` | Enforce scenario-driven configuration. | Feed scenario definitions with varying redundancy options and assert session setup honours VLAN priority, supervision timers, and dataset mappings. |
| Milestone 3 Story 1 | Schema validator | Validate scenario file parsing. | Run schema validator over valid/invalid YAML; confirm descriptive error paths. |

### 1.2 Integration Tests
| Requirement Source | Scope | Objective | Scenarios |
| --- | --- | --- | --- |
| Milestone 1 Exit Criteria | Communication + mock TRDP stack | Exercise PD/MD flows end-to-end via in-memory loopback. | Launch mock stack server; publish PD telegrams at configured intervals; verify callback telemetry and MD acknowledgements. |
| Milestone 2 Exit Criteria | Simulation engine + repository | Validate scenario loading and execution orchestration. | Load YAML scenario with timetable; assert scheduler emits PD/MD traffic via stubbed session manager, records telemetry timeline, and enforces failure triggers. |
| Milestone 3 Exit Criteria | Repository + persistence | Confirm artefact lifecycle. | Import/export scenario assets; execute run and ensure outputs persisted to configured backend; replay stored run for audit. |
| Milestone 4 Stories 1 & 2 | CLI/API + engine | Ensure automation interfaces control simulations. | Use CLI to start/pause/resume; verify REST/gRPC adapter issues equivalent commands and receives run status updates. |

### 1.3 System / Acceptance Tests
| Requirement Source | Scenario | Objective | Validation Steps |
| --- | --- | --- | --- |
| Milestone 2 User Story 3 | Full simulation with telemetry | Demonstrate replayable, auditable run. | Execute canonical "scheduled PD" scenario through CLI; capture telemetry, verify report integrity, and confirm deterministic rerun within tolerance. |
| Milestone 4 Story 3 | UI dashboard | Validate operator visibility. | Run long-lived scenario with induced failure; verify UI telemetry tiles update within latency budget and fault indicators resolve after recovery. |
| Milestone 5 Story 2 | Resilience under redundancy | Demonstrate failover handling. | Simulate redundant network path cutover; ensure PD/MD delivery meets availability target and alarms emitted. |
| Design §1.1–§1.5 | End-to-end workflow | Confirm all subsystems interoperate. | Load scenario from repository, execute via engine, communicate through TRDP stack, collect telemetry, export report. |

## 2. Automated Regression Scenarios and Tooling

| Scenario | Trigger | Regression Goal | Automation Tooling |
| --- | --- | --- | --- |
| Scheduled PD timetable | Milestone 2 requirement for deterministic schedule | Detect regressions in timing offsets and PD payload integrity. | `ctest` target invoking GoogleTest-based fixture with fake TRDP stack; wrap in `pytest` using `subprocess` for CLI smoke. |
| MD acknowledgement workflow | Milestone 1 MD send/receive | Verify segmentation, retries, and callback wiring. | GoogleTest integration using custom mock harness; fuzz payload sizes with `rapidcheck`. |
| Fault injection and recovery | Milestone 2 triggers + Milestone 5 resilience | Ensure engine applies faults, telemetry logs events, and redundancy recovers. | Python `pytest` suite driving CLI/API via `asyncio`; record results for snapshot comparison. |
| Scenario schema validation | Milestone 3 validation | Guard against schema regressions. | `pytest` tests leveraging `jsonschema` to validate sample YAML; integrate with pre-commit hook. |
| Telemetry export pipeline | Milestone 5 observability | Prevent regressions in metrics/log formats. | Integration tests using `pytest` + `pydantic` models to check exported JSON/CSV; golden-file comparisons. |

Framework selections:
- **GoogleTest** for C++ unit/integration tests compiled via CMake/CTest.
- **pytest** harness for black-box CLI/API flows and schema checks.
- **Custom TRDP mock harness** providing deterministic responses and latency injection to simulate stack behaviour.
- **RapidCheck** (property-based testing) for payload fuzzing and resilience edges.

## 3. Performance and Reliability Validation

| Metric | Requirement | Test Method | Tooling |
| --- | --- | --- | --- |
| PD telegram cycle time | Meet timetable jitter ≤ 5% of configured period. | Run high-frequency scenario (e.g., 10 ms cycle) on dedicated hardware; capture timestamps via telemetry sink; compute jitter distribution. | Instrumented GoogleTest perf harness + `benchmark` library; export metrics to Prometheus for CI dashboards. |
| MD throughput | Sustain ≥ 100 msgs/s with acknowledgements < 50 ms. | Stress test with concurrent MD sessions and variable payload sizes; measure round-trip latency. | Load generator built atop custom TRDP harness; use `pytest-benchmark` for statistical reporting. |
| Failover recovery | Resume service within 200 ms after link failure. | Scripted fault injection toggling redundant paths; validate PD delivery gap and telemetry alerts. | Reliability suite leveraging `pytest` + network emulation (tc/netem). |
| Long-run stability | 24-hour soak without resource leaks or missed deadlines. | Execute representative scenario set continuously; monitor memory, file descriptors, and timing drifts. | CI nightly job with `systemd-run` harness, metrics collected via `collectd`. |

## 4. Acceptance Criteria and Continuous Integration Gates

### 4.1 Acceptance Criteria
- **Functional:** All system tests in §1.3 pass with documented evidence, including telemetry reports and replay validation artefacts.
- **Performance:** Metrics in §3 meet or exceed thresholds (jitter, throughput, failover, stability).
- **Reliability:** Fault injection suite shows no unhandled exceptions; redundancy policies verified across supported configurations.
- **Documentation:** Test evidence, performance dashboards, and release notes archived per milestone.

### 4.2 Continuous Integration Pipeline
1. **Pre-merge checks**
   - Format/lint C++ (`clang-format`, `clang-tidy`) and Python (`ruff`).
   - Build and run unit tests via `ctest` with GoogleTest coverage.
   - Execute fast `pytest` scenarios (schema validation, CLI smoke).
   - Enforce coverage thresholds (≥80% for core modules) using `lcov`/`gcovr`.
2. **Nightly jobs**
   - Extended integration suite with custom TRDP harness, including fault injection.
   - Performance microbenchmarks storing results in trend dashboards; alert on ±10% deviation.
   - Long-run soak test with resource leak detection (ASAN/UBSAN). 
3. **Release candidate gates**
   - Full system test matrix, including UI verification (manual witness + screenshot archive).
   - Performance certification rerun on target hardware; publish report attachments.
   - Security scan (`clang --analyze`, dependency audit) and artefact signing.

## 5. Regression Management
- Tag baseline scenarios and golden outputs per release; store in repository or artefact storage.
- Automate comparison scripts to fail CI on drift in telemetry schemas or performance regressions.
- Maintain traceability matrix mapping requirements to tests; update alongside feature changes.

