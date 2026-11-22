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
