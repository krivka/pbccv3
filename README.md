pbccv3
======

PicoBlaze-6 port for SDCC 3+

How to compile
======

Decompress the whole SDCC tree directly into the repository, to have SDCC's src folder overlap PBCC's one.

Then run glue.sh with SDCC_HOME environment variable set to the repository root.

After that, it's just good old autoconf; ./configure; make 
