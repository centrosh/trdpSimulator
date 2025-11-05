"""Minimal TestClient compatible with the FastAPI shim."""
from __future__ import annotations

import json
from typing import Any

from .responses import Response

__all__ = ["TestClient"]


class _TestResponse:
    def __init__(self, response: Response) -> None:
        self.status_code = response.status_code
        self._content = response.content
        self.headers = response.headers

    def json(self) -> Any:
        if isinstance(self._content, (dict, list, int, float, bool)) or self._content is None:
            return self._content
        raise ValueError("Response does not contain JSON content")

    @property
    def text(self) -> str:
        if isinstance(self._content, str):
            return self._content
        return json.dumps(self._content)


class TestClient:
    """Exercise FastAPI routes without spinning up an HTTP server."""

    def __init__(self, app: Any) -> None:
        self._app = app

    def get(self, path: str) -> _TestResponse:
        return self.request("GET", path)

    def post(self, path: str, json: Any | None = None) -> _TestResponse:
        return self.request("POST", path, json=json)

    def request(self, method: str, path: str, json: Any | None = None) -> _TestResponse:
        response = self._app._dispatch(method, path, json)  # type: ignore[attr-defined]
        return _TestResponse(response)


TestClient.__test__ = False  # type: ignore[attr-defined]
