# TRDP XML Example Reference

This note summarises the structure and intent of the XML configuration files that
ship with the TCNOpen TRDP test assets. The files control how a TRDP device
allocates memory, configures its interfaces, and declares the telegrams and
data sets it publishes or subscribes to. The examples live under
`external/tcnopen_trdp/trdp/test/xml` once the upstream stack is checked out as
instructed in [docs/TCNOpen_TRDP.md](TCNOpen_TRDP.md).

## Core elements and attributes

The examples share a common shape:

```xml
<device host-name="device1" leader-name="device1" type="dummy">
  <device-configuration memory-size="65535">
    <mem-block-list>
      <mem-block size="72" preallocate="256" />
    </mem-block-list>
  </device-configuration>
  <bus-interface-list>
    <bus-interface network-id="1" name="eth0" host-ip="10.0.1.100">
      ...
    </bus-interface>
  </bus-interface-list>
  <com-parameter-list>
    <com-parameter id="1" qos="5" ttl="64" />
  </com-parameter-list>
  <data-set-list>
    <data-set name="testDS1001" id="1001">...</data-set>
  </data-set-list>
  <debug file-name="trdp.log" file-size="1000000" level="E" />
</device>
```

Key attributes appear repeatedly:

- **`host-name` / `leader-name` / `type`**: Identify the device within the TRDP
  domain. The examples use `dummy` types suitable for simulations.
- **`memory-size`**: Bounds the pool available to the TRDP stack. Larger
  scenarios (for example the speed tests) allocate additional bytes to buffer
  high throughput telegrams.
- **`mem-block` entries**: Preallocate buffers of a given `size` and `preallocate`
  count so the stack avoids dynamic allocation during runtime. Large payload
  tests add 1480-byte blocks for jumbo telegrams.
- **`bus-interface`**: Describes a physical/logical interface. Attributes such
  as `network-id`, `name`, and optional `host-ip` pick the NIC and addressing.
- **`trdp-process`**: Tunes the scheduling of the process managing the interface.
  `cycle-time`, `priority`, and `traffic-shaping` decide when telegrams are
  exchanged.
- **`pd-com-parameter`**: Default Process Data communication behaviour for
  telegrams bound to the interface. Port numbers, QoS, TTL, timeout handling,
  and optional callbacks control the PD stack.
- **`md-com-parameter`**: Similar defaults for Message Data transfers. UDP and
  TCP examples illustrate how to select the transport, retry budget, and timeout
  windows.
- **`telegram`**: Binds a COM-ID to a declared data set and optional
  communication parameters. Telegram-level `pd-parameter` elements refine cycle
  times, marshalling, and validity handling per flow. `source` and `destination`
  entries specify participants and may attach SDT supervision parameters.
- **`com-parameter` list**: Defines reusable QoS/TTL profiles referenced from
  telegrams via `com-parameter-id`.
- **`data-set` list**: Enumerates the payload layout. Elements can embed
  primitive TRDP types, arrays, or references to other data sets for nesting.
- **`debug`**: Enables logging, including file names, rotation thresholds, and
  verbosity levels.

## Walkthrough of the provided examples

### `device1.xml`

A unicast PD echo scenario where a single interface (`eth0`) on
`10.0.1.100` exchanges telegram `1001` with a peer at `10.0.1.101`.
Marshalling is enabled for both the default PD profile and the telegram,
showcasing how SDT supervision parameters (`sdt-parameter`) can be
attached to both the source and the destination. Data sets `1001`–`1004`
demonstrate scalar, array, time/date, and mixed payload layouts, while the
COM parameter list defines shared QoS profiles for PD and MD traffic.

### `device2.xml`

Complements `device1.xml` by modelling the peer host at `10.0.1.101`.
Marshalling is disabled to exercise the raw byte-path in the TRDP stack.
The data-set catalogue mirrors `device1` except for a variant where the
character element uses raw `type="28"`, highlighting interoperability with
legacy numeric type identifiers.

### `example.xml`

A richer dual-interface device combining multicast and unicast telegrams.
Interface `eth0` advertises two telegrams: one multicast to `239.2.13.0`
with SDT supervision, and one unicast with two destinations leveraging a
custom COM parameter profile (`com-parameter-id="4"`). Interface `eth1`
illustrates TCP-based Message Data defaults and a telegram without explicit
PD parameters, relying entirely on interface-level defaults. The data-set
list uses symbolic type names (for example `UINT8`, `TIMEDATE64`) and adds a
nested data-set reference (`type="1001"`) to show compositional payloads.

### `pdsend_example.xml`

Focuses on a simple multicast publisher that emits five telegrams to
separate multicast groups. All telegrams reuse the interface defaults and
exercise disabled PD marshalling. The data sets align with `example.xml`
but tweak `testDS1005` to carry a fixed-size array of `UINT32` values to
mimic a flat binary payload generator.

### `nestedDS.xml`

Demonstrates payload nesting and time-related types. Telegram `1001` uses
`data-set-id="2004"`, which itself carries arrays of `TIMEDATE` flavours.
Auxiliary data sets show how a data set could embed another by referencing
its numeric ID (`type="2002"` is commented out) while mixing signed and
unsigned integers.

### `test_sdt.xml`

Mirrors `example.xml` but focuses on SDT supervision by enabling `lmi-max`
limits on both the source and destination. This example clarifies how to
bound the number of late message indications the stack tolerates before it
raises errors, useful for acceptance testing of supervision behaviour.

### `speedtest1.xml` and `speedtest2.xml`

High-throughput scenarios that declare 40 `source-sink` telegrams using the
same large data set (`test178*8Byte`). `speedtest1` publishes from host
`10.0.1.100` to `10.0.1.101`; `speedtest2` mirrors the flow in the reverse
direction. Both files boost the device memory pool and preallocate large
mem-blocks so the stack can handle sustained 1424-byte payloads at a 10 ms
cycle without allocating at runtime. These configurations are useful for
profiling CPU load, memory pressure, and throughput.

## Using the examples

1. Add the upstream TRDP stack as described in `docs/TCNOpen_TRDP.md` so the
   XML files become available under `external/tcnopen_trdp/trdp/test/xml`.
2. Point the simulator configuration loader at one of the XML files above to
   bootstrap a device definition matching your scenario.
3. Adapt the telegram, data set, and COM parameter entries to match your TRDP
   network topology and payload schema.
