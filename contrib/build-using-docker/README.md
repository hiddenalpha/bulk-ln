
Showcase how to build and install
=================================

Make and install inside a dockerimage.

```sh
    curl -sSL http://git.hiddenalpha.ch/bulk-ln.git/plain/contrib/build-using-docker/Dockerfile | sudo docker build . -f -
```

Afer that we can use the image hash printed in last line to refer to our built
image (replace IMG_REF in commands below by that hash). Alternatively we could
add [`--tag`](https://docs.docker.com/engine/reference/commandline/build/)
option to our build command to give the resulting image a name.

Most probably we wanna get the distribution archive. We can copy it out the
dockerimage to our host using:

```sh
    sudo docker run --rm -i  IMG_REF  sh -c 'true && cd dist && tar c *' | tar x
```

Or if we wanna browse the image or play around with the built utility we could
launch a shell. Once in the shell, try `bulk-ln --help`.

```sh
    sudo docker run --rm -ti  IMG_REF  sh
```

