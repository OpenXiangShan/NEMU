#!/usr/bin/env bash
set -Eeuo pipefail

: "${PR_URL:?PR_URL is required}"
: "${PR_AGENT_GITHUB_TOKEN:?PR_AGENT_GITHUB_TOKEN is required}"
: "${PR_AGENT_BIN:?PR_AGENT_BIN is required}"

# node004 keeps the LAN gateway credentials in a group-readable file so the
# existing `cirunner` service account can use them.  GitHub Actions secrets,
# when present, override these local defaults.
ACTION_OPENAI_API_KEY="${OPENAI_API_KEY:-}"
ACTION_OPENAI_BASE_URL="${OPENAI_BASE_URL:-}"
LOCAL_ENV=""
for candidate in \
  /nfs/home/cirunner/.config/pr-agent/env \
  /nfs/home/fengkehan/.local/pr-agent-config/env; do
  if [[ -r "$candidate" ]]; then LOCAL_ENV="$candidate"; break; fi
done
if [[ -n "$LOCAL_ENV" ]]; then
  set -a
  # shellcheck disable=SC1090
  . "$LOCAL_ENV"
  set +a
fi
[[ -z "$ACTION_OPENAI_API_KEY" ]] || OPENAI_API_KEY="$ACTION_OPENAI_API_KEY"
[[ -z "$ACTION_OPENAI_BASE_URL" ]] || OPENAI_BASE_URL="$ACTION_OPENAI_BASE_URL"

: "${OPENAI_API_KEY:?OPENAI_API_KEY is required (set PR_AGENT_OPENAI_API_KEY or the node004 env file)}"
: "${OPENAI_BASE_URL:?OPENAI_BASE_URL is required (set PR_AGENT_OPENAI_BASE_URL or the node004 env file)}"

# PR-Agent's CLI reads these standard OpenAI-compatible settings.  Keep all
# secrets in the runner/Actions secret store; never put them in the repository.
export CONFIG__GITHUB__USER_TOKEN="$PR_AGENT_GITHUB_TOKEN"
export CONFIG__GITHUB__DEPLOYMENT_TYPE="user"
export CONFIG__OPENAI__KEY="$OPENAI_API_KEY"
export CONFIG__OPENAI__API_BASE="$OPENAI_BASE_URL"
export CONFIG__CONFIG__GIT_PROVIDER="github"

exec "$PR_AGENT_BIN" -m pr_agent.cli \
  --pr_url="$PR_URL" \
  --config_path="$GITHUB_WORKSPACE/.github/pr-agent/.pr_agent.toml" \
  review
