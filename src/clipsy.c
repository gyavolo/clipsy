#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XRes.h>

#ifdef ALLOW_PARENTS
#include <proc/readproc.h>
#endif

#define MAX_PROPERTY_VALUE_LEN 4096

Atom CLIPBOARD;
Atom TARGETS;
Atom UTF8_STRING;

char *input = NULL;
size_t input_size = 0;

void send_no(Display *dpy, XSelectionRequestEvent *sev) {
  XSelectionEvent ssev;

  /* All of these should match the values of the request. */
  ssev.type = SelectionNotify;
  ssev.serial = sev->serial;
  ssev.send_event = sev->send_event;
  ssev.display = sev->display;
  ssev.requestor = sev->requestor;
  ssev.selection = sev->selection;
  ssev.target = sev->target;
  ssev.property = None; /* signifies "nope" */
  ssev.time = sev->time;

  XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

void send_targets(Display *dpy, XSelectionRequestEvent *sev) {
  XSelectionEvent ssev;

  XChangeProperty(dpy, sev->requestor, sev->property, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&UTF8_STRING, 1);

  ssev.type = SelectionNotify;
  ssev.serial = sev->serial;
  ssev.send_event = sev->send_event;
  ssev.display = sev->display;
  ssev.requestor = sev->requestor;
  ssev.selection = sev->selection;
  ssev.target = sev->target;
  ssev.property = sev->property;
  ssev.time = sev->time;

  XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

void send_data(Display *dpy, XSelectionRequestEvent *sev) {
  XSelectionEvent ssev;

  XChangeProperty(dpy, sev->requestor, sev->property, sev->target, 8,
                  PropModeReplace, (unsigned char *)input, input_size);

  ssev.type = SelectionNotify;
  ssev.serial = sev->serial;
  ssev.send_event = sev->send_event;
  ssev.display = sev->display;
  ssev.requestor = sev->requestor;
  ssev.selection = sev->selection;
  ssev.target = sev->target;
  ssev.property = sev->property;
  ssev.time = sev->time;

  XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

char *get_property(Display *disp, Window win, Atom xa_prop_type,
                   const char *prop_name, ulong *size) {
  Atom xa_prop_name;
  Atom xa_ret_type;
  int ret_format;
  ulong ret_nitems;
  ulong ret_bytes_after;
  ulong tmp_size;
  unsigned char *ret_prop;
  char *ret;

  xa_prop_name = XInternAtom(disp, prop_name, False);

  if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4,
                         False, xa_prop_type, &xa_ret_type, &ret_format,
                         &ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
    return NULL;
  }

  if (xa_ret_type != xa_prop_type) {
    XFree(ret_prop);
    return NULL;
  }

  /* null terminate the resultr */
  tmp_size = (ret_format / 8) * ret_nitems;
  /* Correct 64 Architecture implementation of 32 bit data */
  if (ret_format == 32)
    tmp_size *= sizeof(long) / 4;
  ret = (char *)malloc(tmp_size + 1);
  memcpy(ret, ret_prop, tmp_size);
  ret[tmp_size] = '\0';

  if (size) {
    *size = tmp_size;
  }

  XFree(ret_prop);
  return ret;
}

#ifdef ALLOW_PARENTS
static pid_t get_ppid(pid_t pid) {
  pid_t ppid = 0;
  PROCTAB *ptp;
  static proc_t proc;
  pid_t pids[2] = {pid, 0};

  ptp = openproc(PROC_PID | PROC_FILLSTATUS, pids);
  if (!ptp)
    return 0;
  if (readproc(ptp, &proc))
    ppid = proc.ppid;
  closeproc(ptp);

  return ppid;
}
#endif

static pid_t get_local_pid(Display *dpy, Window window) {
  pid_t pid;
  XResClientIdSpec spec;
  long num_ids;
  XResClientIdValue *client_ids;
  long i;

  pid = -1;

  spec.client = window;
  spec.mask = XRES_CLIENT_ID_PID_MASK;

  XResQueryClientIds(dpy, 1, &spec, &num_ids, &client_ids);

  for (i = 0; i < num_ids; i++) {
    if (client_ids[i].spec.mask == XRES_CLIENT_ID_PID_MASK) {
      pid = XResGetClientPid(&client_ids[i]);
      break;
    }
  }

  XResClientIdsDestroy(num_ids, client_ids);

  return pid;
}

void timer_callback(int signum) {
  printf("Job done. Over and out!\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <window id>\n"
           "   or: %s `xdotool selectwindow`\n",
           argv[0], argv[0]);
    return 1;
  }
  Window target_window = strtoul(argv[1], NULL, 10);

  Display *dpy;
  Window owner, root;
  int screen;
  XEvent ev;
  XSelectionRequestEvent *sev;

  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "Could not open X display!\n");
    return 1;
  }

  pid_t target_pid = get_local_pid(dpy, target_window);

  printf("Target window 0x%lx PID: %d\n", target_window, target_pid);

  printf("Input: ");
  if (getline(&input, &input_size, stdin) == -1) {
    fprintf(stderr, "Could not read the input!\n");
  }
  printf("\n");
  input[input_size - 1] = 0;
  input_size = strlen(input);
  if (input_size < 2) {
    printf("No input, so goodbye!\n");
    return 0;
  }
  input[--input_size] = 0; // remove the \n

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  /* Prepare some global atoms. */
  CLIPBOARD = XInternAtom(dpy, "CLIPBOARD", False);
  TARGETS = XInternAtom(dpy, "TARGETS", False);
  UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", False);

  /* We need a window to receive messages from other clients. */
  owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

  /* Claim ownership of the clipboard. */
  XSetSelectionOwner(dpy, CLIPBOARD, owner, CurrentTime);
  XSetSelectionOwner(dpy, XA_PRIMARY, owner, CurrentTime);

  for (;;) {
    XNextEvent(dpy, &ev);
    switch (ev.type) {
    case SelectionClear: {
      printf("Lost selection ownership\n");
      return 1;
    } break;
    case SelectionRequest: {
      sev = (XSelectionRequestEvent *)&ev.xselectionrequest;
      printf("Requestor: 0x%lx", sev->requestor);
      char *window_name =
          get_property(dpy, sev->requestor, XA_STRING, "WM_NAME", NULL);
      if (window_name) {
        printf(" %s", window_name);
      }
      pid_t pid = get_local_pid(dpy, sev->requestor);
      printf(" PID: %d\n", pid);
      if (sev->target == TARGETS && sev->property != None) {
        printf("Send TARGETS to %s PID: %d\n", window_name, pid);
        send_targets(dpy, sev);
      } else if ((sev->target != UTF8_STRING && sev->target != XA_STRING) ||
                 sev->property == None) {
        printf("Unable to send target %s\n", XGetAtomName(dpy, sev->target));
        send_no(dpy, sev);
      } else {
#ifdef ALLOW_PARENTS
        while (pid && pid != target_pid) {
          pid = get_ppid(pid);
        }
#endif
        if (pid != target_pid) {
          printf("Denied!\n");
          send_no(dpy, sev);
        } else {
          printf("Send %s selection as %s to %s PID: %d\n",
                 XGetAtomName(dpy, sev->selection),
                 XGetAtomName(dpy, sev->target), window_name, pid);
          send_data(dpy, sev);
          signal(SIGALRM, timer_callback);
          alarm(1);
        }
      }
    } break;
    }
  }
}
