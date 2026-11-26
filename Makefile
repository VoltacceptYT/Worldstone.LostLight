# Same layout as gtasands: use a local BlocksDS if present, otherwise build in Docker.
BLOCKSDS ?= /opt/wonderful/thirdparty/blocksds/core

ifeq ($(wildcard $(BLOCKSDS)/sys/crts/ds_arm9.specs),)
IMAGE ?= skylyrac/blocksds:slim-v1.24.0
JOBS ?= 8
.DEFAULT_GOAL := all
.PHONY: all build build-ds build-dsi clean
all build build-ds build-dsi clean:
	@docker run --rm --user "$$(id -u):$$(id -g)" -v "$(CURDIR):/work:Z" -w /work $(IMAGE) \
	    make -f Makefile.blocksds -j$(JOBS) $@
else
include Makefile.blocksds
endif
