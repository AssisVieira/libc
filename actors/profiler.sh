#!/bin/bash

### Requirements
# apt install python
# apt install python3-pip
# pip install gprof2dot
# apt install graphviz

bazel clean

bazel build demo --compilation_mode=opt --linkopt=-pg --copt=-pg

bazel-bin/demo/demo 4 10 1000000 8 false

gprof bazel-bin/demo/demo > gprof-report.txt

cat gprof-report.txt

gprof2dot gprof-report.txt > graph-report.dot

dot -Tpng -o call-graph.png graph-report.dot

