
log_files=$(shell find /home/chenlu/program/nemu/build/ -name "bb*")

bb-count: $(log_files)
	mkdir -p build
	$(foreach file, $(log_files), \
		$(shell awk '{++freq[$$1]; sum[$$1]+=$$2; inst[$$1]=$$3} END{for (pc in sum) print pc " " sum[pc] " " inst[pc]}' $(file) > build/$(notdir $(file))) \
		$(shell sort -g -k 1 -o build/$(notdir $(file)) build/$(notdir $(file))) \
		$(shell sed -i '1i\pc count inst_num' build/$(notdir $(file))) \
		)


.PHONY: bb_count