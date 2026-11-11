# Forward commands to the submission's source directory.
.PHONY: all run test clean
all run test clean:
	$(MAKE) -C src $@
