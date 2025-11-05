"""FastAPI application exposing simulation control endpoints."""
from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from fastapi import FastAPI, HTTPException
from fastapi.responses import HTMLResponse, JSONResponse

from trdp_simulator.communication.wrapper import PdMessage
from trdp_simulator.api.catalog import CatalogService
from trdp_simulator.simulation.controller import ControllerStatus, SimulationController
from trdp_simulator.simulation.engine import RunOptions, RunState


@dataclass
class MessageSpec:
    """Input specification for PD messages."""

    dataset_id: int
    payload: bytes

    @classmethod
    def from_dict(cls, data: dict[str, object]) -> "MessageSpec":
        try:
            dataset_id = int(data["dataset_id"])
            payload_raw = data.get("payload", "")
        except (KeyError, TypeError, ValueError) as exc:  # pragma: no cover - defensive
            raise HTTPException(status_code=400, detail="Invalid message specification") from exc
        if not isinstance(payload_raw, str):
            raise HTTPException(status_code=400, detail="Payload must be a string")
        return cls(dataset_id=dataset_id, payload=payload_raw.encode("utf-8"))


def _to_pd_messages(specs: Iterable[MessageSpec]) -> list[PdMessage]:
    return [PdMessage(dataset_id=spec.dataset_id, payload=spec.payload) for spec in specs]


def _status_response(status: ControllerStatus) -> JSONResponse:
    return JSONResponse(
        {
            "run_id": status.run_id,
            "scenario_id": status.scenario_id,
            "state": status.state.name.lower(),
            "telemetry": status.telemetry_events,
            "realtime": status.realtime,
            "detail": status.detail,
        }
    )


def create_app(
    controller: SimulationController | None = None,
    catalog: CatalogService | None = None,
) -> FastAPI:
    """Create a configured FastAPI application."""

    controller = controller or SimulationController()
    catalog = catalog or CatalogService()
    app = FastAPI(title="TRDP Simulator Automation API")

    ui_path = Path(__file__).resolve().parents[3] / "resources" / "ui" / "dashboard.html"
    ui_content = "<h1>TRDP Simulator</h1>"
    if ui_path.exists():
        ui_content = ui_path.read_text(encoding="utf-8")

    @app.get("/ui")
    def ui_page() -> HTMLResponse:
        return HTMLResponse(ui_content)

    @app.post("/runs")
    def start_run(payload: dict[str, object]) -> JSONResponse:
        scenario_id = payload.get("scenario_id")
        if not isinstance(scenario_id, str) or not scenario_id:
            raise HTTPException(status_code=400, detail="scenario_id is required")

        messages_input = payload.get("messages", [])
        if not isinstance(messages_input, list):
            raise HTTPException(status_code=400, detail="messages must be a list")
        message_specs = [MessageSpec.from_dict(item) for item in messages_input]
        flat_messages = _to_pd_messages(message_specs)

        options_dict = payload.get("options", {})
        if not isinstance(options_dict, dict):
            raise HTTPException(status_code=400, detail="options must be an object")
        step_delay = float(options_dict.get("step_delay", 0.0))
        realtime = bool(options_dict.get("realtime", False))
        options = RunOptions(realtime=realtime, step_delay=step_delay)

        try:
            run_id = controller.start_run(scenario_id, flat_messages, options)
        except RuntimeError as exc:
            raise HTTPException(status_code=409, detail=str(exc)) from exc
        status = controller.status(run_id)
        body = {
            "run_id": run_id,
            "state": status.state.name.lower(),
            "telemetry": status.telemetry_events,
            "realtime": status.realtime,
        }
        return JSONResponse(body, status_code=201)

    @app.get("/runs/{run_id}")
    def get_status(run_id: str) -> JSONResponse:
        try:
            status = controller.status(run_id)
        except RuntimeError as exc:
            raise HTTPException(status_code=404, detail=str(exc)) from exc
        return _status_response(status)

    @app.post("/runs/{run_id}/pause")
    def pause_run(run_id: str) -> JSONResponse:
        try:
            controller.pause_run(run_id)
            status = controller.status(run_id)
        except RuntimeError as exc:
            raise HTTPException(status_code=400, detail=str(exc)) from exc
        return _status_response(status)

    @app.post("/runs/{run_id}/resume")
    def resume_run(run_id: str) -> JSONResponse:
        try:
            controller.resume_run(run_id)
            status = controller.status(run_id)
        except RuntimeError as exc:
            raise HTTPException(status_code=400, detail=str(exc)) from exc
        return _status_response(status)

    @app.get("/scenarios")
    def list_scenarios() -> JSONResponse:
        items = [
            {
                "id": summary.identifier,
                "device": summary.device,
                "path": str(summary.path),
                "events": summary.events,
            }
            for summary in catalog.list_scenarios()
        ]
        return JSONResponse({"items": items})

    @app.get("/devices")
    def list_devices() -> JSONResponse:
        items = [
            {
                "name": device.name,
                "path": str(device.path),
            }
            for device in catalog.list_devices()
        ]
        return JSONResponse({"items": items})

    @app.post("/scenarios/validate")
    def validate_scenario(payload: dict[str, object]) -> JSONResponse:
        content = payload.get("content")
        if not isinstance(content, str):
            raise HTTPException(status_code=400, detail="content must be a string")
        valid, errors = catalog.validate_scenario(content)
        status_code = 200 if valid else 422
        return JSONResponse({"valid": valid, "errors": errors}, status_code=status_code)

    return app


def run_api() -> None:
    """Launch the API server using uvicorn if available."""

    try:
        import uvicorn
    except ImportError as exc:  # pragma: no cover - executed when optional dep missing
        raise SystemExit("uvicorn dependency is required to run the API server") from exc

    app = create_app()
    uvicorn.run(app, host="127.0.0.1", port=8000)
