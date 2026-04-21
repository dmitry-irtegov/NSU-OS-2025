main_package_path = ./cmd/waldeproxy
binary_name = waldeproxy

.PHONY: build 
build: 
	go build -o ${binary_name} ${main_package_path} 

.PHONY: run 
run:
	go run ${main_package_path} 

.PHONY: clean 
clean: 
	go clean 
	rm -f ${binary_name}