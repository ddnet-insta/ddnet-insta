#!/bin/bash

INCLUDES_CONFIG="src/game/server/instagib/includes/config_variables.h"
INCLUDES_RCON_COMMANDS="src/game/server/instagib/includes/rcon_commands.h"
INCLUDES_CHAT_COMMANDS="src/game/server/instagib/includes/chat_commands.h"

README_FILE="README.md"
TEMP_DIR="scripts"
TEMP_FILE="$TEMP_DIR/tmp.swp"

mkdir -p "$TEMP_DIR"

arg_is_dry=0

for arg in "$@"; do
	case "$arg" in
	--dry-run)
		arg_is_dry=1
		;;
	*)
		echo "Unknown argument: $arg"
		echo "Usage: $0 [--dry-run]"
		exit 1
		;;
	esac
done

process_includes() {
	local include_file="$1"
	local includes=()

	while IFS= read -r line; do
		if [[ "$line" =~ ^[[:space:]]*#include[[:space:]]*[\"\<]([^\"\>]+)[\"\>] ]]; then
			local include_path="${BASH_REMATCH[1]}"
			includes+=("src/${include_path}")
		fi
	done < "$include_file"

	printf '%s\n' "${includes[@]}"
}

get_ignore_list() {
	local header_file="$1"
	local ignore_list=()

	while read -r line; do
		if [[ "$line" =~ ^[[:space:]]*//[[:space:]]*doc[[:space:]]gen[[:space:]]ignore:[[:space:]]*(.+) ]]; then
			local ignore_commands="${BASH_REMATCH[1]}"
			IFS=',' read -ra commands <<< "$ignore_commands"
			for command in "${commands[@]}"; do
				ignore_list+=("$(echo "$command" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')")
			done
		fi
	done < "$header_file"

	printf '%s\n' "${ignore_list[@]}"
}

gen_configs() {
	local cfg
	local desc
	local cmd

	# shellcheck disable=SC2016
	echo '+ `sv_gametype` Game type (gctf, ictf, gdm, idm, gtdm, itdm, ctf, dm, tdm, zcatch, bolofng, solofng, boomfng, fng)'
	for file in $(process_includes "$INCLUDES_CONFIG"); do
		while read -r cfg; do
			desc="$(echo "$cfg" | cut -d',' -f7- | cut -d'"' -f2-)"
			desc="${desc::-2}"
			cmd="$(echo "$cfg" | cut -d',' -f2 | xargs)"
			echo "+ \`$cmd\` $desc"
		done < <(grep '^MACRO_CONFIG_INT' "$file")
		while read -r cfg; do
			desc="$(echo "$cfg" | cut -d',' -f6- | cut -d'"' -f2-)"
			desc="${desc::-2}"
			cmd="$(echo "$cfg" | cut -d',' -f2 | xargs)"
			echo "+ \`$cmd\` $desc"
		done < <(grep '^MACRO_CONFIG_STR' "$file")
	done
}

gen_console_cmds() {
	local prefix="$1"
	local header_file="$2"
	local cfg
	local desc
	local cmd
	local ignore_list=()
	while IFS= read -r ignore_cmd; do
		[[ -n "$ignore_cmd" ]] && ignore_list+=("$ignore_cmd")
	done < <(get_ignore_list "$header_file")

	local ignore_pattern=""
	if [ ${#ignore_list[@]} -gt 0 ]; then
		ignore_pattern="($(
			IFS='|'
			echo "${ignore_list[*]}"
		))"
	fi

	while read -r cfg; do
		desc="$(echo "$cfg" | cut -d',' -f3- | cut -d'"' -f2-)"
		desc="${desc::-2}"
		cmd="$(echo "$cfg" | cut -d',' -f1 | cut -d'"' -f2)"

		if [[ -z "$ignore_pattern" ]] || ! [[ "$cmd" =~ $ignore_pattern ]]; then
			echo "+ \`$prefix$cmd\` $desc"
		fi
	done < <(grep '^CONSOLE_COMMAND' "$header_file")
}

gen_rcon_cmds() {
	for file in $(process_includes "$INCLUDES_RCON_COMMANDS"); do
		gen_console_cmds "" "$file"
	done
}

gen_chat_cmds() {
	for file in $(process_includes "$INCLUDES_CHAT_COMMANDS"); do
		gen_console_cmds "/" "$file"
	done
}

insert_at() {
	# insert_at [from_pattern] [to_pattern] [new content] [filename]
	local from_pattern="$1"
	local to_pattern="$2"
	local content="$3"
	local filename="$4"
	local from_ln
	local to_ln
	if ! grep -q "$from_pattern" "$filename"; then
		echo "Error: pattern '$from_pattern' not found in '$filename'"
		exit 1
	fi
	from_ln="$(grep -n "$from_pattern" "$filename" | cut -d':' -f1 | head -n1)"
	from_ln="$((from_ln + 1))"
	to_ln="$(tail -n +"$from_ln" "$filename" | grep -n "$to_pattern" | cut -d':' -f1 | head -n1)"
	to_ln="$((from_ln + to_ln - 2))"

	{
		head -n "$((from_ln - 1))" "$filename"
		printf '%b\n' "$content"
		tail -n +"$to_ln" "$filename"
	} > "$TEMP_FILE"
	if [ "$arg_is_dry" == "1" ]; then
		if [ "$(cat "$TEMP_FILE")" != "$(cat "$filename")" ]; then
			echo "Error: missing docs for $filename"
			echo "       run ./scripts/gendocs_instagib.sh"
			git diff --no-index --color "$filename" "$TEMP_FILE"
			exit 1
		fi
	else
		mv "$TEMP_FILE" "$filename"
	fi
}

insert_at '^## ddnet-insta configs$' '^# ' "\n$(gen_configs)" "$README_FILE"
insert_at '^# Rcon commands$' '^# ' "\n$(gen_rcon_cmds)" "$README_FILE"
insert_at '^+ `/drop flag' '^# ' "$(gen_chat_cmds)" "$README_FILE"

[[ -f "$TEMP_FILE" ]] && rm "$TEMP_FILE"

if [ "$arg_is_dry" == "1" ]; then
	echo "Dry-run completed successfully. Documentation is up to date."
else
	echo "Documentation updated successfully."
fi

exit 0
