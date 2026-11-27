#!/usr/bin/env bash
# Runs inside the container on every start (devcontainer "postStartCommand").
# Points the container's Claude Code session folder for this workspace at the host folder mounted
# on /mnt/claude-project. The workspace has the same absolute path as on the host, so the key matches.
set -euo pipefail

workspace="$(pwd)"
source "$workspace/.devcontainer/claude-project-key.sh"

config_dir="${CLAUDE_CONFIG_DIR:-$HOME/.claude}"
mkdir -p "$config_dir/projects"
ln -sfn /mnt/claude-project "$config_dir/projects/$(claude_project_key "$workspace")"
