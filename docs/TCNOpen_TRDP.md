# Integrating the TCNOpen TRDP Stack

The TRDP (Train Real-time Data Protocol) stack published by TCNOpen is
required for the simulator to communicate with real TRDP networks. The
project is intended to consume the official TCNOpen implementation as a git
submodule. Once the repository is available, add it with:

```bash
git submodule add https://github.com/tcnopen/trdp external/tcnopen_trdp
git submodule update --init --recursive
```

> **Note**
> As of this change the repository `https://github.com/tcnopen/trdp` is not
> publicly accessible. Initialising the submodule therefore fails with a 404
> (Not Found) response from GitHub. Monitor the repository for availability or
> obtain access credentials before attempting to add it as a submodule.

After the submodule is available, regenerate the build system to allow CMake to
pick up the TRDP headers and libraries.
