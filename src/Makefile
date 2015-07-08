include ../Makefile.config

.PHONY: all
all: cppincludeclean

cppincludeclean: cppincludeclean.cc
	$(Q)$(call P,LINK,$@)$(CXX) $^ $(CXXFLAGS) -o $@

.PHONY: clean
clean:
	$(Q)$(RM) *.o *.deps cppincludeclean
