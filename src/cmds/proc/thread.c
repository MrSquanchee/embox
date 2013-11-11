/**
 * @file
 * @brief Prints threads statistic for Embox
 *
 * @date 02.12.10
 * @author Gleb Efimov
 * @author Alina Kramar
 * @author Roman Oderov
 */

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <kernel/thread.h>
#include <kernel/thread/state.h>
#include <kernel/task.h>
#include <kernel/sched.h>

#include <embox/cmd.h>

EMBOX_CMD(exec);

static void print_usage(void) {
	printf("Usage: thread [-h] [-s] [-k <thread_id>]\n");
}

static void print_stat(void) {
	struct thread *thread;
	int running, sleeping, suspended;
	int total;
	struct task *task;

	printf(" %4s  %4s  %8s %18s %10s\n", "id", "tid", "priority", "state", "time");

	running = sleeping = suspended = 0;

	sched_lock();
	{
		task_foreach(task) {
			task_foreach_thread(thread, task) {
				thread_state_t s = thread->state;
				const char *state = NULL;
				sched_priority_t prior;

				prior = sched_priority_thread(task->priority,
										thread_priority_get(thread));

				if ((__THREAD_STATE_READY | __THREAD_STATE_RUNNING) & s) {
					state = "running";
					running++;
				} else if (__THREAD_STATE_WAITING & s) {
					state = "sleeping";
					sleeping++;
				}


				printf(" %4d%c %4d  %8d %18s %9lds\n",
					thread->id, __THREAD_STATE_RUNNING & s ? '*' : ' ',
					thread->task->tid,
					prior,
					state,
					thread_get_running_time(thread)/CLOCKS_PER_SEC);
			}
		}
	}
	sched_unlock();
	total = running + sleeping;

	printf("Total %d threads: \n"
		"\t%d running\n"
		"\t%d sleeping\n", total, running, sleeping);
}

static struct thread *thread_lookup(thread_id_t id) {
	struct thread *t;
	struct task * task;

	sched_lock();
	{
		task_foreach(task) {
			task_foreach_thread(t, task) {
				if (t->id == id) {
					goto out;

				}
			}
		}
		t = NULL;
	}
out:
	sched_unlock();

	return t;
}

static void kill_thread(thread_id_t thread_id) {
	struct thread *thread;

	if (!(thread = thread_lookup(thread_id))) {
		printf("No thread with id: %d\n", thread_id);
		return;
	}

	printf("Thread %d killed\n", thread_id);
}

static int exec(int argc, char **argv) {
	int opt;

	if (argc <= 1) {
		print_usage();
		return -EINVAL;
	}

	getopt_init();

	while (-1 != (opt = getopt(argc, argv, "hsk:"))) {
		printf("\n");
		switch (opt) {
		case '?':
			printf("Invalid command line option\n");
			/* FALLTHROUGH */
		case 'h':
			print_usage();
			break;
		case 's':
			print_stat();
			break;
		case 'k': {
			thread_id_t thread_id;
			if ((optarg == NULL) || (!sscanf(optarg, "%d", &thread_id))) {
				print_usage();
				break;
			}
			kill_thread(thread_id);
			break;
		}
		default:
			break;
		}
	}

	return 0;
}
