"""Unit tests for the simulation engine scaffolding."""
from trdp_simulator.communication.wrapper import (
    PdMessage,
    TrdpConfig,
    TrdpContext,
    TrdpSessionManager,
)
from trdp_simulator.simulation.engine import RunOptions, Scenario, SimulationEngine


def test_engine_runs_scenario_and_collects_telemetry() -> None:
    context = TrdpContext(TrdpConfig(application_name="test"))
    context.start()
    manager = TrdpSessionManager(context)
    engine = SimulationEngine(manager)

    scenario = Scenario(identifier="demo", pd_messages=[PdMessage(dataset_id=99, payload=b"demo")])

    result = engine.run(scenario, RunOptions(realtime=True))

    assert result.success is True
    assert result.telemetry_events == ["pd:99:True"]


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
