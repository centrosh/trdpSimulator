# Contributing to the TRDP Simulator

Thank you for your interest in contributing! This document outlines the
standards, workflows, and quality gates agreed upon during the design
review.

## Coding Standards

- **Language:** Python 3.11+ for orchestration logic. The TRDP C wrapper
  will be encapsulated behind Python interfaces so it can be swapped with
  native modules later.
- **Style:** Follow [PEP 8] with a 100-character line limit enforced via
  `ruff`. Docstrings use the NumPy style to aid documentation tooling.
- **Type Hints:** All new public functions and classes must include
  Python type hints. Enable `from __future__ import annotations` in new
  modules to avoid runtime imports.
- **Error Handling:** Wrap TRDP error codes in typed exceptions and avoid
  swallowing stack traces. Use `RuntimeError` placeholders until the
  concrete wrapper is implemented.
- **Logging & Telemetry:** Prefer structured logging (JSON or key/value)
  and emit telemetry events through the telemetry subsystem.

## Testing Strategy

- **Unit Tests:** Use `pytest` for unit testing. Each new module should
  include tests for happy-path and failure-path behaviour.
- **Integration Tests:** Provide lightweight TRDP mocks to run
  integration tests without the native stack. Integration tests will live
  under `tests/integration/` once implemented.
- **Coverage:** Keep coverage above 85% for new code. Run `pytest --cov`
  locally before opening a PR when coverage-sensitive changes are made.
- **Static Analysis:** Run `ruff check` locally and ensure CI passes
  before submitting changes. Future work will add type checking via
  `mypy` once interfaces stabilise.

## Development Workflow

1. Fork the repository and create a feature branch following the naming
   convention `feature/<short-description>`.
2. Install dependencies with `make install` (requires Python 3.11).
3. Run `make check` to execute linting and tests before committing.
4. Write clear commit messages using the imperative mood (e.g., "Add
   scenario loader").
5. Open a pull request referencing any relevant issue IDs (see
   [`docs/issue-tracker.md`](docs/issue-tracker.md)).
6. Ensure the PR includes a summary, testing notes, and screenshots for
   UI changes.

## Code Review Guidelines

- Reviewers should ensure that new changes respect the architecture
  defined in [`docs/design.md`](docs/design.md) and align with the
  milestones in [`docs/milestones.md`](docs/milestones.md).
- Request changes for missing tests, absent documentation, or deviations
  from the coding standards.
- Use GitHub suggestions where small inline fixes can unblock the PR.

## Branch Protection & CI

- The `main` branch is protected. Merges require at least one approving
  review and a passing CI run (`lint` + `pytest`).
- CI is configured via GitHub Actions in `.github/workflows/ci.yml`.

## Release Management

- Tag releases using semantic versioning (`vMAJOR.MINOR.PATCH`).
- Update `docs/milestones.md` and `docs/issue-tracker.md` with progress
  notes when closing a milestone.

## Reporting Security Issues

Please report security vulnerabilities privately to the maintainers. Do
not open public issues containing sensitive details.

[PEP 8]: https://peps.python.org/pep-0008/
