# Dev container

A Fedora image in two stages: `toolchain` has GCC, Clang (libstdc++ and libc++), CMake, Ninja, clangd, clang-format,
clang-tidy and gdb, and is what CI uses; `dev` adds the Claude Code CLI and a non-root user for VS Code.
[CONTRIBUTING.md](../CONTRIBUTING.md) explains how to use it.

The podman run flags:
- `--userns=keep-id`: files created in the container belong to your host user.
- The workspace mount uses `relabel=shared`, and `init-host.sh` gives the Claude Code session folder the same shared
  SELinux label, so a confined container can use both but nothing else in your home folder.
- `--cap-add=SYS_PTRACE`: needed by gdb and LeakSanitizer.

## Claude Code
The workspace is mounted at the same path as on the host, because Claude Code finds sessions and project memory by
path. The container sees only this project's session folder from the host (linked by `init-host.sh` and
`init-container.sh`) and keeps its own login and settings in a named volume, so host credentials, settings and other
projects stay out of it.

The container is a reproducible toolchain, not a sandbox: code running in it can change the workspace and this
project's sessions and memory, which the host's Claude Code reads.

## SELinux: the VS Code container runs unconfined
With podman on Linux, the Dev Containers extension always adds `--security-opt label=disable` to `podman run`,
whatever `devcontainer.json` says, and offers no setting to turn this off. The container started from VS Code
therefore runs without SELinux confinement.

Containers started from the command line as in [CONTRIBUTING.md](../CONTRIBUTING.md) do not get this flag; with the
`:z` mount they run confined.
