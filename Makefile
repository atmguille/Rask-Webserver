BUILD_DIR := cmake-build-debug

.PHONY: server clean run

server:
	mkdir -p $(BUILD_DIR)
	cmake -B./$(BUILD_DIR)
	make -C $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

run:
	cd $(BUILD_DIR) && ./server