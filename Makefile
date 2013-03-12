default: all

.DEFAULT:
	cd ctrace && $(MAKE) $@

.PHONY: test
test:
	nosetests
