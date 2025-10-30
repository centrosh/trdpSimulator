"""Command-line entrypoints for the TRDP simulator scaffolding."""
from __future__ import annotations

import argparse
from collections.abc import Iterable

from trdp_simulator.communication.wrapper import (
    PdMessage,
    TrdpConfig,
    TrdpContext,
    TrdpSessionManager,
)
from trdp_simulator.simulation.engine import RunOptions, Scenario, SimulationEngine


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="TRDP Simulator prototype")
    parser.add_argument("scenario", help="Scenario identifier to execute")
    parser.add_argument("--realtime", action="store_true", help="Run in realtime mode")
    return parser


def run_cli(argv: Iterable[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    context = TrdpContext(TrdpConfig(application_name="trdp-sim"))
    context.start()
    session = TrdpSessionManager(context)
    engine = SimulationEngine(session)
    scenario = Scenario(
        identifier=args.scenario,
        pd_messages=[PdMessage(dataset_id=1, payload=b"demo")],
    )
    engine.run(scenario, RunOptions(realtime=args.realtime))
    session.shutdown()
    return 0


if __name__ == "__main__":  # pragma: no cover - manual invocation
    raise SystemExit(run_cli())
