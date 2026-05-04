main_package_path = ./cmd/sem2-lab20
binary_name = sem2-lab20

.PHONY: build 
build: 
	[ -f go.mod ] || go mod init sem2-lab20
	go build -o ${binary_name} ${main_package_path} 

.PHONY: run 
run:
	[ -f go.mod ] || go mod init sem2-lab20
	go run ${main_package_path} 

.PHONY: clean 
clean: 
	go clean 
	rm -f ${binary_name}