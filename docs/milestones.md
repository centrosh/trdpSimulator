# Development Milestones & User Stories

The approved architecture in [`design.md`](design.md) is decomposed into
incremental milestones to enable iterative delivery. Each milestone
contains themed user stories that can be executed independently.

## Milestone 1 – Communication Core Bootstrap
- **Goal:** Provide a thin Pythonic wrapper around the TRDP stack and
  exercise basic PD/MD telegram flows.
- **Duration:** 2 sprints
- **Exit Criteria:** Automated tests proving PD publish, MD send, and
  callback hooks.

### User Stories
1. *As a simulator developer, I can initialise and shutdown the TRDP
   context so that I can manage session lifecycles.*
2. *As a simulator developer, I can publish process data telegrams with
   configurable metadata so that scenarios can emit deterministic
   traffic.*
3. *As a simulator developer, I can send and receive message data
   telegrams including acknowledgement callbacks so that failure
   injection workflows operate end-to-end.*
4. *As a telemetry engineer, I can register diagnostic callbacks so that
   communication issues are captured for later analysis.*

## Milestone 2 – Scenario Orchestration Engine
- **Goal:** Build the deterministic scheduler that executes scenarios and
  coordinates with the communication layer.
- **Duration:** 3 sprints
- **Exit Criteria:** Scenario runner supporting configurable timetables
  and verifying expected telegrams via automated tests.

### User Stories
1. *As a simulation engineer, I can load scenario definitions from YAML
   so that I can version control test topologies.*
2. *As a scenario author, I can describe triggers and failure events so
   that the engine can inject them during a run.*
3. *As a test analyst, I can replay a scenario and receive structured
   telemetry so that I can audit behaviour.*
4. *As a developer, I can stub TRDP interactions for offline execution so
   that integration tests can run in CI.*

## Milestone 3 – Scenario Repository & Persistence
- **Goal:** Persist reusable scenarios and simulation artefacts with
  validation.
- **Duration:** 2 sprints
- **Exit Criteria:** Repository service storing scenarios, schema
  validation, and artefact export CLI.

### User Stories
1. *As a tooling engineer, I can validate scenario files against schemas
   so that errors are caught before execution.*
2. *As a scenario curator, I can version assets and import/export them
   between environments so that scenarios are portable.*
3. *As a compliance analyst, I can store simulation outputs and replay
   them on demand so that evidence is auditable.*

## Milestone 4 – UI & Automation Interfaces
- **Goal:** Provide CLI, REST, and optional UI controls for running
  simulations and visualising results.
- **Duration:** 3 sprints
- **Exit Criteria:** CLI feature parity with engine, API server with
  scenario control endpoints, and UI telemetry dashboards.

### User Stories
1. *As an operator, I can start, pause, and resume simulations from the
   CLI so that I can manage runs from automation scripts.*
2. *As an integrator, I can control simulations via REST/gRPC APIs so
   that I can integrate with third-party orchestrators.*
3. *As a user, I can view topology and live telemetry from a web UI so
   that I can monitor progress visually.*
4. *As an operator, I can export results in CSV/JSON formats so that they
   can be ingested by external systems.*

## Milestone 5 – Observability & Production Hardening
- **Goal:** Instrument the simulator for production readiness with
  comprehensive telemetry and resilience features.
- **Duration:** 2 sprints
- **Exit Criteria:** Metrics/trace pipeline, configurable persistence
  backend, and resilience features validated via chaos tests.

### User Stories
1. *As an SRE, I can collect structured logs, metrics, and traces so that
   production issues can be diagnosed quickly.*
2. *As a reliability engineer, I can configure redundancy and retry
   policies so that the simulator meets availability targets.*
3. *As a compliance officer, I can generate reports from persisted
   simulation outputs so that audits are supported.*
