#include <stdio.h>
#include <fcntl.h>  
#include <unistd.h>
#include <stdlib.h>
#define CAP 20
#define BLOCK_SIZE 50
#define MAX_ROW_SIZE 
#define RECORD_SIZE sizeof(record)
#define READ_ERROR -112
#define EMPTY_FILE -323
#define REALLOC_ERROR -142

typedef struct record {
	off_t padding;
	size_t size; 
} record; 

typedef struct dynamic_table {
	size_t size; 
	record* values; 
	size_t capacity; 
} dynamic_table;

int AddValue (dynamic_table* table, off_t point, size_t size) {
	table->size += 1; 
	if (table->size > table->capacity) {
		record* newPtr; 
		table->capacity = table->capacity * 2; 
		newPtr = (record*) realloc (table->values, RECORD_SIZE * table->capacity); 
		if (newPtr == NULL) 
			return REALLOC_ERROR; // проверка на то, что нормально реаллоцировалось 
		else 
			table->values = newPtr; 
	}
	table->values[table->size - 1].padding = point;
	table->values[table->size - 1].size = size;  
	return 1; // нормально добавилось
}

void PrintTable (dynamic_table* table) {
	for (size_t i = 0; i < table->size; i++)
		printf("Row[%lu]: padding = %ld length = %lu\n", i + 1, table->values[i].padding, table->values[i].size);
}

int CreateTable(dynamic_table* table, int file) {
	off_t readBytes = 0, padding = 0, processedBytes = 0;
	int avCode; 
	size_t stringSize = 0;
	char buffer[BLOCK_SIZE];
	ssize_t readCode = read(file, buffer, BLOCK_SIZE);
	while (readCode > 0) {
		readBytes += (off_t) readCode;
		for (ssize_t i = 0; i < readCode; i++) {
			processedBytes++; 
			if (buffer[i] == '\n') {
				avCode = AddValue(table, padding, stringSize); 
				if (avCode == REALLOC_ERROR) 
					return REALLOC_ERROR; // проверека на корректное добавление новой строки в таблицу 
				stringSize = 0; 
				padding = processedBytes; 
			}
			else 
				stringSize++; 
		} 
		readCode = read(file, buffer, BLOCK_SIZE);
	}
	if (readCode == -1) 
		return READ_ERROR; 
	else if (!readCode && stringSize > 0) {
		avCode = AddValue(table, padding, stringSize); 
			if (avCode == REALLOC_ERROR) 
				return REALLOC_ERROR; // проверка на корректное добавление новой строки в таблицу 
	}
	else if (!readCode && !readBytes)
		return EMPTY_FILE; 
	return 1; // таблица без проблем создалась 
} 

size_t FindMaxLength(dynamic_table* table) {
	size_t maxLength = 0; 
	for (size_t i = 0; i < table->size; ++i) {
		if (table->values[i].size > maxLength)
			maxLength = table->values[i].size;
	}
	return maxLength;
}

int main()
{
	int file = open("test.txt", O_RDONLY); 
	int csCode; 
	if (file == -1) {
		perror("Cannot open the file\n");
		return 0; 
	}
	dynamic_table rowTable = {0, (record*) malloc(RECORD_SIZE * CAP), CAP}; 
	if (rowTable.values == NULL) {
		printf("Cannot create array for the table\n"); 
		free(rowTable.values);
		csCode = close(file); 
		if (csCode == -1) 
			printf("Closing file error...\n");
		return 0; 
	}
	int ctCode = CreateTable(&rowTable, file);
	if (ctCode == EMPTY_FILE) {
		printf("The file is empty\n");
		free(rowTable.values);
		csCode = close(file);
		if (csCode == -1) 
			printf("Closing file error...\n");
		return 0; 
	} 
	else if (ctCode == REALLOC_ERROR) {
		printf("Cannot realloc memory for the table\n");
		free(rowTable.values); 
		csCode = close(file);
		if (csCode == -1) 
			printf("Closing file error...\n");
		return 0;  
	}
	else if (ctCode == READ_ERROR) {
		printf("Reading error\n");
		free(rowTable.values); 
		csCode = close(file);
		if (csCode == -1) 
			printf("Closing file error...\n");
		return 0; 
	}
	else {
		PrintTable(&rowTable);
		int qAmount;
		int rowNum;
		ssize_t readCode;    
		off_t lskCode; 
		size_t rowSize; 
		size_t maxLength = FindMaxLength(&rowTable);
		printf("Enter the query amount: ");  
		scanf("%d", &qAmount);
		char row[maxLength + 1]; 
		for (int query = 1; qAmount > 0 && query <= qAmount; query++) {
			scanf("%d", &rowNum);
			if (rowNum < 1) {
				printf("Row number MUST BE positive...\n");
				continue; 
			}
			if ((size_t) rowNum > rowTable.size) {
				printf("Row number MUST BE lower than table size...\n"); 
				continue; 
			}
			if (rowNum == 1) 
				lskCode = lseek(file, 0, SEEK_SET);
			else
				lskCode = lseek(file, rowTable.values[rowNum - 1].padding, SEEK_SET);
			if (lskCode == -1) {
				free(rowTable.values); 
				csCode = close(file);
				if (csCode == -1) 
					printf("Closing file error...\n");
				printf("This file does not support changing position...\n");
				return 0;  
			}
			rowSize = rowTable.values[rowNum - 1].size; 
			readCode = read(file, row, rowSize);
			if (readCode == 0) {
				printf("String is empty\n");
				continue;  
			}
			if (readCode == -1) {
				printf("Reading error...\n");
				continue;  
			}
			row[rowSize] = '\0'; 
			printf("Row: %d - [%s]\n", rowNum, row);
		} 
		free(rowTable.values); 
		return 0; 
	}
}