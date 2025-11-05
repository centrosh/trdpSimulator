from __future__ import annotations

from trdp_simulator.cli.main import run_cli


def test_cli_run_command_invokes_api(monkeypatch) -> None:
    captured = {}

    def fake_request(url: str, method: str = "GET", payload: dict | None = None) -> dict:
        captured["url"] = url
        captured["method"] = method
        captured["payload"] = payload
        return {"run_id": "abc", "state": "running", "telemetry": [], "realtime": False}

    monkeypatch.setattr("trdp_simulator.cli.main._api_request", fake_request)
    exit_code = run_cli(["run", "demo", "--message", "1:data", "--api-url", "http://example"])
    assert exit_code == 0
    assert captured["url"].endswith("/runs")
    assert captured["method"] == "POST"
    assert captured["payload"]["scenario_id"] == "demo"


def test_cli_pause_resume_and_status(monkeypatch, capsys) -> None:
    responses = iter(
        [
            {"run_id": "abc", "state": "paused", "telemetry": [], "realtime": False},
            {"run_id": "abc", "state": "running", "telemetry": [], "realtime": False},
            {"run_id": "abc", "state": "completed", "telemetry": ["pd:1:True"], "realtime": False},
        ]
    )

    def fake_request(url: str, method: str = "GET", payload: dict | None = None) -> dict:
        return next(responses)

    monkeypatch.setattr("trdp_simulator.cli.main._api_request", fake_request)
    assert run_cli(["pause", "abc"]) == 0
    assert run_cli(["resume", "abc"]) == 0
    assert run_cli(["status", "abc"]) == 0
    output = capsys.readouterr().out
    assert "state=completed" in output
    assert "pd:1:True" in output


def test_cli_catalog_commands(monkeypatch, capsys, tmp_path) -> None:
    captured: dict[str, object] = {}

    def fake_request(url: str, method: str = "GET", payload: dict | None = None) -> dict:
        captured.setdefault(url, 0)
        captured[url] = int(captured[url]) + 1
        if url.endswith("/scenarios"):
            return {
                "items": [
                    {"id": "loop", "device": "device1", "events": 2, "path": "demo.yaml"},
                ]
            }
        if url.endswith("/devices"):
            return {"items": [{"name": "device1", "path": "device1.xml"}]}
        if url.endswith("/scenarios/validate"):
            return {"valid": True, "errors": []}
        raise AssertionError(f"Unexpected URL {url}")

    monkeypatch.setattr("trdp_simulator.cli.main._api_request", fake_request)

    assert run_cli(["scenarios"]) == 0
    assert run_cli(["devices"]) == 0

    scenario_file = tmp_path / "scenario.yaml"
    scenario_file.write_text("scenario: loop\ndevice: device1\nevents:\n  - type: pd\n    label: start\n")
    assert run_cli(["validate", str(scenario_file)]) == 0

    output = capsys.readouterr().out
    assert "loop" in output
    assert "device1" in output
    assert "Scenario validated successfully" in output
