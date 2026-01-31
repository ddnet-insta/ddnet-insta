#!/bin/bash

set -eu

shopt -s globstar

SKIP_FILES=(
	# TODO: not sure what to do here
	#       i like the clean filename ddrace.h
	#       needs some thinking
	src/insta/server/gamemodes/ddrace/ddrace.h

	# eh .. probably fine idk
	src/insta/server/gamemodes/instagib/gctf/gctf.h
	src/insta/server/gamemodes/instagib/gdm/gdm.h
	src/insta/server/gamemodes/instagib/gtdm/gtdm.h
	src/insta/server/gamemodes/instagib/ictf/ictf.h
	src/insta/server/gamemodes/instagib/idm/idm.h
	src/insta/server/gamemodes/instagib/itdm/itdm.h
	src/insta/server/gamemodes/vanilla/ctf/ctf.h
	src/insta/server/gamemodes/vanilla/dm/dm.h
	src/game/server/gamemodes/ddnet.h
)

VERBOSE=0
ERRORS=0

# convert lower_snake_case
# to UpperCamelCase
snake_to_camel() {
	local snake="$1"
	case "$snake" in
	ddrace)
		printf 'DDRace'
		;;
	*)
		printf '%s' "$snake" | sed -E 's/(^|_)([a-z])/\U\2/g' | tr -d '_'
		;;
	esac
}

# print if verbose
v_print() {
	[ "$VERBOSE" -gt 0 ] || return 0

	printf '%s\n' "$1"
}

snake_to_ctrl_name() {
	local snake_case_name="$1"
	local camel_name
	camel_name="$(snake_to_camel "$snake_case_name")"
	printf 'CGameController%s' "$camel_name"
}

check_file() {
	local header_path="$1"
	local class_name
	class_name="$(basename "$header_path" .h)"
	class_name="$(snake_to_ctrl_name "$class_name")"

	local skip_file
	for skip_file in "${SKIP_FILES[@]}"; do
		if [ "$skip_file" = "$header_path" ]; then
			v_print "[*] skipping file $header_path"
			return 0
		fi
	done

	# shellcheck disable=SC2015
	[ "$VERBOSE" -eq 0 ] && printf '.' || true
	v_print "checking header: $header_path $class_name"

	if ! grep -q "class $class_name " "$header_path"; then
		v_print ""
		printf '[-] Error: missing class %s in file %s\n' "$class_name" "$header_path"
		v_print ""
		ERRORS="$((ERRORS + 1))"
	fi
}

check_root_files() {
	for header_path in src/game/server/gamemodes/*.h; do
		# should never happen but would be an bash error
		# if the code gets refactored
		[ -f "$header_path" ] || continue

		check_file "$header_path"
	done
}

check_all_dirs() {
	# depends on globstar
	for dir in src/game/server/gamemodes/**/; do
		name="$(basename "$dir")"
		header_path="$dir$name.h"
		# not all folders are controllers
		# some just represent categories such as instagib/ or vanilla/
		[ -f "$header_path" ] || continue

		check_file "$header_path"
	done
}

check_all_files() {
	check_root_files
	check_all_dirs

	# shellcheck disable=SC2015
	[ "$VERBOSE" -eq 0 ] && printf '\n' || true
}

show_help() {
	cat <<- EOF
		usage: $0 [FLAG..] [OPTION..]
		description:
		  checks if the controller filenames match the class names
		flags:
		  -v     verbose
		  -vv    even more verbose
		options:
		  --help     show this help
	EOF
}

parse_args() {
	local arg
	while [ "$#" -gt 0 ]; do
		arg="$1"
		shift

		if [ "${arg::2}" = "--" ]; then
			local option
			option="${arg:2}"
			case "$option" in
			help)
				show_help
				exit 0
				;;
			*)
				printf "[-] Unexpected option '%s'\n" "$arg"
				exit 1
				;;
			esac
		elif [ "${arg::1}" = "-" ]; then
			local flags
			local flag
			local i
			flags="${arg:1}"
			for ((i = 0; i < ${#flags}; i++)); do
				flag="${flags:$i:1}"
				case "$flag" in
				v)
					VERBOSE="$((VERBOSE + 1))"
					;;
				*)
					printf "[-] Unexpected flag '-%s'\n" "$flag"
					exit 1
					;;
				esac
			done
		else
			if [ "$arg" = "help" ]; then
				show_help
				exit 0
			else
				printf "[-] Unexpected arg '%s'\n" "$arg"
				exit 1
			fi
		fi
	done
}

parse_args "$@"
check_all_files

if [ "$ERRORS" -eq 0 ]; then
	printf '[*] OK\n'
else
	printf '[-] FAILED: Not all controller class names match their filename!\n'
	exit 1
fi
