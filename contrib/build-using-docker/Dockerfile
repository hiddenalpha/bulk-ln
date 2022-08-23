
ARG PARENT_IMAGE=alpine:3.16.0
FROM $PARENT_IMAGE

ARG GIT_TAG=master
ARG CONFIGURE_OPTS=
ARG PKGINIT=true
ARG PKGADD="apk add"
ARG PKGDEL="apk del"
ARG PKGCLEAN=true
ARG PKGS_TO_ADD="curl gcc git make musl-dev tar"
ARG PKGS_TO_DEL="curl gcc git make musl-dev tar"

WORKDIR /work

RUN true \
    && $PKGINIT \
    && $PKGADD $PKGS_TO_ADD \
    && git clone --depth 42 --branch "${GIT_TAG:?}" https://github.com/hiddenalpha/bulk-ln.git . \
    && git config advice.detachedHead false \
    && if test -n "$GIT_TAG"; then git checkout "$GIT_TAG"; fi \
    && ./configure $CONFIGURE_OPTS \
    && make clean && make && make install \
    && find . -not -wholename './dist*' -delete \
    && find /work -exec chown 1000:1000 {} + \
    && $PKGDEL $PKGS_TO_DEL \
    && $PKGCLEAN \
    && true

USER 1000:1000

# run for max 10 hrs. Because this container is not meant to stay running
# forever.
CMD ["sleep", "36000"]

