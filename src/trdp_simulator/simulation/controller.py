"""Automation-friendly controller for managing simulation runs."""
from __future__ import annotations

from dataclasses import dataclass
import threading
from typing import Iterable
from uuid import uuid4

from trdp_simulator.communication.wrapper import PdMessage, TrdpConfig, TrdpContext, TrdpSessionManager
from trdp_simulator.simulation.engine import RunOptions, RunState, Scenario, SimulationEngine


@dataclass
class ControllerStatus:
    """Status metadata returned to automation clients."""

    run_id: str | None
    scenario_id: str | None
    state: RunState
    telemetry_events: list[str]
    realtime: bool
    detail: str | None = None


@dataclass
class RunHandle:
    """Track metadata about the active simulation run."""

    run_id: str
    scenario_id: str
    options: RunOptions


class SimulationController:
    """Manage the lifecycle of simulation runs for CLI/API automation."""

    def __init__(self) -> None:
        self._context = TrdpContext(TrdpConfig(application_name="trdp-sim"))
        self._manager = TrdpSessionManager(self._context)
        self._engine = SimulationEngine(self._manager)
        self._lock = threading.RLock()
        self._active: RunHandle | None = None
        self._last_status: ControllerStatus | None = None

    def start_run(self, scenario_id: str, messages: Iterable[PdMessage], options: RunOptions | None = None) -> str:
        """Start executing a scenario and return the run identifier."""

        with self._lock:
            if self._active is not None and self._engine.status().state in {RunState.RUNNING, RunState.PAUSED}:
                raise RuntimeError("A simulation run is already active")
            if not self._context.running:
                self._context.start()
            scenario = Scenario(identifier=scenario_id, pd_messages=list(messages))
            run_id = str(uuid4())
            run_options = options or RunOptions()
            self._engine.start(scenario, run_options)
            self._active = RunHandle(run_id=run_id, scenario_id=scenario_id, options=run_options)
            return run_id

    def pause_run(self, run_id: str) -> None:
        """Pause the active simulation run."""

        with self._lock:
            self._validate_run_id(run_id)
            self._engine.pause()

    def resume_run(self, run_id: str) -> None:
        """Resume a paused simulation run."""

        with self._lock:
            self._validate_run_id(run_id)
            self._engine.resume()

    def status(self, run_id: str | None = None) -> ControllerStatus:
        """Return status for the requested run or the latest known state."""

        with self._lock:
            active = self._active
            if run_id is not None and (active is None or active.run_id != run_id):
                if self._last_status is not None and self._last_status.run_id == run_id:
                    return self._last_status
                raise RuntimeError(f"Unknown run id: {run_id}")

            engine_status = self._engine.status()
            if active is None and self._last_status is not None:
                return self._last_status

            if active is None:
                return ControllerStatus(
                    run_id=None,
                    scenario_id=None,
                    state=engine_status.state,
                    telemetry_events=engine_status.telemetry_events,
                    realtime=engine_status.realtime,
                    detail=engine_status.detail,
                )

            status = ControllerStatus(
                run_id=active.run_id,
                scenario_id=active.scenario_id,
                state=engine_status.state,
                telemetry_events=engine_status.telemetry_events,
                realtime=engine_status.realtime,
                detail=engine_status.detail,
            )

            if status.state in {RunState.COMPLETED, RunState.FAILED}:
                self._engine.wait()
                result = self._engine.result()
                if result is not None:
                    status.telemetry_events = result.telemetry_events
                self._last_status = status
                self._active = None
                if self._context.running:
                    self._context.stop()

            return status

    def _validate_run_id(self, run_id: str) -> None:
        if self._active is None or self._active.run_id != run_id:
            raise RuntimeError(f"Unknown run id: {run_id}")


__all__ = ["ControllerStatus", "SimulationController"]
