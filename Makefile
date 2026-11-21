# Forward commands to the submission's source directory.
.PHONY: all run test clean lint format format-check empacotar
all run test clean lint format format-check:
	$(MAKE) -C src $@

empacotar:
	python3 testes/empacotar.py
