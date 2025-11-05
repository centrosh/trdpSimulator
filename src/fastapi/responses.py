"""Response primitives compatible with the lightweight FastAPI shim."""
from __future__ import annotations

from typing import Any, Dict

__all__ = ["Response", "JSONResponse", "HTMLResponse"]


class Response:
    """Base HTTP response container."""

    def __init__(
        self,
        content: Any,
        *,
        status_code: int = 200,
        media_type: str = "text/plain",
        headers: Dict[str, str] | None = None,
    ) -> None:
        self.content = content
        self.status_code = status_code
        self.media_type = media_type
        base_headers = {"content-type": media_type}
        if headers:
            base_headers.update(headers)
        self.headers = base_headers


class JSONResponse(Response):
    """Response with a JSON payload."""

    def __init__(self, content: Any, *, status_code: int = 200) -> None:
        super().__init__(content, status_code=status_code, media_type="application/json")


class HTMLResponse(Response):
    """Response returning HTML content."""

    def __init__(self, content: str, *, status_code: int = 200) -> None:
        super().__init__(content, status_code=status_code, media_type="text/html")
