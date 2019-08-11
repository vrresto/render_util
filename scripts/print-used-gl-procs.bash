#!/bin/bash

set -e

program_args="$@"

alphanum_string_pattern="(([a-z]|[A-Z]|[0-9])*)"
name_pattern="(([a-z]|[A-Z])+)($alphanum_string_pattern)"
pattern_no_prefix="::$name_pattern"
pattern_full="gl::$name_pattern"


function printProcs {
  cd "$1"

  file_list=$(git ls-files)

  egrep --no-messages --no-filename --only-matching "$pattern_full" $file_list |
    sort | uniq |
    egrep --only-matching "$pattern_no_prefix" |
    egrep --only-matching "$name_pattern"
}


function printAllProcs {
  set -e

  for dir in $program_args
  do
    printProcs "$dir"
  done
}


all_procs=$(printAllProcs)

for name in $all_procs
do
  echo $name
done | sort | uniq
