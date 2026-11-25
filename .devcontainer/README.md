# Dev container

A Fedora image in two stages: `toolchain` has GCC, Clang (libstdc++ and libc++), CMake, Ninja, clangd, clang-format,
clang-tidy and gdb, and is what CI uses; `dev` adds the Claude Code CLI and a non-root user for VS Code.
[CONTRIBUTING.md](../CONTRIBUTING.md) explains how to use it.

The podman run flags:
- `--userns=keep-id`: files created in the container belong to your host user.
- SELinux stays on: the workspace mount uses `relabel=shared`, and `init-host.sh` labels the Claude Code session folder
  the same way, so the container can use both but nothing else in your home folder. These are podman options.
- `--cap-add=SYS_PTRACE`: needed by gdb and LeakSanitizer.

## Claude Code
The workspace is mounted at the same path as on the host, because Claude Code finds sessions and project memory by
path. The container sees only this project's session folder from the host (linked by `init-host.sh` and
`init-container.sh`) and keeps its own login and settings in a named volume, so host credentials, settings and other
projects stay out of it.

The container is a reproducible toolchain, not a sandbox: code running in it can change the workspace and this
project's sessions and memory, which the host's Claude Code reads.
