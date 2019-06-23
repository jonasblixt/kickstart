#!/bin/sh
docker run -it -u $(id -u $USER) --rm -e HOSTNAME='asdf' -e KS_ROOT_HASH=$KS_ROOT_HASH -v $(readlink -f .):/work ks-builder:latest
