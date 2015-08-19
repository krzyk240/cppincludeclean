include Makefile.config

DESTDIR = build

.PHONY: all
all:
ifeq ($(MAKELEVEL), 0)
	@printf "CC -> $(CC)\nCXX -> $(CXX)\n"
endif
	$(Q)$(MAKE) -C src/
ifeq ($(MAKELEVEL), 0)
	@printf "\033[;32mBuild finished\033[0m\n"
endif

.PHONY: install
install: all
	$(UPDATE) src/cppincludeclean /usr/local/bin/

	@printf "\033[;32mInstallation finished\033[0m\n"

.PHONY: clean
clean:
	$(Q)$(MAKE) clean -C src/

.PHONY: help
help:
	@echo "Nothing is here yet..."
