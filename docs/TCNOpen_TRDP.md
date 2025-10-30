# Integrating the TCNOpen TRDP Stack

The TRDP (Train Real-time Data Protocol) stack published by TCNOpen is
required for the simulator to communicate with real TRDP networks. The
project is intended to consume the official TCNOpen implementation as a git
submodule. The maintained mirror currently lives at
`https://github.com/T12z/TCNopen.git`. Add it with:

```bash
git submodule add https://github.com/T12z/TCNopen.git external/tcnopen_trdp
git submodule update --init --recursive
```

> **Note**
> The TCNOpen organisation previously referenced
> `https://github.com/tcnopen/trdp`, but that repository currently responds with
> 404 (Not Found). Use the T12z mirror above unless the official URL becomes
> reachable again.

After the submodule is available, regenerate the build system to allow CMake to
pick up the TRDP headers and libraries.
