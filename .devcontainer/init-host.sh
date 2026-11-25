#!/usr/bin/env bash
# Runs on the host before the container starts (devcontainer "initializeCommand").
# Links .devcontainer/.claude-project to this project's Claude Code session folder on the host,
# creating it if needed, so the container can bind-mount exactly that folder and nothing else.
set -euo pipefail

workspace="$(pwd)"
source "$workspace/.devcontainer/claude-project-key.sh"

session_dir="$HOME/.claude/projects/$(claude_project_key "$workspace")"
mkdir -p "$session_dir"
ln -sfn "$session_dir" "$workspace/.devcontainer/.claude-project"

# On SELinux hosts, give the session folder the shared container label so the confined container can use it.
# podman's relabel option would label the symlink instead of the folder, so label the folder here.
if command -v selinuxenabled >/dev/null && selinuxenabled; then
	chcon -R -t container_file_t -l s0 "$session_dir"
fi
