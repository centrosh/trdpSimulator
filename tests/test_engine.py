"""Unit tests for the simulation engine scaffolding."""
from trdp_simulator.communication.wrapper import (
    PdMessage,
    TrdpConfig,
    TrdpContext,
    TrdpSessionManager,
)
import time

from trdp_simulator.simulation.engine import RunOptions, RunState, Scenario, SimulationEngine


def test_engine_runs_scenario_and_collects_telemetry() -> None:
    context = TrdpContext(TrdpConfig(application_name="test"))
    context.start()
    manager = TrdpSessionManager(context)
    engine = SimulationEngine(manager)

    scenario = Scenario(identifier="demo", pd_messages=[PdMessage(dataset_id=99, payload=b"demo")])

    result = engine.run(scenario, RunOptions(realtime=True))

    assert result.success is True
    assert result.telemetry_events == ["pd:99:True"]


def test_engine_supports_pause_and_resume() -> None:
    context = TrdpContext(TrdpConfig(application_name="test"))
    context.start()
    manager = TrdpSessionManager(context)
    engine = SimulationEngine(manager)

    scenario = Scenario(
        identifier="demo",
        pd_messages=[
            PdMessage(dataset_id=1, payload=b"a"),
            PdMessage(dataset_id=2, payload=b"b"),
            PdMessage(dataset_id=3, payload=b"c"),
        ],
    )

    engine.start(scenario, RunOptions(step_delay=0.05))
    time.sleep(0.01)
    engine.pause()
    assert engine.status().state == RunState.PAUSED
    engine.resume()
    engine.wait()
    status = engine.status()
    assert status.state == RunState.COMPLETED
    assert len(status.telemetry_events) == 3


def test_engine_requires_context_before_configuring() -> None:
    context = TrdpContext(TrdpConfig(application_name="test"))
    manager = TrdpSessionManager(context)
    engine = SimulationEngine(manager)
    scenario = Scenario(identifier="demo", pd_messages=[])

    try:
        engine.run(scenario)
    except RuntimeError as exc:  # pragma: no cover - raised branch validated
        assert "TRDP context" in str(exc)
    else:  # pragma: no cover - the test fails if no exception raised
        raise AssertionError("Expected RuntimeError when context not started")
