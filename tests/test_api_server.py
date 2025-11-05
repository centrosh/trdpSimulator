from __future__ import annotations

import time

from fastapi.testclient import TestClient

from trdp_simulator.api.catalog import CatalogService
from trdp_simulator.api.server import create_app
from trdp_simulator.simulation.controller import SimulationController
from trdp_simulator.simulation.engine import RunState


def test_api_lifecycle_endpoints() -> None:
    controller = SimulationController()
    app = create_app(controller)
    client = TestClient(app)

    response = client.post(
        "/runs",
        json={
            "scenario_id": "demo",
            "messages": [
                {"dataset_id": 1, "payload": "alpha"},
                {"dataset_id": 2, "payload": "beta"},
            ],
            "options": {"step_delay": 0.05},
        },
    )
    assert response.status_code == 201
    run_id = response.json()["run_id"]

    pause = client.post(f"/runs/{run_id}/pause")
    assert pause.status_code == 200
    assert pause.json()["state"] == "paused"

    resume = client.post(f"/runs/{run_id}/resume")
    assert resume.status_code == 200

    # wait for completion
    deadline = time.time() + 1.0
    final_state = None
    while time.time() < deadline:
        status = client.get(f"/runs/{run_id}")
        assert status.status_code == 200
        final_state = status.json()["state"]
        if final_state in {"completed", "failed"}:
            break
        time.sleep(0.05)
    assert final_state == "completed"

    status = client.get(f"/runs/{run_id}")
    payload = status.json()
    assert payload["telemetry"] == ["pd:1:True", "pd:2:True"]


def test_api_rejects_duplicate_runs() -> None:
    controller = SimulationController()
    app = create_app(controller)
    client = TestClient(app)

    first = client.post(
        "/runs",
        json={
            "scenario_id": "demo",
            "messages": [{"dataset_id": 1, "payload": "alpha"}],
            "options": {"step_delay": 0.1},
        },
    )
    assert first.status_code == 201
    run_id = first.json()["run_id"]

    second = client.post("/runs", json={"scenario_id": "demo"})
    assert second.status_code == 409

    deadline = time.time() + 1.0
    while time.time() < deadline:
        state = controller.status(run_id).state
        if state in {RunState.COMPLETED, RunState.FAILED}:
            break
        time.sleep(0.05)


def test_catalog_endpoints_and_ui(tmp_path) -> None:
    catalog_root = tmp_path
    scenario = catalog_root / "demo.yaml"
    scenario.write_text(
        "scenario: demo\n"
        "device: unit1\n"
        "events:\n"
        "  - type: pd\n"
        "    label: start\n"
        "    com_id: 1\n"
        "    dataset_id: 1\n",
        encoding="utf-8",
    )
    device = catalog_root / "unit1.xml"
    device.write_text("<device name='unit1' />", encoding="utf-8")

    catalog = CatalogService(base_path=catalog_root)
    controller = SimulationController()
    app = create_app(controller, catalog=catalog)
    client = TestClient(app)

    scenarios = client.get("/scenarios")
    assert scenarios.status_code == 200
    payload = scenarios.json()
    assert payload["items"][0]["id"] == "demo"

    devices = client.get("/devices")
    assert devices.status_code == 200
    assert devices.json()["items"][0]["name"] == "unit1"

    validation = client.post("/scenarios/validate", json={"content": scenario.read_text()})
    assert validation.status_code == 200
    assert validation.json()["valid"] is True

    invalid = client.post("/scenarios/validate", json={"content": "device: missing"})
    assert invalid.status_code == 422
    assert invalid.json()["valid"] is False

    ui = client.get("/ui")
    assert "TRDP" in ui.text
