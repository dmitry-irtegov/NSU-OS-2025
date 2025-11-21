#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE 256

void run_child()
{
	char buffer[BUFFER_SIZE];
	size_t nread;

	while ((nread = fread(buffer, 1, BUFFER_SIZE, stdin)) > 0)
	{
		for (size_t i = 0; i < nread; i++)
		{
			buffer[i] = toupper((unsigned char)buffer[i]);
		}
		fwrite(buffer, 1, nread, stdout);
	}
}

int main(int argc, char *argv[])
{
	if (argc > 1 && strcmp(argv[1], "--child") == 0)
	{
		run_child();
		return 0;
	}

	const char *message = "BIG, small, bOtH.\n";

	char abs_path[256];
	if (!realpath(argv[0], abs_path))
	{
		perror("realpath failed");
		return 1;
	}

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "%s --child", abs_path);

	FILE *pipe = popen(cmd, "w");
	if (!pipe)
	{
		perror("popen failed");
		return 1;
	}

	fwrite(message, 1, strlen(message), pipe);

	pclose(pipe);

	return 0;
}
