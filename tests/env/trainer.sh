# export the environment variables for trainer
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "This script must be sourced. Use 'source $0 path_to_env_file' instead of '$0 path_to_env_file'."
    exit 1
fi

file_name="trainer.env"

source "$(dirname "${BASH_SOURCE[0]}")/load_env.sh" "$(dirname "${BASH_SOURCE[0]}")/$file_name"