all:
	$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build"
kissat:
	$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build" kissat
tissat:
	$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build" tissat
clean:
	rm -f "/home/yuhangq/PRS-sc25/third_party/kissat-inc"/makefile
	-$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build" clean
	rm -rf "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build"
coverage:
	$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build" coverage
indent:
	$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build" indent
test:
	$(MAKE) -C "/home/yuhangq/PRS-sc25/third_party/kissat-inc/build" test
.PHONY: all clean coverage indent kissat test tissat
