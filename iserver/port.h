/******************************************************************
**  RCSID: $Id: port.h,v 1.1 1998/11/24 19:56:45 cmalek Exp $
** Program: iserver
**  Module: port.h
**  Author: cmalek
** Descrip: define some prototypes for SunOS so gcc stops complaining
*******************************************************************/

#ifdef sparc
extern int fprintf(), printf(), vprintf(), vsprintf();
extern int fscanf(), scanf(), sscanf();
extern int puts(), fputs(), fputc();
extern int atoi();
extern int unlink();
extern int getpid();
extern int chdir();
extern int write(), read(), close();
extern int dup(), dup2();
extern int system();
extern char *getenv();
extern char *getcwd();
extern void perror();
extern void bcopy(), bzero();
extern unsigned int alarm(), usleep();
extern time_t time();
extern long random();
extern int gettimeofday();
extern int vfprintf();
extern int rename();
extern int getpriority(), setpriority();
extern int mlock(), munlock();
extern int sigpause();
extern int shmctl();
extern void rewind();
extern char *memset();
extern int 		fwrite(), fread(), fclose(), pclose(), fflush();
#endif
