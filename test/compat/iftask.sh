#!/bin/sh

. ../test.sh.inc

trap 'rm -f $TEST_TMP; exit $EXIT_KILLED' $TEST_TRAP_SIGS

./iftask >$TEST_TMP 2>&1
if [ $(grep '^constructor is called$' <$TEST_TMP | wc -l) -eq 1 ] &&
   [ $(fgrep 'array[' <$TEST_TMP | wc -l) -eq 0 ] &&
   [ $(grep '^destructor is called$' <$TEST_TMP | wc -l) -eq 1 ]
then
	test_exit_status=$EXIT_PASS
fi
rm -f $TEST_TMP
exit $test_exit_status
