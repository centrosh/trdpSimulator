"""Utilities for discovering scenarios and devices for the automation API."""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import xml.etree.ElementTree as ET
from typing import Tuple


@dataclass(slots=True)
class ScenarioSummary:
    identifier: str
    device: str
    path: Path
    events: int


@dataclass(slots=True)
class DeviceSummary:
    name: str
    path: Path


class CatalogService:
    """Discover and validate catalogue assets used by the automation surfaces."""

    def __init__(self, base_path: Path | None = None) -> None:
        root = Path(__file__).resolve().parents[3]
        default_base = root / "resources" / "trdp"
        self._base = Path(base_path) if base_path is not None else default_base

    def list_scenarios(self) -> list[ScenarioSummary]:
        items: list[ScenarioSummary] = []
        for path in sorted(self._base.glob("*.yaml")):
            try:
                content = path.read_text(encoding="utf-8")
            except OSError:
                continue
            valid, _ = self.validate_scenario(content)
            if not valid:
                continue
            metadata = _extract_metadata(content)
            if metadata is None:
                continue
            identifier, device, events = metadata
            items.append(ScenarioSummary(identifier=identifier, device=device, path=path, events=events))
        return items

    def list_devices(self) -> list[DeviceSummary]:
        items: list[DeviceSummary] = []
        for path in sorted(self._base.glob("*.xml")):
            items.append(DeviceSummary(name=_device_name(path), path=path))
        return items

    def validate_scenario(self, content: str) -> Tuple[bool, list[str]]:
        """Validate a scenario definition using lightweight rules."""

        errors: list[str] = []
        if not content.strip():
            return False, ["Scenario content is empty"]

        scenario = None
        device = None
        in_events = False
        event_fields: set[str] = set()
        event_count = 0

        def flush_event() -> None:
            nonlocal event_fields, event_count
            if not event_fields:
                return
            required = {"type", "label"}
            missing = sorted(required - event_fields)
            if missing:
                errors.append(f"Event missing required fields: {', '.join(missing)}")
            event_fields = set()

        for raw_line in content.splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            if line == "events:":
                in_events = True
                flush_event()
                continue
            if not in_events:
                if line.startswith("scenario:"):
                    scenario = line.split(":", 1)[1].strip()
                elif line.startswith("device:"):
                    device = line.split(":", 1)[1].strip()
                continue

            if line.startswith("- "):
                flush_event()
                event_count += 1
                payload = line[2:].strip()
                if payload:
                    key, _, value = payload.partition(":")
                    event_fields.add(key.strip())
                    if key.strip() in {"com_id", "dataset_id"} and not value.strip().isdigit():
                        errors.append(f"Event field {key.strip()} must be numeric")
                continue

            key, _, value = line.partition(":")
            key = key.strip()
            value = value.strip()
            if not key:
                continue
            event_fields.add(key)
            if key in {"com_id", "dataset_id"} and value and not value.isdigit():
                errors.append(f"Event field {key} must be numeric")

        flush_event()

        if scenario is None or not scenario:
            errors.append("scenario field is required")
        if device is None or not device:
            errors.append("device field is required")
        if event_count == 0:
            errors.append("At least one event must be defined")

        return len(errors) == 0, errors


def _extract_metadata(content: str) -> Tuple[str, str, int] | None:
    scenario = None
    device = None
    events = 0
    in_events = False
    for raw_line in content.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if line == "events:":
            in_events = True
            continue
        if not in_events:
            if line.startswith("scenario:"):
                scenario = line.split(":", 1)[1].strip()
            elif line.startswith("device:"):
                device = line.split(":", 1)[1].strip()
            continue
        if line.startswith("- "):
            events += 1
    if scenario is None or not scenario or device is None or not device:
        return None
    return scenario, device, events


def _device_name(path: Path) -> str:
    try:
        tree = ET.parse(path)
        root = tree.getroot()
        name = root.attrib.get("name")
        if name:
            return name
        candidate = root.findtext("name") or root.findtext("Name")
        if candidate:
            return candidate
    except ET.ParseError:
        pass
    return path.stem


__all__ = ["CatalogService", "DeviceSummary", "ScenarioSummary"]
