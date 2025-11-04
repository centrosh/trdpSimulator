# Operations Handbook

This handbook captures operational practices for distributing, releasing, and sustaining the
TRDP Simulator project.

## Distribution Strategy and Deployment Scripts

The simulator is delivered through two complementary channels:

1. **Docker image (`trdp-simulator`):** Provides a reproducible runtime that bundles the C++
   simulator binaries together with the Python CLI entry point. The Dockerfile builds the C++
   targets, installs them under `/opt/trdp`, and pre-installs the Python wheel so the container
   can be invoked directly with `trdp_sim_cli`.
2. **Python package (`trdp-simulator`):** Publishes the CLI tooling to PyPI so teams can embed
   simulator orchestration into their own automation.

The `scripts/` directory contains automation for creating and pushing artefacts:

- `scripts/build_docker.sh [image-tag]` builds the Docker image using `docker/Dockerfile`.
- `scripts/publish_docker.sh <image-tag> <registry>` retags the local image and pushes it to the
  target registry (for example `ghcr.io/my-org`).
- `scripts/build_python_package.sh` creates the wheel and source distribution in `dist/`.
- `scripts/publish_python_package.sh [repository-url]` uploads the Python artefacts to PyPI or an
  alternative repository.

## Device Profile & Scenario Management

Operators configure test runs through TRDP XML device definitions and YAML
scenario files. The CLI fronts the management workflow:

1. **Register a device profile** – `./trdp_sim_cli <scenario> --device-xml <path>`
   copies the XML into `~/.trdp-simulator/devices/<id>.xml`, validates it with
   the bundled schema, and records the checksum and timestamp in
   `manifest.db`. Re-uploading a file with the same checksum reuses the
   original identifier.
2. **List catalogued profiles** – inspect `manifest.db` or integrate against
   `DeviceProfileRepository::list()` for automation.
3. **Persist scenarios** – providing `--scenario-file <path>` loads the YAML
   definition, verifies that the referenced `device` exists, and stores a copy
   under `~/.trdp-simulator/scenarios/<scenario>.yaml` for future reuse.
4. **Inline execution** – if YAML is unavailable, pass `--device <id>` along
   with `--event` arguments to exercise the loopback stack directly. Inline
   events require a previously registered device profile.

Validation failures surface as CLI errors and are also recorded in the wrapper
diagnostics so that automation can detect and report misconfiguration.

### Deployment checklist

1. Run the build and test pipeline (`make test`).
2. Build the Docker image with the desired version tag (for example `scripts/build_docker.sh trdp-simulator:v0.2.0`).
3. Build the Python artefacts (`scripts/build_python_package.sh`).
4. Smoke-test both artefacts locally before pushing (e.g., run `docker run --rm trdp-simulator:v0.2.0 --help`
   and `pip install dist/trdp_simulator-*.whl`).
5. Publish the Docker image and Python package using the scripts above once validation succeeds.

## Release Management Procedures

### Versioning strategy

- Adopt [Semantic Versioning](https://semver.org/) across both the C++ and Python components.
- Keep the project version in `CMakeLists.txt` and `pyproject.toml` aligned. Bump both files in the
  release commit.
- Pre-release builds should use an appended label (e.g., `0.2.0-rc1`) and be published only to
  staging registries.

### Release cadence and branching

- Maintain a protected `main` branch for production-ready code.
- Use feature branches (`feature/<slug>`) or hotfix branches (`hotfix/<slug>`) for focused work.
- Cut release branches (`release/vX.Y.Z`) when stabilising a version. Only fixes approved by release
  managers should merge into a release branch.

### Changelog management

1. Maintain a human-readable `CHANGELOG.md` at the repository root.
2. For each merged pull request, update the **Unreleased** section with a short description.
3. When publishing a release:
   - Move the accumulated entries into a new section labelled with the version and release date.
   - Ensure links to comparison commits are updated.
4. Tag the release in Git (`git tag vX.Y.Z`) after the changelog is committed.

### Release checklist

- [ ] Version bumped in `CMakeLists.txt` and `pyproject.toml`.
- [ ] `CHANGELOG.md` updated and reviewed.
- [ ] Tests passing in CI and locally (`make test`).
- [ ] Docker image and Python package built and smoke-tested.
- [ ] Release branch merged into `main` and tagged.
- [ ] Artefacts published via scripts.

## Maintenance Workflows

### Bug triage

1. Review new issues at least weekly.
2. Validate reproducibility using provided logs or steps; request additional data if needed.
3. Label issues with severity (`S0` critical, `S1` high, `S2` medium, `S3` low) and component tags
   (`communication`, `simulation`, `cli`, etc.).
4. Assign triaged bugs to the appropriate milestone in `docs/milestones.md` and create a patch plan
   referencing relevant modules.

### Feature requests

1. Capture use cases and acceptance criteria in the issue body.
2. Evaluate impact on simulator architecture; record rationale in `docs/design.md` if the change is
   significant.
3. Prioritise via the backlog (`docs/issue-tracker.md`) and schedule into milestones.
4. Break down larger features into trackable subtasks before starting implementation.

### Dependency upgrades

1. Perform a monthly dependency review covering both system packages (Dockerfile) and Python
   dependencies (`pyproject.toml`).
2. Use dedicated `chore/dependency-upgrade-<date>` branches for grouped updates.
3. Run the full test suite and targeted regression tests for affected components.
4. Document notable dependency changes in the changelog.

## User Support and Feedback

- **Support channels:**
  - GitHub Discussions for community Q&A and knowledge sharing.
  - GitHub Issues for bug reports and feature requests.
  - Email alias `support@trdp-simulator.org` for enterprise users requiring private support.
- **Response expectations:**
  - Acknowledge new requests within two business days.
  - Provide initial triage outcomes (accepted, needs info, rejected) within five business days.
- **Feedback loop:**
  - Summarise recurring questions and feature requests in a quarterly report circulated to the core
    maintainers.
  - Feed validated customer requirements back into `docs/issue-tracker.md` and upcoming
    milestones.
  - Use post-release surveys to capture satisfaction metrics and monitor adoption trends.

Adhering to these practices keeps releases predictable, makes maintenance manageable, and ensures
user feedback directly shapes the simulator roadmap.
