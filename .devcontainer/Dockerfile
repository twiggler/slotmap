# Development and CI image for slotmap: newest GCC, Clang (libstdc++ and libc++), CMake, Ninja,
# clangd/clang-format/clang-tidy, gdb, and the Claude Code CLI.
FROM registry.fedoraproject.org/fedora:44

RUN dnf install -y --setopt=install_weak_deps=False \
		gcc-c++ libasan libubsan \
		clang compiler-rt libcxx-devel \
		clang-tools-extra \
		cmake ninja-build \
		gdb git diffutils \
		nodejs npm \
	&& dnf clean all

# Claude Code CLI, pinned. Auto-updates are disabled via DISABLE_AUTOUPDATER in devcontainer.json.
ARG CLAUDE_CODE_VERSION=2.1.330
RUN npm install -g @anthropic-ai/claude-code@${CLAUDE_CODE_VERSION} && npm cache clean --force

# Non-root user with the host's usual UID; podman's --userns=keep-id maps it to the host user.
ARG USERNAME=vscode
ARG USER_UID=1000
RUN useradd --create-home --uid ${USER_UID} ${USERNAME} \
	&& mkdir -p /home/${USERNAME}/.claude \
	&& chown ${USERNAME}:${USERNAME} /home/${USERNAME}/.claude
USER ${USERNAME}
