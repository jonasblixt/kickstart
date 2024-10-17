################################################################################
#
# Bitpacker
#
################################################################################

BPAK_VERSION = 863f36631c4da0f4bb5ec2483ffe7fcac8652b95
BPAK_SITE = https://github.com/jonasblixt/bpak.git
BPAK_SITE_METHOD = git
BPAK_INSTALL_STAGING = YES
BPAK_LICENSE = BSD-3-Clause
BPAK_LICENSE_FILES = License.txt

# When we build the host package we always wan't the tool
HOST_BPAK_CONF_OPTS = -DBPAK_BUILD_TOOL=on

ifeq ($(BR2_PACKAGE_BPAK_TOOL),y)
BPAK_CONF_OPTS = -DBPAK_BUILD_TOOL=on
else
BPAK_CONF_OPTS = -DBPAK_BUILD_TOOL=off
endif

$(eval $(cmake-package))
$(eval $(host-cmake-package))
