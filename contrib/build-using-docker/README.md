
Showcase how to build and install
=================================

Sometimes happy developers (like me) have no choice but using horribly
restricted systems where setting up tools to run even something as simple as
configure/make/install becomes a nightmare. I found it to be easier to have a
Dockerfile to build on a totally unrelated machine (but where I have the needed
privileges) and then just copy-paste the built result over to where I need it.


## Setup variable to reduce annoying repetitions

```sh
IMG=bulk-ln-showcase:latest
```

## Make and install dockerimage

```sh
curl -sSL http://git.hiddenalpha.ch/bulk-ln.git/plain/contrib/build-using-docker/Dockerfile | sudo docker build . -f - -t "${IMG:?}"
```

## Grab distribution archive

Most probably we wanna get the distribution archive. We can copy it out the
dockerimage to our host using:

```sh
sudo docker run --rm -i "${IMG:?}" sh -c 'true && cd dist && tar c *' | tar x
```


## Play around

Or if we wanna browse the image or play around with the built utility we could
launch a shell. Once in the shell, try `bulk-ln --help`.

```sh
sudo docker run --rm -ti "${IMG:?}" sh
```


## Other targets

The dockerfile is parameterized and should work for other systems too. For
example to compile for debian we could use command like below. May set *IMG*
differently if you need to keep multiple images.

```sh
curl -sSL http://git.hiddenalpha.ch/bulk-ln.git/plain/contrib/build-using-docker/Dockerfile | sudo docker build . -f - -t "${IMG:?}" \
    --build-arg PARENT_IMAGE=debian:buster-20220622-slim \
    --build-arg PKGS_TO_ADD="curl gcc git make libc-dev ca-certificates tar" \
    --build-arg PKGS_TO_DEL="curl gcc git make libc-dev ca-certificates" \
    --build-arg PKGINIT="apt update" \
    --build-arg PKGADD="apt install -y --no-install-recommends" \
    --build-arg PKGDEL=true \
    --build-arg PKGCLEAN="apt clean"
```

