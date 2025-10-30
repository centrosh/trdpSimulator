"""Communication layer abstractions for TRDP interactions.

These classes provide seam points to integrate the real TCNopen TRDP
stack while keeping the simulator core testable.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Protocol, runtime_checkable


@dataclass
class TrdpConfig:
    """Configuration values used to initialise TRDP sessions."""

    application_name: str
    redundancy: bool = False


@dataclass
class SessionConfig:
    """Logical endpoint settings for a TRDP session."""

    dataset_id: int
    source_uri: str
    target_uri: str


@dataclass
class PdMessage:
    """Process data telegram definition."""

    dataset_id: int
    payload: bytes


@dataclass
class MdMessage:
    """Message data telegram definition."""

    label: str
    payload: bytes


@dataclass
class DeliveryReport:
    """Lightweight delivery report placeholder."""

    success: bool
    detail: str | None = None


@runtime_checkable
class PdHandler(Protocol):
    """Callable that can consume PD telegram payloads."""

    def __call__(self, message: PdMessage) -> None:  # pragma: no cover - protocol
        ...


class TrdpContext:
    """Manage the lifecycle of the underlying TRDP stack.

    The concrete implementation will wrap the production TRDP C APIs.
    """

    def __init__(self, config: TrdpConfig) -> None:
        self._config = config
        self._running = False

    @property
    def running(self) -> bool:
        """Whether the context is currently active."""

        return self._running

    def start(self) -> None:
        """Start the TRDP application session."""

        self._running = True

    def stop(self) -> None:
        """Stop the TRDP application session."""

        self._running = False


class TrdpSessionManager:
    """Coordinate TRDP session operations for the simulation engine."""

    def __init__(self, context: TrdpContext) -> None:
        self._context = context

    def configure(self, _scenario: object) -> None:
        """Configure TRDP endpoints based on the provided scenario."""

        if not self._context.running:
            raise RuntimeError("TRDP context must be running before configuration")

    def publish_pd(self, message: PdMessage) -> DeliveryReport:
        """Publish a PD telegram (placeholder implementation)."""

        return DeliveryReport(success=self._context.running, detail="pd")

    def send_md(self, message: MdMessage) -> DeliveryReport:
        """Send an MD telegram (placeholder implementation)."""

        return DeliveryReport(success=self._context.running, detail=message.label)

    def subscribe_pd(self, _subscription: object, handler: PdHandler) -> str:
        """Register a PD subscription handler."""

        handler(PdMessage(dataset_id=0, payload=b""))
        return "sub-1"

    def shutdown(self) -> None:
        """Tear down all TRDP sessions."""

        self._context.stop()
