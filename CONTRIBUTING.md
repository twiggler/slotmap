# Contributing

## Building and testing
Builds use CMake presets. `cmake --workflow --preset <name>` configures, builds and tests in Debug and Release.

| Preset         | Toolchain                                                   |
|----------------|-------------------------------------------------------------|
| `gcc`          | GCC                                                         |
| `clang`        | Clang with libstdc++                                        |
| `clang-libcxx` | Clang with libc++                                           |
| `msvc`         | MSVC (Windows)                                              |
| `asan`         | Clang with AddressSanitizer and UndefinedBehaviorSanitizer  |
| `tidy`         | Clang with clang-tidy (build only)                          |

Visual Studio, VS Code (CMake Tools) and CLion pick up the presets directly.

Randomized tests use a fixed seed. Set `SLOTMAP_TEST_SEED` to replay a failure or to try other sequences.

## Dev container
The [dev container](.devcontainer) has every toolchain the presets need, so nothing has to be installed on the host.

- **VS Code:** "Dev Containers: Reopen in Container". With podman, set the user setting `"dev.containers.dockerPath": "podman"`.
- **Command line:**
  ```sh
  podman build --target toolchain -t slotmap-dev .devcontainer
  podman run --rm --userns=keep-id --security-opt label=disable -v "$PWD":"$PWD" -w "$PWD" slotmap-dev cmake --workflow --preset gcc
  ```

[.devcontainer/README.md](.devcontainer/README.md) describes what the container contains and how it works with Claude Code.

## Style
Formatting follows `.clang-format` and is checked in CI. `.clang-tidy` is the linter; run it with the `tidy` preset.
Formatting-only commits are listed in `.git-blame-ignore-revs`, so `git blame` skips them
(`git config blame.ignoreRevsFile .git-blame-ignore-revs`).

## Continuous integration
CI runs the newest GCC and Clang, the sanitizers, clang-tidy and clang-format in the dev container's toolchain stage,
and GCC 14 and Clang 18 (the oldest supported compilers), Apple Clang and MSVC on GitHub's runners.
