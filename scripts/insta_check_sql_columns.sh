#!/bin/bash

errors=0
check_all_file() {
	while read -r header; do
		if ! grep -q "$header" src/insta/server/sql_columns_all.h; then
			echo "Error: header $header is missing in src/insta/server/sql_columns_all.h"
			errors=$((errors + 1))
		fi
	done < <(find . -name "sql_columns.h" | cut -c 7-)
}

is_valid_file() {
	local file="$1"
	[ "$file" != "src/" ] || return 1
	[ "$file" != "" ] || return 1
	[ -f "$file" ] || {
		echo "no such file $file"
		return 1
	}

	return 0
}

check_include_path() {
	local line
	local fs_file
	local arg_file
	local from_dir
	local to_dir
	while read -r line; do
		fs_file="$(printf '%s' "$line" | cut -d':' -f1)"
		arg_file="$(printf '%s' "$line" | cut -d'<' -f2)"
		arg_file="src/$(printf '%s' "$arg_file" | cut -d'>' -f1)"

		is_valid_file "$fs_file" || {
			echo "Invalid file: $fs_file"
			exit 1
		}
		is_valid_file "$arg_file" || {
			echo "Invalid file: $arg_file"
			exit 1
		}
		[ "$fs_file" = "src/insta/server/column_template.h" ] && continue

		from_dir="$(dirname "$fs_file")"
		to_dir="$(dirname "$arg_file")"
		[ "$from_dir" = "$to_dir" ] && continue

		echo "Error: sql column header included from different directory"
		echo "       the following source file"
		echo "        $(tput bold)$fs_file$(tput sgr0)"
		echo "       includes the following header"
		echo "        $(tput bold)$arg_file$(tput sgr0)"
		echo ""
		echo "       directory mismatch!"
		echo ""
		echo "       expected: $from_dir"
		echo "            got: $to_dir"
		echo ""
		errors=$((errors + 1))
	done < <(grep -r 'SQL_COLUMN_FILE <' src/)
}

check_include_path
check_all_file

if [ "$errors" -gt 0 ]; then
	exit 1
fi
