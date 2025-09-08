#!/usr/bin/env bash
# tmux-deploy.sh
# Usage:
#   tmux-deploy.sh start script1 [script2 ...]
#   tmux-deploy.sh list
#   tmux-deploy.sh attach <session>
#   tmux-deploy.sh stop <session>

SESS_FILE="${HOME}/.analysis_sessions"
LOG_DIR="${HOME}/analysis_logs"

usage() {
  echo "Usage:"
  echo "  $0 start script1 [script2 ...]"
  echo "  $0 list"
  echo "  $0 attach <session>"
  echo "  $0 stop <session>"
}

mkdir -p "$LOG_DIR"
touch "$SESS_FILE"

case "$1" in
  start)
    shift
    for script in "$@"; do
      # session name derived from script file name
      session=$(basename "$script" | tr . _)
      if tmux has-session -t "$session" 2>/dev/null; then
        echo "Session '$session' already exists; skipping."
        continue
      fi
      tmux new-session -d -s "$session" \
        "cd ana && bash \"$script\" > \"$LOG_DIR/$session.log\" 2>&1"
      echo "$session $(realpath "$script")" >> "$SESS_FILE"
      echo "Started $script in session '$session'"
    done
    ;;
  list)
    echo "Tracked sessions:"
    cat "$SESS_FILE"
    echo ""
    echo "Active tmux sessions:"
    tmux ls 2>/dev/null || echo "None"
    ;;
  attach)
    [ -n "$2" ] && tmux attach -t "$2" || usage
    ;;
  stop)
    [ -n "$2" ] || { usage; exit 1; }
    tmux kill-session -t "$2" 2>/dev/null && \
      sed -i "/^$2 /d" "$SESS_FILE" && \
      echo "Stopped session '$2'"
    ;;
  *)
    usage
    ;;
esac
