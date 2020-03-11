BUILD_DIR := cmake-build-debug
SERVER_NAME := rask

.PHONY: server clean run install uninstall

install: server
	sudo cp $(BUILD_DIR)/$(SERVER_NAME) /usr/local/bin/
	sudo mkdir -p /etc/$(SERVER_NAME)
	sudo cp files/$(SERVER_NAME).conf /etc/$(SERVER_NAME)/
	sudo cp -r www/* /var/www/
	sudo cp files/$(SERVER_NAME).service /lib/systemd/system/

uninstall:
	sudo rm -rf /etc/$(SERVER_NAME)
	sudo rm /usr/local/bin/$(SERVER_NAME)

server:
	mkdir -p $(BUILD_DIR)
	cmake -B./$(BUILD_DIR)
	make -C $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

run:
	cd $(BUILD_DIR) && ./server