"""Top-level package for the TRDP Simulator implementation.

This package provides a modular architecture matching the approved
design in :mod:`docs.design` with clear separation between
communication, simulation orchestration, scenario management, user
interfaces, and telemetry capture.
"""

from .communication.wrapper import TrdpContext, TrdpSessionManager
from .simulation.engine import SimulationEngine

__all__ = [
    "SimulationEngine",
    "TrdpContext",
    "TrdpSessionManager",
]
