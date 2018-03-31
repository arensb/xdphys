/******************************************************************
**  RCSID: $Id: is-bind.c,v 1.5 1998/11/24 19:56:45 cmalek Exp $
** Program: xdphys
**  Module: is-bind.c
**  Author: cmalek
** Descrip: tcl interface functions 
**
*******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <unistd.h>
#endif /* __linux__ */
#include <tcl.h>

#include "iserver.h"
#include "iserverInt.h"
#include "att.h"

/*-----------------------------------------------------------------------
  Local Prototypes
  ----------------------------------------------------------------------- */
static int Is_Init(ClientData, Tcl_Interp	*, int, char **);
static int Is_Shutdown(ClientData, Tcl_Interp	*, int, char **);
static int Is_Load(ClientData, Tcl_Interp	*, int, char **);
static int Is_Play(ClientData, Tcl_Interp	*, int, char **);
static int Is_Record(ClientData, Tcl_Interp	*, int, char **);
static int Is_Fc(ClientData, Tcl_Interp	*, int, char **);
static int Is_Att(ClientData, Tcl_Interp	*, int, char **);

static int max_duration;

/* ---------------------------------------------------------------------
          					GLOBAL FUNCTIONS
   ---------------------------------------------------------------------
	Iserver_Init
   -------------------------------------------------------------------- */
int Iserver_Init(
     Tcl_Interp *interp)
{
	Tcl_CreateCommand(interp, "is_init", Is_Init, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "is_load", Is_Load, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "is_play", Is_Play, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "is_record", Is_Record, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "is_shutdown", Is_Shutdown, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "is_fc", Is_Fc, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
	Tcl_CreateCommand(interp, "is_att", Is_Att, (ClientData) NULL,
						(Tcl_CmdDeleteProc *) NULL);
  return(TCL_OK);
}


/*-----------------------------------------------------------------------
 			Functions seen only by this file below this line
   ---------------------------------------------------------------------
   Is_Init
   -------------------------------------------------------------------- */
static int Is_Init(
     ClientData z,		/* Not used. */
     Tcl_Interp *interp,	/* Current interpreter. */
     int ac,			/* Number of arguments. */
     char **av)			/* Argument strings. */
{
	double	fc;
	int		epoch;

	if (ac < 3 || (ac > 1 && strcmp(av[1], "-?") == 0)) {
		Tcl_AppendResult(interp, "usage: is_init fc dur",
						 (char *) NULL);
		return TCL_ERROR;
	} else {

		if (Tcl_GetDouble(interp, av[1], &fc) != TCL_OK)
			return(TCL_ERROR);
		if (Tcl_GetInt(interp, av[2], &epoch) != TCL_OK)
			return(TCL_ERROR);
		if (is_init((float) fc, (float) fc, -1.0, epoch, epoch) 
			== IS_ERROR) {
			Tcl_AppendResult(interp, "\nis_init() failed.", (char *) NULL);
			max_duration = 0;
			return(TCL_ERROR);
		} else {
			max_duration = epoch;
			return(TCL_OK);
		}
	}
}

/* ---------------------------------------------------------------------
   Is_Shutdown
   -------------------------------------------------------------------- */
static int Is_Shutdown(
     ClientData z,					/* Not used. */
     Tcl_Interp *interp,			/* Current interpreter. */
     int ac,						/* Number of arguments. */
     char **av)						/* Argument strings. */
{
	if (ac != 1 || (ac > 1 && strcmp(av[1], "-?") == 0)) {
		Tcl_AppendResult(interp, "usage: is_shutdown",
						 (char *) NULL);
		return TCL_ERROR;
    }

    if (is_shutdown() == IS_ERROR) {
		Tcl_AppendResult(interp, "\nis_shutdown() failed.", (char *) NULL);
		return(TCL_ERROR);
    } else {
		return(TCL_OK);
    }
}


/* -------------------------------------------------------------------- 
   Is_Play
   ----------------------------------------------------------------------- */
static int Is_Play(
     ClientData z,				/* Not used. */
     Tcl_Interp *interp,		/* Current interpreter. */
     int ac,					/* Number of arguments. */
     char **av)					/* Argument strings. */
{
	int 		flags = 0;
	double 		timestamp;
	char 		buf[20];

	if ((ac > 2) && (Tcl_StringMatch(av[1], "-?"))) {
		Tcl_AppendResult(interp, "usage: is_play <-left> <-right>", 
			(char *) NULL);
		return(TCL_ERROR);
	} 

	if (ac > 1) {
		if (Tcl_StringMatch(av[1], "-l*"))
			flags |= X_LEFT;
		if (Tcl_StringMatch(av[1], "-r*"))
			flags |= X_RIGHT;
	}
	if (is_sample(flags, max_duration, &timestamp) == IS_OK) 
		sprintf(buf, "%f", timestamp);
	else 
		sprintf(buf, "-1.0");

	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return(TCL_OK);
}

/* -------------------------------------------------------------------- 
   Is_Load
   ----------------------------------------------------------------------- */
static int Is_Load(
     ClientData z,				/* Not used. */
     Tcl_Interp *interp,		/* Current interpreter. */
     int ac,					/* Number of arguments. */
     char **av)					/* Argument strings. */
{
	if ((ac > 1) && (Tcl_StringMatch(av[1], "-?"))) {
		Tcl_AppendResult(interp, "usage: is_load", 
			(char *) NULL);
		return(TCL_ERROR);
	} 

	if (is_load(0) != IS_OK) {
		Tcl_AppendResult(interp, "is_load: error loading play buffers!", 
			(char *) NULL);
		return(TCL_ERROR);
	} else
		return(TCL_OK);
}

/* -------------------------------------------------------------------- 
   Is_Record
   ----------------------------------------------------------------------- */
static int Is_Record(
     ClientData z,				/* Not used. */
     Tcl_Interp *interp,		/* Current interpreter. */
     int ac,					/* Number of arguments. */
     char **av)					/* Argument strings. */
{

	if ((ac > 1) && (Tcl_StringMatch(av[1], "-?"))) {
		Tcl_AppendResult(interp, "usage: is_record", (char *) NULL);
		return(TCL_ERROR);
	} 

	if (is_record() != IS_OK) {
		Tcl_AppendResult(interp, "is_load: error retrieving rec buffers!", 
			(char *) NULL);
		return(TCL_ERROR);
	} else
		return(TCL_OK);
}


/* -------------------------------------------------------------------- 
   Is_Fc
   -------------------------------------------------------------------- */
static int Is_Fc(
     ClientData z,				/* Not used. */
     Tcl_Interp *interp,		/* Current interpreter. */
     int ac,					/* Number of arguments. */
     char **av)					/* Argument strings. */
{
	char buf[20];

	if (ac != 2 || (ac > 1 && strcmp(av[1], "-?") == 0)) {
		Tcl_AppendResult(interp, "usage: is_fc [-ad | -da]",
			(char *) NULL);
		return(TCL_ERROR);
	} 

	if (Tcl_StringMatch(av[1], "-a*")) {
		sprintf(buf, "%d", is_adFc);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
	} else {
		sprintf(buf, "%d", is_daFc);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
	}

	return(TCL_OK);
}

/* -------------------------------------------------------------------- 
   Is_Att
   -------------------------------------------------------------------- */
static int Is_Att(
     ClientData z,				/* Not used. */
     Tcl_Interp *interp,		/* Current interpreter. */
     int ac,					/* Number of arguments. */
     char **av)					/* Argument strings. */
{
	double 		latten, ratten;
	char		buf[80];
	int			i = 1;

	if (ac < 3 || (ac > 1 && strcmp(av[1], "-?") == 0)) {
		Tcl_AppendResult(interp, "usage: is_att -l <atten> | -r <atten>",
			(char *) NULL);
		return(TCL_ERROR);
	} 

	getRack(&latten, &ratten);
	while (i < ac) {
		if (Tcl_StringMatch(av[i], "-l*")) {
			if (Tcl_GetDouble(interp, av[++i], &latten) != TCL_OK) 
				return(TCL_ERROR);
		} else if (Tcl_StringMatch(av[i], "-r*")) {
			if (Tcl_GetDouble(interp, av[++i], &ratten) != TCL_OK)
				return(TCL_ERROR);
		} else {
			sprintf(buf, "is_att: invalid flag (%s)", av[i]);
			Tcl_AppendResult(interp, buf, (char *) NULL);
			return(TCL_ERROR);
		}
		i++;
	}

	if (!setRack(0.0, (float )latten, (float )ratten)) {
		Tcl_AppendResult(interp, "is_att: unable to set attenuators!", 
			(char *) NULL);
		return(TCL_ERROR);
	} 

	return(TCL_OK);
}
