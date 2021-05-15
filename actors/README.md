# Actors: Actor Model in C. 


Work in Progress...

## Building

bazel build demo

## Executing

bazel run demo \<num-workers\> \<num-actors-pinger\> \<max-pings\> \<enable-debug\>

ex:

bazel run demo 4 4 40000000 false


