from __future__ import annotations

import time

from trdp_simulator.communication.wrapper import PdMessage
from trdp_simulator.simulation.controller import SimulationController
from trdp_simulator.simulation.engine import RunOptions, RunState


def test_controller_start_pause_resume_and_complete() -> None:
    controller = SimulationController()
    run_id = controller.start_run(
        "demo",
        [
            PdMessage(dataset_id=1, payload=b"a"),
            PdMessage(dataset_id=2, payload=b"b"),
        ],
        RunOptions(step_delay=0.05),
    )

    status = controller.status(run_id)
    assert status.state in {RunState.RUNNING, RunState.PAUSED}

    controller.pause_run(run_id)
    paused = controller.status(run_id)
    assert paused.state == RunState.PAUSED

    controller.resume_run(run_id)
    deadline = time.time() + 1.0
    final = controller.status(run_id)
    while final.state not in {RunState.COMPLETED, RunState.FAILED} and time.time() < deadline:
        time.sleep(0.05)
        final = controller.status(run_id)
    assert final.state == RunState.COMPLETED
    assert final.telemetry_events == ["pd:1:True", "pd:2:True"]


def test_controller_rejects_unknown_run() -> None:
    controller = SimulationController()
    try:
        controller.pause_run("missing")
    except RuntimeError as exc:
        assert "Unknown run id" in str(exc)
    else:  # pragma: no cover - guard against false negatives
        raise AssertionError("Expected RuntimeError when pausing unknown run")
