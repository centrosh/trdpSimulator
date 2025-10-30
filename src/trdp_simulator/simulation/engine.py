"""Core simulation engine orchestration primitives."""
from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass
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

    def register_observer(self, observer: SimulationObserver) -> None:
        """Register an observer to receive lifecycle events."""

        self._observers.append(observer)

    def _notify(self, event: str) -> None:
        for observer in self._observers:
            observer.on_event(event)

    def run(self, scenario: Scenario, options: RunOptions | None = None) -> SimulationResult:
        """Execute the provided scenario using the TRDP session manager."""

        opts = options or RunOptions()
        self._notify(f"starting:{scenario.identifier}")
        telemetry: list[str] = []
        self._comm.configure(scenario)
        for message in scenario.pd_messages:
            report = self._comm.publish_pd(message)
            telemetry.append(f"pd:{message.dataset_id}:{report.success}")
        self._notify(f"finished:{scenario.identifier}:{opts.realtime}")
        return SimulationResult(success=True, telemetry_events=telemetry)

    def apply_fault(self, fault: FaultDescriptor) -> DeliveryReport:
        """Demonstrate how faults trigger MD communications."""

        report = self._comm.send_md(MdMessage(label=fault.name, payload=b""))
        self._notify(f"fault:{fault.name}:{fault.severity}")
        return report
