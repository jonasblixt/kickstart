################################################################################
#
# NanoMQ MQTT Broker
#
################################################################################

NANOMQ_VERSION = fcac8526335fa67f13dc0088b1c8930df02321ab
NANOMQ_SITE = https://github.com/nanomq/nanomq.git
NANOMQ_SITE_METHOD = git
NANOMQ_GIT_SUBMODULES = YES
NANOMQ_INSTALL_STAGING = YES
NANOMQ_LICENSE = MIT
NANOMQ_LICENSE_FILES = LICENSE.txt
# Setting -DBUILD_STATIC_LIB=off -DBUILD_SHARED_LIBS=off enables build of the server binary
NANOMQ_CONF_OPTS = -DNNG_ENABLE_TLS=on -DBUILD_STATIC_LIB=off -DBUILD_SHARED_LIBS=off -DNNG_TLS_ENGINE="wolf"
NANOMQ_DEPENDENCIES = wolfssl

$(eval $(cmake-package))
