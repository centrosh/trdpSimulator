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

## Building the TRDP libraries

The TRDP source tree ships with a flexible make-based build that can target
different operating systems and toolchains. The build is controlled by
configuration files located in `TCNopen/trdp/config` that follow the
instructions from `readme-makefile.txt` in the same directory:

1. Copy a configuration template that matches your target (for example
   `POSIX_X86_config`) to a new file and adjust the compiler, flags and paths to
   match the environment you plan to build in.
2. Run `make <MY_TARGET>_config` to copy the configuration into
   `config/config.mk`. This step only has to be done once per build directory.
3. Run `make` from the `TCNopen/trdp` directory. The selected configuration will
   be used to compile the stack and write the build artifacts to
   `TCNopen/trdp/bld/output`.

You can inspect the available configurations and options by running
`make help`. Passing `DEBUG=TRUE` to `make` produces debug variants of the
libraries.

## Consuming the static libraries

The default make configuration produces static libraries (for example,
`libtrdp.a`) and public headers under `TCNopen/trdp`. A typical integration flow
looks like this:

1. Add the TRDP headers to your include search path. From CMake that can be
   achieved with:

   ```cmake
   target_include_directories(trdp_simulator PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}/external/tcnopen_trdp/trdp/api
       ${CMAKE_CURRENT_SOURCE_DIR}/external/tcnopen_trdp/trdp/vos/api
   )
   ```

   Adjust the directories for the API components you require in your source
   files.
2. Link the produced static library when building your targets. If you keep the
   compiled `libtrdp.a` inside the submodule, CMake can link it directly:

   ```cmake
   target_link_libraries(trdp_simulator PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}/external/tcnopen_trdp/trdp/bld/output/libtrdp.a
   )
   ```

   When the project is distributed on systems that cannot rely on prebuilt
   artifacts, call the TRDP makefile via `add_custom_target` or your packaging
   workflow to ensure the library is rebuilt for the local toolchain before
   linking.

3. Include the TRDP headers from your source code as documented by the stack,
   for example:

   ```cpp
   #include <trdp_if_light.h>
   #include <tau_marshall.h>
   #include <vos_utils.h>
   ```

   Depending on your include directory layout you may prefer fully qualified
   include paths (for example
   `<tcn-open-static-lib/api/trdp_if_light.h>`). Align the include directives
   with the include directories you expose to the compiler.

With these steps, the simulator can call directly into the TRDP implementation
while keeping the vendor source in a dedicated submodule. Any project-specific
configuration (for example, a custom `_config` file) should live in a separate
branch or be added to the repository if it is reusable.
