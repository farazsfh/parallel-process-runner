#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


// Linked list
struct item {
    char* data;
	int fd0;
	int fd1;
    struct item *next;
};

struct item *head = NULL;

void insert(char* data) {
    struct item *n;

    if ((n = malloc(sizeof(struct item))) == NULL) {
        perror("malloc");
        exit(1);
    }

    n->data = data;
	n->fd0 = -1;
	n->fd1 = -1;
    n->next = head;
	head = n;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s command\n", argv[0]);
		return(1);
	}

	// Check if argv[1] has atleast one %
	int hasPercent;
	char *firstPercent;
	if ((firstPercent = strchr(argv[1], '%')) == NULL) {
		hasPercent = 0;
	} else {
		hasPercent = 1;
	}

	/* If argv[1] has a %, calculate the strings to put before
	and after the substitution */
	char *preSub;
	char *postSub;
	int index = firstPercent - argv[1];
	if (hasPercent == 1) {
		if (index == 0 && index == strlen(argv[1]) - 1) {
			preSub = "";
			postSub = "";
		} else if (index == 0) {
			preSub = "";
			postSub = strtok(argv[1], "%");
		} else if (index == strlen(argv[1]) - 1) {
			preSub = strtok(argv[1], "%");
			postSub = "";
		} else {
			preSub = strtok(argv[1], "%");
			postSub = strtok(NULL, "");
		}
	}

	// Create a char pointer to handle input
	int p_size = 256;
	char *p = malloc(p_size);
	if (p == NULL) {
		perror("malloc");
		return(1);
	}

	int i = 0, c;
	while ((c = getchar()) != EOF) {
		// Newline means we're done with the current substitution
		if (c == '\n') {
			p[i] = '\0';

			// Create char pointer to big enough to contain substitution and original command
			char *concat = malloc(strlen(argv[1]) + p_size + 10);
			if (concat == NULL) {
				perror("malloc");
				return(1);
			}

			// If substitution has a percent, sub it into the command, otherwise just store the command itself
			if (hasPercent == 1) {
				strcpy(concat, preSub);
				strcat(concat, p);
				strcat(concat, postSub);
			} else {
				strcpy(concat, argv[1]);
			}

			// Insert completed line into the linked list
			insert(concat);
			i = 0;
		} else {
			p[i] = c;
			i += 1;
			// If we've run out of space, reallocate 2 * original size
			if (i == p_size) {
				p_size = p_size * 2;
				p = realloc(p, p_size);
				if (p == NULL) {
					perror("realloc");
					return(1);
				}
			}
		}
	}

	struct item *curr = head;
	int x;
	int fd[2];

	while (curr != NULL) {
		// Create new pipe
		if ((pipe(fd)) == -1) {
			perror("pipe");
			return(1);
		}

		x = fork();
		if (x > 0) {
			/* parent */
			if (close(fd[1]) == -1) {
				perror("close");
				return(1);
			}
		} else if (x == 0) {
			/* child */

			// Redirect stdin from /dev/null
			if (close(0) == -1) {
				perror("close");
				return(1);
			}

			if (open("/dev/null", O_RDONLY) == -1) {
				perror("open");
				return(1);
			}

			if (close(fd[0]) == -1) {
				perror("close");
				return(1);
			}

			// Redirect stdout and stderr to pipe
			dup2(fd[1], fileno(stderr));
			dup2(fd[1], fileno(stdout));

			if (close(fd[1]) == -1) {
				perror("close");
				return(1);
			}

			execl("/bin/sh", "sh", "-c", curr->data, (char *)NULL);
			perror("/bin/sh");
        	return(1);
		} else {
			perror("fork");
			return(1);
		}
		// Set file descriptors in linked list node so we can access later
		curr->fd0 = fd[0];
		curr->fd1 = fd[1];
		curr = curr->next;
	}

	curr = head;
	char buf;
	while (curr != NULL) {
		printf("---- %s ----\n", curr->data);
		// Read from current node file descriptor till EOF
		while(read(curr->fd0, &buf, 1) > 0) {
			if (write(fileno(stdout), &buf, 1) == -1) {
				perror("write");
				return(1);
			}
		}
		curr = curr->next;
	}

	return(0);
}

