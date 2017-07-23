#!/bin/sh
#
# Copyright 2017 Shivansh Rai
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $FreeBSD$
#
# Script for generating annotations based on generated tests.

pwd=$(pwd)
suffix="_test"
extension=".sh"

printf "Updated files:\n"
for f in "generated_tests"/*
do
  annotations=""
  file=$(basename $f)
  test=${file%$extension}
  utility=${test%$suffix}
  dir="/usr/tests/bin/$utility"

  (
  cd $dir
  report=$(kyua report)
  i=2

  while [ 1 ]
  do
    testcase=$(printf "$report" | awk 'NR=='"$i"' {print $1}')
    status=$(printf "$report" | awk 'NR=='"$i"' {print $3}')
    check=$(printf "$testcase" | cut -s -f1 -d":")

    if [ "$check" != "$test" ]; then
      if [ "$annotations" ]; then
	annotations_file="$pwd/annotations/$test.annot"
	# Append only the new annotations
	printf "$annotations" > "$annotations_file.temp"
	echo $annotations_file
	[ ! -e "$annotations_file" ] && touch "$annotations_file"
	comm -13 "$annotations_file" "$annotations_file.temp" >> "$annotations_file" && printf "\tannotations/$test.annot\n"
	rm -f "$annotations_file.temp"
      fi

      break
    fi

    if [ "$status" == "failed:" ]; then
      testcase=${testcase#"$test:"}
      annotations="$annotations$testcase\n"
    fi

    i=$((i+1))
  done
  )

done

printf "==============================================================================\n"
