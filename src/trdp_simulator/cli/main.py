"""Automation-friendly CLI for controlling simulator runs."""
from __future__ import annotations

import argparse
import json
from typing import Iterable
import urllib.error
import urllib.request


DEFAULT_API_URL = "http://127.0.0.1:8000"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="TRDP Simulator automation CLI")
    subparsers = parser.add_subparsers(dest="command", required=True)

    run_parser = subparsers.add_parser("run", help="Start a simulation run")
    run_parser.add_argument("scenario", help="Scenario identifier to execute")
    run_parser.add_argument(
        "--message",
        action="append",
        metavar="DATASET:PAYLOAD",
        help="Process data message to include (can be repeated)",
    )
    run_parser.add_argument("--realtime", action="store_true", help="Execute in realtime mode")
    run_parser.add_argument(
        "--step-delay",
        type=float,
        default=0.0,
        help="Delay in seconds between PD messages",
    )
    run_parser.add_argument("--api-url", default=DEFAULT_API_URL, help="Automation API endpoint")

    pause_parser = subparsers.add_parser("pause", help="Pause an active run")
    pause_parser.add_argument("run_id", help="Run identifier returned by the run command")
    pause_parser.add_argument("--api-url", default=DEFAULT_API_URL, help="Automation API endpoint")

    resume_parser = subparsers.add_parser("resume", help="Resume a paused run")
    resume_parser.add_argument("run_id", help="Run identifier returned by the run command")
    resume_parser.add_argument("--api-url", default=DEFAULT_API_URL, help="Automation API endpoint")

    status_parser = subparsers.add_parser("status", help="Inspect run status")
    status_parser.add_argument("run_id", help="Run identifier returned by the run command")
    status_parser.add_argument("--api-url", default=DEFAULT_API_URL, help="Automation API endpoint")

    return parser


def _api_request(url: str, method: str = "GET", payload: dict[str, object] | None = None) -> dict[str, object]:
    data = json.dumps(payload).encode("utf-8") if payload is not None else None
    request = urllib.request.Request(url, data=data, method=method)
    request.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            raw = response.read().decode("utf-8")
    except urllib.error.HTTPError as exc:  # pragma: no cover - exercised via CLI tests
        detail = exc.read().decode("utf-8")
        raise RuntimeError(f"API error {exc.code}: {detail}") from exc
    except urllib.error.URLError as exc:  # pragma: no cover - network issues
        raise RuntimeError(f"Failed to reach API: {exc.reason}") from exc
    return json.loads(raw)


def _parse_messages(specs: Iterable[str] | None) -> list[dict[str, object]]:
    messages: list[dict[str, object]] = []
    if not specs:
        return messages
    for spec in specs:
        parts = spec.split(":", 1)
        if len(parts) != 2:
            raise ValueError(f"Invalid message specification: {spec}")
        dataset, payload = parts
        messages.append({"dataset_id": int(dataset), "payload": payload})
    return messages


def run_cli(argv: Iterable[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.command == "run":
        try:
            messages = _parse_messages(args.message)
        except ValueError as exc:
            parser.error(str(exc))
            return 2
        body = {
            "scenario_id": args.scenario,
            "messages": messages,
            "options": {"realtime": args.realtime, "step_delay": args.step_delay},
        }
        response = _api_request(f"{args.api_url}/runs", method="POST", payload=body)
        run_id = response["run_id"]
        print(f"Run started: {run_id} (state={response['state']})")
        return 0

    if args.command == "pause":
        response = _api_request(f"{args.api_url}/runs/{args.run_id}/pause", method="POST")
        print(f"Run {response['run_id']} paused (state={response['state']})")
        return 0

    if args.command == "resume":
        response = _api_request(f"{args.api_url}/runs/{args.run_id}/resume", method="POST")
        print(f"Run {response['run_id']} resumed (state={response['state']})")
        return 0

    if args.command == "status":
        response = _api_request(f"{args.api_url}/runs/{args.run_id}")
        telemetry = ", ".join(response.get("telemetry", []))
        detail = response.get("detail")
        extra = f", detail={detail}" if detail else ""
        print(
            f"Run {response['run_id']} state={response['state']} realtime={response['realtime']}"
            f" telemetry=[{telemetry}]{extra}"
        )
        return 0

    parser.error("Unknown command")
    return 2


if __name__ == "__main__":  # pragma: no cover - manual invocation
    raise SystemExit(run_cli())
