#! /usr/bin/env bash
#
# Quick script that compiles and runs the samples, then cleans up.
# Used for release testing.

set -exuo pipefail

zapc -oc++ addressbook.zap
c++ -std=c++23 -Wall addressbook.c++ addressbook.zap.c++ \
    $(pkg-config --cflags --libs zap) -o addressbook
./addressbook write | ./addressbook read
./addressbook dwrite | ./addressbook dread
rm addressbook addressbook.zap.c++ addressbook.zap.h

zapc -oc++ calculator.zap
c++ -std=c++23 -Wall calculator-client.c++ calculator.zap.c++ \
    $(pkg-config --cflags --libs zap-rpc) -o calculator-client
c++ -std=c++23 -Wall calculator-server.c++ calculator.zap.c++ \
    $(pkg-config --cflags --libs zap-rpc) -o calculator-server
rm -f /tmp/zap-calculator-example-$$
./calculator-server unix:/tmp/zap-calculator-example-$$ &
sleep 0.1
./calculator-client unix:/tmp/zap-calculator-example-$$
kill %+
wait %+ || true
rm calculator-client calculator-server calculator.zap.c++ calculator.zap.h /tmp/zap-calculator-example-$$
