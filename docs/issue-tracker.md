# Issue Tracker Backlog

This backlog mirrors the milestones defined in
[`milestones.md`](milestones.md) and can be imported into the issue
tracker of choice. Each issue references its milestone and includes a
story-point estimate.

| ID | Title | Milestone | Priority | Estimate |
| --- | --- | --- | --- | --- |
| COMM-1 | Bootstrap TRDP context lifecycle management | Milestone 1 | P0 | 3 |
| COMM-2 | Implement PD publish wrapper with retry policy | Milestone 1 | P0 | 5 |
| COMM-3 | Implement MD send/receive with callbacks | Milestone 1 | P0 | 5 |
| COMM-4 | Add diagnostic hooks for TRDP layer | Milestone 1 | P1 | 3 |
| ENG-1 | Load scenario definitions from YAML | Milestone 2 | P0 | 5 |
| ENG-2 | Implement event scheduler for scenario timelines | Milestone 2 | P0 | 8 |
| ENG-3 | Provide deterministic replay telemetry export | Milestone 2 | P1 | 5 |
| ENG-4 | Build TRDP mock layer for CI integration tests | Milestone 2 | P1 | 5 |
| REPO-1 | Define JSON/YAML schemas for scenario validation | Milestone 3 | P0 | 5 |
| REPO-2 | Implement scenario repository service | Milestone 3 | P0 | 8 |
| REPO-3 | Add import/export tooling for scenarios and assets | Milestone 3 | P1 | 5 |
| UI-1 | Extend CLI with run/pause/resume commands | Milestone 4 | P0 | 5 |
| UI-2 | Implement REST control plane | Milestone 4 | P0 | 8 |
| UI-3 | Create telemetry visualisation UI dashboard | Milestone 4 | P1 | 8 |
| UI-4 | Implement results export formats | Milestone 4 | P1 | 3 |
| OBS-1 | Add structured logging and metrics pipeline | Milestone 5 | P0 | 5 |
| OBS-2 | Implement redundancy/retry configuration | Milestone 5 | P0 | 5 |
| OBS-3 | Provide reporting interface for persisted simulations | Milestone 5 | P1 | 5 |

## Import Instructions

1. Create milestones in your issue tracker matching the titles from
   [`milestones.md`](milestones.md).
2. For each row above, create an issue with the given identifier as the
   external reference, link it to the milestone, and apply the priority
   label.
3. Track progress using burn-down charts per milestone, ensuring that
   P0 issues are completed before moving to the next milestone.
