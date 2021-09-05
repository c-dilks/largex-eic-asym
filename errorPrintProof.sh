#!/bin/bash

# grep for errors in PROOF logs; make sure $PROOF_LOG is set correctly (by `source env.sh`)
# since it greps for the word "error" (ignoring case), it may also print uncertainty prints,
# which is not so helpful when checking for run-time errors

grep -i error $PROOF_LOG/worker-*-*.log > tempolog.1
sort tempolog.1 | uniq > tempolog.2
grep --color -i error tempolog.2
#grep --color -i error tempolog.1
rm tempolog.*

