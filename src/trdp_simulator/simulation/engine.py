"""Core simulation engine orchestration primitives."""
from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
from enum import Enum, auto
import threading
import time
from typing import Protocol

from trdp_simulator.communication.wrapper import (
    DeliveryReport,
    MdMessage,
    PdMessage,
    TrdpSessionManager,
)


@dataclass
class RunOptions:
    """Execution options controlling simulation runs."""

    realtime: bool = False
    step_delay: float = 0.0


@dataclass
class SimulationResult:
    """Container for simulation outputs."""

    success: bool
    telemetry_events: list[str]


@dataclass
class FaultDescriptor:
    """Describe a fault injected into the simulation."""

    name: str
    severity: str


@dataclass
class Scenario:
    """Simplified scenario placeholder used for scaffolding."""

    identifier: str
    pd_messages: Iterable[PdMessage]


class RunState(Enum):
    """Track the state of the currently executing run."""

    IDLE = auto()
    RUNNING = auto()
    PAUSED = auto()
    COMPLETED = auto()
    FAILED = auto()


@dataclass
class RunStatus:
    """Expose live status information about the active simulation."""

    state: RunState
    scenario_id: str | None
    telemetry_events: list[str]
    realtime: bool
    detail: str | None = None


class SimulationObserver(Protocol):
    """Receive notifications about simulation progress."""

    def on_event(self, event: str) -> None:  # pragma: no cover - protocol
        ...


class SimulationEngine:
    """Co-ordinate TRDP sessions, scenarios and telemetry hooks."""

    def __init__(
        self,
        comm: TrdpSessionManager,
    ) -> None:
        self._comm = comm
        self._observers: list[SimulationObserver] = []
        self._state = RunState.IDLE
        self._telemetry: list[str] = []
        self._detail: str | None = None
        self._realtime = False
        self._scenario_id: str | None = None
        self._options = RunOptions()
        self._lock = threading.RLock()
        self._pause_event = threading.Event()
        self._pause_event.set()
        self._completion = threading.Event()
        self._thread: threading.Thread | None = None
        self._result: SimulationResult | None = None

    def register_observer(self, observer: SimulationObserver) -> None:
        """Register an observer to receive lifecycle events."""

        self._observers.append(observer)

    def _notify(self, event: str) -> None:
        for observer in self._observers:
            observer.on_event(event)

    def start(self, scenario: Scenario, options: RunOptions | None = None) -> None:
        """Start executing the provided scenario in the background."""

        with self._lock:
            if self._state in {RunState.RUNNING, RunState.PAUSED}:
                raise RuntimeError("Simulation already in progress")
            self._options = options or RunOptions()
            self._scenario_id = scenario.identifier
            self._realtime = self._options.realtime
            self._telemetry = []
            self._detail = None
            self._result = None
            self._completion.clear()
            self._pause_event.set()
            self._state = RunState.RUNNING
            self._notify(f"starting:{scenario.identifier}")

            def _run() -> None:
                success = True
                detail: str | None = None
                try:
                    self._comm.configure(scenario)
                    for message in scenario.pd_messages:
                        self._pause_event.wait()
                        with self._lock:
                            if self._state == RunState.FAILED:
                                success = False
                                detail = self._detail
                                break
                        report = self._comm.publish_pd(message)
                        with self._lock:
                            self._telemetry.append(f"pd:{message.dataset_id}:{report.success}")
                        if self._options.step_delay > 0:
                            # Allow pause() to block between iterations.
                            waited = 0.0
                            while waited < self._options.step_delay:
                                remaining = self._options.step_delay - waited
                                chunk = min(0.05, remaining)
                                start = time.perf_counter()
                                self._pause_event.wait(timeout=chunk)
                                waited += time.perf_counter() - start
                                with self._lock:
                                    if self._state == RunState.FAILED:
                                        success = False
                                        detail = self._detail
                                        break
                                with self._lock:
                                    paused = self._state == RunState.PAUSED
                                if paused:
                                    self._pause_event.wait()
                                if not success:
                                    break
                            if not success:
                                break
                    if success:
                        self._result = SimulationResult(success=True, telemetry_events=list(self._telemetry))
                except Exception as exc:  # pragma: no cover - defensive branch
                    success = False
                    detail = str(exc)
                finally:
                    with self._lock:
                        if success and self._result is None:
                            self._result = SimulationResult(success=True, telemetry_events=list(self._telemetry))
                            self._state = RunState.COMPLETED
                        elif success:
                            self._state = RunState.COMPLETED
                        else:
                            self._detail = detail
                            self._result = SimulationResult(success=False, telemetry_events=list(self._telemetry))
                            self._state = RunState.FAILED
                    self._notify(f"finished:{scenario.identifier}:{self._realtime}")
                    self._completion.set()

            self._thread = threading.Thread(target=_run, name="simulation-runner", daemon=True)
            self._thread.start()

    def run(self, scenario: Scenario, options: RunOptions | None = None) -> SimulationResult:
        """Execute the provided scenario using the TRDP session manager."""

        self.start(scenario, options)
        self.wait()
        result = self.result()
        if result is None:
            raise RuntimeError("Simulation did not produce a result")
        if not result.success and self._detail is not None:
            raise RuntimeError(self._detail)
        return result

    def wait(self, timeout: float | None = None) -> bool:
        """Block until the current run completes."""

        completed = self._completion.wait(timeout)
        if completed and self._thread is not None:
            self._thread.join(timeout=0)
            self._thread = None
        return completed

    def pause(self) -> None:
        """Pause the active simulation."""

        with self._lock:
            if self._state != RunState.RUNNING:
                raise RuntimeError("Simulation is not running")
            self._state = RunState.PAUSED
            self._pause_event.clear()

    def resume(self) -> None:
        """Resume a paused simulation."""

        with self._lock:
            if self._state != RunState.PAUSED:
                raise RuntimeError("Simulation is not paused")
            self._state = RunState.RUNNING
            self._pause_event.set()

    def fail(self, detail: str) -> None:
        """Force the current simulation to fail."""

        with self._lock:
            if self._state not in {RunState.RUNNING, RunState.PAUSED}:
                raise RuntimeError("No active simulation to fail")
            self._detail = detail
            self._state = RunState.FAILED
            self._pause_event.set()

    def status(self) -> RunStatus:
        """Return the current run status."""

        with self._lock:
            telemetry = list(self._telemetry)
            return RunStatus(
                state=self._state,
                scenario_id=self._scenario_id,
                telemetry_events=telemetry,
                realtime=self._realtime,
                detail=self._detail,
            )

    def result(self) -> SimulationResult | None:
        """Return the final simulation result, if available."""

        with self._lock:
            return self._result

    def apply_fault(self, fault: FaultDescriptor) -> DeliveryReport:
        """Demonstrate how faults trigger MD communications."""

        report = self._comm.send_md(MdMessage(label=fault.name, payload=b""))
        self._notify(f"fault:{fault.name}:{fault.severity}")
        return report
