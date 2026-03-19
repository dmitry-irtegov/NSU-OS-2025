#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	LINES_TO_PRINT = 10,
	TURN_PARENT = 0,
	TURN_CHILD = 1,
};

typedef struct SharedState {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int turn;
} SharedState;

static void die_pthread(int err, const char *what)
{
	fprintf(stderr, "%s: %s\n", what, strerror(err));
	exit(EXIT_FAILURE);
}

static void *child_thread(void *arg)
{
	SharedState *state = (SharedState *)arg;

	for (int i = 1; i <= LINES_TO_PRINT; ++i) {
		int rc = pthread_mutex_lock(&state->mutex);
		if (rc != 0) {
			die_pthread(rc, "pthread_mutex_lock (child)");
		}

		while (state->turn != TURN_CHILD) {
			rc = pthread_cond_wait(&state->cond, &state->mutex);
			if (rc != 0) {
				die_pthread(rc, "pthread_cond_wait (child)");
			}
		}

		printf("child: line %d\n", i);
		state->turn = TURN_PARENT;

		rc = pthread_cond_signal(&state->cond);
		if (rc != 0) {
			die_pthread(rc, "pthread_cond_signal (child)");
		}

		rc = pthread_mutex_unlock(&state->mutex);
		if (rc != 0) {
			die_pthread(rc, "pthread_mutex_unlock (child)");
		}
	}

	return NULL;
}

int main(void)
{
	pthread_t thread;
	SharedState state;

	int rc = pthread_mutex_init(&state.mutex, NULL);
	if (rc != 0) {
		die_pthread(rc, "pthread_mutex_init");
	}

	rc = pthread_cond_init(&state.cond, NULL);
	if (rc != 0) {
		die_pthread(rc, "pthread_cond_init");
	}

	state.turn = TURN_PARENT;

	rc = pthread_create(&thread, NULL, child_thread, &state);
	if (rc != 0) {
		die_pthread(rc, "pthread_create");
	}

	for (int i = 1; i <= LINES_TO_PRINT; ++i) {
		rc = pthread_mutex_lock(&state.mutex);
		if (rc != 0) {
			die_pthread(rc, "pthread_mutex_lock (parent)");
		}

		while (state.turn != TURN_PARENT) {
			rc = pthread_cond_wait(&state.cond, &state.mutex);
			if (rc != 0) {
				die_pthread(rc, "pthread_cond_wait (parent)");
			}
		}

		printf("parent: line %d\n", i);
		state.turn = TURN_CHILD;

		rc = pthread_cond_signal(&state.cond);
		if (rc != 0) {
			die_pthread(rc, "pthread_cond_signal (parent)");
		}

		rc = pthread_mutex_unlock(&state.mutex);
		if (rc != 0) {
			die_pthread(rc, "pthread_mutex_unlock (parent)");
		}
	}

	rc = pthread_join(thread, NULL);
	if (rc != 0) {
		die_pthread(rc, "pthread_join");
	}

	rc = pthread_cond_destroy(&state.cond);
	if (rc != 0) {
		die_pthread(rc, "pthread_cond_destroy");
	}

	rc = pthread_mutex_destroy(&state.mutex);
	if (rc != 0) {
		die_pthread(rc, "pthread_mutex_destroy");
	}

	return 0;
}
