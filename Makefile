PAYLOADS += sflash-dumper
PAYLOADS += slot-versions

Q      ?= @
LOG    ?= @echo $@

all: $(addsuffix _build,$(PAYLOADS))

%_build:
	$Q$(MAKE) -C $*

clean: $(addsuffix _clean,$(PAYLOADS))

%_clean:
	$Q$(MAKE) -C $* clean

.PHONY: all clean
