# Contributing to the TRDP Simulator

Thank you for your interest in contributing! This document outlines the
standards, workflows, and quality gates agreed upon during the design
review.

## Coding Standards

- **Language:** C++20 for the simulator core and CLI utilities.
- **Style:** Follow the [C++ Core Guidelines] and keep lines under
  120 characters. Use `clang-format` with the LLVM style as a baseline.
- **Error Handling:** Prefer exceptions for recoverable issues and
  document invariants in header files. Avoid silent failures.
- **Logging & Telemetry:** Provide structured data via dedicated helper
  utilities in the communication layer. Avoid direct `std::cout`
  logging in library code.

## Testing Strategy

- **Unit Tests:** Implement using CTest-compatible executables. Each new
  module should exercise both success and failure flows.
- **Integration Tests:** Provide lightweight TRDP mocks to run integration
  tests without the native stack. Integration tests will live under
  `tests/integration/` once implemented.
- **Coverage:** Keep coverage above 85% for new code. Integrate with
  `gcov`/`llvm-cov` tooling as part of future work.
- **Static Analysis:** Enable `-Wall -Wextra -Wpedantic` warnings and
  address new findings before merging.

## Development Workflow

1. Fork the repository and create a feature branch following the naming
   convention `feature/<short-description>`.
2. Configure the project with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`.
3. Build and run tests locally via `cmake --build build` and
   `ctest --test-dir build`.
4. Write clear commit messages using the imperative mood (e.g., "Add
   scenario scheduler").
5. Open a pull request referencing any relevant issue IDs (see
   [`docs/issue-tracker.md`](docs/issue-tracker.md)).
6. Ensure the PR includes a summary, testing notes, and screenshots for UI
   changes.

## Code Review Guidelines

- Reviewers should ensure that new changes respect the architecture
  defined in [`docs/design.md`](docs/design.md) and align with the
  milestones in [`docs/milestones.md`](docs/milestones.md).
- Request changes for missing tests, absent documentation, or deviations
  from the coding standards.
- Use GitHub suggestions where small inline fixes can unblock the PR.

## Branch Protection & CI

- The `main` branch is protected. Merges require at least one approving
  review and a passing CI run (configure, build, test).
- CI is configured via GitHub Actions in `.github/workflows/ci.yml`.

## Release Management

- Tag releases using semantic versioning (`vMAJOR.MINOR.PATCH`).
- Update `docs/milestones.md` and `docs/issue-tracker.md` with progress
  notes when closing a milestone.

## Reporting Security Issues

Please report security vulnerabilities privately to the maintainers. Do
not open public issues containing sensitive details.

[C++ Core Guidelines]: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines
