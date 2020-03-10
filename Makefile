BUILD_DIR := cmake-build-debug
SERVER_NAME := rask

.PHONY: server clean run install uninstall

install: server
	sudo cp $(BUILD_DIR)/$(SERVER_NAME) /usr/bin/
	sudo mkdir -p /etc/$(SERVER_NAME)
	sudo cp files/$(SERVER_NAME).conf /etc/$(SERVER_NAME)/
	sudo cp -r www/* /var/www/

uninstall:
	sudo rm -rf /etc/$(SERVER_NAME)
	sudo rm /usr/bin/$(SERVER_NAME)

server:
	mkdir -p $(BUILD_DIR)
	cmake -B./$(BUILD_DIR)
	make -C $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

run:
	cd $(BUILD_DIR) && ./server