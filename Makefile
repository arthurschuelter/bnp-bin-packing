# Forward commands to the submission's source directory.
.PHONY: all run test clean lint format format-check
all run test clean lint format format-check:
	$(MAKE) -C src $@
