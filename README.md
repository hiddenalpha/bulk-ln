
BulkLn
================

`ln` alternative in case you think `ln` in a shell loop takes too much time to
create many links.


## How to build / install

Minimal way is to do:

```sh
    curl -sSL http://git.hiddenalpha.ch/bulk-ln.git/snapshot/bulk-ln-master.tar.gz | tar xz
    cd bulk-ln-master
    ./configure
    make
    make install
```

As usual, configure provides some help:

```sh
    ./configure --help
```

Its worth browsing the "contrib/" directory. It contains potentially useful
scripts for building.

