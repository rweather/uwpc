/*
 *	uw_utmp - /etc/utmp handling
 *
 * Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifdef UTMP

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pwd.h>
#include <utmp.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>

#include "uw_param.h"

struct utinfo {
	struct utinfo	*ui_next;
	struct utinfo	*ui_chain;
	char		*ui_line;
	int		ui_slot;
	int		ui_inuse;
};

static struct utinfo *hash[31];
static struct utinfo *head;

static char *myname;
static fildes_t utmpfd;

extern time_t time();

utmp_init(fd)
fildes_t fd;
{
	register char *cp, *cq;
	register struct utinfo *ui;
	register int hashidx, slot;
	struct passwd *pw;
	FILE *fp;
	char line[256];

	if ((utmpfd = fd) >= 0 && (fp = fopen("/etc/ttys", "r")) == NULL) {
		(void)close(utmpfd);
		utmpfd = -1;
	}
	if (utmpfd >= 0) {
		slot = 0;
		while (fgets(line, sizeof line, fp) != NULL) {
#ifdef V7TTYS
			if (!line[0] || !line[1]) {	/* malformed line */
				slot++;
				continue;
			}
			cp = line+2;	/* skip flag and speed index */
#else
			for (cp=line; *cp && isspace(*cp); cp++)
				;
			if (*cp == '#')
				continue;
#endif
			slot++;
			if ((ui=(struct utinfo *)malloc(sizeof *ui)) != NULL) {
				for (cq=cp; *cq && !isspace(*cq); cq++)
					;
				if ((ui->ui_line=malloc(cq-cp+1)) != NULL) {
					(void)strncpy(ui->ui_line, cp, cq-cp);
					ui->ui_line[cq-cp] = '\0';
				} else {
					free((char *)ui);
					ui = (struct utinfo *)0;
				}
			}
			if (ui != NULL) {
				ui->ui_slot = slot;
				ui->ui_inuse = 0;
				ui->ui_chain = head;
				head = ui;
				hashidx = utmp_hash(ui->ui_line);
				ui->ui_next = hash[hashidx];
				hash[hashidx] = ui;
			}
		}
		(void)fclose(fp);
	}
	if ((pw = getpwuid(getuid())) != NULL &&
	    (myname=malloc(1+strlen(pw->pw_name))) != NULL)
		(void)strcpy(myname, pw->pw_name);
}

static
struct utinfo *
utmp_find(tty)
char *tty;
{
	register char *cp;
	register struct utinfo *ui;

	if ((cp = rindex(tty, '/')) != NULL)
		cp++;
	else
		cp = tty;
	ui = hash[utmp_hash(cp)];
	while (ui != NULL && strcmp(ui->ui_line, cp) != 0)
		ui = ui->ui_next;
	return(ui);
}

utmp_add(tty)
char *tty;
{
	register struct utinfo *ui;
	struct utmp ut;

	if ((ui = utmp_find(tty)) != NULL) {
		(void)strncpy(ut.ut_line, ui->ui_line, sizeof ut.ut_line);
		(void)strncpy(ut.ut_name, myname, sizeof ut.ut_name);
		(void)strncpy(ut.ut_host, "", sizeof ut.ut_host);
		ut.ut_time = (long)time((time_t)0);
		ui->ui_inuse = 1;
		utmp_write(ui->ui_slot, &ut);
	}
}


utmp_rm(tty)
char *tty;
{
	register struct utinfo *ui;
	struct utmp ut;

	if ((ui = utmp_find(tty)) != NULL) {
		(void)strncpy(ut.ut_line, ui->ui_line, sizeof ut.ut_line);
		(void)strncpy(ut.ut_name, "", sizeof ut.ut_name);
		(void)strncpy(ut.ut_host, "", sizeof ut.ut_host);
		ut.ut_time = (long)time((time_t)0);
		ui->ui_inuse = 0;
		utmp_write(ui->ui_slot, &ut);
	}
}

utmp_exit()
{
	register struct utinfo *ui;
	struct utmp ut;

	for (ui=head; ui; ui=ui->ui_chain) {
		if (ui->ui_inuse) {
			(void)strncpy(ut.ut_line,ui->ui_line,sizeof ut.ut_line);
			(void)strncpy(ut.ut_name, "", sizeof ut.ut_name);
			(void)strncpy(ut.ut_host, "", sizeof ut.ut_host);
			ut.ut_time = (long)time((time_t)0);
			ui->ui_inuse = 0;
			utmp_write(ui->ui_slot, &ut);
		}
	}
}

utmp_write(slot, ut)
register int slot;
struct utmp *ut;
{
	extern off_t lseek();

	if (utmpfd >= 0 &&
	    lseek(utmpfd, slot*sizeof(*ut), L_SET) == (off_t)(slot*sizeof(*ut)))
		(void)write(utmpfd, (char *)ut, sizeof *ut);
}

static
utmp_hash(s)
register char *s;
{
	register short h;

	for (h=0; *s; s++)
		h = (h << ((*s)&7)) | (h >> (sizeof h - ((*s)&7))) + *s;
	return(h % sizeof hash / sizeof hash[0]);
}
#else
utmp_add(tty)
char *tty;
{
}

utmp_rm(tty)
char *tty;
{
}

utmp_exit()
{
}
#endif
