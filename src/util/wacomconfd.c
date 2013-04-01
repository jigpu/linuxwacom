/*
 * This program puts the wacom tablet into "pen" mode, and
 * sets configuration parameters from the file passed in as an
 * argument to the "-s" option.  Once started, the program
 * becomes a daemon.  Only one instance of the daemon is allowed
 * to run at a time.  Whenever a tablet is added to the system,
 * the daemon is notified by the sysevent daemon and puts the
 * tablet into pen mode.  If you want to change parameters once
 * the daemon has started, either kill the daemon and run it again, or
 * modify the configuration file and send a SIGHUP to the running
 * daemon.  Currently, the program does nothing when a tablet is removed
 * from the system.  If needed, that can be added later.  We don't bother
 * with locking since the only action taken is ioctl, and it does not
 * hurt to do the ioctl multiple times.  All ioctls are sent to the
 * consms driver, which in turn propagate to all lower streams.
 * Lower streams which are not associated with wacom tablets will
 * ignore these or send up an error.  The error is ignored if there is at
 * least one wacom tablet that indicated success on the system.  The
 * streamhead will ensure that multiple ioctls to the same stream (consms),
 * will be run serially, so we shouldn't need locks.  (see strdoioctl() in
 * usr/src/uts/common/os/streamio.c for details).
 */

#include <sys/types.h>
#include <stropts.h>
#include <sys/conf.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <libdevinfo.h>
#include <pthread.h>
#include <signal.h>

#include <libsysevent.h>
#include <libnvpair.h>
#include <libgen.h>
#include <sys/sunddi.h>

#include <sys/termios.h>

#include "usbms.h"

typedef void (sehfn)(sysevent_t *ev);
static void do_conf(void);
static pid_t enter_daemon_lock(void);
static void exit_daemon_lock(void);

sehfn handler;

/* the lock file is used to only allow one wacomconfd to run at a time */
#define DAEMON_LOCK_FILE "/etc/wacom/wacomconfd.lock"
static char local_lock_file[PATH_MAX+1];
static int hold_daemon_lock;
static int daemon_lock_fd = -1;

char *rootdir = "";   /* relative root path for lock file */

static sigset_t mask;
static void *sighandler(void *);

#define MOUSEDEV "/dev/mouse"
#define MOUSE "mouse"

int parse(char *, struct wacom_confparms *);
void printconf(struct wacom_confparms *);
struct wacom_confparms wcf;
char *progname;
char *cfile = NULL;  /* configuration file name */


int wacomfound = 0;
int foundall = 0;

int
find_wacom_node(di_node_t node, void *arg)
{
  di_prop_t prop;
  int *prop_ints;
  int64_t *prop_int64;
  char *prop_strs;
  unsigned char *prop_bytes;
  int i, n;
  int *vendorid = 0;
  int *productid = 0;

  prop = di_prop_next(node, DI_PROP_NIL);
  if (prop == DI_PROP_NIL)
    return DI_WALK_CONTINUE;
  do {
    if ((strcmp(di_prop_name(prop), "usb-vendor-id") == 0)) {
      if ((n = di_prop_ints(prop, &vendorid)) != 1)
	return DI_WALK_CONTINUE;
      if (*vendorid == USB_WACOM_VENDOR_ID) {
	++wacomfound;
	if (productid != 0) {
	  wcf.wacom_vendorid = *vendorid;
	  wcf.wacom_productid = *productid;
	  wcf.wacom_instance[wacomfound-1] = di_instance(node);
	  wcf.wacom_ninstances = wacomfound;
#ifdef DEBUG
	  printf("instance = %x, vendorid = %x, productid = %x\n",
		 wcf.wacom_instance[wacomfound-1], wcf.wacom_vendorid, wcf.wacom_productid);
#endif
	  ++foundall;
	  return DI_WALK_CONTINUE;
	}
      }
    } 
    if ((strcmp(di_prop_name(prop), "usb-product-id") == 0)) {
      if (di_prop_type(prop) == DI_PROP_TYPE_INT) {
	if ((n = di_prop_ints(prop, &productid)) != 1)
	  return DI_WALK_CONTINUE;
      }
      if (vendorid && *vendorid == USB_WACOM_VENDOR_ID) {
	wcf.wacom_vendorid = *vendorid;
	wcf.wacom_productid = *productid;
#ifdef DEBUG
	printf("vendorid = %x, productid = %x\n", wcf.wacom_vendorid, wcf.wacom_productid);
#endif
	++foundall;
	return DI_WALK_CONTINUE;
      }
    }
  } while ((prop = di_prop_next(node, prop)) != DI_PROP_NIL);
}

int
main(int argc, char *argv[])
{

  unsigned short curpress;
  di_node_t mouse_node;
  char c;
  int listflg = 0;
  int errflg = 0;
  int i;
  sysevent_handle_t *seh;
  const char *subclass_list[] = {ESC_DEVFS_DEVI_ADD};
  struct strioctl str;
  int devfd, pid;
  pthread_t stid;
  di_node_t root_node;
  int fd;


  progname = argv[0];

  while ((c = getopt(argc, argv, "ls:r:")) != -1) {
    switch(c) {
    case 'l':
      listflg++;
      break;
    case 's':
      cfile = optarg;
      break;
    case 'r':
      rootdir = malloc(strlen(optarg)+1);
      if (rootdir == NULL) {
	fprintf(stderr, "cannot allocate space for rootdir name %s\n", optarg);
	exit(1);
      }
      (void) strcpy(rootdir, optarg);
      break;
    case ':': /* -s or -r without operand */
      fprintf(stderr, "Option -%c requires an operand\n", optopt);
      errflg++;
      break;
    case '?':
      fprintf(stderr, "Unrecognized option: -%c\n", optopt);
      errflg++;
    }
  }
  if (errflg) {
    fprintf(stderr, "Usage: %s [-l] [-s config_file] [-r rootdir]\n", progname);
    exit(1);
  }

  if (listflg == 0 && cfile == NULL)
    ++listflg;  /* no args, list current config parms */

  if (listflg) {
    if ((root_node = di_init("/", DINFOCPYALL)) == DI_NODE_NIL) {
      fprintf(stderr, "di_init failed\n");
      exit(1);
    }

    di_walk_node(root_node, DI_WALK_CLDFIRST, NULL, find_wacom_node);

    if (!wacomfound) {
      fprintf(stderr, "%s: no wacom tablet found on system\n", argv[0]);
      exit(1);
    }

    if ((devfd = open(MOUSEDEV, O_RDWR)) < 0) {
      fprintf(stderr, "%s: cannot open %s\n", progname, MOUSEDEV);
      exit(1);
    }

    str.ic_cmd = USBMS_GETCONFPARMS;
    str.ic_timout = -1;
    str.ic_len = sizeof(wcf);
    str.ic_dp = (char *)&wcf;

    if (ioctl(devfd, I_STR, &str) < 0) {
      fprintf(stderr, "%s: ioctl failed, no tablets on system?\n", progname);
      exit(1);
    } else {
      printconf(&wcf);
      close(devfd);
      exit(0);
    }
  }


  /* daemonize */
  
  if ((pid = fork()) > 0)
    exit(0);  /* parent exits */
  else if (pid == (pid_t)-1) {
     fprintf(stderr, "%s: fork failed\n", progname);
     exit_daemon_lock();
     exit(1);
  }

  if ((fd = open("/dev/tty", 2)) > 0) {
    (void) ioctl(fd, (u_long) TIOCNOTTY, (char *) 0);
    (void) close(fd);
    (void) setpgrp();
  }

  /* only one instance of wacomconfd can run at a time */
  /* this code comes from syseventd */
  if ((pid = enter_daemon_lock()) != getpid()) {
    fprintf(stderr, "%s: pid %ld already running\n", progname, pid);
    fprintf(stderr, "To change config parameters, modify wacom.conf and 'kill -HUP %ld'\n", pid);
    fprintf(stderr, "or 'svcadm restart wacomconfd'\n");
    exit(1);
  }

  sigfillset(&mask);
  pthread_sigmask(SIG_BLOCK, &mask, NULL);
  pthread_create(&stid, NULL, sighandler, NULL);

  do_conf();

  seh = sysevent_bind_handle(handler);
  if (seh == NULL) {
    perror("sysevent_bind_handle");
    exit_daemon_lock();;
  }

  if (sysevent_subscribe_event(seh, EC_DEVFS, subclass_list, 1) != 0) {
    perror("sysevent_subscribe_event");
    exit_daemon_lock();
  }
  while (1)
    pause();
}

void
do_conf(void)
{
  di_node_t root_node;
  int i, j;
  int devfd, conffd;
  struct strioctl str;
  int numtries = 10;

  /*
   * Must reset this every time through here.  
   */
  foundall = 0;
  wacomfound = 0;

  if ((root_node = di_init("/", DINFOCPYALL)) == DI_NODE_NIL) {
    fprintf(stderr, "di_init failed\n");
    exit_daemon_lock();
  }

  di_walk_node(root_node, DI_WALK_CLDFIRST, NULL, find_wacom_node);

  if (!wacomfound) {
    fprintf(stderr, "%s: no wacom tablet found on system\n", progname);
  }

  for (i = 0; i < foundall; i++) {
    if ((devfd = open(MOUSEDEV, O_RDWR)) < 0) {
      fprintf(stderr, "%s: cannot open %s\n", progname, MOUSEDEV);
      exit_daemon_lock();
    }

    if (cfile) {
      if (parse(cfile, &wcf) < 0)
	exit_daemon_lock();

      str.ic_cmd = USBMS_SETCONFPARMS;
      str.ic_timout = -1;
      str.ic_len = sizeof(wcf);
      str.ic_dp = (char *)&wcf;

      if (ioctl(devfd, I_STR, &str) < 0) {
	fprintf(stderr, "%s: ioctl failed, errno = %d\n", progname, errno);
      }
    }
    close(devfd);
  }
}


int
parse(char *f, struct wacom_confparms *wcp)
{
  FILE *cfp;
  char line[512];  /* should be enough */
  char id[512];
  char token[128];
  long value;
  int lineno = 0;
  char *p;
  int rval, i;

  if ((cfp = fopen(f, "r")) == (FILE *) NULL) {
    fprintf(stderr, "Cannot open conf file %s\n", f);
    return(-1);
  }

  while (fgets(line, sizeof(line), cfp) != NULL) {
    lineno++;
    if (line[strlen(line)-1] != '\n') {
      fprintf(stderr, "line in conf file %s: line number %d: is too long\n", f, lineno);
      return(-1);
    }

    if ((p = strchr(line, '#')) != (char *) NULL)
      *p = '\0';  /* strip off comment */

    for (p = line; p != '\0' && isspace(*p); p++);  /* strip leading space */
    if (*p == '\0')  /* line is empty or comment */
      continue; 
    strcpy(id, p);
    for (i = 0; *p && *p != '=' && !isspace(*p); i++, p++)
      token[i] = *p;
    if (*p == '\0')
      continue;  /* token with no value */
    token[i] = '\0';
    ++p;
    for (; (*p && *p != '=') || isspace(*p); p++);
    if (*p++ == '\0')
      continue;
    value = strtol(p, (char **)NULL, 0);

    if (strcmp(token, PENPRESSTHRESH) == 0) {
      if (value < MINPENCLICKTHRESH || value > MAXPENCLICKTHRESH) {
	fprintf(stderr, "%s must be >= %d, and <= %d\n",
		PENPRESSTHRESH, MINPENCLICKTHRESH, MAXPENCLICKTHRESH);
	return(-1);
      }
      wcp->wacom_pen_click_threshhold = (unsigned short)value;
    } else if (strcmp(token, XSTARTPOS) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", XSTARTPOS);
	return(-1);
      }
      wcp->wacom_xstart = value;
    } else if (strcmp(token, YSTARTPOS) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", YSTARTPOS);
	return(-1);
      }
      wcp->wacom_ystart = value;
    } else if (strcmp(token, XSIZE) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", XSIZE);
	return(-1);
      }
      wcp->wacom_xsize = value;
    } else if (strcmp(token, YSIZE) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", YSIZE);
	return(-1);
      }
      wcp->wacom_ysize = value;
    } else if (strcmp(token, PENHEIGHTTHRESH) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", PENHEIGHTTHRESH);
	return(-1);
      }
      wcp->wacom_pen_height_threshhold = value;
    } else if (strcmp(token, SCREENWIDTH) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", SCREENWIDTH);
	return(-1);
      }
      wcp->wacom_screen_width = value;
    } else if (strcmp(token, SCREENHEIGHT) == 0) {
      if (value < 0) {
	fprintf(stderr, "%s must be >= 0\n", SCREENHEIGHT);
	return(-1);
      }
      wcp->wacom_screen_height = value;
    } else {
      fprintf(stderr, "error: unknown token %s at lineno %d\n", token, lineno);
      return(-1);
    }
  }
  fclose(cfp);
  return(0);
}

void
printconf(struct wacom_confparms *wcp)
{
  printf("%s: %d\n", XSTARTPOS, wcp->wacom_xstart);
  printf("%s: %d\n", YSTARTPOS, wcp->wacom_ystart);
  printf("%s: %d\n", XSIZE, wcp->wacom_xsize);
  printf("%s: %d\n", YSIZE, wcp->wacom_ysize);
  printf("%s: %d\n", PENPRESSTHRESH, wcp->wacom_pen_click_threshhold);
  printf("%s: %d\n", PENHEIGHTTHRESH, wcp->wacom_pen_height_threshhold);
  printf("%s: %d\n", SCREENWIDTH, wcp->wacom_screen_width);
  printf("%s: %d\n", SCREENHEIGHT, wcp->wacom_screen_height);
}

void
handler(sysevent_t *ev)
{
  nvlist_t *nvlist;
  nvpair_t *nvpp;
  char *class, *subclass;
  unsigned int n, i;
  char *str;

  if (strcmp(EC_DEVFS, sysevent_get_class_name(ev)) != 0) {
      return;
  }
  if (strcmp(ESC_DEVFS_DEVI_ADD, sysevent_get_subclass_name(ev)) != 0) {
    return;
  }

  if (sysevent_get_attr_list(ev, &nvlist) != 0) {
    fprintf(stderr, "no nvlist\n");
    return;
  }

  nvpp = NULL;
  while ((nvpp = nvlist_next_nvpair(nvlist, nvpp)) != NULL) {
    if (strcmp(DEVFS_PATHNAME,  nvpair_name(nvpp)) == 0) {
      if (nvpair_type(nvpp) == DATA_TYPE_STRING) {
	nvpair_value_string(nvpp, &str);
	if (strncmp(basename(str), MOUSE, strlen(MOUSE)) == 0)
	  do_conf();
      }
      break;
    }
  }
}


/*
 * enter_daemon_lock - lock the daemon file lock
 *
 * Use an advisory lock to ensure that only one daemon process is active
 * in the system at any point in time.	If the lock is held by another
 * process, do not block but return the pid owner of the lock to the
 * caller immediately.	The lock is cleared if the holding daemon process
 * exits for any reason even if the lock file remains, so the daemon can
 * be restarted if necessary.  The lock file is DAEMON_LOCK_FILE.
 */
static pid_t
enter_daemon_lock(void)
{
	struct flock	lock;

	if (snprintf(local_lock_file, sizeof (local_lock_file), "%s%s",
		     rootdir, DAEMON_LOCK_FILE) >= sizeof (local_lock_file)) {
	  fprintf(stderr, "%s: error creating lock file name\n");
	  exit(3);
	}

	daemon_lock_fd = open(local_lock_file, O_CREAT|O_RDWR, 0644);
	if (daemon_lock_fd < 0) {
		fprintf(stderr, "%s: cannot open lock file, %s, errno = %d\n", 
			progname, local_lock_file, errno);
		exit(3);
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(daemon_lock_fd, F_SETLK, &lock) == -1) {
		if (fcntl(daemon_lock_fd, F_GETLK, &lock) == -1) {
			exit(2);
		}
		return (lock.l_pid);
	}
	hold_daemon_lock = 1;

	return (getpid());
}

/*
 * exit_daemon_lock - release the daemon file lock
 */
static void
exit_daemon_lock(void)
{
	struct flock lock;

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(daemon_lock_fd, F_SETLK, &lock) == -1) {
		exit(4);
	}

	if (close(daemon_lock_fd) == -1) {
		exit(-1);
	}
}

void *
sighandler(void *arg)
{
	int signo;

	while (sigwait(&mask, &signo) == 0) {
		switch(signo) {
		case SIGHUP:
			do_conf();
			break;
		default:
			if (hold_daemon_lock) {
				exit_daemon_lock();
			}
			exit(2);
		}
	}
}


			
