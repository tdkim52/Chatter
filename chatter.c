/*
  Timothy Kim
  
  chatter version 5
  
  Most commands work except source which has not been implemented yet.
  Read also has some formatting differences due to newline
  characters and a buffer limit.

  Works best when ran with no command line arguments
  For some reason running with various arguments and flags
  causes certain commands not to behave correctly.

  Program blocks when in inserting mode and will not update
  windows until esc is hit to go back to command mode.
 
  Program also blocks when after the : key is pressed as
  it waits for user to press enter to indicate the full 
  command.
  
  Blocking also occurs whenever a listen command is entered
  
*/

// Headers //
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curses.h>
#include <locale.h>

// Constants //
#define NOLEFT "-noleft"
#define NORIGHT "-noright"
#define YESRADDR "-raddr"
#define YESLADDR "-laddr"
#define YESLRPORT "-lrport"
#define YESLLPORT "-llport"
#define DISLR "-dsplr"
#define DISRL "-dsprl"
#define LOOPR "-loopr"
#define LOOPL "-loopl"
#define NOCMD "-nocmd"
#define DPORT 36753
#define BACKLOG 10
#define BUFSIZE 1024

#define WILDCARD "*"
#define STDIN 0
#define maxwin 6

static WINDOW *w[maxwin];
static ww[maxwin];
static wh[maxwin];
static wrpos[maxwin];
static wcpos[maxwin];
static chtype ls,rs,ts,bs,tl,tr,bl,br;

void wAddstr(int i, char s[132]) {
  int j,l,y,x;
  getyx(w[i],y,x);
  y = y?y:!y;
  x = x?x:!x;
  wrpos[i] = y;
  wcpos[i] = x - 1;
  l = strlen(s);
  
  for (j = 0; j < l; j++) {
/*    if (s[j] == '\n') {
      wrpos[i]++;
      wcpos[i] = 1;
      j++;
    }  */
    //if (s[j] == '\n') {
    if (++wcpos[i] == ww[i]) {
      wcpos[i] = 1;
      if (++wrpos[i] == wh[i]) {
	wrpos[i] = 1;
      }
    }
    if (s[j] != '\n') {
      mvwaddch(w[i], wrpos[i], wcpos[i], (chtype) s[j]);
    }
//     else {
//       wcpos[i] = 0;
//       wrpos[i]++;
//       if (wrpos[i] == wh[i]) {
// 	wrpos[i] = 0;
//       }
//     }
  }
  wrefresh(w[i]);
}

void wAddch(int i, char s) {
  int j,l,y,x;
  getyx(w[i],y,x);
  y = y?y:!y;
  x = x?x:!x;
  wrpos[i] = y;
  wcpos[i] = x - 1;
  l = 1;
  for (j = 0; j < l; j++) {
    if (++wcpos[i] == ww[i]) {
      wcpos[i] = 1;
      if (++wrpos[i] == wh[i]) {
	wrpos[i] = 1;
      }
    }
    mvwaddch(w[i], wrpos[i], wcpos[i], (chtype) s);
  }
  wrefresh(w[i]);
}

void strippers (char *line, int xeol) {
  
  int len = strlen(line);
  int i = 0;
  
  if (xeol == 0) {
    for (i = 0; i < len; i++) {
      if (line[i] < 32) { // non-printable
	memmove(&line[i], &line[i+1], strlen(line)-i);
	len = len - 1;
      }
    }
  }
  else if (xeol == 1) {
    for (i = 0; i < len; i++) {
      if (line[i] < 32) {
	if (line[i] != 10 && line[i] != 13) { // LINE FEED && CARRIAGE RETURN
	  memmove(&line[i], &line[i+1], strlen(line)-i);
	  len = len - 1;
	}
      }
    }
  }
}
      
// Prototypes //


// Program main //

int main (int argc, char *argv[])
{
  // variables
  int leftsocket;
  int flag = 1;
  int sin_size;
  int insocket;
  int len;
  int nbytes;
  int sent;
  int first = 1;
  char buf[BUFSIZE+1];
  char buf1[BUFSIZE+1];
  char buf2[BUFSIZE+1];
  char buf3[BUFSIZE+1];
  char buf4[BUFSIZE+1];
  char filebuf[BUFSIZE+1];
  //char *filebuf = NULL;
  
  char *raddr;
  char *laddr;
  char *taddr;
  int rightsocket;
  int thissocket;
  
  struct sockaddr_in rsa;
  struct sockaddr_in lsa;
  struct sockaddr_in tsa;
  
  struct hostent *he;
  struct hostent *she;
  struct in_addr **raddr_list;
  struct in_addr **laddr_list;
  struct in_addr **taddr_list;
  
  int lrport;
  int llport = DPORT;
  int rlport = DPORT;
  int rrport;
  
  // flags for arguemnts //
  int is_tail = 0;
  int is_head = 0;
  int is_raddr = 0;
  int is_laddr = 0;
  int is_llport = 0;
  int is_lrport = 0;
  int is_rlport = 0;
  int is_rrport = 0;
  int is_dsplr = 0;
  int is_dsprl = 0;
  int is_loopr = 0;
  int is_loopl = 0;
  int is_outputl = 0;
  int is_outputr = 1;
  int is_stlrnp = 0;
  int is_strlnp = 0;
  int is_stlrnpxeol = 0;
  int is_strlnpxeol = 0;
  int is_loglrpre = 0;
  int is_logrlpre = 0;
  int is_loglrpost = 0;
  int is_logrlpost = 0;
  int is_externallr = 0;
  int is_externalrl = 0;
  int is_nocmd = 0;
  
  int has_left = 0;
  int has_right = 0;
  int has_middle = 0;
  int no_args = 0;
  
  int has_file = 0;
  FILE *myfile;
  FILE *lrprelog;
  FILE *rlprelog;
  FILE *lrpostlog;
  FILE *rlpostlog;
  
  int i;
  int j, a, b, c, d, nch, cbuf;
  int y, x;
  chtype ch;
  char response[132];
  ch = (chtype) " ";
  ls = (chtype) 0;
  rs = (chtype) 0;
  ts = (chtype) 0;
  bs = (chtype) 0;
  tl = (chtype) 0;
  tr = (chtype) 0;
  bl = (chtype) 0;
  br = (chtype) 0;
  
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  nonl();
  halfdelay(1);
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);

  if (!(LINES == 43) || !(COLS == 132)) {
    clear();
    move(0,0);
    addstr("--->Piggy3 requires a screen size of 132 columns and 43 rows");
    move(1,0);
    addstr("--->Set screen size to 132 by 43 and try again");
    move(2,0);
    addstr("--->Press enter to terminate program");
    refresh();
    getstr(response);
    endwin();
    exit(EXIT_FAILURE);
  }
  
  clear();
  w[0] = newwin(0, 0, 0, 0);
  touchwin(w[0]);
  wmove(w[0], 0, 0);
  wrefresh(w[0]);
  
  a = 18;
  b = 66;
  c = 0;
  d = 0;
  w[1] = subwin(w[0], a, b, c, d);
  w[2] = subwin(w[0], a, b, c, b);
  w[3] = subwin(w[0], a, b, a, c);
  w[4] = subwin(w[0], a, b, a, b);
  w[5] = subwin(w[0], 7, 132, 36, c);
  
  for (i = 1; i < maxwin-1; i++) {
    ww[i] =  b - 1;
    wh[i] = a - 1;
  }
  
  ww[5] = 131;
  wh[5] = 6;
  
  for (i = 1; i < maxwin; i++) {
    wattron(w[i], A_NORMAL);
    wborder(w[i], ls, rs, ts, bs, tl, tr, bl ,br);
    wrefresh(w[i]);
    wattroff(w[i], A_NORMAL);
    wrpos[i] = 1;
    wcpos[i] = 1;
  }
  wAddstr(1, "Data entering from the left:");
  wmove(w[1], 3, 1);
  wAddstr(2, "Data leaving from left to right:");
  wmove(w[2], 3, 1);
  wAddstr(3, "Data leaving from right to left:");
  wmove(w[3], 3, 1);
  wAddstr(4, "Data entering from the right:");
  wmove(w[4], 3, 1);
  
  wmove(w[5], 1, 1);
  //wprintw(w[5], "Hit enter to begin waiting for connnections...");
  //wgetstr(w[5], response);
  wmove(w[5], 1, 1);
  wclrtoeol(w[5]);
  wrefresh(w[5]);


//////////////////////////////////////////////////////////////////////////
  
  char *ip;
  char *ip2;
  int port;
  int port2;
      
  for (i = 0; i < argc; i++)
  {
    // -noleft
    if (strcmp(argv[i], NOLEFT) == 0)
    {
      is_head = 1;
      is_outputr = 1;
      is_outputl = 0;
    }
    // -noright
    else if (strcmp(argv[i], NORIGHT) == 0)
    {
      is_tail = 1;
      is_outputl = 1;
      is_outputr = 0;
    }
    // -raddr
    else if (strcmp(argv[i], YESRADDR) == 0)
    {
      is_raddr = 1;
      if ((i + 1) == argc)
      {
	mvwprintw(w[5], 1, 2, "--->raddr: Right Address much be specified unless -noright flag");
      }
      // converts address to proper ip format
      else if ((he = gethostbyname(argv[i+1])) == NULL)
      {
	mvwprintw(w[5], 1, 2, "--->gethostname");
      }
      else {
	raddr_list = (struct in_addr **)he->h_addr_list;
	raddr = inet_ntoa(*raddr_list[0]);
      }
    }
    // -laddr
    else if (strcmp(argv[i], YESLADDR) == 0)
    {
      is_laddr = 1;
      if ((i + 1) == argc)
      {
	mvwprintw(w[5], 1, 2, "--->laddr: Left Address must be specified if flagged (* for default or omit flag)");
      }
      else {
	if (strcmp(argv[i+1], WILDCARD) == 0) {
	  is_laddr = 0;
	}
	else {
	  if ((he = gethostbyname(argv[i+1])) == NULL)
	  {
	    mvwprintw(w[5], 1, 2, "--->gethostname");
	  }
	  else {
	    laddr_list = (struct in_addr **)he->h_addr_list;
	    laddr = inet_ntoa(*laddr_list[0]);
	  }
	}
      }
    }
    // -lrport
    else if (strcmp(argv[i], YESLRPORT) == 0)
    {
      is_lrport = 1;
      if ((i + 1) == argc)
      {
	mvwprintw(w[5], 1, 2, "--->lrport: Left Remote Port must be specified if flagged (\"*\" for any or omit flag");
      }
      else {
	if (strcmp(argv[i+1], WILDCARD) == 0) {
	  is_lrport = 0;
	}
	else
	{
	  lrport = atoi(argv[i+1]);
	  if (lrport < 0 || lrport > 65535)
	  {
	    mvwprintw(w[5], 1, 2, "--->lrport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
	  }
	}
      }
    }
    // -llport
    else if (strcmp(argv[i], YESLLPORT) == 0)
    {
      is_llport = 1;
      if ((i + 1) == argc)
      {
	mvwprintw(w[5], 1, 2, "--->llport: Left Local Port must be specified if flagged (\"*\" for default or omit flag");
      }
      else {
	llport = atoi(argv[i+1]);
	if (llport < 0 || llport > 65535)
	{
	  mvwprintw(w[5], 1, 2, "--->llport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
	}
      }
    }
    // -dsplr
    else if (strcmp(argv[i], DISLR) == 0)
    {
      is_dsplr = 1;
    }
    // -dsprl
    else if (strcmp(argv[i], DISRL) ==0)
    {
      is_dsprl = 1;
    }
    // -loopr
    else if (strcmp(argv[i], LOOPR) == 0)
    {
      is_loopr = 1;
    }
    // -loopl
    else if (strcmp(argv[i], LOOPL) == 0)
    {
      is_loopl = 1;
    }
    else if (strcmp(argv[i], NOCMD) == 0) {
      is_nocmd = 1;
    }
  }
  
  // Flag error handling //
  if (is_tail == 1 && is_head == 1)
  {
    mvwprintw(w[5], 1, 2, "--->ERROR: Cannot be both head AND tail of chain");
  }
  if (is_tail == 1 && is_raddr == 1)
  {
    mvwprintw(w[5], 1, 2, "--->ERROR: Cannot have right address as tail");
  }
  if (is_tail == 0 && is_head == 0 && is_raddr == 0)
  {
    mvwprintw(w[5], 1, 2, "--->ERROR: Requires at least one connection side");
    no_args = 1;
  }
  // checks if both flags
  if (is_dsplr == 1 && is_dsprl == 1) {
    mvwprintw(w[5], 1, 2, "--->ERROR: Cannot have both -dsp flags, reverting to default (-dsplr)");
    is_dsprl = 0;
  }
  // sets default if no flag
  if (is_dsplr == 0 && is_dsprl == 0) {
    is_dsplr = 1;
  }
  // if -noright/-noleft is given displays the only availible stream
  if (is_tail == 1) {
    is_dsplr = 1;
    is_dsprl = 0;
  }
  else if (is_head == 1) {
    is_dsplr = 0;
    is_dsprl = 1;
  } 
   wrefresh(w[5]);
   
//////////////////////////////////////////////////////////////////////////////////////  
  
  // Implements connections //
  
  // Handles right side //
  if (is_tail == 0 && is_raddr == 1)
  { 
    // Makes the rightside connection 
    if (is_raddr == 0)
    {
      mvwprintw(w[5], 1, 2, "---> ERROR: Right address must be specified unless -noright flag");
    }
    else
    {
      rightsocket = socket(PF_INET, SOCK_STREAM, 0);
      memset(&rsa, 0, sizeof(rsa)); // resets the structure
      rsa.sin_family = AF_INET;
      rsa.sin_port = htons(DPORT);
      rsa.sin_addr.s_addr = inet_addr(raddr);

      if (connect(rightsocket, (struct sockaddr *)&rsa, sizeof(struct sockaddr)) < 0)
      {
	mvwprintw(w[5], 1, 2, "---> connect");
      }
      else {
	mvwprintw(w[5], 1, 2, "right connection established");
	wrefresh(w[5]);
	has_right = 1;
      }
    }
  }
  
    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    
    if ((she = gethostbyname(hostname)) == NULL) {
      //
    }
    
    taddr = inet_ntoa(*(struct in_addr*)*she->h_addr_list);
    
    thissocket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&tsa, 0, sizeof(tsa));
    tsa.sin_family = AF_INET;
    
    if (is_llport == 1)
    {
      tsa.sin_port = htons(llport);
      port2 = llport;
    }
    else
    {
      tsa.sin_port = htons(DPORT);
      port2 = DPORT;
    }
    
    tsa.sin_addr.s_addr = htonl(INADDR_ANY);
    ip2 = inet_ntoa(tsa.sin_addr);  
  
    
  // Handles left side //
  if (is_head == 0 && no_args == 0)
  {
    // prevents "address already in use" error message
    if (setsockopt(thissocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1)
    {
      mvwprintw(w[5], 1, 2, "---> setsockopt");
    }
    if (bind(thissocket, (struct sockaddr *)&tsa, sizeof(struct sockaddr)) < 0)
    {
      mvwprintw(w[5], 1, 2, "---> bind");
    }
    else {
      has_middle == 1;
    }
    if (listen(thissocket, BACKLOG) < 0)
    {
      mvwprintw(w[5], 1, 2, "---> listen : return value -1; ");
    }
    sin_size = sizeof(struct sockaddr_in);
    if ((leftsocket = accept(thissocket, (struct sockaddr *)&lsa, &sin_size)) < 0)
    {
      mvwprintw(w[5], 1, 2, "---> accept: return value -1");
    }
    else {
      mvwprintw(w[5], 1, 2, "left connection established ");
      wrefresh(w[5]);
      has_left = 1;
    }
    
    if (is_laddr == 1 && lsa.sin_addr.s_addr != inet_addr(laddr))
    {
      mvwprintw(w[5], 1, 2, "--->incoming address doesnt not match laddr");
      close(leftsocket);
    }
    if (is_lrport == 1 && lsa.sin_port != htons(lrport))
    {
      mvwprintw(w[5], 1, 2, "--->the remote source port does not match lrport");
      close(leftsocket);
    }
  }  
 
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

  fd_set rfds;
  int fdmax;
  char *pos;
  int is_insert = 0;
  int is_first = 1;
  
  int acceptl = 0;
  int acceptr = 0;
  
  memset(buf, '\0', sizeof(char)*BUFSIZE);
  memset(buf2, '\0', sizeof(char)*BUFSIZE);
  memset(buf3, '\0', sizeof(char)*BUFSIZE);
  memset(buf4, '\0', sizeof(char)*BUFSIZE);    
  
  i = 0;
  nch = 0x20;
  struct timeval tv;
  tv.tv_sec = 1;
  
  while (!(nch == 'q') && is_nocmd == 0) {
    halfdelay(1);
    noecho();
    wmove(w[5], 1, 2);
    wclrtoeol(w[5]);
    wprintw(w[5], "COMMAND MODE");
    wmove(w[5], 3 , 2);
    
    memset(buf, '\0', sizeof(char)*BUFSIZE);    
    memset(buf3, '\0', sizeof(char)*BUFSIZE);
    memset(buf4, '\0', sizeof(char)*BUFSIZE);    
    
    FD_ZERO(&rfds);
    
    if (has_middle == 1) {
      FD_SET(thissocket, &rfds);
      if (thissocket > fdmax) {
	fdmax = thissocket;
      }
      //fdmax = thissocket;
    }
    if (has_right == 1) {
      FD_SET(rightsocket, &rfds);
      if (rightsocket > fdmax) {
	fdmax = rightsocket;
      }
      //fdmax = rightsocket;
    }
    if (has_left == 1) {
      FD_SET(leftsocket, &rfds);
      if (leftsocket > fdmax) {
	fdmax = leftsocket;
      }
      //fdmax = leftsocket;
    }


    if (select(fdmax+1, &rfds, NULL, NULL, &tv) == -1) {
      mvwprintw(w[5], 1, 2, "---> select: a connection was closed");
      wmove(w[5], 3, 2);
      wrefresh(w[5]);
    }    
    
    wrefresh(w[5]);
    
    if ((nch = wgetch(w[5])) == ERR) {
      // when no input is availible
      i++;
      wrefresh(w[5]);
      
      if (has_left == 1) {
	if (FD_ISSET(leftsocket, &rfds)) {
	  if((nbytes = recv(leftsocket, buf, sizeof(buf), 0)) < 0) {
	    mvwprintw(w[5], 1, 2, "---> recv: ");
	  }
	  else {
	    if (nbytes == 0) {
	      close(leftsocket);
	      FD_CLR(leftsocket, &rfds);
	    }
	    if (is_loopr == 1) {
	      strncpy(buf2, buf, strlen(buf));
	    }
	    if (is_loopl == 1) {
	      if ((pos = strchr(buf, '\n')) != NULL) {
		*pos = '\0';
	      }
	      strncat(buf, buf2, strlen(buf2));
	    }
	    if (is_loglrpre == 1) {
	      fputs(buf, lrprelog);
	    }
	    wAddstr(1, buf);
	    wrefresh(w[1]);
	  }
	  if (has_right == 1 && is_loopr == 0 && is_outputr == 1) {
	    if (is_stlrnp == 1) {
	      strippers(buf, 0);
	    }
	    else if (is_stlrnpxeol == 1) {
	      strippers(buf, 1);
	    }
	    if ((sent = write(rightsocket, buf, sizeof(buf))) < 0) {
	      mvwprintw(w[5], 1, 2, "--->write: rightsocket");
	    }
	    else {
	      if (is_loglrpost == 1 && (is_stlrnp == 1 || is_stlrnpxeol == 1)) {
		fputs(buf, lrpostlog);
	      }
	      wAddstr(2, buf);
	      wrefresh(w[2]);
	      memset(buf2, '\0', sizeof(char)*BUFSIZE);
	    }
	  }
	}
      }
      
      if (has_right == 1) {
	if (FD_ISSET(rightsocket, &rfds)) {
	  if ((nbytes = recv(rightsocket, buf, sizeof(buf), 0)) < 0) {
	    mvwprintw(w[5], 1, 2, "---> recv: ");
	  }
	  else {
	    if (nbytes == 0) {
	      close(rightsocket);
	      FD_CLR(rightsocket, &rfds);
	    }
	    if (is_loopr == 1) {
	      if ((pos = strchr(buf, '\n')) != NULL) {
		*pos = '\0';
	      }
	      strncat(buf, buf2, strlen(buf2));
	    }
	    if (is_loopl == 1) {
	      strncpy(buf2, buf, strlen(buf));
	    }
	    if (is_logrlpre == 1) {
	      fputs(buf, rlprelog);
	    }
	    wAddstr(4, buf);
	    wrefresh(w[4]);
	  }
	  if (has_left == 1 && is_loopl == 0 && is_outputl == 1) {
	    if (is_strlnp == 1) {
	      strippers(buf, 0);
	    }
	    else if (is_strlnpxeol == 1) {
	      strippers(buf, 1);
	    }
	    if ((sent = write(leftsocket, buf, sizeof(buf))) < 0) {
	      mvwprintw(w[5], 1, 2, "---> write: leftsocket1");
	    }
	    else {
	      if (is_logrlpost == 1 && (is_strlnp == 1 || is_strlnpxeol == 1)) {
		fputs(buf, rlpostlog);
	      }
	      wAddstr(3, buf);
	      wrefresh(w[3]);
	      memset(buf2, '\0', sizeof(char)*BUFSIZE);
	    }
	  }
	}
      }
      
      if (has_middle == 1) {
	if (FD_ISSET(thissocket, &rfds)) {
	  sin_size = sizeof(struct sockaddr_in);
	  if (acceptl == 1) {
	    if ((leftsocket = accept(thissocket, (struct sockaddr *)&lsa, &sin_size)) < 0) {
	      mvwprintw(w[5], 2, 2, "---> accept: return value -1");
	    }
	    else {
	      mvwprintw(w[5], 2, 2, "left connection established ");
// 	      FD_SET(leftsocket, &rfds);
// 	      if (leftsocket > fdmax) {
// 		fdmax = leftsocket;
// 	      }
	      has_left = 1;
	      has_middle = 0;
	      acceptl = 0;
	      acceptr = 0;
	      close(thissocket);
	      FD_CLR(thissocket, &rfds);
	      if (has_right == 0) {
		is_outputl = 1;
		is_outputr = 0;
	      }
	    }
	    if (is_laddr == 1 && lsa.sin_addr.s_addr != inet_addr(laddr)) {
	      mvwprintw(w[5], 1, 2, "--->incoming address does not match laddr");
	      close(leftsocket);
	    }
	    if (is_lrport == 1 && lsa.sin_port != htons(lrport)) {
	      mvwprintw(w[5], 1, 2, "--->the remote source port does not match lrport");
	      close(leftsocket);
	    }
	  }
	  else if (acceptr == 1) {
	    if ((rightsocket = accept(thissocket, (struct sockaddr *)&rsa, &sin_size)) < 0) {
	      mvwprintw(w[5], 2, 2, "---> accept: return value -1");
	    }
	    else {
	      mvwprintw(w[5], 2, 2, "right connection established ");
// 	      FD_SET(rightsocket, &rfds);
// 	      if (rightsocket > fdmax) {
// 		fdmax = rightsocket;
// 	      }
	      has_right = 1;
	      has_middle = 0;
	      acceptr = 0;
	      acceptl = 0;
	      close(thissocket);
	      FD_CLR(thissocket, &rfds);
	      if (has_left == 0) {
		is_outputl = 0;
		is_outputr = 1;
	      }
	    }
	    if (is_raddr == 1 && rsa.sin_addr.s_addr != inet_addr(raddr)) {
	      mvwprintw(w[5], 1, 2, "--->incoming address does not match raddr");
	      close(rightsocket);
	    }
	    if (is_rrport == 1 && rsa.sin_port != htons(rrport)) {
	      mvwprintw(w[5], 1, 2, "--->the remote source port does not match rrport");
	      close(rightsocket);
	    }
	  }
	}
      }      
      
      // FIND OUT IF DATA SENT FROM THE FILTERING PIGGY IS FILTERED BEFORE SENDING
      // I.E. IF INPUTED CHARACTERS ARE FILTERED BEFORE SENDING OR ONLY FILTER IF
      // THE DATA IS COMING FROM THE LEFT GOING TO RIGHT VISA VERSA
      // CURRENT VERSION ONLY FILTERS IF DATA ARRIVES FROM A DIFFERENT PIGGY
      
      if (has_file == 1) {
	has_file = 0;
	if (is_loopr == 1) {
	  strncat(filebuf, buf2, strlen(buf2));
	}
	if (is_loopl == 1) {
	  strncpy(buf2, filebuf, strlen(filebuf));
	}
	if (has_left == 1 && is_loopl == 0 && is_outputl == 1) {
	  if ((sent = write(leftsocket, filebuf, sizeof(filebuf))) < 0) {
	    mvwprintw(w[5], 1, 2, "---> write: myfile");
	  }
	  else {
	    wAddstr(3, filebuf);
	    wrefresh(w[3]);
	    memset(buf2, '\0', sizeof(char)*BUFSIZE);
	    memset(filebuf, '\0', sizeof(char)*BUFSIZE);
	  }
	}
	else if (has_right == 1 && is_loopr == 0 && is_outputr == 1) {
	  if ((sent = write(rightsocket, filebuf, sizeof(filebuf))) < 0) {
	    mvwprintw(w[5], 1, 2, "---> write: myfile");
	  }
	  else {
	    wAddstr(2, filebuf);
	    wrefresh(w[2]);
	    memset(buf2, '\0', sizeof(char)*BUFSIZE);
	    memset(filebuf, '\0', sizeof(char)*BUFSIZE);
	  }
	}
	//free(filebuf);
      }
	  
    }
    else {
      wmove(w[5], 3, 2);
      if (nch == 'i') { // i entering insert mode
	wmove(w[5], 1, 2);
	wclrtoeol(w[5]);
	wprintw(w[5], "INSERT MODE");
	wmove(w[5], 3, 2);
	wclrtoeol(w[5]);
	while (!(nch == 27)) { // esc	  
	  mvwprintw(w[5], 3, 2, "> ");
	  if ((nch = wgetch(w[5])) == ERR) {
	    wrefresh(w[5]);	    
	  }
	  else {
	    if (nch == 27) {
	      //
	    }
	    else {
	      if (is_loopr == 1 || is_loopl == 1) {
		//strncpy(buf2, buf, len); 
	      }
	      if (has_right == 1 && is_loopr == 0 && is_outputr == 1) {	
		if ((sent = write(rightsocket, &nch, 1)) <= 0) {
		  mvwprintw(w[5], 1, 2, "---> write: no right connection");
		}
		else {
		  wAddch(2, nch);
		  wrefresh(w[2]);
		}
	      }
	      if (has_left == 1 && is_loopl == 0 && is_outputl == 1) {
		if ((sent = write(leftsocket, &nch, 1)) <= 0) {
		  mvwprintw(w[5], 1, 2, "---> write: no left connection");
		}
		else {
		  wAddch(3, nch);
		  wrefresh(w[3]);
		}
	      }
	      wmove(w[5], 2, 2);
	      wclrtoeol(w[5]);
	      wmove(w[5], 3, 4);
	      wclrtoeol(w[5]);
	      mvwprintw(w[5], 3, 4, "%c", nch);
	      wrefresh(w[5]);
	    }
	  }
	  wmove(w[5], 3, 2);
	  wclrtoeol(w[5]);
	}
      }
      else if (nch == ':') { // :
	echo();
	wmove(w[5], 3, 2);
	wclrtoeol(w[5]);
	mvwprintw(w[5], 3, 2, ":");
	if ((nch = wgetstr(w[5], response)) == ERR) {
	  mvwaddstr(w[5], 2, 2, "NOTHING");
	  wrefresh(w[5]);
	}
	else {
	  len = strlen(response);
	  strncpy(buf3, response, len);
	  if (strcmp(buf3, "outputl") == 0) {
	    is_outputl = 1;
	    is_outputr = 0;
	  }
	  else if (strcmp(buf3, "outputr") == 0) {
	    is_outputr = 1;
	    is_outputl = 0;
	  }
	  else if (strcmp(buf3, "output") == 0) {
	    if (is_outputr == 1 && is_outputl == 0) {
	      wmove(w[5], 2, 2);
	      wclrtoeol(w[5]);
	      mvwprintw(w[5], 2, 2, "output: left --> right");
	    }
	    else {
	      wmove(w[5], 2, 2);
	      wclrtoeol(w[5]);
	      mvwprintw(w[5], 2, 2, "output: left <-- right");
	    }
	  }
	  else if (strcmp(buf3, "dropr") == 0) {
	    if (has_right == 1) {
	      has_right = 0;
	      close(rightsocket);
	      FD_CLR(rightsocket, &rfds);
	      memset(&rsa, 0, sizeof(rsa));
	    }
	  }
	  else if (strcmp(buf3, "dropl") == 0) {
	    if (has_left == 1) {
	      has_left = 0;
	      close(leftsocket);
	      FD_CLR(leftsocket, &rfds);
	      memset(&lsa, 0, sizeof(lsa));
	    }
	  }
	  else if (strcmp(buf3, "rpair") == 0) {
	    ip = inet_ntoa(rsa.sin_addr);
	    port = rsa.sin_port;
	    wmove(w[5], 2, 2);
	    wclrtoeol(w[5]);
	    mvwprintw(w[5], 2, 2, "%s:%d:%s:%d", hostname, port2, ip, port);
	  }
	  else if (strcmp(buf3, "lpair") == 0) {
	    ip = inet_ntoa(lsa.sin_addr);
	    port = lsa.sin_port;
	    wmove(w[5], 2, 2);
	    wclrtoeol(w[5]);
	    mvwprintw(w[5], 2, 2, "%s:%d:%s:%d", ip, port, hostname, port2);
	  }
	  else if (strcmp(buf3, "loopr") == 0) {
	    is_loopr = 1;
	    is_loopl = 0;
	  }
	  else if (strcmp(buf3, "loopl") == 0) {
	    is_loopl = 1;
	    is_loopr = 0;
	  }
	  else if (strcmp(buf3, "stlrnp") == 0) {
	    is_stlrnp = 1;
	    is_stlrnpxeol = 0;
	  }
	  else if (strcmp(buf3, "strlnp") == 0) {
	    is_strlnp = 1;
	    is_strlnpxeol = 0;
	  }
	  else if (strcmp(buf3, "stlrnpxeol") == 0) {
	    is_stlrnp = 0;
	    is_stlrnpxeol = 1;
	  }
	  else if (strcmp(buf3, "strlnpxeol") == 0) {
	    is_strlnp = 0;
	    is_strlnpxeol = 1;
	  }	  
	  else if (strcmp(buf3, "q") == 0) {
	    endwin();
	    exit(0);
	  }
	  else if (strcmp(buf3, "listenl") == 0) {
	    wmove(w[5], 2, 2);
	    wclrtoeol(w[5]);
	    thissocket = socket(PF_INET, SOCK_STREAM, 0);
	    memset(&tsa, 0, sizeof(tsa));
	    tsa.sin_family = AF_INET;
	    tsa.sin_port = htons(DPORT);
	    port2 = DPORT;
	    tsa.sin_addr.s_addr = htonl(INADDR_ANY);
	    
	    if (setsockopt(thissocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
	      mvwprintw(w[5], 2, 2, "---> setsockopt");
	    }
	    if (bind(thissocket, (struct sockaddr *)&tsa, sizeof(struct sockaddr)) < 0) {
	      mvwprintw(w[5], 2, 2, "---> bind");
	    }
	    else {
	      has_middle = 1;
	      acceptl = 1;
	      acceptr = 0;
	    }
	    if (listen(thissocket, BACKLOG) < 0) {
	      mvwprintw(w[5], 2, 2, "---> listen : return value -1; ");
	    }
	  }
	  else if (strcmp(buf3, "listenr") == 0) {
	    wmove(w[5], 2, 2);
	    wclrtoeol(w[5]);
	    //mvwprintw(w[5], 2, 2, "listen right on default port: %d", DPORT);
	    
	    thissocket = socket(PF_INET, SOCK_STREAM, 0);
	    memset(&tsa, 0, sizeof(tsa));
	    tsa.sin_family = AF_INET;
	    tsa.sin_port = htons(DPORT);
	    port2 = DPORT;
	    tsa.sin_addr.s_addr = htonl(INADDR_ANY);
	    
	    if (setsockopt(thissocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
	      mvwprintw(w[5], 2, 2, "---> setsockopt");
	    }
	    if (bind(thissocket, (struct sockaddr *)&tsa, sizeof(struct sockaddr)) < 0) {
	      perror("bind");
	      mvwprintw(w[5], 2, 2, "---> bind");
	    }
	    else {
	      has_middle = 1;
	      acceptr = 1;
	      acceptl = 0;
	    }
	    if (listen(thissocket, BACKLOG) < 0) {
	      mvwprintw(w[5], 2, 2, "---> listen : return value -1; ");
	    }
	  }
	  else {
	    int k = 0;
	    while (buf3[k] != ' ' && buf3[k] != '\0' && k < len) {
	      k++;
	    }
	    if (buf3[k] == ' ') {
	      buf3[k] = '\0';
	      int clength = strlen(buf);
	      if (strcmp(buf3, "connectr") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "connect right to IP: %s", &buf3[k+1]);
		int p = 0;
		while (buf3[k+1+p] != ' ' && buf3[k+1+p] != '\0' && k+1+p < len) {
		  p++;
		}
		if (buf3[k+1+p] == ' ' || buf3[k+1+p] == '\0') {
		  buf3[k+1+p] = '\0';
		  if ((he = gethostbyname(&buf3[k+1])) == NULL) {
		    mvwprintw(w[5], 2, 2, "gethostname: %s not valid IP", &buf3[k+1]);
		  }
		  else {
		    raddr_list = (struct in_addr **)he->h_addr_list;
		    raddr = inet_ntoa(*raddr_list[0]);
		    is_raddr = 1;
		  }
		  buf3[k+1+p] = ' ';
		  int tport = DPORT;
		  if (k+2+p < len) {
		    tport = atoi(&buf3[k+2+p]);
		  }  
		  rightsocket = socket(PF_INET, SOCK_STREAM, 0);
		  memset(&rsa, 0, sizeof(rsa)); // resets the structure
		  rsa.sin_family = AF_INET;
		  rsa.sin_port = htons(tport);
		  rsa.sin_addr.s_addr = inet_addr(raddr);
		  if (connect(rightsocket, (struct sockaddr *)&rsa, sizeof(struct sockaddr)) < 0) {
		    mvwprintw(w[5], 2, 2, "connect: could not connect to %s", &buf3[k+1]);
		  }
		  else {
		    mvwprintw(w[5], 2, 2, "right connection established");
		    has_right = 1;
		    if (has_left == 0) {
		      is_outputr = 1;
		      is_outputl = 0;
		    }
		  }
		}
	      }
	      else if (strcmp(buf3, "connectl") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		int p = 0;
		while (buf3[k+1+p] != ' ' && buf3[k+1+p] != '\0' && k+1+p < len) {
		  p++;
		}
		if (buf3[k+1+p] == ' ' || buf3[k+1+p] == '\0') {
		  buf3[k+1+p] = '\0';
		  if ((he = gethostbyname(&buf3[k+1])) == NULL) {
		    mvwprintw(w[5], 2, 2, "gethostname: %s not valid IP", &buf3[k+1]);
		  }
		  else {
		    laddr_list = (struct in_addr **)he->h_addr_list;
		    laddr = inet_ntoa(*laddr_list[0]);
		  }
		  buf3[k+1+p] = ' ';
		  int tport = DPORT;
		  if (k+2+p < len) {
		    tport = atoi(&buf3[k+2+p]);
		  }  
		  leftsocket = socket(PF_INET, SOCK_STREAM, 0);
		  memset(&lsa, 0, sizeof(lsa)); // resets the structure
		  lsa.sin_family = AF_INET;
		  lsa.sin_port = htons(tport);
		  lsa.sin_addr.s_addr = inet_addr(laddr);
		  if (connect(leftsocket, (struct sockaddr *)&lsa, sizeof(struct sockaddr)) < 0) {
		    mvwprintw(w[5], 2, 2, "connect: could not connect to %s", &buf3[k+1]);
		  }
		  else {
		    mvwprintw(w[5], 2, 2, "left connection established");
		    has_left = 1;
		    if (has_right == 0) {
		      is_outputl = 1;
		      is_outputr = 0;
		    }
		  }
		}
	      }
	      else if (strcmp(buf3, "listenl") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "listen left on port: %s", &buf3[k+1]);
		int tport = atoi(&buf3[k+1]);
		if (llport < 0 || llport > 65535) {
		  mvwprintw(w[5], 2, 2, "llport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
		}
		else {
		  is_llport = 1;
		  llport = tport;
		  thissocket = socket(PF_INET, SOCK_STREAM, 0);
		  memset(&tsa, 0, sizeof(tsa));
		  tsa.sin_family = AF_INET;
		  tsa.sin_port = htons(llport);
		  port2 = llport;
		  tsa.sin_addr.s_addr = htonl(INADDR_ANY);
		  
		  if (setsockopt(thissocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
		    mvwprintw(w[5], 2, 2, "---> setsockopt");
		  }
		  if (bind(thissocket, (struct sockaddr *)&tsa, sizeof(struct sockaddr)) < 0) {
		    mvwprintw(w[5], 2, 2, "---> bind");
		  }
		  else {
		    has_middle = 1;
		    acceptl = 1;
		    acceptr = 0;
		  }
		  if (listen(thissocket, BACKLOG) < 0) {
		    mvwprintw(w[5], 2, 2, "---> listen : return value -1; ");
		  }
		}
	      }
	      else if (strcmp(buf3, "listenr") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "listen right on port: %s", &buf3[k+1]);
		int tport = atoi(&buf3[k+1]);
		if (tport < 0 || tport > 65535) {
		  mvwprintw(w[5], 2, 2, "rlport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
		}
		else {
		  is_rlport = 1;
		  rlport = tport;
		  thissocket = socket(PF_INET, SOCK_STREAM, 0);
		  memset(&tsa, 0, sizeof(tsa));
		  tsa.sin_family = AF_INET;
		  tsa.sin_port = htons(rlport);
		  port2 = rlport;
		  tsa.sin_addr.s_addr = htonl(INADDR_ANY);
		  
		  if (setsockopt(thissocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) == -1) {
		    mvwprintw(w[5], 2, 2, "---> setsockopt");
		  }
		  if (bind(thissocket, (struct sockaddr *)&tsa, sizeof(struct sockaddr)) < 0) {
		    mvwprintw(w[5], 2, 2, "---> bind");
		  }
		  else {
		    has_middle = 1;
		    acceptr = 1;
		    acceptl = 0;
		  }
		  if (listen(thissocket, BACKLOG) < 0) {
		    mvwprintw(w[5], 2, 2, "---> listen : return value -1; ");
		  }
		}				
	      }
	      else if (strcmp(buf3, "read") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		mvwprintw(w[5], 2, 2, "read file: %s and send to output", &buf3[k+1]);
		if ((myfile = fopen(&buf3[k+1], "rb")) == NULL) {
		  mvwprintw(w[5], 2, 2, "read file: %s could not be opened", &buf3[k+1]);
		}
		else {
		  
		  // attempt at sending the entire file size 
// 		  fseek(myfile, 0, SEEK_END);
// 		  long fsize = ftell(myfile);
// 		  filebuf = (char *) malloc (sizeof(char) *fsize);
// 		  fseek(myfile, 0, SEEK_SET);
// 		  size_t bread = fread(filebuf, sizeof(char), fsize, myfile);
		  
		  size_t bread = fread(filebuf, sizeof(char), BUFSIZE, myfile);
		  has_file = 1;
		  
// 		  if (bread != fsize) {
// 		    mvwprintw(w[5], 2, 2, "read file: error reading %s not the same amount of bytes", &buf3[k+1]);
// 		  }
		  
		  if (feof(myfile)) {
		    fclose(myfile);
		  }
		  else if (ferror(myfile)) {
		    fclose(myfile);
		    mvwprintw(w[5], 2, 2, "read file: error reading %s", &buf3[k+1]);
		  }
		  //free(filebuf);
		}
	      }
	      else if (strcmp(buf3, "source") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		mvwprintw(w[5], 2, 2, "execute script file: %s", &buf3[k+1]);
	      }
	      else if (strcmp(buf3, "llport") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "bind local port: %s for left", &buf3[k+1]);
		int tport = atoi(&buf3[k+1]);
		if (llport < 0 || llport > 65535) {
		  mvwprintw(w[5], 2, 2, "llport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
		}
		else {
		  llport = tport;
		  is_llport = 1;
		}
	      }
	      else if (strcmp(buf3, "rlport") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "bind local port: %s for right", &buf3[k+1]);
		int tport = atoi(&buf3[k+1]);
		if (tport < 0 || tport > 65535) {
		  mvwprintw(w[5], 2, 2, "rlport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
		}
		else {
		  rlport = tport;
		  is_rlport = 1;
		}
	      }
	      else if (strcmp(buf3, "lrport") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "connect to remote left port: %s", &buf3[k+1]);
		int tport = atoi(&buf3[k+1]);
		if (tport < 0 || tport > 65535) {
		  mvwprintw(w[5], 2, 2, "lrport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
		}
		else {
		  lrport = tport;
		  is_lrport = 1;
		}		
	      }
	      else if (strcmp(buf3, "rrport") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "connect to remote right port: %s", &buf3[k+1]);
		int tport = atoi(&buf3[k+1]);
		if (tport < 0 || tport > 65535) {
		  mvwprintw(w[5], 2, 2, "rrport: Please use valid port address 0..65535 (0-1023 requires ROOT)");
		}
		else {
		  rrport = tport;
		  is_rrport = 1;
		}			
	      }
	      else if (strcmp(buf3, "laddr") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "connect left if remote IP: %s", &buf3[k+1]);
		if ((he = gethostbyname(&buf3[k+1])) == NULL) {
		  mvwprintw(w[5], 2, 2, "gethostname: %s not valid IP", &buf3[k+1]);
		}
		else {
		  laddr_list = (struct in_addr **)he->h_addr_list;
		  laddr = inet_ntoa(*laddr_list[0]);
		  is_laddr = 1;
		}
	      }
	      else if (strcmp(buf3, "raddr") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		//mvwprintw(w[5], 2, 2, "connect right to IP: %s", &buf3[k+1]);
		if ((he = gethostbyname(&buf3[k+1])) == NULL) {
		  mvwprintw(w[5], 2, 2, "gethostname: %s not valid IP", &buf3[k+1]);
		}
		else {
		  raddr_list = (struct in_addr **)he->h_addr_list;
		  raddr = inet_ntoa(*raddr_list[0]);
		  is_raddr = 1;
		}	
	      }
	      else if (strcmp(buf3, "loglrpre") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		mvwprintw(w[5], 2, 2, "logging: left->right traffic before filtering to file %s", &buf3[k+1]);
		if ((lrprelog = fopen(&buf3[k+1], "ab")) == NULL) {
		  mvwprintw(w[5], 2, 2, "log file: %s could not be opened/created", &buf3[k+1]);
		}
		else {
		  is_loglrpre = 1;
		}
	      }
	      else if (strcmp(buf3, "logrlpre") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		mvwprintw(w[5], 2, 2, "logging: left<-right traffic before filtering to file %s", &buf3[k+1]);
		if ((rlprelog = fopen(&buf3[k+1], "ab")) == NULL) {
		  mvwprintw(w[5], 2, 2, "log file: %s could not be opened/created", &buf3[k+1]);
		}
		else {
		  is_logrlpre = 1;
		}
	      }
	      else if (strcmp(buf3, "loglrpost") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		mvwprintw(w[5], 2, 2, "logging: left->right traffic after filtering to file %s", &buf3[k+1]);
		if ((lrpostlog = fopen(&buf3[k+1], "ab")) == NULL) {
		  mvwprintw(w[5], 2, 2, "log file: %s could not be opened/created", &buf3[k+1]);
		}
		else {
		  is_loglrpost = 1;
		}
	      }
	      else if (strcmp(buf3, "logrlpost") == 0) {
		wmove(w[5], 2, 2);
		wclrtoeol(w[5]);
		mvwprintw(w[5], 2, 2, "logging: left<-right traffic before filtering to file %s", &buf3[k+1]);
		if ((rlpostlog = fopen(&buf3[k+1], "ab")) == NULL) {
		  mvwprintw(w[5], 2, 2, "log file: %s could not be opened/created", &buf3[k+1]);
		}
		else {
		  is_logrlpost = 1;
		}
	      }	      
	
	      
	      buf3[k] = ' ';
	    }
	    else {
	      //
	    }
	  }
	  wmove(w[5], 3, 2);
	  wclrtoeol(w[5]);
	  wrefresh(w[5]);	  
	  wmove(w[5], 2, 2);
	  wrefresh(w[5]);
	}
      }
      else if ((nch < 32) || (nch == 127)) {
	wmove(w[5], 3, 2);
	wclrtoeol(w[5]);
      }
      i = 0;
      wrefresh(w[5]);
    }
  }
  if (lrprelog != NULL) {
    fclose(lrprelog);
  }
  else if (lrpostlog != NULL) {
    fclose(lrpostlog);
  }
  else if (rlprelog != NULL) {
    fclose(rlprelog);
  }
  else if (rlpostlog != NULL) {
    fclose(rlpostlog);
  }
  endwin();
}  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
    