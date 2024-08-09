# load_env.sh

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "This script must be sourced. Use 'source $0 path_to_env_file' instead of '$0 path_to_env_file'."
    exit 1
fi

if [ -z "$1" ]; then
    echo "Usage: source $0 path_to_env_file"
    exit 1
fi

env_file="$1"

while IFS= read -r line; do
    if [[ $line =~ ^\#.* ]] || [[ -z $line ]]; then
        continue
    fi
    
    # check if the line is in the format of "key=value"
    if [[ $line =~ ^[^=]+=[^=]+$ ]]; then
        # use eval to avoid quoting issues
        echo "export $line"
        eval "export $line"
    else
        echo "Warning: Skipping invalid line: $line"
    fi
done < "$env_file"