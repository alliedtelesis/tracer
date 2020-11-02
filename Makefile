default: all

.DEFAULT:
	cd ctrace && $(MAKE) $@

.PHONY: test
test:
	python3 -m unittest discover -v -s test
