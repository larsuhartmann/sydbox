
GTESTER = gtester

TESTS =

test: $(TESTS)
	@test -z "$(TESTS)" || $(GTESTER) --verbose $(TESTS)
	@test -z "$(SUBDIRS)" || \
	   for subdir in $(SUBDIRS) ; do \
	       test "$$subdir" = "." || \
	           ( cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $@ ) || exit $? ; \
	   done

.PHONY: test

check-local: test

