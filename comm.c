/* revision 1.2 - August 1 1999 - making it compilable under g++ */
/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *     ROM 2.4 is copyright 1993-1996 Russ Taylor                          *
 *     ROM has been brought to you by the ROM consortium                   *
 *         Russ Taylor (rtaylor@pacinfo.com)                               *
 *         Gabrielle Taylor (gtaylor@pacinfo.com)                          *
 *         Brian Moore (rom@rom.efn.org)                                   *
 *     By using this code, you have agreed to follow the terms of the      *
 *     ROM license, in the file Rom24/doc/rom.license                      *
 ***************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <wait.h>

#include "merc.h"
#include "recycle.h"
#include "clan.h"
#include "deity.h"
#include "utils.h"


/* command procedures needed */
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_skills);
DECLARE_DO_FUN(do_outfit);
DECLARE_DO_FUN(do_echo);
DECLARE_DO_FUN(do_unread);
DECLARE_DO_FUN(do_reboot);
void init_rainupdate();
void cprintf_private(CHAR_DATA *ch, bool inColour, bool showCodes, const char *txt, va_list ap);
extern void load_vote();
extern void load_citems();
extern void load_clan_report();
extern void save_clan_report();
extern void load_tips();
extern int calculate_offering_tax_qp(OBJ_DATA *);
extern int calculate_offering_tax_gold(OBJ_DATA *);

/*
 * Malloc debugging stuff.
 */

#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern int malloc_debug(int);
extern int malloc_verify(void);

#endif

#define socklen_t unsigned int

/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#include <signal.h>



/*
 * Socket and TCP/IP stuff.
 */
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "telnet.h"
#include <signal.h>
const char echo_off_str[] =
{IAC, WILL, TELOPT_ECHO, '\0'};
const char echo_on_str[] =
{IAC, WONT, TELOPT_ECHO, '\0'};
const char go_ahead_str[] =
{IAC, GA, '\0'};



/*
 * OS-dependent declarations.
 */

#if     defined(interactive)
#include <net/errno.h>
#include <sys/fnctl.h>
#endif

/*
   Linux shouldn't need these. If you have a problem compiling, try
   uncommenting accept and bind.
   int accept (int s, struct sockaddr *addr, int *addrlen);
   int bind (int s, struct sockaddr *name, int namelen);
 */

int close(int fd);

/*
   int getpeername (int s, struct sockaddr * name, int *namelen);
   int getsockname (int s, struct sockaddr * name, int *namelen);
   int gettimeofday (struct timeval * tp, struct timezone * tzp);
   int listen (int s, int backlog);
 */
int select(int width, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval *timeout);
int socket(int domain, int type, int protocol);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t fork(void);
int kill(pid_t pid, int sig);
int pipe(int filedes[2]);
int dup2(int oldfd, int newfd);
int execl(const char *path, const char *arg,...);
void init_signals();
void do_crash_save();
void sig_handler(int sig);

/*
 * Global variables.
 */
DESCRIPTOR_DATA *descriptor_list;   /* All open descriptors         */
DESCRIPTOR_DATA *d_next;      /* Next descriptor in loop      */
FILE *fpReserve;            /* Reserved file handle         */
bool god;                  /* All new chars are gods!      */
bool merc_down;               /* Shutdown                     */
bool wizlock;               /* Game is wizlocked            */
bool newlock;               /* Game is newlocked            */
char str_boot_time[MAX_INPUT_LENGTH];
time_t current_time;         /* time of this pulse */
int mudport;               /* mudport */
CHAR_DATA *System = NULL;      // Automated character man. POWER!

/*bool DNSup; no longer needed.. we are non-blocking!!!! */
bool MOBtrigger = TRUE;         /* act() switch                 */
char last_cprintf[MAX_STRING_LENGTH];

AUCTION_DATA *auction;

void game_loop_unix(int control);
int init_socket(int port);
void init_descriptor(int control);
bool read_from_descriptor(DESCRIPTOR_DATA * d);
bool write_to_descriptor(int desc, char *txt, int length);

DECLARE_DO_FUN(do_save);




/*
 * Other local functions (OS-independent).
 */



bool check_reconnect(DESCRIPTOR_DATA * d, char *name, bool fConn);
bool check_playing(DESCRIPTOR_DATA * d, char *name);
int main(int argc, char **argv);
void nanny(DESCRIPTOR_DATA * d, char *argument);
bool process_output(DESCRIPTOR_DATA * d, bool fPrompt);
void read_from_buffer(DESCRIPTOR_DATA * d);
void stop_idling(CHAR_DATA * ch);
void bust_a_prompt(CHAR_DATA * ch);
void message_all(char *arg);
void reboot_all();

time_t start_time;

int
main(int argc, char **argv)
{
   struct timeval now_time;
   int port;
   struct rlimit rl;
   int control;

   /*
    * Memory debugging if needed.
    */
#if defined(MALLOC_DEBUG)
   malloc_debug(2);
#endif

   /*
    * Init time.
    */
   gettimeofday(&now_time, NULL);
   current_time = (time_t) now_time.tv_sec;
   strcpy(str_boot_time, ctime(&current_time));
   start_time = (time_t) now_time.tv_sec;

   getrlimit(RLIMIT_CORE, &rl);
   rl.rlim_cur = rl.rlim_max;
   setrlimit(RLIMIT_CORE, &rl);

   /*
    * Reserve one channel for our use.
    */
   if ((fpReserve = fopen(NULL_FILE, "r")) == NULL)
   {
      perror(NULL_FILE);
      exit(1);
   }

   /*DNSup = TRUE; */

   /*
    * Get the port number.
    */
   port = 4000;
   if (argc > 1)
   {
      if (!is_number(argv[1]))
      {
         fprintf(stderr, "Usage: %s [port #]\n", argv[0]);
         exit(1);
      }
      else if ((port = atoi(argv[1])) <= 1024)
      {
         fprintf(stderr, "Port number must be above 1024.\n");
         exit(1);
      }
   }
   /*
    * Run the game.
    */

   mudport = port;
   control = init_socket(port);
   init_signals();            /* For the use of the signal handler. -Ferric */
   boot_db();
   init_rainupdate();
   load_vote();
   load_citems();
   load_clan_report();
   load_tips();

   // Say hello to our best friend System.
   System = new_char();
   System->level = 61;
   System->trust = 61;
   System->name = str_dup("System");
   System->desc = NULL;
   System->invis_level = 0;
   System->affected = NULL;
   SET_BIT(System->act, ACT_IS_NPC);
   System->in_room = get_room_index(ROOM_VNUM_LIMBO);
   System->short_descr = str_dup("Mud");

   log_string("Redemption is ready to rock on port %d.", port);
   game_loop_unix(control);
   close(control);

   /*
    * That's all, folks.
    */
   log_string("Normal termination of game.");
   exit(0);
   return 0;
}


#ifndef NO_CRASH_SAVE
void
init_signals()
{
   signal(SIGBUS, sig_handler);
   signal(SIGTERM, sig_handler);
   signal(SIGABRT, sig_handler);
   signal(SIGSEGV, sig_handler);
}
#endif

#ifdef NO_CRASH_SAVE
void
init_signals()
{
   return;
}
#endif

void
sig_handler(int sig) {
	switch (sig) {
		case SIGBUS:
		case SIGTERM:
		case SIGABRT:
		case SIGSEGV:
			// Save state
			do_crash_save();
			log_string("Signal Caught: Calling crash save and exiting.");

			// Determine which singal to raise, and turn off the appropriate handler
			if (sig == SIGABRT) {
				// Turn off signal handling for, and raise, SIGSEGV
				signal(SIGSEGV, SIG_DFL);
				raise(SIGSEGV);
			} else {
				// Turn off signal handling for, and raise, SIGABRT
				signal(SIGABRT, SIG_DFL);
				raise(SIGABRT);
			}
	}
}

int
init_socket(int port)
{
   static struct sockaddr_in sa_zero;
   struct sockaddr_in sa;
   int x = 1;
   int fd;

   if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("Init_socket: socket");
      exit(1);
   }
   if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &x, sizeof(x)) < 0)
   {
      perror("Init_socket: SO_REUSEADDR");
      close(fd);
      exit(1);
   }
#if defined(SO_DONTLINGER) && !defined(SYSV)
   {
      struct linger ld;

      ld.l_onoff = 1;
      ld.l_linger = 1000;

      if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER,
                  (char *) &ld, sizeof(ld)) < 0)
      {
         perror("Init_socket: SO_DONTLINGER");
         close(fd);
         exit(1);
      }
   }
#endif

   sa = sa_zero;
   sa.sin_family = AF_INET;
   sa.sin_port = htons(port);

   if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
   {
      perror("Init socket: bind");
      close(fd);
      exit(1);
   }
   if (listen(fd, 3) < 0)
   {
      perror("Init socket: listen");
      close(fd);
      exit(1);
   }
   return fd;
}

int
check_valid(CHAR_DATA * ch)
{                        /* oldvalidity check for merc removed, Russ */
   return (1);
}


/*
 * Here comes the ident driver code.
 * - Wreck
 */

/*
 * Almost the same as read_from_buffer...
 */
bool
read_from_ident(int fd, char *buffer)
{
   static char inbuf[MAX_STRING_LENGTH * 2];
   unsigned int iStart;
   int i, j, k;

   /* Check for overflow. */
   iStart = strlen(inbuf);
   if (iStart >= sizeof(inbuf) - 10)
   {
      log_string("Ident input overflow!!!");
      return FALSE;
   }
   /* Snarf input. */
   for (;;)
   {
      int nRead;

      nRead = read(fd, inbuf + iStart, sizeof(inbuf) - 10 - iStart);
      if (nRead > 0)
      {
         iStart += nRead;
         if (inbuf[iStart - 2] == '\n' || inbuf[iStart - 2] == '\r')
            break;
      }
      else if (nRead == 0)
      {
         return FALSE;
      }
      else if (errno == EWOULDBLOCK)
         break;
      else
      {
         perror("Read_from_ident");
         return FALSE;
      }
   }

   inbuf[iStart] = '\0';
   /*
    * Look for at least one new line.
    */
   for (i = 0; inbuf[i] != '\n' && inbuf[i] != '\r'; i++)
   {
      if (inbuf[i] == '\0')
         return FALSE;
   }

   /*
    * Canonical input processing.
    */
   for (i = 0, k = 0; inbuf[i] != '\n' && inbuf[i] != '\r'; i++)
   {
      if (inbuf[i] == '\b' && k > 0)
         --k;
      else if (isascii(inbuf[i]) && isprint(inbuf[i]))
         buffer[k++] = inbuf[i];
   }

   /*
    * Finish off the line.
    */
   if (k == 0)
      buffer[k++] = ' ';
   buffer[k] = '\0';

   /*
    * Shift the input buffer.
    */
   while (inbuf[i] == '\n' || inbuf[i] == '\r')
      i++;
   for (j = 0; (inbuf[j] = inbuf[i + j]) != '\0'; j++);

   return TRUE;
}

/*
 * Process input that we got from the ident process.
 */
void
process_ident(DESCRIPTOR_DATA * d)
{
   char buffer[MAX_INPUT_LENGTH];
   char address[MAX_INPUT_LENGTH];
   CHAR_DATA *ch = CH(d);
   char *user;
   int results = 0;
   int status;

   buffer[0] = '\0';

   if (!read_from_ident(d->ifd, buffer) || IS_NULLSTR(buffer))
      return;

   /* using first arg since we want to keep case */
   user = first_arg(buffer, address, FALSE);

   /* replace and set some states */
   if (!IS_NULLSTR(user))
   {
      replace_string(d->ident, user);
      SET_BIT(results, 2);
   }
   if (!IS_NULLSTR(address))
   {
      replace_string(d->host, address);
      SET_BIT(results, 1);
   }
   /* do sensible output */
   if (results == 1)
   {                     /* address only */
      /*
       * Change the two lines below to your notification function...
       * (wiznet, ..., whatever)
       *
       sprintf( outbuf, "$n has address #B%s#b. Username unknown.", address );
       give_info( outbuf, ch, NULL, NULL, NOTIFY_IDENT, IMMORTAL );
       */
      log_string("%s has address %s.", ch->name, address);
   }
   else if (results == 2 || results == 3)
   {                     /* ident only, or both */
      /*
       * Change the two lines below to your notification function...
       * (wiznet, ..., whatever)
       *
       sprintf( outbuf, "$n is #B%s@%s#b.", user, address );
       give_info( outbuf, ch, NULL, NULL, NOTIFY_IDENT, IMMORTAL );
       */
      log_string("%s is %s@%s.", ch->name, user, address);
   }
   else
   {
      log_string("%s could not be identified.", ch->name);
   }

   /* close descriptor and kill ident process */
   close(d->ifd);
   d->ifd = -1;
   /*
    * we don't have to check here,
    * cos the child is probably dead already. (but out of safety we do)
    *
    * (later) I found this not to be true. The call to waitpid( ) is
    * necessary, because otherwise the child processes become zombie
    * and keep lingering around... The waitpid( ) removes them.
    */
   waitpid(d->ipid, &status, 0);
   d->ipid = -1;

   return;
}


void
create_ident(DESCRIPTOR_DATA * d, long ip, int port)
{
   int fds[2];
   pid_t pid;

   /* create pipe first */
   if (pipe(fds) != 0)
   {
      perror("Create_ident: pipe: ");
      return;
   }
   if (dup2(fds[1], STDOUT_FILENO) != STDOUT_FILENO)
   {
      perror("Create_ident: dup2(stdout): ");
      return;
   }

   close(fds[1]);

   if ((pid = fork()) > 0)
   {
      /* parent process */
      d->ifd = fds[0];
      d->ipid = pid;
   }
   else if (pid == 0)
   {
      /* child process */
      char str_ip[64], str_local[64], str_remote[64];

      d->ifd = fds[0];
      d->ipid = pid;

      sprintf(str_local, "%d", mudport);
      sprintf(str_remote, "%d", port);
      sprintf(str_ip, "%ld", ip);
      //execl("resolve", "resolve", str_local, str_ip, str_remote, );
      execl("resolve", "resolve", str_local, str_ip, str_remote, (char*)NULL);
      /* Still here --> hmm. An error. */
      //log_string("Exec failed; Closing child.");
      d->ifd = -1;
      d->ipid = -1;
      exit(0);
   }
   else
   {
      /* error */
      perror("Create_ident: fork");
      close(fds[0]);
   }
}


void
game_loop_unix(int control)
{
   static struct timeval null_time;
   struct timeval last_time;
   int toggle[] = { 0, 0, 0, 0 };

   signal(SIGPIPE, SIG_IGN);
   gettimeofday(&last_time, NULL);
   current_time = (time_t) last_time.tv_sec;

   /* Main loop */
   while (!merc_down)
   {
      fd_set in_set;
      fd_set out_set;
      fd_set exc_set;
      DESCRIPTOR_DATA *d;
      int maxdesc;

#if defined(MALLOC_DEBUG)
      if (malloc_verify() != 1)
         abort();
#endif

      /*
       * Poll all active descriptors.
       */
      FD_ZERO(&in_set);
      FD_ZERO(&out_set);
      FD_ZERO(&exc_set);
      FD_SET(control, &in_set);
      maxdesc = control;
      for (d = descriptor_list; d; d = d->next)
      {
         maxdesc = UMAX(maxdesc, d->descriptor);
         FD_SET(d->descriptor, &in_set);
         FD_SET(d->descriptor, &out_set);
         FD_SET(d->descriptor, &exc_set);
         if (d->ifd != -1 && d->ipid != -1)
         {
            maxdesc = UMAX(maxdesc, d->ifd);
            FD_SET(d->ifd, &in_set);
         }
      }

      if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0)
      {
         perror("Game_loop: select: poll");
         exit(1);
      }
      /*
       * New connection?
       */
      if (FD_ISSET(control, &in_set))
         init_descriptor(control);

      /*
       * Kick out the freaky folks.
       */
      for (d = descriptor_list; d != NULL; d = d_next)
      {
         d_next = d->next;
         if (FD_ISSET(d->descriptor, &exc_set))
         {
            FD_CLR(d->descriptor, &in_set);
            FD_CLR(d->descriptor, &out_set);
            if (d->character && d->character->level > 1)
               save_char_obj(d->character, TRUE);
            d->outtop = 0;
            close_socket(d);
         }
      }

      /*
       * Process input.
       */
      for (d = descriptor_list; d != NULL; d = d_next)
      {
         d_next = d->next;
         d->fcommand = FALSE;

         /* check for in_room = 0 freaks */
         if (CH(d) != NULL && CH(d)->in_room == NULL)
         {
            /* HOLD ON! */
            if (CH(d)->was_in_room == NULL)
            {
               char_to_room(CH(d), get_room_index(ROOM_VNUM_LIMBO));
            }
            else
            {
               char_to_room(CH(d), CH(d)->was_in_room);
            }
         }

         if (FD_ISSET(d->descriptor, &in_set))
         {
            if (d->character != NULL)
               d->character->timer = 0;
            if (!read_from_descriptor(d))
            {
               FD_CLR(d->descriptor, &out_set);
               if (d->character != NULL && d->character->level > 1)
                  save_char_obj(d->character, TRUE);
               d->outtop = 0;
               close_socket(d);
               continue;
            }
         }
         /* check for input from the ident */
         if ((d->connected == CON_PLAYING || CH(d) != NULL) &&
            d->ifd != -1 && FD_ISSET(d->ifd, &in_set))
            process_ident(d);

         if (d->character != NULL && d->character->daze > 0)
            --d->character->daze;

         /* check for input from the ident */
         if ((d->connected == CON_PLAYING || CH(d) != NULL) &&
            d->ifd != -1 && FD_ISSET(d->ifd, &in_set))
            process_ident(d);

         if (d->character != NULL && d->character->wait > 0)
         {
            --d->character->wait;
            continue;
         }
         read_from_buffer(d);
         if (d->incomm[0] != '\0')
         {
            d->fcommand = TRUE;
            stop_idling(d->character);

            /* OLC */
            if (d->showstr_point)
               show_string(d, d->incomm);
            else if (d->pString)
               string_add(d->character, d->incomm);
            else
               switch (d->connected)
               {
               case CON_PLAYING:
                  /*if (!run_olc_editor(d))
                     substitute_alias(d, d->incomm);*/
                  break;
               default:
                  nanny(d, d->incomm);
                  break;
               }

            d->incomm[0] = '\0';
         }
         /* macro */
         else
         {
            CHAR_DATA *ch = d->original;

            if (d->original)
            {
               ch = d->original;
               if (ch->desc)
                  ch = ch->desc->original ? ch->desc->original : ch;
            }
            else
               ch = d->character;

            if (ch && !IS_NPC(ch) && !IS_SET(ch->act, PLR_FREEZE))
               do_next_command(ch);
         }
      }

      /*
       * Autonomous game motion.
       */
      update_handler();

      /* web stuff added by Delstar */
      /* handle_web(); */

      /* kill off all errant processes, and grab their
         status so they can die */
      {
         int x;

         waitpid(-1, &x, WNOHANG);
      }

      /*
       * Output.
       */
      for (d = descriptor_list; d != NULL; d = d_next)
      {
         d_next = d->next;

         if ((d->fcommand || d->outtop > 0)
            && FD_ISSET(d->descriptor, &out_set))
         {
            if (!process_output(d, TRUE))
            {
               if (d->character != NULL && d->character->level > 1)
                  save_char_obj(d->character, TRUE);
               d->outtop = 0;
               close_socket(d);
            }
         }
      }


      /*
       * Synchronize to a clock.
       * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
       * Careful here of signed versus unsigned arithmetic.
       */
      {
         struct timeval now_time;
         long secDelta;
         long usecDelta;

         gettimeofday(&now_time, NULL);
         usecDelta = ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
            + 1000000 / PULSE_PER_SECOND;
         secDelta = ((int) last_time.tv_sec) - ((int) now_time.tv_sec);
         while (usecDelta < 0)
         {
            usecDelta += 1000000;
            secDelta -= 1;
         }

         while (usecDelta >= 1000000)
         {
            usecDelta -= 1000000;
            secDelta += 1;
         }

         if (secDelta > 0 || (secDelta == 0 && usecDelta > 0))
         {
            struct timeval stall_time;

            stall_time.tv_usec = usecDelta;
            stall_time.tv_sec = secDelta;
            if (select(0, NULL, NULL, NULL, &stall_time) < 0)
            {
               if (errno == EINTR)
               {
                  /* wiznet("Game_loop: select: interput", NULL, NULL, WIZ_ON, 0, 0); */
               }
               else
               {
                  perror("Game_loop: select: stall");
                  exit(1);
               }
            }
         }
      }


      gettimeofday(&last_time, NULL);

      /* update played_perm for each char */
      for (d = descriptor_list; d != NULL; d = d_next)
      {
         d_next = d->next;

         if (d->connected == CON_PLAYING &&
            CH(d) != NULL &&
            CH(d)->in_room != NULL &&
            CH(d)->in_room != get_room_index(ROOM_VNUM_LIMBO) &&
            CH(d)->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA) &&
            !IS_SET(CH(d)->comm, COMM_AFK) &&
            !IS_SET(CH(d)->act, PLR_FREEZE) &&
            CH(d)->desc != NULL)
         {
            CH(d)->played_perm += last_time.tv_sec - current_time;
            CH(d)->played += last_time.tv_sec - current_time;
         }
      }

      current_time = (time_t) last_time.tv_sec;

      /* Auto reboot code by Tsongas 8/15/01 */
      /* Every day at 4AM - 5 hour difference! Looks like it's 9AM (32400) */
      /* Time in seconds...Modulate it by 24 hours (86400) to get usable number! */

         /* 3:50 */
         if( (current_time % 86400 == 31800) && (toggle[0] == 0) )
         {
         message_all("{GRedemption will be auto-rebooting in {Y10 minutes{G!{x" );
         message_all("{RYou will be restored and saved during this process.{x");
         toggle[0] = 1; /* Message printed, toggle it off */
         }

         /* 3:55 */
         if( (current_time % 86400 == 32100) && (toggle[1] == 0) )
         {
         message_all("{GRedemption will be auto-rebooting in {Y5 minutes{G!{x" );
         message_all("{RYou will be restored and saved during this process.{x");
         toggle[1] = 1; /* Message printed, toggle it off */
         }

         /* 3:59 */
         if( (current_time % 86400 == 32340) && (toggle[2] == 0) )
         {
         message_all("{GRedemption will be auto-rebooting in {Y1 minute{G!{x" );
         message_all("{RYou will be restored and saved during this process.{x");
         toggle[2] = 1; /* Message printed, toggle it off */
         }

         /* 3:59:50 */
         if( (current_time % 86400 == 32390) && (toggle[3] == 0) )
         {
         message_all("{GRedemption will be auto-rebooting in {Y10 seconds{G!{x" );
         message_all("{RYou will be restored and saved during this process.{x");
         toggle[3] = 1; /* Message printed, toggle it off */
         }

         /* 4:00 Reboot! */
         if( current_time % 86400 == 32400 )
         {
         message_all("{GRedemption is auto-rebooting {YNOW{G!{x" );
         message_all("{RYou will now be restored and saved. Please stand-by.{x");
         reboot_all(); /* Shutdown and restore - shell script brings us back up! */
         }
   }

   return;
}


void
init_descriptor(int control)
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *dnew;
   struct sockaddr_in sock;

   /*  struct hostent *from; */
   int desc;
   int size = sizeof(sock);

   getsockname(control, (struct sockaddr *) &sock, (socklen_t *) & size);

   if ((desc = accept(control, (struct sockaddr *) &sock, (socklen_t *) & size)) < 0)
   {
      perror("New_descriptor: accept");
      return;
   }
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

   if (fcntl(desc, F_SETFL, FNDELAY) == -1)
   {
      perror("New_descriptor: fcntl: FNDELAY");
      return;
   }

   /*
    * Cons a new descriptor.
    */
   dnew = (DESCRIPTOR_DATA *) new_descriptor();

   dnew->descriptor = desc;
   dnew->connected = CON_GET_NAME;
   dnew->showstr_head = NULL;
   dnew->showstr_point = NULL;
   dnew->outsize = 2000;
   dnew->pREdit = NULL;      /* OLC */
   dnew->pAEdit = NULL;
   dnew->pOEdit = NULL;
   dnew->pMEdit = NULL;
   dnew->pString = NULL;      /* OLC */
   dnew->editor = 0;         /* OLC */
   dnew->outbuf = (char *) alloc_mem(dnew->outsize);
   dnew->ident = str_dup("???");
   dnew->ifd = -1;
   dnew->ipid = -1;


   size = sizeof(sock);
   if (getpeername(desc, (struct sockaddr *) &sock, (socklen_t *) & size) < 0)
   {
      perror("New_descriptor: getpeername");
      dnew->host = str_dup("(unknown)");
   }
   else
   {

      /*
       * Would be nice to use inet_ntoa here but it takes a struct arg,
       * which ain't very compatible between gcc and system libraries.
       */
      int addr;

      addr = ntohl(sock.sin_addr.s_addr);
      sprintf(buf, "%d.%d.%d.%d",
            (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF, (addr) & 0xFF
         );

      /* fill variable for ident, raw socket data */
      dnew->addr = sock.sin_addr.s_addr;
      dnew->port = ntohs(sock.sin_port);

      log_string("Sock.sinaddr:  %s", buf);

      /* remove this and go non-blocking! */
      /*
         if (DNSup)
         {
         from = gethostbyaddr((char *) &sock.sin_addr,
         sizeof(sock.sin_addr), AF_INET);
         if (from == NULL
         && (!str_cmp(buf, "134.117.136.26")
         || !str_cmp(buf, "134.117.136.25")
         || !str_cmp(buf, "134.117.136.24")
         || !str_cmp(buf, "134.117.136.23")
         || !str_cmp(buf, "134.117.136.22")
         || !str_cmp(buf, "134.117.136.20") ))
         {
         DNSup = FALSE;
         dnew->host = str_dup(buf);
         }
         else if (from == NULL)
         dnew->host = str_dup(buf);
         else
         dnew->host = str_dup(from->h_name);
         }
         else
         {

         } */

      sprintf(dnew->iphost, "%s", buf);
      dnew->host = str_dup(buf);
      /* now run non-blockig lookup */
      create_ident(dnew, dnew->addr, dnew->port);
   }

   /*
    * Swiftest: I added the following to ban sites.  I don't
    * endorse banning of sites, but Copper has few descriptors now
    * and some people from certain sites keep abusing access by
    * using automated 'autodialers' and leaving connections hanging.
    *
    * Furey: added suffix check by request of Nickel of HiddenWorlds.
    */
   if (check_ban(dnew->host, BAN_ALL))
   {
      write_to_descriptor(desc,
                   "Your site has been banned from this mud.\n\r", 0);
      close(desc);
      free_descriptor(dnew);
      return;
   }
   /*
    * Init descriptor data.
    */
   dnew->next = descriptor_list;
   descriptor_list = dnew;

   /*
    * Send the greeting.
    */
   {
      extern char *help_greeting;

      if (help_greeting == NULL) {
         write_to_buffer(dnew, "Welcome to Redemption MUD\n\r\n\rGreeting temporarily offline.\n\r\n\rBy what name do you wish to be known? \n\r");
      } else if (help_greeting[0] == '.') {
         write_to_buffer(dnew, help_greeting + 1);
      } else {
         write_to_buffer(dnew, help_greeting);
      }
   }

   return;
}



void
close_socket(DESCRIPTOR_DATA * dclose)
{
   char buf[255];
   CHAR_DATA *ch;
   ROOM_INDEX_DATA *location;

   if (dclose->ipid > -1)
   {
      int status;

      kill(dclose->ipid, SIGKILL);
      waitpid(dclose->ipid, &status, 0);
   }
   if (dclose->ifd > -1)
      close(dclose->ifd);

   if (dclose->outtop > 0)
      process_output(dclose, FALSE);

   if (dclose->snoop_by != NULL)
   {
      write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r");
   }
   {
      DESCRIPTOR_DATA *d;

      for (d = descriptor_list; d != NULL; d = d->next)
      {
         if (d->snoop_by == dclose)
            d->snoop_by = NULL;
      }
   }

   if ((ch = dclose->character) != NULL)
   {
      log_string("Closing link to %s.", ch->name);

      /* if ch is writing note */
      if ((dclose->connected == CON_PLAYING)
         || ((dclose->connected >= CON_NOTE_TO)
            && (dclose->connected <= CON_NOTE_FINISH)))
      {
         act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
         sprintf(buf, "Net death has claimed $N.");
         wiznet(buf, ch, NULL, WIZ_LINKS, 0, get_trust(ch));

         ch->desc = NULL;


	 if (in_enemy_hall(ch))
         {
            stop_fighting(ch, TRUE);

            if (ch->in_room->area->continent == 0)
               location = get_room_index(ROOM_VNUM_LIMBO);
            else
               location = get_room_index(ROOM_VNUM_LIMBO_DOMINIA);

            char_from_room(ch);
            char_to_room(ch, location);
            ch->was_in_room = NULL;
            save_char_obj(ch, FALSE);
         }
         else if (ch->no_quit_timer <= 0
                && !IS_IMMORTAL(ch)
                && ch->in_room != get_room_index(1212)
                && IS_SET(ch->toggles, TOGGLES_LINKDEAD))
         {
            ch->was_in_room = ch->in_room;
            act("$n disappears into the void.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
            if (ch->in_room->area->continent == 0)
               location = get_room_index(ROOM_VNUM_LIMBO);
            else
               location = get_room_index(ROOM_VNUM_LIMBO_DOMINIA);
            char_from_room(ch);
            char_to_room(ch, location);
            save_char_obj(ch, FALSE);
         }
      }
      else
      {
         nuke_pets(dclose->original ? dclose->original : dclose->character);
         free_char(dclose->original ? dclose->original :
                 dclose->character);
      }
   }
   if (d_next == dclose)
      d_next = d_next->next;

   if (dclose == descriptor_list)
   {
      descriptor_list = descriptor_list->next;
   }
   else
   {
      DESCRIPTOR_DATA *d;

      for (d = descriptor_list; d && d->next != dclose; d = d->next);
      if (d != NULL)
         d->next = dclose->next;
      else
         bug("Close_socket: dclose not found.", 0);
   }

   close(dclose->descriptor);
   free_string(dclose->ident);
   free_descriptor(dclose);
   return;
}



bool
read_from_descriptor(DESCRIPTOR_DATA * d)
{
   unsigned int iStart;

   /* Hold horses if pending command already. */
   if (d->incomm[0] != '\0')
      return TRUE;

   /* Check for overflow. */
   iStart = strlen(d->inbuf);
   if (iStart >= sizeof(d->inbuf) - 10)
   {
      log_string("%s input overflow!", d->host);
      write_to_descriptor(d->descriptor, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
      return FALSE;
   }

   for (;;)
   {
      int nRead;

      nRead = read(d->descriptor, d->inbuf + iStart,
                sizeof(d->inbuf) - 10 - iStart);
      if (nRead > 0)
      {
         iStart += nRead;
         if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
            break;
      }
      else if (nRead == 0)
      {
         log_string("EOF ( comm )encountered on read.");
         return FALSE;
      }
      else if (errno == EWOULDBLOCK)
         break;
      else
      {
         perror("Read_from_descriptor");
         return FALSE;
      }
   }

   d->inbuf[iStart] = '\0';
   return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void
read_from_buffer(DESCRIPTOR_DATA * d)
{
   int i, j, k;

   /*
    * Hold horses if pending command already.
    */
   if (d->incomm[0] != '\0')
      return;

   /*
    * Look for at least one new line.
    */
   for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
   {
      if (d->inbuf[i] == '\0')
         return;
   }

   /*
    * Canonical input processing.
    */
   for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
   {
      if (k >= MAX_INPUT_LENGTH - 2)
      {
         write_to_descriptor(d->descriptor, "Line too long.\n\r", 0);

         /* skip the rest of the line */
         for (; d->inbuf[i] != '\0'; i++)
         {
            if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
               break;
         }
         d->inbuf[i] = '\n';
         d->inbuf[i + 1] = '\0';
         break;
      }
      if (d->inbuf[i] == '\b' && k > 0)
         --k;
      else if (isascii(d->inbuf[i]) && isprint(d->inbuf[i]))
         d->incomm[k++] = d->inbuf[i];
   }

   /*
    * Finish off the line.
    */
   if (k == 0)
      d->incomm[k++] = ' ';
   d->incomm[k] = '\0';

   /*
    * Deal with bozos with #repeat 1000 ...
    */

   if (k > 1 || d->incomm[0] == '!')
   {
      if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast))
      {
         d->repeat = 0;
      }
      else
      {
         if (++d->repeat >= 25 && d->character
            && d->connected == CON_PLAYING)
         {
            log_string("%s %s@%s input spamming!", (d->character ? d->character->name : "unknown"), d->ident, d->host);
            wiznet("{*Spam {cspam {gspam $N spam spam spam spam spam!", d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
            if (d->incomm[0] == '!')
               wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
            else
               wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));

            d->repeat = 0;
/*
   write_to_descriptor( d->descriptor,
   "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
   strcpy( d->incomm, "quit" );
 */
         }
      }
   }
   /*
    * Do '!' substitution.
    */
   if (d->incomm[0] == '!')
      strcpy(d->incomm, d->inlast);
   else
      strcpy(d->inlast, d->incomm);

   /*
    * Shift the input buffer.
    */
   while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
      i++;
   for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++);
   return;
}



/*
 * Low level output function.
 */
bool
process_output(DESCRIPTOR_DATA * d, bool fPrompt)
{
   extern bool merc_down;

   /*
    * Bust a prompt.
    */
   if (!merc_down)
   {
      if (d->showstr_point)
         write_to_buffer(d, "\n\r[Hit Return to continue]\n\r");
      else if (fPrompt && d->pString && d->connected == CON_PLAYING)
         write_to_buffer(d, "> ");
      else if (fPrompt && (d->connected == CON_PLAYING || d->original != NULL))
      {
         CHAR_DATA *ch;
         CHAR_DATA *victim;

         ch = d->character;

         /* battle prompt */
         if ((victim = ch->fighting) != NULL && can_see(ch, victim))
         {
            int percent;
            char wound[100];
            char *pbuff;
            char buf[MAX_STRING_LENGTH];
            char buffer[MAX_STRING_LENGTH * 2];

            if (victim->max_hit > 0)
               percent = victim->hit * 100 / MAX_HP(victim);
            else
               percent = -1;

            if (IS_AFFECTED(victim, AFF_GRANDEUR))
               sprintf(wound, "is in excellent condition.");
            else if (IS_AFFECTED(victim, AFF_MINIMATION))
               sprintf(wound, "is in awful condition.");
            else
            {
               if (percent >= 100)
                  sprintf(wound, "is in excellent condition.");
               else if (percent >= 90)
                  sprintf(wound, "has a few scratches.");
               else if (percent >= 75)
                  sprintf(wound, "has quite a few wounds and bruises.");
               else if (percent >= 50)
                  sprintf(wound, "has quite a few wounds.");
               else if (percent >= 30)
                  sprintf(wound, "has some big nasty wounds and scratches.");
               else if (percent >= 15)
                  sprintf(wound, "looks pretty hurt.");
               else if (percent >= 0)
                  sprintf(wound, "is in awful condition.");
               else
                  sprintf(wound, "is bleeding to death.");
            }

            sprintf(buf, "%s %s \n\r",
            IS_NPC(victim) ? victim->short_descr : victim->name, wound);
            buf[0] = UPPER(buf[0]);
            pbuff = buffer;
            colourconv(pbuff, buf, d->character);
            write_to_buffer(d, buffer);
         }
         ch = d->original ? d->original : d->character;
         if (!IS_SET(ch->comm, COMM_COMPACT))
            write_to_buffer(d, "\n\r");


         if (IS_SET(ch->comm, COMM_PROMPT))
            bust_a_prompt(d->character);

         if (IS_SET(ch->comm, COMM_TELNET_GA))
            write_to_buffer(d, go_ahead_str);
      }
   }
   /*
    * Short-circuit if nothing to write.
    */
   if (d->outtop == 0)
      return TRUE;

   /*
    * Snoop-o-rama.
    */
   if (d->snoop_by != NULL)
   {
      if (d->character != NULL)
         write_to_buffer(d->snoop_by, d->character->name);
      write_to_buffer(d->snoop_by, "> ");
      write_to_buffer(d->snoop_by, d->outbuf);
   }
   /*
    * OS-dependent output.
    */
   if (!write_to_descriptor(d->descriptor, d->outbuf, d->outtop))
   {
      d->outtop = 0;
      return FALSE;
   }
   else
   {
      d->outtop = 0;
      return TRUE;
   }

}


/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void
bust_a_prompt(CHAR_DATA * ch) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
    char buffer[MAX_STRING_LENGTH * 2];
    char doors[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    const char *dir_name[] = {"N", "E", "S", "W", "U", "D"};
    int door;
    int a = get_age(ch);
    CHAR_DATA *victim;
    char wound[20];
    char wound2[20];
    int percent;
    char lastColour = 'x';
    char closedExitColour = 'c';

    point = buf;
    str = ch->prompt;

    /* Shapeshift added by Delstar */
    if (ch->shift_short != NULL && is_affected(ch, gsn_shapeshift)) {
        Cprintf(ch, "[Shapeshift: %s]\n\r", ch->shift_short);
    }

    if (!str || str[0] == '\0') {
        Cprintf(ch, "{c<%dhp %dm %dmv>{x %s", ch->hit, ch->mana, ch->move, ch->prefix);
        return;
    }

    if (IS_SET(ch->wiznet, WIZ_OLC)) {
        Cprintf(ch, "[OLC] ");
        return;
    }

    if (IS_SET(ch->comm, COMM_AFK)) {
        Cprintf(ch, "<AFK> ");
        return;
    }

    while (*str != '\0') {
        if (*str == '{') {
            if (*(str + 1) != '\0') {
                if (*(str + 1) != '{') {
                    lastColour = *(str + 1);
                }

                *point++ = *str++;
            }

            *point++ = *str++;
            continue;
        }

        if (*str != '%') {
            *point++ = *str++;
            continue;
        }

        ++str;

        switch (*str) {
            default:
                i = " ";
                break;

            case 'e':
                found = FALSE;
                doors[0] = '\0';

                if (lastColour == 'c') {
                    closedExitColour = 'm';
                }

                for (door = 0; door < 6; door++) {
                    if ((pexit = ch->in_room->exit[door]) != NULL
                            && pexit->u1.to_room != NULL
                            && (can_see_room(ch, pexit->u1.to_room)
                                    || (IS_AFFECTED(ch, AFF_INFRARED)
                                            && !IS_AFFECTED(ch, AFF_BLIND)))
                                            && !is_affected(ch, gsn_confusion)) {
                        found = TRUE;
                        if (!IS_SET(pexit->exit_info, EX_CLOSED)) {
                            strcat(doors, dir_name[door]);
                        } else {
                            sprintf(buf3, "{%c%s{%c", closedExitColour, dir_name[door], lastColour);
                            strcat(doors, buf3);
                        }
                    }
                }

                if (!found) {
                    strcat(buf, "none");
                }

                sprintf(buf2, "%s", doors);
                i = buf2;
                break;

            case 'b':
                sprintf(buf2, "%d", ((ch->level < 10) ? 0 : ch->breath) / 10);
                i = buf2;
                break;

            case 'B':
                sprintf(buf2, "%d", ((ch->level < 10) ? 0 : ((a > 100) ? 10 : (a > 80) ? 9 : (a > 60) ? 8 : (a > 50) ? 7 : (a > 40) ? 6 : (a > 30) ? 5 : (a > 25) ? 4 : 3)));
                i = buf2;
                break;

            case 'c':
                sprintf(buf2, "%s", "\n\r");
                i = buf2;
                break;

            case 'd':
                sprintf(buf2, "%d", ch->deity_points);
                i = buf2;
                break;

            case 'g':
                sprintf(buf2, "%ld", ch->gold);
                i = buf2;
                break;

            case 'h':
                sprintf(buf2, "%d", ch->hit);
                i = buf2;
                break;

            case 'H':
                sprintf(buf2, "%d", MAX_HP(ch));
                i = buf2;
                break;

            case 'i':
                sprintf(buf2, "%d", ch->carry_number);
                i = buf2;
                break;

            case 'I':
                sprintf(buf2, "%d", can_carry_n(ch));
                i = buf2;
                break;

            case 'm':
                sprintf(buf2, "%d", ch->mana);
                i = buf2;
                break;

            case 'M':
                sprintf(buf2, "%d", MAX_MANA(ch));
                i = buf2;
                break;

            case 'q':
                sprintf(buf2, "%d", ch->questpoints);
                i = buf2;
                break;

            case 'Q':
                if(!IS_NPC(ch)) {
                    sprintf(buf2, "%d", ch->pcdata->quest.timer);
                } else {
                    sprintf(buf2, "-1");
                }

                i = buf2;
                break;

            case 'v':
                sprintf(buf2, "%d", ch->move);
                i = buf2;
                break;

            case 'V':
                sprintf(buf2, "%d", MAX_MOVE(ch));
                i = buf2;
                break;

            case 'w':
                sprintf(buf2, "%d", (int) (get_carry_weight(ch) / 10));
                i = buf2;
                break;

            case 'W':
                sprintf(buf2, "%d", (int) (can_carry_w(ch) / 10));
                i = buf2;
                break;

            case 'x':
                sprintf(buf2, "%d", ch->exp);
                i = buf2;
                break;

            case 'X':
                sprintf(buf2, "%d", IS_NPC(ch) ? 0 : (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);
                i = buf2;
                break;

            case 's':
                sprintf(buf2, "%ld", ch->silver);
                i = buf2;
                break;

            case 'a':
                if (ch->level > 9) {
                    sprintf(buf2, "%d", ch->alignment);
                } else {
                    sprintf(buf2, "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ? "evil" : "neutral");
                }

                i = buf2;
                break;

            case 'r':
                if (ch->in_room != NULL) {
                    if (room_is_affected(ch->in_room, gsn_winter_storm)) {
                        sprintf(buf2, "A raging winter storm");
                    } else {
                        sprintf(buf2, "%s", ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) || (!IS_AFFECTED(ch, AFF_BLIND) && !room_is_dark(ch->in_room))) ? ch->in_room->name : "darkness");
                    }
                } else {
                    sprintf(buf2, " ");
                }

                i = buf2;
                break;

            case 'R':
                if (IS_IMMORTAL(ch) && ch->in_room != NULL) {
                    sprintf(buf2, "%d", ch->in_room->vnum);
                } else {
                    sprintf(buf2, " ");
                }

                i = buf2;
                break;

            case 'z':
                if (IS_IMMORTAL(ch) && ch->in_room != NULL) {
                    sprintf(buf2, "%s", ch->in_room->area->name);
                } else {
                    sprintf(buf2, " ");
                }

                i = buf2;
                break;

            case '%':
                sprintf(buf2, "%%");
                i = buf2;
                break;

            case 'o':
                //sprintf(buf2, "%s", olc_ed_name(ch));
                i = buf2;
                break;

            case 'O':
                //sprintf(buf2, "%s", olc_ed_vnum(ch));
                i = buf2;
                break;
                /* Enemy power bars coded by Tsongas 01/09/01 */

            case 'l':
                if ((victim = ch->fighting) != NULL) {
                    if (victim->max_hit > 0) {
                        percent = victim->hit * 100 / MAX_HP(victim);
                    } else {
                        percent = -1;
                    }

                    /* Don't want them seeing their REAL condition, do we? */
                    if (!can_see(ch, victim)) {
                        sprintf(wound, "{x         {x");
                    } else if (IS_AFFECTED(victim, AFF_MINIMATION) || IS_AFFECTED(victim, AFF_GRANDEUR)) {
                        if (IS_AFFECTED(victim, AFF_MINIMATION)) {
                            sprintf(wound, "{R+        {x");
                        }

                        if (IS_AFFECTED(victim, AFF_GRANDEUR)) {
                            sprintf(wound, "{R+++{Y+++{G+++{x");
                        }
                    } else {
                        if (percent >= 100) {
                            sprintf(wound, "{R+++{Y+++{G+++{x");
                        } else if (percent >= 90) {
                            sprintf(wound, "{R+++{Y+++{G++ {x");
                        } else if (percent >= 80) {
                            sprintf(wound, "{R+++{Y+++{G+  {x");
                        } else if (percent >= 70) {
                            sprintf(wound, "{R+++{Y+++   {x");
                        } else if (percent >= 58) {
                            sprintf(wound, "{R+++{Y++    {x");
                        } else if (percent >= 45 ) {
                            sprintf(wound, "{R+++{Y+     {x");
                        } else if (percent >= 30) {
                            sprintf(wound, "{R+++      {x");
                        } else if (percent >= 28) {
                            sprintf(wound, "{R++       {x");
                        } else if (percent >= 15) {
                            sprintf(wound, "{R+        {x");
                        } else {
                            sprintf(wound, "         {x");
                        }
                    }

                    sprintf(buf2, "%s", wound);
                    i = buf2;
                } else {
                                i = "{x         {x";
                }
                break;

                /* Self power bar (graphical) coded by Tsongas 01/09/01 */
            case 'L':
                percent = ch->hit * 100 / MAX_HP(ch);

                if (percent >= 100) {
                    sprintf(wound2, "{R+++{Y+++{G+++{x");
                } else if (percent >= 90) {
                    sprintf(wound2, "{R+++{Y+++{G++ {x");
                } else if (percent >= 80) {
                    sprintf(wound2, "{R+++{Y+++{G+  {x");
                } else if (percent >= 70) {
                    sprintf(wound2, "{R+++{Y+++   {x");
                } else if (percent >= 58) {
                    sprintf(wound2, "{R+++{Y++    {x");
                } else if (percent >= 45 ) {
                    sprintf(wound2, "{R+++{Y+     {x");
                } else if (percent >= 30) {
                    sprintf(wound2, "{R+++      {x");
                } else if (percent >= 28) {
                    sprintf(wound2, "{R++       {x");
                } else if (percent >= 15) {
                    sprintf(wound2, "{R+        {x");
                } else {
                    sprintf(wound2, "         {x");
                }

                sprintf(buf2, "%s", wound2);
                i = buf2;
                break;
        }

        ++str;
        while ((*point = *i) != '\0') {
            ++point, ++i;
        }
    }

    *point = '\0';
    pbuff = buffer;
    colourconv(pbuff, buf, ch);
    write_to_buffer(ch->desc, buffer);

    if (ch->prefix[0] != '\0') {
        write_to_buffer(ch->desc, ch->prefix);
    }

    return;
}



/*
 * Append onto an output buffer.
 */
void
write_to_buffer(DESCRIPTOR_DATA * d, const char *txt) {
    int length = strlen(txt);

    /*
     * Initial \n\r if needed.
     */
    if (d->outtop == 0 && !d->fcommand) {
        d->outbuf[0] = '\n';
        d->outbuf[1] = '\r';
        d->outtop = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while (d->outtop + length >= d->outsize) {
        char *outbuf;

        if (d->outsize >= 32000) {
            bug("Buffer overflow. Closing.\n\r", 0);
            close_socket(d);

            return;
        }

        outbuf = (char *) alloc_mem(2 * d->outsize);
        strncpy(outbuf, d->outbuf, d->outtop);
        free_mem(d->outbuf, d->outsize);
        d->outbuf = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strcpy (d->outbuf + d->outtop, txt);
    d->outtop += length;

    return;
}



/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool
write_to_descriptor(int desc, char *txt, int length)
{
   int iStart;
   int nWrite;
   int nBlock;

   if (length <= 0)
      length = strlen(txt);

   for (iStart = 0; iStart < length; iStart += nWrite)
   {
      nBlock = UMIN(length - iStart, 4096);
      if ((nWrite = write(desc, txt + iStart, nBlock)) < 0)
      {
         perror("Write_to_descriptor");
         return FALSE;
      }
   }

   return TRUE;
}



/*
 * Deal with sockets that haven't logged in yet.
 */
void
nanny(DESCRIPTOR_DATA * d, char *argument) {
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    CHAR_DATA *vch;
    char *pwdnew;
    char *p;
    int iClass, race, i, weapon, theroom = ROOM_VNUM_SCHOOL;
    int cont=0;
    int percent=0;
    bool fOld;
    int deity_num;

    if (strlen(argument) > 0) {
        /* kill spaces, unless they are note writing */
        if (d->connected != CON_NOTE_TEXT) {
            while (isspace(*argument)) {
                argument++;
            }
        }
    }

    ch = d->character;

    switch (d->connected) {
        default:
            bug("Nanny: bad d->connected %d.", d->connected);
            close_socket(d);
            return;

        case CON_GET_NAME:
            if (argument[0] == '\0') {
                close_socket(d);
                return;
            }

            argument[0] = UPPER(argument[0]);

            if (!check_parse_name(argument)) {
                
                write_to_buffer(d, "Illegal name, try another.\n\rName: ");
                return;
            }

            fOld = load_char_obj(d, argument);
            ch = d->character;

            if (IS_SET(ch->act, PLR_DENY)) {
                log_string("Denying access to %s@%s.", argument, d->host);
                write_to_buffer(d, "You are denied access.\n\r");
                close_socket(d);
                return;
            }

            if (check_ban(d->host, BAN_PERMIT) && !IS_SET(ch->act, PLR_PERMIT)) {
                write_to_buffer(d, "Your site has been banned from this mud.\n\r");
                close_socket(d);
                return;
            }

            if (check_reconnect(d, argument, FALSE)) {
                fOld = TRUE;
            } else {
                if (wizlock && !IS_IMMORTAL(ch)) {
                    write_to_buffer(d, "The game is wizlocked.\n\r");
                    close_socket(d);
                    return;
                }
            }

            if (fOld) {
                /* Old player */
                write_to_buffer(d, "Password: ");
                write_to_buffer(d, echo_off_str);
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            } else {
                /* New player */
                if (newlock) {
                    write_to_buffer(d, "The game is newlocked.\n\r");
                    close_socket(d);
                    return;
                }

                if (check_ban(d->host, BAN_NEWBIES)) {
                    write_to_buffer(d, "New players are not allowed from your site.\n\r");
                    close_socket(d);
                    return;
                }

                sprintf(buf, "Did I get that right, %s (Y/N)? ", argument);
                write_to_buffer(d, buf);
                d->connected = CON_CONFIRM_NEW_NAME;

                /* Records the site for the pfile - Added by Delstar */
                if (d->host != NULL) {
                    strcpy(buf, d->host);
                    ch->site = str_dup(buf);
                } else {
                    ch->site = str_dup("No Site.");
                }

                return;

            }
            break;

        case CON_GET_OLD_PASSWORD:
            write_to_buffer(d, "\n\r");

            if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
                write_to_buffer(d, "Wrong password.\n\r");

                d->attempt++;

                if (d->attempt >= 3) {
                    sprintf(log_buf, "Denying access to %s@%s (bad password).", ch->name, d->host);

                    log_string("%s", log_buf);
                    wiznet(log_buf, NULL, NULL, WIZ_LOGINS, 0, get_trust(ch));

                    if (d->character->pet) {
                        CHAR_DATA *pet = d->character->pet;

                        char_to_room(pet, get_room_index(ROOM_VNUM_LIMBO));
                        stop_follower(pet);
                        extract_char(pet, TRUE);
                    }

                    write_to_buffer(d, "Goodbye.\n\r");
                    close_socket(d);
                } else {
                    write_to_buffer(d, "\n\rPassword: ");
                }

                return;
            }

            write_to_buffer(d, echo_on_str);

            if (check_playing(d, ch->name)) {
                return;
            }

            if (check_reconnect(d, ch->name, TRUE)) {
                return;
            }

            sprintf(log_buf, "%s@%s has connected.", ch->name, d->host);
            log_string("%s", log_buf);
            wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

            if (IS_SET(ch->toggles, TOGGLES_SOUND)) {
                Cprintf(ch, "!!SOUND(sounds/wav/redempt.wav V=80 P=20 T=admin)");
            }

            if (IS_IMMORTAL(ch)) {
                do_help(ch, "imotd");
                d->connected = CON_READ_IMOTD;
            } else {
                do_help(ch, "motd");
                d->connected = CON_READ_MOTD;
            }
            break;

            /* RT code for breaking link */

        case CON_BREAK_CONNECT:
            switch (*argument) {
                case 'y':
                case 'Y':
                    for (d_old = descriptor_list; d_old != NULL; d_old = d_next) {
                        d_next = d_old->next;

                        if (d_old == d || d_old->character == NULL) {
                            continue;
                        }

                        if (str_cmp(ch->name, d_old->original ? d_old->original->name : d_old->character->name)) {
                            continue;
                        }

                        close_socket(d_old);
                    }

                    if (check_reconnect(d, ch->name, TRUE)) {
                        return;
                    }

                    write_to_buffer(d, "Reconnect attempt failed.\n\rName: ");
                    if (d->character != NULL) {
                        nuke_pets(d->character);
                        free_char(d->character);
                        d->character = NULL;
                    }

                    d->connected = CON_GET_NAME;
                    break;

                case 'n':
                case 'N':
                    write_to_buffer(d, "Name: ");

                    if (d->character != NULL) {
                        nuke_pets(d->character);
                        free_char(d->character);
                        d->character = NULL;
                    }

                    d->connected = CON_GET_NAME;
                    break;

                default:
                    write_to_buffer(d, "Please type Y or N? ");
                    break;
            }
            break;

        case CON_CONFIRM_NEW_NAME:
            switch (*argument) {
                case 'y':
                case 'Y':
                    do_help(d->character, "names");
                    sprintf(buf, "\n\rNew character.\n\rGive me a password for %s: %s", ch->name, echo_off_str);
                    write_to_buffer(d, buf);
                    d->connected = CON_GET_NEW_PASSWORD;
                    break;

                case 'n':
                case 'N':
                    write_to_buffer(d, "Ok, what IS it, then? ");
                    nuke_pets(d->character);
                    free_char(d->character);
                    d->character = NULL;
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    write_to_buffer(d, "Please type Yes or No? ");
                    break;
            }
            break;

        case CON_GET_NEW_PASSWORD:
            write_to_buffer(d, "\n\r");

            if (strlen(argument) < 5) {
                write_to_buffer(d, "Password must be at least five characters long.\n\rPassword: ");
                return;
            }

            pwdnew = crypt(argument, ch->name);

            for (p = pwdnew; *p != '\0'; p++) {
                if (*p == '~') {
                    write_to_buffer(d, "New password not acceptable, try again.\n\rPassword: ");
                    return;
                }
            }

            free_string(ch->pcdata->pwd);
            ch->pcdata->pwd = str_dup(pwdnew);
            write_to_buffer(d, "Please retype password: ");
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
            write_to_buffer(d, "\n\r");

            if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
                write_to_buffer(d, "Passwords don't match.\n\rRetype password: ");
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }

            write_to_buffer(d, echo_on_str);
            write_to_buffer(d, "The following races are available:\n\r");
            write_to_buffer(d, " Human   Elf    Dwarf     Giant   Hatchling\n\r");
            write_to_buffer(d, " Sliver  Troll  Gargoyle  Kirre   Marid\n\r");
            write_to_buffer(d, "\n\r");
            write_to_buffer(d, "What is your race (help for more information)? ");
            d->connected = CON_GET_NEW_RACE;
            break;

        case CON_GET_NEW_RACE:
            one_argument(argument, arg);

            if (!strcmp(arg, "help")) {
                argument = one_argument(argument, arg);

                if (argument[0] == '\0') {
                    do_help(ch, "race help");
                } else {
                    do_help(ch, argument);
                }

                write_to_buffer(d, "What is your race (help for more information)? ");
                break;
            }

            race = race_lookup(argument);

            if ((race == 0 || !race_table[race].pc_race)
                    || race == race_lookup("black dragon")
                    || race == race_lookup("blue dragon")
                    || race == race_lookup("green dragon")
                    || race == race_lookup("red dragon")
                    || race == race_lookup("white dragon"))
            {
                write_to_buffer(d, "That is not a valid race.\n\r");
                write_to_buffer(d, "The following races are available:\n\r  ");
                write_to_buffer(d, " Human   Elf    Dwarf     Giant   Hatchling\n\r");
                write_to_buffer(d, " Sliver  Troll  Gargoyle  Kirre   Marid\n\r");
                write_to_buffer(d, "\n\r");
                write_to_buffer(d, "What is your race? (help for more information) ");
                break;
            }

            ch->race = race;

            // A good place as any
            ch->version = 7;

            /* initialize stats */
            for (i = 0; i < MAX_STATS; i++) {
                ch->perm_stat[i] = pc_race_table[race].stats[i];
            }

            ch->affected_by = ch->affected_by | race_table[race].aff;
            ch->imm_flags = ch->imm_flags | race_table[race].imm;
            ch->res_flags = ch->res_flags | race_table[race].res;
            ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
            ch->form = race_table[race].form;
            ch->parts = race_table[race].parts;

            /* Auto flags set - added by Delstar */

            SET_BIT(ch->act, PLR_MOBASSIST);
            SET_BIT(ch->act, PLR_AUTOEXIT);
            SET_BIT(ch->act, PLR_AUTOGOLD);
            SET_BIT(ch->act, PLR_AUTOLOOT);
            SET_BIT(ch->act, PLR_AUTOSAC);
            SET_BIT(ch->act, PLR_AUTOSPLIT);
            SET_BIT(ch->act, PLR_AUTOTITLE);
            SET_BIT(ch->act, PLR_NOCAN);
            SET_BIT(ch->comm, COMM_NOCGOSS);

            /* add skills */
            for (i = 0; i < 5; i++) {
                if (pc_race_table[race].skills[i] == NULL) {
                    break;
                }

                group_add(ch, pc_race_table[race].skills[i], FALSE);
            }

            /* add cost */
            ch->pcdata->points = pc_race_table[race].points;
            ch->size = pc_race_table[race].size;

            write_to_buffer(d, "\n\rDo you wish to be able to kill or be killed by other players? (Y/N)? ");
            write_to_buffer(d, "\n\r(Choose yes, and you will autoloner at level 10.)");
            write_to_buffer(d, "\n\r(Yes, is necessary to join the pkill and clan system.)");
            d->connected = CON_CAN_CLAN;
            break;

        case CON_CAN_CLAN:
            switch (argument[0]) {
                case 'y':
                case 'Y':
                    SET_BIT(ch->wiznet, CAN_CLAN);
                    break;

                case 'n':
                case 'N':
                    REMOVE_BIT(ch->wiznet, CAN_CLAN);
                    break;

                default:
                    write_to_buffer(d, "\n\rThat is not yes or no\n\rWill you clan? ");
                    return;
            }

            write_to_buffer(d, "\n\rWhat is your sex (M/F)? ");
            d->connected = CON_GET_NEW_SEX;
            break;

        case CON_GET_NEW_SEX:
            switch (argument[0]) {
                case 'm':
                case 'M':
                    ch->sex = SEX_MALE;
                    ch->pcdata->true_sex = SEX_MALE;
                    break;

                case 'f':
                case 'F':
                    ch->sex = SEX_FEMALE;
                    ch->pcdata->true_sex = SEX_FEMALE;
                    break;

                default:
                    write_to_buffer(d, "\n\rThat's not a sex.\n\rWhat IS your sex? ");
                    return;
            }

            strcpy(buf, "Select a class\n\r---------------\n\r");
            for (iClass = 0; iClass < MAX_CLASS; iClass++) {
                if (iClass > 0) {
                    strcat(buf, " \n\r");
                }

                strcat(buf, class_table[iClass].name);
            }

            strcat(buf, "\n\r: ");
            write_to_buffer(d, buf);
            d->connected = CON_GET_NEW_CLASS;
            break;

        case CON_GET_NEW_CLASS:
            one_argument(argument, arg);

            if (!str_cmp(arg, "help")) {
                argument = one_argument(argument, arg);

                if (argument[0] == '\0') {
                    do_help(ch, "class help");
                } else {
                    do_help(ch, argument);
                }

                write_to_buffer(d, "Select a class: ");
                break;
            }

            iClass = class_lookup(argument);

            if (iClass == -1) {
                write_to_buffer(d, "That's not a class.\n\rWhat IS your class? ");
                return;
            }

            ch->charClass = iClass;

            sprintf(log_buf, "%s@%s new player.", ch->name, d->host);
            log_string("%s", log_buf);
            wiznet("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
            wiznet(log_buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

            write_to_buffer(d, "\n\r");
            write_to_buffer(d, "You may be good, neutral, or evil.\n\r");
            write_to_buffer(d, "Which alignment (G/N/E)? ");
            d->connected = CON_GET_ALIGNMENT;
            break;

        case CON_GET_ALIGNMENT:
            switch (argument[0]) {
                case 'g':
                case 'G':
                    ch->alignment = 750;
                    break;

                case 'n':
                case 'N':
                    ch->alignment = 0;
                    break;

                case 'e':
                case 'E':
                    ch->alignment = -750;
                    break;

                default:
                    write_to_buffer(d, "That's not a valid alignment.\n\r");
                    write_to_buffer(d, "Which alignment (G/N/E)? ");
                    return;
            }

            write_to_buffer(d, "\n\r");

            group_add(ch, "rom basics", FALSE);
            group_add(ch, class_table[ch->charClass].base_group, FALSE);
            ch->pcdata->learned[gsn_recall] = 50;
            write_to_buffer(d, "Do you wish to customize this character?\n\r");
            write_to_buffer(d, "Customization takes time, but allows a wider range of skills and abilities.\n\r");
            write_to_buffer(d, "Customize (Y/N)? ");
            d->connected = CON_DEFAULT_CHOICE;
            break;

        case CON_DEFAULT_CHOICE:
            write_to_buffer(d, "\n\r");
            switch (argument[0]) {
                case 'y':
                case 'Y':
                    ch->gen_data = new_gen_data();
                    ch->gen_data->points_chosen = ch->pcdata->points;
                    do_help(ch, "group header");
                    list_group_costs(ch);
                    write_to_buffer(d, "You already have the following skills:\n\r");
                    do_skills(ch, "");
                    do_help(ch, "menu choice");
                    d->connected = CON_GEN_GROUPS;
                    break;

                case 'n':
                case 'N':
                    group_add(ch, class_table[ch->charClass].default_group, TRUE);
                    write_to_buffer(d, "\n\r");
                    write_to_buffer(d, "Please pick a weapon from the following choices:\n\r");
                    buf[0] = '\0';

                    for (i = 0; weapon_table[i].name != NULL; i++) {
                        if (skill_table[*weapon_table[i].gsn].skill_level[ch->charClass] == 1 && ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                            strcat(buf, weapon_table[i].name);
                            strcat(buf, " ");
                        }
                    }

                    strcat(buf, "\n\rYour choice? ");
                    write_to_buffer(d, buf);
                    d->connected = CON_PICK_WEAPON;
                    break;

                default:
                    write_to_buffer(d, "Please answer (Y/N)? ");
                    return;
            }
            break;

        case CON_PICK_WEAPON:
            write_to_buffer(d, "\n\r");
            weapon = weapon_lookup(argument);

            if (weapon == -1 || ch->pcdata->learned[*weapon_table[weapon].gsn] < 1 || skill_table[*weapon_table[weapon].gsn].skill_level[ch->charClass] > 1) {
                write_to_buffer(d, "That's not a valid selection. Choices are:\n\r");
                buf[0] = '\0';
                for (i = 0; weapon_table[i].name != NULL; i++) {
                    if (skill_table[*weapon_table[i].gsn].skill_level[ch->charClass] == 1 && ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                        strcat(buf, weapon_table[i].name);
                        strcat(buf, " ");
                    }
                }

                strcat(buf, "\n\rYour choice? ");
                write_to_buffer(d, buf);
                return;
            }

            ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
            ch->played = 0;
            write_to_buffer(d, "\n\r");
            d->connected = CON_PICK_DEITY;

            write_to_buffer(d, "Which deity do you wish to worship?  You are free to change your choice during game play.\n\r");
            write_to_buffer(d, "Your choices are:\n\r");
            write_to_buffer(d, "     none (to worship no deity)\n\r");
            for (i = 0; deity_table[i].name != NULL; i++) {
                sprintf(buf, "     %s (for the gift '%s')\n\r", deity_table[i].name, deity_table[i].spell);
                write_to_buffer(d, buf);
            }

            write_to_buffer(d, "Deity (help for more information)? ");

            break;

        case CON_PICK_DEITY:
            one_argument(argument, arg);

            if (!strcmp(arg, "help")) {
                argument = one_argument(argument, arg);

                if (argument[0] == '\0') {
                    do_help(ch, "worship favor favour");
                } else {
                    do_help(ch, argument);
                }

                Cprintf(ch, "\n\rDeity (help for more information)? ");
                return;
            }

            if (!str_prefix(arg, "none")) {
                ch->deity_type = 0;
            } else {
                deity_num = deity_lookup(arg);

                if (deity_num == 0) {
                    write_to_buffer(d, "There is no such God for you to worship.\n\r");
                    write_to_buffer(d, "\n\rDeity (help for more information)? ");
                    return;
                }

                ch->deity_type = deity_num;
            }

            ch->deity_points = 0;
            d->connected = CON_READ_MOTD;

            write_to_buffer(d, "\n\r");
            do_help(ch, "motd");
            d->connected = CON_READ_MOTD;
            break;

        case CON_GEN_GROUPS:
            Cprintf(ch, "\n\r");
            if (!str_cmp(argument, "done")) {
                Cprintf(ch, "Creation points: %d\n\r", ch->pcdata->points);
                Cprintf(ch, "Experience per level: %d\n\r", exp_per_level(ch, ch->gen_data->points_chosen));

                if (ch->pcdata->points < 40) {
                    ch->train = (40 - ch->pcdata->points + 1) / 2;
                }

                free_gen_data(ch->gen_data);
                ch->gen_data = NULL;
                
                // Patron change: good place as any for this
                ch->pass_along_limit = exp_per_level(ch, ch->pcdata->points) / 2;
                write_to_buffer(d, "\n\r");
                write_to_buffer(d, "Please pick a weapon from the following choices:\n\r");
                buf[0] = '\0';
                for (i = 0; weapon_table[i].name != NULL; i++) {
                    if (skill_table[*weapon_table[i].gsn].skill_level[ch->charClass] == 1 && ch->pcdata->learned[*weapon_table[i].gsn] > 0) {
                        strcat(buf, weapon_table[i].name);
                        strcat(buf, " ");
                    }
                }

                strcat(buf, "\n\rYour choice? ");
                write_to_buffer(d, buf);
                d->connected = CON_PICK_WEAPON;
                break;
            }

            if (!parse_gen_groups(ch, argument)) {
                Cprintf(ch, "Choices are: list,learned,premise,add,drop,info,help, and done.\n\r");
            }

            do_help(ch, "menu choice");
            break;

        case CON_READ_IMOTD:
            write_to_buffer(d, "\n\r");
            do_help(ch, "motd");
            d->connected = CON_READ_MOTD;
            break;

        case CON_READ_MOTD:
            if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0') {
                write_to_buffer(d, "Warning! Null password!\n\r");
                write_to_buffer(d, "Please report old password with bug.\n\r");
                write_to_buffer(d, "Type 'password null <new password>' to fix.\n\r");
            }

            write_to_buffer(d, "\n\rWelcome to ROM 2.4.  Please do not feed the mobiles.\n\r");
            ch->next = char_list;
            char_list = ch;
            d->connected = CON_PLAYING;
            reset_char(ch);

            if (ch->level == 0) {
                ch->perm_stat[class_table[ch->charClass].attr_prime] += 3;

                ch->level = 1;
                ch->exp = exp_per_level(ch, ch->pcdata->points);
                ch->hit = ch->max_hit;
                ch->mana = ch->max_mana;
                ch->move = ch->max_move;
                ch->train = 3;
                ch->practice = 5;
                sprintf(buf, "the %s", title_table[ch->charClass][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
                set_title(ch, buf);

                do_outfit(ch, "");

                if (ch->race == race_lookup("troll") ||
                        ch->race == race_lookup("sliver") ||
                        ch->race == race_lookup("gargoyle") ||
                        ch->race == race_lookup("kirre")) {
                    char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL_DOMINIA));
                } else if (ch->race == race_lookup("human")) {
                    char_to_room(ch, get_room_index(theroom));
                } else {
                    char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
                }

                Cprintf(ch, "\n\r");
                do_help(ch, "NEWBIE INFO");
                Cprintf(ch, "\n\r");
            } else if (ch->in_room != NULL) {
                // This little dance is necessary so the
                // character doesn't look like he's already
                // in the room when we load him.
                ROOM_INDEX_DATA *start_room = ch->in_room;
                ch->in_room = NULL;
                char_to_room(ch, start_room);
            } else if (ch->was_in_room != NULL) {
                ROOM_INDEX_DATA *start_room = ch->was_in_room;
                ch->was_in_room = NULL;
                char_to_room(ch, start_room);
            } else if (IS_IMMORTAL(ch)) {
                char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
            } else {
                char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
            }

            // Heal up the injured since last logon.
            percent = (current_time - ch->lastlogoff) * 25 / (2 * 60 * 60);

            percent = UMIN(percent, 100);

            if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON) && !IS_AFFECTED(ch, AFF_PLAGUE)) {
                ch->hit += (MAX_HP(ch) - ch->hit) * percent / 100;
                ch->mana += (MAX_MANA(ch) - ch->mana) * percent / 100;
                ch->move += (MAX_MOVE(ch) - ch->move) * percent / 100;
            }

            act("$n has entered the game.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

            if (!IS_IMMORTAL(ch) && in_enemy_hall(ch)) {
                act("$n disappears in a flash of light.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                cont = ch->in_room->area->continent;
                char_from_room(ch);
                char_to_room(ch, get_room_index(cont ? 31004 : 3014));
                act("$n arrives from a puff of smoke.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
                Cprintf(ch, "You have been moved out of an ememy clan hall.\n\r");
            }

            do_look(ch, "auto");

            wiznet("$N has left real life behind.", ch, NULL, WIZ_LOGINS, 0, get_trust(ch));

            if (race_lookup("sliver") == ch->race) {
                for (vch = char_list; vch != NULL; vch = vch->next) {
                    if (vch == ch) {
                        continue;
                    }

                    if (vch == NULL) {
                        break;
                    }

                    if (race_lookup("sliver") == vch->race
                            && vch->remort > 0
                            && can_see(vch, ch)
                            && !IS_IMMORTAL(ch)) {
                        act("{MYou feel a kinship with $N stirring within you.{x", vch, NULL, ch, TO_CHAR, POS_RESTING);
                    }
                }
            }

            if (ch->pet != NULL) {
                char_to_room(ch->pet, ch->in_room);
                act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM, POS_RESTING);
            }

            do_board(ch, "");
            break;

        case CON_NOTE_TO:
            handle_con_note_to(ch, argument);
            break;

        case CON_NOTE_SUBJECT:
            handle_con_note_subject(ch, argument);
            break;

        case CON_NOTE_EXPIRE:
            handle_con_note_expire(ch, argument);
            break;

        case CON_NOTE_TEXT:
            handle_con_note_text(ch, argument);
            break;

        case CON_NOTE_FINISH:
            handle_con_note_finish(ch, argument);
            break;
    }

    return;
}



/*
 * Parse a name for acceptability.
 */
bool
check_parse_name(char *name)
{
   DESCRIPTOR_DATA *d, *dnext;
   int count;

   /*
    * Reserved words.
    */
   if (is_name(name,
            "all auto immortal self someone something the you demise balance circle loner honor"))
      return FALSE;

   if (str_cmp(capitalize(name), "Alander") && (!str_prefix("Alan", name)
                                 || !str_suffix("Alander", name)))
      return FALSE;

   if (!str_prefix("Goon", name))
   {
      return FALSE;
   }
   /*
    * Length restrictions.
    */

   if (strlen(name) < 2)
      return FALSE;


   if (strlen(name) > 12)
      return FALSE;

   /*
    * Alphanumerics only.
    * Lock out IllIll twits.
    */
   {
      char *pc;
      bool fIll, adjcaps = FALSE, cleancaps = FALSE;
      unsigned int total_caps = 0;

      fIll = TRUE;
      for (pc = name; *pc != '\0'; pc++)
      {
         if (!isalpha(*pc))
            return FALSE;

         if (isupper(*pc))
         {               /* ugly anti-caps hack */
            if (adjcaps)
               cleancaps = TRUE;
            total_caps++;
            adjcaps = TRUE;
         }
         else
            adjcaps = FALSE;

         if (LOWER(*pc) != 'i' && LOWER(*pc) != 'l')
            fIll = FALSE;
      }

      if (fIll)
         return FALSE;

      if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
         return FALSE;
   }

   /*
    * Prevent players from naming themselves after mobs.
    */
   {
      extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
      MOB_INDEX_DATA *pMobIndex;
      int iHash;

      for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
      {
         for (pMobIndex = mob_index_hash[iHash];
             pMobIndex != NULL;
             pMobIndex = pMobIndex->next)
         {
            if (is_name(name, pMobIndex->player_name))
               return FALSE;
         }
      }
   }

   /*
    * check names of people playing. Yes, this is necessary for multiple
    * newbies with the same name (thanks Saro)
    */
   if (descriptor_list)
   {
      count = 0;
      for (d = descriptor_list; d != NULL; d = dnext)
      {
         dnext = d->next;
         if (d->connected != CON_PLAYING && d->character && d->character->name
          && d->character->name[0] && !str_cmp(d->character->name, name))
         {
            count++;
            close_socket(d);
         }
      }
      if (count)
      {
         sprintf(log_buf, "Double newbie alert (%s)", name);
         wiznet(log_buf, NULL, NULL, WIZ_LOGINS, 0, 0);

         return FALSE;
      }
   }


   return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool
check_reconnect(DESCRIPTOR_DATA * d, char *name, bool fConn)
{
   char buf[255];
   CHAR_DATA *ch;

   for (ch = char_list; ch != NULL; ch = ch->next)
   {
      if (!IS_NPC(ch)
         && (!fConn || ch->desc == NULL)
         && !str_cmp(d->character->name, ch->name))
      {
         if (fConn == FALSE)
         {
            free_string(d->character->pcdata->pwd);
            d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
         }
         else
         {
            nuke_pets(d->character);
            if (d->character->pet)
            {
               CHAR_DATA *pet = d->character->pet;

               char_to_room(pet, get_room_index(ROOM_VNUM_LIMBO));
               stop_follower(pet);
               extract_char(pet, TRUE);
            }
            free_char(d->character);
            d->character = ch;
            ch->desc = d;
            ch->timer = 0;
            if (ch->in_room == NULL)
            {
               if (ch->was_in_room == NULL)
               {
                  char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
               }
               else
               {
                  char_to_room(ch, ch->was_in_room);
               }
            }

            Cprintf(ch, "Reconnecting. Type replay to see missed tells.\n\r");
            act("$n has reconnected.", ch, NULL, NULL, TO_ROOM, POS_RESTING);

            log_string("%s@%s reconnected.", ch->name, d->host);
            sprintf(buf, "$N groks the fullness of $S link.");
            wiznet(buf, ch, NULL, WIZ_LINKS, 0, get_trust(ch));

            d->connected = CON_PLAYING;

            if (ch->pcdata->in_progress)
               Cprintf(ch, "You have a note in progress. Type 'note write' to continue.\n\r");

            // Remove the note-write status on reconnect
            REMOVE_BIT(ch->comm, COMM_NOTE_WRITE);
         }
         return TRUE;
      }
   }

   return FALSE;
}



/*
 * Check if already playing.
 */
bool
check_playing(DESCRIPTOR_DATA * d, char *name)
{
   DESCRIPTOR_DATA *dold;

   for (dold = descriptor_list; dold; dold = dold->next)
   {
      if (dold != d
         && dold->character != NULL
         && dold->connected != CON_GET_NAME
         && dold->connected != CON_GET_OLD_PASSWORD
         && !str_cmp(name, dold->original
                  ? dold->original->name : dold->character->name))
      {
         write_to_buffer(d, "That character is already playing.\n\r");
         write_to_buffer(d, "Do you wish to connect anyway (Y/N)?");
         d->connected = CON_BREAK_CONNECT;
         return TRUE;
      }
   }

   return FALSE;
}



void
stop_idling(CHAR_DATA * ch)
{
   int clan;
   bool found;
   if (ch == NULL
      || ch->desc == NULL
      || ch->desc->connected != CON_PLAYING
      || ch->was_in_room == NULL)
   {
      return;
   }

   if (ch->in_room != get_room_index(ROOM_VNUM_LIMBO_DOMINIA) &&
      ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
   {
      return;
   }

   found = FALSE;

   for (clan = MIN_PKILL_CLAN; clan <= MAX_PKILL_CLAN; clan++)
   {
	if (ch->was_in_room->clan == clan
	 && ch->clan != clan)
	 	found = TRUE;
   }

   if (!found)
   {
      ch->timer = 0;
      char_from_room(ch);
      char_to_room(ch, ch->was_in_room);
      ch->was_in_room = NULL;
      act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM, POS_RESTING);
   }

   ch->timer = 0;
   return;
}

/*
 * Write to one char.
 */
void
send_to_char_bw(const char *txt, CHAR_DATA * ch)
{
   if (txt != NULL && ch->desc != NULL)
      write_to_buffer(ch->desc, txt);
   return;
}

/*
 * Write to one character.  This should never be called by anybody other than
 * cprintf and Cprintf.
 */
void
cprintf_private(CHAR_DATA *ch, bool inColour, bool showCodes, const char *txt, va_list ap) {
    const char *point;
    char *point2;
    char buf[MAX_STRING_LENGTH * 4];
    int skip = 0;
    char buffer[4 * MAX_STRING_LENGTH];

    sprintf(last_cprintf, "%s: ", ch->name);
    strcat(last_cprintf, txt);

    vsprintf(buffer, txt, ap);

    buf[0] = '\0';
    point2 = buf;
    if (*buffer && ch->desc) {
        if (inColour) {
            for (point = buffer; *point; point++) {
                if (*point == '{' && (*(point + 1) != '\n') && (*(point + 1) != '\r') && (*(point + 1) != '\0')) {
                    point++;
                    skip = colour(*point, ch, point2);
                    while (skip-- > 0) {
                        ++point2;
                    }

                    continue;
                }

                *point2 = *point;
                *++point2 = '\0';
            }

            *point2 = '\0';
            write_to_buffer(ch->desc, buf);
        } else {
            for (point = buffer; *point; point++) {
                if (*point == '{' && (*(point + 1) != '\n') && (*(point + 1) != '\r') && (*(point + 1) != '\0')) {
                    if (!showCodes) {
                        point++;
                        continue;
                    }
                }

                *point2 = *point;
                *++point2 = '\0';
            }

            *point2 = '\0';

            write_to_buffer(ch->desc, buf);
        }
    }

    return;
}

/*
 * Write to one char, specifying whether to use colour, and, if not using
 * colour, whether to use colour codes.  Override's the character's colour
 * setting.
 */
void
cprintf(CHAR_DATA *ch, bool inColour, bool showCodes, const char *txt, ...) {
    va_list ap;
    va_start(ap, txt);

    cprintf_private(ch, inColour, showCodes, txt, ap);
    
    va_end(ap);
}

/*
 * Write to one char.  (improved send_to_char)
 */
void
Cprintf(CHAR_DATA *ch, const char *txt, ...) {
    va_list ap;
    va_start(ap, txt);

    cprintf_private(ch, IS_SET(ch->act, PLR_COLOUR), FALSE, txt, ap);
    
    va_end(ap);
}

/*
 * Send a page to one char.
 */
void
page_to_char_bw(const char *txt, CHAR_DATA * ch)
{
   if (txt == NULL || ch->desc == NULL)
      if (ch->lines == 0)
      {
         Cprintf(ch, "%s", txt);
         return;
      }
   if (ch->desc->showstr_head &&
      (strlen(txt) + strlen(ch->desc->showstr_head) + 1) < 32000)
   {
      char *temp = (char *) alloc_mem(strlen(txt) + strlen(ch->desc->showstr_head) + 1);

      strcpy(temp, ch->desc->showstr_head);
      strcat(temp, txt);
      ch->desc->showstr_point = temp +
         (ch->desc->showstr_point - ch->desc->showstr_head);
      free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
      ch->desc->showstr_head = temp;
   }
   else
   {
      if (ch->desc->showstr_head)
         free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
      ch->desc->showstr_head = (char *) alloc_mem(strlen(txt) + 1);
      strcpy(ch->desc->showstr_head, txt);
      ch->desc->showstr_point = ch->desc->showstr_head;
      show_string(ch->desc, "");
   }
}

/*
 * Page to one char, new colour version, by Lope.
 */
void
page_to_char(const char *txt, CHAR_DATA * ch)
{
   const char *point;
   char *point2;
   char buf[MAX_STRING_LENGTH * 4];
   int skip = 0;

   buf[0] = '\0';
   point2 = buf;
   if (txt && ch->desc)
   {
      if (IS_SET(ch->act, PLR_COLOUR))
      {
         for (point = txt; *point; point++)
         {
            if (*point == '{' && (*(point + 1) != '\n') && (*(point + 1) != '\r') && (*(point + 1) != '\0'))
            {
               point++;
               skip = colour(*point, ch, point2);
               while (skip-- > 0)
                  ++point2;
               continue;
            }
            *point2 = *point;
            *++point2 = '\0';
         }
         *point2 = '\0';
         ch->desc->showstr_head = (char *) alloc_mem(strlen(buf) + 1);
         strcpy(ch->desc->showstr_head, buf);
         ch->desc->showstr_point = ch->desc->showstr_head;
         show_string(ch->desc, "");
      }
      else
      {
         for (point = txt; *point; point++)
         {
            if (*point == '{' && (*(point + 1) != '\n') && (*(point + 1) != '\r') && (*(point + 1) != '\0'))
            {
               point++;
               continue;
            }
            *point2 = *point;
            *++point2 = '\0';
         }
         *point2 = '\0';
         ch->desc->showstr_head = (char *) alloc_mem(strlen(buf) + 1);
         strcpy(ch->desc->showstr_head, buf);
         ch->desc->showstr_point = ch->desc->showstr_head;
         show_string(ch->desc, "");
      }
   }
   return;
}


/* string pager */
void
show_string(struct descriptor_data *d, char *input)
{
   char buffer[4 * MAX_STRING_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   register char *scan, *chk;
   int lines = 0, toggle = 1;
   int show_lines;
   int i;

   one_argument(input, buf);
   if (buf[0] != '\0')
   {
      if (d->showstr_head)
      {
         free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
         d->showstr_head = 0;
      }
      d->showstr_point = 0;
      return;
   }
   if (d->character)
      show_lines = d->character->lines;
   else
      show_lines = 0;

   for (scan = buffer;; scan++, d->showstr_point++)
   {
      if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
         && (toggle = -toggle) < 0)
         lines++;

      else if (!*scan || (show_lines > 0 && lines >= show_lines))
      {
         *scan = '\0';
         write_to_buffer(d, buffer);
         i = 0;
         for (chk = d->showstr_point; isspace(*chk) && i < 3000; i++, chk++);
         {
            if (!*chk)
            {
               if (d->showstr_head)
               {
                  free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
                  d->showstr_head = 0;
               }
               d->showstr_point = 0;
            }
         }
         return;
      }
   }
   return;
}


/* quick sex fixer */
void
fix_sex(CHAR_DATA * ch)
{
   if (ch->sex < 0 || ch->sex > 2)
      ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void
act(const char *format, CHAR_DATA * ch, const void *arg1, const void *arg2, int type, int min_pos) {
    static char *const he_she[] = {"it", "he", "she"};
    static char *const him_her[] = {"it", "him", "her"};
    static char *const his_her[] = {"its", "his", "her"};

    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA *) arg2;
    const char *str;
    char *i = NULL;
    char *point;
    char *pbuff;
    char buffer[MAX_STRING_LENGTH * 2];
    char buf[MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    bool fColour = FALSE;
    int count;

    /*
     * Discard null and zero-length messages.
     */
    if (!format || !*format) {
        return;
    }

    /* discard null rooms and chars */
    if (!ch || !ch->in_room) {
        return;
    }

    to = ch->in_room->people;
    if (type == TO_VICT) {
        if (!vch) {
            bug("Act: null vch with TO_VICT.", 0);
            return;
        }

        if (!vch->in_room) {
            return;
        }

        to = vch->in_room->people;
    }

    count = 0;
    for (; to; to = to->next_in_room) {
        count++;
        if (count > 1000) {
            bug("INFINITE Loop in act.", 0);
            return;
        }

        if ((!IS_NPC(to) && to->desc == NULL)
                || (IS_NPC(to) && (!HAS_TRIGGER(to, TRIG_ACT))
                        && to->desc == NULL)
                        || to->position < min_pos) {
            continue;
        }

        if ((type == TO_CHAR) && to != ch) {
            continue;
        }

        if (type == TO_VICT && (to != vch || to == ch)) {
            continue;
        }

        if (type == TO_ROOM && to == ch) {
            continue;
        }

        if (type == TO_NOTVICT && (to == ch || to == vch)) {
            continue;
        }

        point = buf;
        str = format;
        while (*str != '\0') {
            if (*str != '$') {
                *point++ = *str++;
                continue;
            }

            fColour = TRUE;
            ++str;
            i = " <@@@> ";

            if (!arg2 && *str >= 'A' && *str <= 'Z') {
                bug("Act: missing arg2 for code %d.", *str);
                i = " <@@@> ";
            } else {
                switch (*str) {
                    default:
                        bug("Act: bad code %d.", *str);
                        i = " <@@@> ";
                        break;

                    /* Thx alex for 't' idea */
                    case 't':
                        if (arg1) {
                            i = (char *) arg1;
                        } else {
                            bug("Act: bad code $t for 'arg1'", 0);
                        }

                        break;

                    case 'T':
                        if (arg2) {
                            i = (char *) arg2;
                        } else {
                            bug("Act: bad code $T for 'arg2'", 0);
                        }

                        break;

                    case 'n':
                        if (ch && to) {
                            i = PERS(ch, to);
                        } else {
                            bug("Act: bad code $n for 'ch' or 'to'", 0);
                        }

                        break;

                    case 'N':
                        if (vch && to) {
                            i = PERS(vch, to);
                        } else {
                            bug("Act: bad code $N for 'ch' or 'to'", 0);
                        }

                        break;

                    case 'e':
                        if (ch) {
                            i = he_she[URANGE(0, ch->sex, 2)];
                        } else {
                            bug("Act: bad code $e for 'ch'", 0);
                        }

                        break;

                    case 'E':
                        if (vch) {
                            i = he_she[URANGE(0, vch->sex, 2)];
                        } else {
                            bug("Act: bad code $E for 'vch'", 0);
                        }

                        break;

                    case 'm':
                        if (ch) {
                            i = him_her[URANGE(0, ch->sex, 2)];
                        } else {
                            bug("Act: bad code $m for 'ch'", 0);
                        }

                        break;

                    case 'M':
                        if (vch) {
                            i = him_her[URANGE(0, vch->sex, 2)];
                        } else {
                            bug("Act: bad code $M for 'vch'", 0);
                        }

                        break;

                    case 's':
                        if (ch) {
                            i = his_her[URANGE(0, ch->sex, 2)];
                        } else {
                            bug("Act: bad code $s for 'ch'", 0);
                        }

                        break;

                    case 'S':
                        if (vch) {
                            i = his_her[URANGE(0, vch->sex, 2)];
                        } else {
                            bug("Act: bad code $S for 'vch'", 0);
                        }

                        break;

                    case 'p':
                        if (to && obj1) {
                            i = can_see_obj(to, obj1) ? obj1->short_descr : "something";
                        } else {
                            bug("Act: bad code $p for 'to' or 'obj1'", 0);
                        }

                        break;

                    case 'P':
                        if (to && obj2) {
                            i = can_see_obj(to, obj2) ? obj2->short_descr : "something";
                        } else {
                            bug("Act: bad code $p for 'to' or 'obj2'", 0);
                        }

                        break;


                    case 'd':
                        if (arg2 == NULL || ((char *) arg2)[0] == '\0') {
                            i = "door";
                        } else {
                            one_argument((char *) arg2, fname);
                            i = fname;
                        }

                        break;
                }
            }

            ++str;

            while ((*point = *i) != '\0') {
                ++point;
                ++i;
            }
        }

        *point++ = '\n';
        *point++ = '\r';
        *point = '\0';
        buf[0] = UPPER(buf[0]);
        pbuff = buffer;
        colourconv(pbuff, buf, to);

        if (to->desc && ((to->desc->connected == CON_PLAYING) || to->desc->original != NULL)) {
            write_to_buffer(to->desc, buffer);
        }
    }

    return;
}




int
colour(char type, CHAR_DATA * ch, char *string)
{
   char code[20];
   char *p = '\0';

   if (IS_NPC(ch))
      return (0);

   switch (type)
   {
   default:
      sprintf(code, CLEAR);
      break;
   case 'x':
      sprintf(code, CLEAR);
      break;
   case 'b':
      sprintf(code, C_BLUE);
      break;
   case 'c':
      sprintf(code, C_CYAN);
      break;
   case 'g':
      sprintf(code, C_GREEN);
      break;
   case 'm':
      sprintf(code, C_MAGENTA);
      break;
   case 'r':
      sprintf(code, C_RED);
      break;
   case 'w':
      sprintf(code, C_WHITE);
      break;
   case 'y':
      sprintf(code, C_YELLOW);
      break;
   case 'B':
      sprintf(code, C_B_BLUE);
      break;
   case 'C':
      sprintf(code, C_B_CYAN);
      break;
   case 'G':
      sprintf(code, C_B_GREEN);
      break;
   case 'M':
      sprintf(code, C_B_MAGENTA);
      break;
   case 'R':
      sprintf(code, C_B_RED);
      break;
   case 'W':
      sprintf(code, C_B_WHITE);
      break;
   case 'Y':
      sprintf(code, C_B_YELLOW);
      break;
   case 'D':
      sprintf(code, C_D_GREY);
      break;
   case 'u':
      sprintf(code, C_UNDERLINE);
      break;
   case '*':
      sprintf(code, "%c", 007);
      break;
   case '{':
      sprintf(code, "%c", '{');
      break;
   }

   p = code;
   while (*p != '\0')
   {
      *string = *p++;
      *++string = '\0';
   }

   return (strlen(code));
}

void
colourconv(char *buffer, const char *txt, CHAR_DATA * ch)
{
   const char *point;
   int skip = 0;

   if (ch->desc && txt)
   {
      if (IS_SET(ch->act, PLR_COLOUR))
      {
         for (point = txt; *point; point++)
         {
            if (*point == '{' && (*(point + 1) != '\n') && (*(point + 1) != '\r'))
            {
               point++;
               skip = colour(*point, ch, buffer);
               while (skip-- > 0)
                  ++buffer;
               continue;
            }
            *buffer = *point;
            *++buffer = '\0';
         }
         *buffer = '\0';

      }
      else
      {
         for (point = txt; *point; point++)
         {
            if (*point == '{' && (*(point + 1) != '{') && (*(point + 1) != '\n')
               && (*(point + 1) != '\r'))
            {
               point++;
               continue;
            }
            *buffer = *point;
            *++buffer = '\0';
         }
         *buffer = '\0';
      }
   }
   return;
}

void
message_all(char *mes)
{
   do_echo(System, mes);
}

void
reboot_all()
{
   do_reboot(System, "");
}

void
do_crash_save() {
	FILE *fp;
	DESCRIPTOR_DATA *d, *d_next;
	OBJ_DATA *obj;
	char buf[255];

	/* This is to write to the file. */
	fclose(fpReserve);
	if ((fp = fopen(LAST_COMMAND, "a")) == NULL) {
		bug("Error in do_auto_save opening last_command.txt", 0);
	}

	fprintf(fp, "Last Command: %s\n", last_command);
	fprintf(fp, "Last Cprintf: %s\n", last_cprintf);

	fclose(fp);
	fpReserve = fopen(NULL_FILE, "r");

	/* K, save off the auction info.. CHECK EVYERTHING, bale out early */
	if (auction->item != NULL && auction->seller != NULL) {
		/* return item to seller */
		log_string("Auction: Item returned to seller during crash save.");
		obj_to_char(auction->item, auction->seller);

		if (auction->bet < 0 && auction->buyer != NULL) {
			/* return item to buyer */
			auction->buyer->gold += auction->bet_gold;      /* give him the money */
			auction->buyer->silver += auction->bet_silver;
		}
	}

	// Uhhhh we'll try this for awhile... return lost corpses.
	for(obj = object_list; obj != NULL; obj = obj->next) {
		if (obj->item_type == ITEM_CORPSE_PC && obj->carried_by == NULL && obj->in_room != NULL) {
			for (d = descriptor_list; d != NULL; d = d_next) {
				sprintf(buf, "Corpse returned to %s during crash save.", d->character->name);

				if (!str_cmp(d->character->name, obj->owner)) {
					log_string("Corpse returned to %s during crash save.", d->character->name);
					obj_from_room(obj);
					obj_to_char(obj, d->character);
				}
			}
		}

		if ((calculate_offering_tax_qp(obj) > 0 || calculate_offering_tax_gold(obj) > 0) && obj->respawn_owner != NULL && obj->carried_by == NULL && obj->in_room != NULL) {
			for (d = descriptor_list; d != NULL; d = d_next) {
				sprintf(buf, "Claimed item returned to %s during crash save.", d->character->name);

				if (!str_cmp(d->character->name, obj->respawn_owner)) {
					log_string("Item returned to %s during crash save.", d->character->name);
					obj_from_room(obj);
					obj_to_char(obj, d->character);
				}
			}
		}
	}

	for (d = descriptor_list; d != NULL; d = d_next) {
		if (d->character) {
			save_char_obj(d->character, TRUE);
		}

		d_next = d->next;
		close_socket(d);
	}

	return;
}

void
load_clan_report()
{
   FILE *fptr;
   int i;

        fptr = fopen("clanreport.txt", "rb");

        if(fptr == NULL)
   {
            for(i = 0; i <= MAX_PKILL_CLAN - MIN_PKILL_CLAN; i++)
      {
         clan_report[i].clan_pkills = 0;
         clan_report[i].clan_pkilled = 0;
         strcpy(clan_report[i].best_pkill, "None");
         strcpy(clan_report[i].worst_pkilled, "None");
         clan_report[i].player_pkills = 0;
         clan_report[i].player_pkilled = 0;
      }

                return;
        }

        fread(&clan_report, sizeof(clan_report), 1, fptr);
        fclose(fptr);
        return;
}

void
save_clan_report()
{
        FILE *fptr;

        fptr = fopen("clanreport.txt", "wb");
        fwrite(&clan_report, sizeof(clan_report), 1, fptr);
        fclose(fptr);
}

void
do_clan_report(CHAR_DATA *ch, char* argument)
{
   int i;
   int j;
   if(str_cmp(argument, "clear") == 0)
   {
   	if (ch->level >= 59) {
                for(i = 0; i <= MAX_PKILL_CLAN - MIN_PKILL_CLAN; i++)
                {
                        clan_report[i].clan_pkills = 0;
                        clan_report[i].clan_pkilled = 0;
                        strcpy(clan_report[i].best_pkill, "None");
                        strcpy(clan_report[i].worst_pkilled, "None");
                        clan_report[i].player_pkills = 0;
                        clan_report[i].player_pkilled = 0;
                }

            Cprintf(ch, "Clan pkill records cleared.\n\r");
            save_clan_report();
            return;
        } else {
            Cprintf(ch, "Huh?\n\r");
            return;
        }
    }

   i = clan_lookup(argument);
   j = i - MIN_PKILL_CLAN;

    if (i != 0) {
        Cprintf(ch, "Clan:         %s\n\r", clan_table[i].name);
        Cprintf(ch, "Clan Pkills:  %d\n\r", clan_report[j].clan_pkills);
        Cprintf(ch, "Clan Pkilled: %d\n\r", clan_report[j].clan_pkilled);
        Cprintf(ch, "Most Pkills:  %s(%d)\n\r", clan_report[j].best_pkill, clan_report[j].player_pkills);
        Cprintf(ch, "Most Pkilled: %s(%d)\n\r", clan_report[j].worst_pkilled, clan_report[j].player_pkilled);
    } else {
        Cprintf(ch, "Clan      Clan Pkills   Clan Pkilled    Most Pkills           Most Pkilled      \n\r");
        Cprintf(ch, "--------------------------------------------------------------------------------\n\r");
        
        for (i = MIN_PKILL_CLAN; i <= MAX_PKILL_CLAN; i++) {
/* Include this stuff if we want Loner to be on clanrep.  And make the for-loop start at 1.  and nameIdx replaces
 * the i-2 for clan_names.
            int nameIdx;
            // These contortions required to include Loner, but skip Outcast.
            if (i == 2) {
                continue;
            }

            if (i == 1) {
                nameIdx = 0;
            } else {
                nameIdx = i - 2;
            }
*/
	    j = i - MIN_PKILL_CLAN;
            Cprintf(ch, "%-9s %-13d %-15d %-12s(%4d)    %-12s(%4d)\n\r",
                        clan_table[i].name,
                        clan_report[j].clan_pkills,
                        clan_report[j].clan_pkilled,
                        clan_report[j].best_pkill,
                        clan_report[j].player_pkills,
                        clan_report[j].worst_pkilled,
                        clan_report[j].player_pkilled);
        }
    }

    return;
}

