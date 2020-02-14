#!/bin/sh

mkdir -p {.,lib/libdynamic,lib/libreactor_core}/{m4,autotools}
#mkdir -p {.,lib/libdynamic,lib/libreactor_core,lib/libreactor-http,lib/libreactor-rest}/{m4,autotools}

autoreconf --force --install
