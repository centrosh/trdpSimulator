# TRDP Simulator Architecture and Design

## 1. Architecture Overview
The TRDP Simulator is structured as a modular system that separates
transport-level communication from simulation logic and user interaction.
The following core modules form the baseline architecture:

### 1.1 Communication Layer
- **Purpose:** Provide a thin, testable wrapper around the TCNopen TRDP
  stack for both Process Data (PD) and Message Data (MD) services.
- **Responsibilities:**
  - Manage TRDP application, session, and dataset lifecycles
    (initialization, configuration, shutdown).
  - Publish and subscribe to PD telegrams with configurable repetition,
    redundancy, and integrity checks.
  - Send, receive, and acknowledge MD telegrams including segmentation
    support.
  - Expose diagnostic and monitoring hooks (e.g., link status, error
    events, statistics) for the simulation engine and UI.

### 1.2 Simulation Engine
- **Purpose:** Orchestrate simulation scenarios and enforce deterministic
  execution.
- **Responsibilities:**
  - Load scenario definitions (topology, timetable, telegram templates,
    triggers) from configuration files or APIs.
  - Coordinate simulation steps, scheduling telegram transmissions,
    stimulus events, and failure injections.
  - Maintain simulation state (virtual train consist, network
    environment, TRDP endpoint status).
  - Record timelines, events, and metrics for reporting and post-analysis.

### 1.3 Scenario Repository & Configuration
- **Purpose:** Persist and version simulation scenarios and reusable
  assets.
- **Responsibilities:**
  - Store scenario definitions (YAML/JSON) with validation schemas.
  - Manage associated assets (binary payloads, topology diagrams, test
    scripts).
  - Provide import/export facilities and integration hooks for CI
    pipelines.

### 1.4 UI & CLI Layer
- **Purpose:** Offer human- and automation-friendly control surfaces.
- **Responsibilities:**
  - CLI commands for scenario management, simulation control, and results
    export.
  - Optional web/desktop UI for visualizing network topology, live
    telemetry, and playback.
  - Provide REST/gRPC API adapters for third-party orchestrators.

### 1.5 Telemetry & Persistence Services
- **Purpose:** Provide observability and storage.
- **Responsibilities:**
  - Collect logs, metrics, and traces from all modules.
  - Store simulation outputs (event logs, captured telegrams, performance
    metrics) in file-based or database backends.
  - Serve reports to the UI/CLI and external systems.

## 2. Data Flow Diagrams
### 2.1 Scenario Execution Data Flow
```mermaid
graph TD
    A[Scenario Definition] -->|Load| B[Simulation Engine]
    B -->|Configure| C[Communication Layer]
    B -->|Schedule Events| C
    C -->|TRDP PD/MD Telegrams| D[TCNopen TRDP Stack]
    D -->|Network IO| E[Railway Network]
    D -->|Callbacks| C
    C -->|Telemetry| F[Telemetry Service]
    B -->|Simulation Metrics| F
    F -->|Reports| G[UI/CLI]
```

### 2.2 Scenario Authoring Data Flow
```mermaid
graph TD
    U[Engineer] -->|Create/Update| R[Scenario Repository]
    R -->|Validate Schema| V[Schema Validator]
    V -->|Feedback| U
    R -->|Provide Scenario| S[Simulation Engine]
```

## 3. Sequence Diagrams for Key Use Cases
### 3.1 Run a Scheduled PD Simulation
```mermaid
sequenceDiagram
    participant User
    participant CLI as CLI/UI
    participant Engine as Simulation Engine
    participant Comm as Communication Layer
    participant TRDP as TCNopen TRDP Stack

    User->>CLI: Start simulation (scenario ID)
    CLI->>Engine: loadScenario(id)
    Engine->>Comm: configureEndpoints(scenario)
    Comm->>TRDP: trdp_apl_open()
    Engine->>Engine: schedule events
    loop Each cycle
        Engine->>Comm: publishPD(dataset, payload)
        Comm->>TRDP: tlp_pdSend()
        TRDP-->>Comm: status callback
        Comm-->>Engine: deliveryReport()
    end
    Engine->>CLI: emitProgress()
    CLI-->>User: Display status
    Engine->>Comm: shutdown()
    Comm->>TRDP: trdp_apl_close()
```

### 3.2 Inject a Failure and Observe Recovery via MD Telegrams
```mermaid
sequenceDiagram
    participant Operator
    participant UI
    participant Engine
    participant Comm
    participant TRDP

    Operator->>UI: Inject failure (link down)
    UI->>Engine: applyFault(faultDescriptor)
    Engine->>Comm: notifyFault(faultDescriptor)
    Comm->>TRDP: trdp_mdSend(alert)
    TRDP-->>Comm: sendResult
    Comm-->>Engine: notifyResult()
    Engine->>Engine: adjustState()
    Engine->>Comm: subscribeMD(recovery)
    Comm->>TRDP: tlp_mdReceive()
    TRDP-->>Comm: recovery telegram
    Comm-->>Engine: processRecovery()
    Engine->>UI: updateStatus()
    UI-->>Operator: Display recovery
```

## 4. Module Specifications and TRDP API Interfaces
### 4.1 Communication Layer Wrapper
The communication layer is implemented as a thin wrapper around the
TCNopen TRDP C APIs. An intermediate `StackAdapter` interface isolates the
simulator from the concrete TRDP integration so unit tests can inject
deterministic behaviour. The C++ façade exposes the following abstractions:

```cpp
class TrdpContext {
public:
    explicit TrdpContext(const TrdpConfig &config);
    ~TrdpContext();

    void start();                // wraps trdp_apl_open()
    void stop();                 // wraps trdp_apl_close()
    TrdpSession createSession(const SessionConfig &config);

private:
    TRDP_APP_SESSION_T appHandle_{};
    TrdpConfig config_{};
};

class TrdpSession {
public:
    void publishPd(const PdMessage &message);   // wraps tlp_pdSend()
    SubscriptionId subscribePd(const PdSubscription &subscription); // wraps tlp_subscribe()
    void sendMd(const MdMessage &message);      // wraps tlp_mdSend()
    MdMessage receiveMd(std::chrono::milliseconds timeout); // wraps tlp_mdReceive()

    void registerCallback(const Callback &cb);  // wraps tlp_setCallback()
};
```

Supporting data structures:
- `TrdpConfig` encapsulates IP addresses, VLAN priority, redundancy
  settings, PD/MD defaults.
- `SessionConfig` identifies logical endpoints and dataset mappings.
- `PdMessage` and `MdMessage` include dataset IDs, payload buffers,
  timing, and QoS metadata.
- `PdSubscription` encodes filtering, sampling rate, and supervision
  parameters.

Current implementation status:
- `ProcessDataMessage` and `MessageDataMessage` structs expose label,
  COM ID, dataset ID, and payload buffers to the wrapper.
- A loopback `StackAdapter` is provided so unit tests and the CLI can
  exercise PD publish/receive and MD send/receive flows while the
  TCNopen TRDP C API is integrated.
- `MessageDataAck` surfaces acknowledgement status (delivered, timeout,
  failed) to the simulation engine so scenarios can react to
  send outcomes.

Error handling strategy:
- Wrap TRDP error codes in typed exceptions (e.g., `TrdpError`).
- Expose retry policies and fallback hooks to the simulation engine.

### 4.2 Simulation Engine Interfaces
```cpp
class ScenarioLoader {
public:
    virtual ~ScenarioLoader() = default;
    virtual Scenario load(const std::string &scenarioId) = 0;
};

class SimulationEngine {
public:
    explicit SimulationEngine(TrdpSessionManager &comm, TelemetrySink &telemetry);

    SimulationResult run(const Scenario &scenario, const RunOptions &options);
    void applyFault(const FaultDescriptor &fault);
    void registerObserver(std::shared_ptr<SimulationObserver> observer);
};

class TrdpSessionManager {
public:
    void configure(const Scenario &scenario);
    DeliveryReport publishPd(const PdMessage &message);
    DeliveryReport sendMd(const MdMessage &message);
    SubscriptionId subscribePd(const PdSubscription &subscription, PdHandler handler);
    void shutdown();
};
```

### 4.3 UI/CLI Interfaces
```cpp
class CommandHandler {
public:
    void start(const std::string &scenarioId, const RunOptions &options);
    void injectFault(const FaultDescriptor &fault);
    std::filesystem::path exportResults(const std::string &resultId, std::string_view format);
};

class TelemetryView {
public:
    void streamMetrics(std::function<void(const Metric &)> consumer);
    void renderTopology(const Topology &topology);
};
```

### 4.4 Telemetry & Persistence Interfaces
```cpp
class TelemetrySink {
public:
    virtual ~TelemetrySink() = default;
    virtual void recordEvent(const SimulationEvent &event) = 0;
    virtual void recordMetric(const Metric &metric) = 0;
    virtual void flush() = 0;
};

class ResultsRepository {
public:
    virtual ~ResultsRepository() = default;
    virtual ResultHandle store(const SimulationResult &result) = 0;
    virtual SimulationResult fetch(const ResultHandle &handle) = 0;
};
```
