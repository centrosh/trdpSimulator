"""Lightweight subset of the FastAPI interface used for tests."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable, Dict, List, Optional, Tuple

from .responses import HTMLResponse, JSONResponse, Response

__all__ = ["FastAPI", "HTTPException"]


class HTTPException(Exception):
    """Exception carrying HTTP status metadata for route handlers."""

    def __init__(self, status_code: int, detail: str | None = None) -> None:
        super().__init__(detail)
        self.status_code = status_code
        self.detail = detail or ""


@dataclass
class _Route:
    method: str
    template: str
    segments: List[str]
    handler: Callable[..., Any]

    def match(self, method: str, path: str) -> Optional[Dict[str, str]]:
        if self.method != method.upper():
            return None
        parts = [segment for segment in path.strip("/").split("/") if segment]
        if not self.segments and not parts:
            return {}
        if len(parts) != len(self.segments):
            return None
        params: Dict[str, str] = {}
        for pattern, value in zip(self.segments, parts):
            if pattern.startswith("{") and pattern.endswith("}"):
                params[pattern[1:-1]] = value
            elif pattern != value:
                return None
        return params


class FastAPI:
    """Minimal router supporting the subset exercised in tests."""

    def __init__(self, title: str | None = None) -> None:
        self.title = title
        self._routes: List[_Route] = []

    def post(self, path: str) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        return self._register("POST", path)

    def get(self, path: str) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        return self._register("GET", path)

    def _register(self, method: str, path: str) -> Callable[[Callable[..., Any]], Callable[..., Any]]:
        segments = [segment for segment in path.strip("/").split("/") if segment]

        def decorator(func: Callable[..., Any]) -> Callable[..., Any]:
            self._routes.append(_Route(method.upper(), path, segments, func))
            return func

        return decorator

    def _resolve(self, method: str, path: str) -> Tuple[Callable[..., Any], Dict[str, str]]:
        for route in self._routes:
            params = route.match(method, path)
            if params is not None:
                return route.handler, params
        raise HTTPException(status_code=404, detail=f"No route for {method.upper()} {path}")

    def _dispatch(
        self, method: str, path: str, body: Any | None = None
    ) -> Response:
        handler, params = self._resolve(method.upper(), path)
        try:
            if body is not None:
                result = handler(body, **params)
            else:
                result = handler(**params)
        except HTTPException as exc:
            return JSONResponse({"detail": exc.detail}, status_code=exc.status_code)

        if isinstance(result, Response):
            return result
        if isinstance(result, (dict, list)):
            return JSONResponse(result)
        if isinstance(result, str):
            return HTMLResponse(result)
        return JSONResponse({"detail": "Unsupported response type"}, status_code=500)
