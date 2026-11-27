# Prints the folder name Claude Code uses under ~/.claude/projects for a working directory:
# every character that is not a letter or digit becomes '-'. Sourced by init-host.sh and init-container.sh.
claude_project_key() {
	printf '%s' "$1" | sed 's/[^A-Za-z0-9]/-/g'
}
