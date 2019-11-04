#!/bin/bash

for file in reactor_core reactor_user reactor_memory reactor_timer reactor_pool reactor_resolver reactor_stream
do
    test=`gcov -b src/reactor/libreactor_test_a-$file | grep -A4 File.*$file`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done

exit 0
