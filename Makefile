BUILD_DIR := cmake-build-debug
SERVER_NAME := rask

.PHONY: server clean run install uninstall

install: server
	# Copy the executable file
	sudo cp $(BUILD_DIR)/$(SERVER_NAME) /usr/local/bin/
	# Copy the configuration file
	sudo mkdir -p /etc/$(SERVER_NAME)
	sudo cp files/$(SERVER_NAME).conf /etc/$(SERVER_NAME)/
	# Copy the website
	sudo mkdir -p /var/www
	sudo cp -r www/* /var/www/
	# If systemd is present, copy the unit file
	if [ -d "/lib/systemd/system" ]; then \
		sudo cp files/$(SERVER_NAME).service /lib/systemd/system/; \
	fi

uninstall:
	# Remove executable file
	sudo rm -f /usr/local/bin/$(SERVER_NAME)
	# Remove configuration folder
	sudo rm -rf /etc/$(SERVER_NAME)
	# Remove unit file (if it exists)
	sudo rm -f /lib/systemd/system/$(SERVER_NAME)

server:
	mkdir -p $(BUILD_DIR)
	cmake -B./$(BUILD_DIR)
	make -C $(BUILD_DIR) -j

clean:
	rm -rf $(BUILD_DIR)

run:
	cd $(BUILD_DIR) && ./server