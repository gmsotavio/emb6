/*============================================================================*/
/*! \file   clp.c

    \author (c) by STZEDN, Heitersheim, Germany, http://www.stzedn.de

    \brief  Command line parser and menu generator.

  \version  1.0.1
*/
/*============================================================================*/

/*==============================================================================
                                 INCLUDE FILES
 =============================================================================*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define __DECL_CLP_H__
#include "clp.h"
#include "bsp.h"

#if (HAL_SUPPORT_UART == TRUE)
/*==============================================================================
                                     MACROS
 =============================================================================*/
#ifndef EBTTR_DBG_PRINTF
#define EBTTR_DBG_PRINTF                  clp_output
#endif

#ifndef DBG_FILE_NAME
#define DBG_FILE_NAME                     "clp.c"
#endif

#if CLP_SORT_ALPHA == TRUE
#define CLP_SORT_ALPHA_DECIDE(a, b)     ((CLP_STRCASECMP((a), (b)) < 0) ? TRUE : FALSE)
#endif
/*==============================================================================
                                     ENUMS
 =============================================================================*/


/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
 =============================================================================*/


/*==============================================================================
                          LOCAL VARIABLE DECLARATIONS
 =============================================================================*/
static struct
{
  //! Pointer to the first menu entry. NULL if the list is empty
  const s_clp_menuEntry_t* p_first;

  //! Current menu parent. If set to NULL the menu is in root
  const s_clp_menuEntry_t* p_currMenuParent;

  //! Current command line
  char c_cmdLine[CLP_COMMAND_LINE_LEN];

  //! Current command line index
  int i_cmdLineWriteIndex;

  //! If echo enabled, every character received is sent back, default is 0
  int i_echoEnabled;

  //! Title for the menu print function
  const char* p_menuPrintTitle;

  //! UART driver handle
  void *p_uart;
}gs_clp;

//! Default help command menu entry
static s_clp_menuEntry_t gs_clp_helpCmd;

/*==============================================================================
                                LOCAL CONSTANTS
 =============================================================================*/


/*==============================================================================
                           LOCAL FUNCTION PROTOTYPES
 =============================================================================*/

static void                       clp_cmdLineClear          (void);
static void                       clp_outputHelp            (int i_argNum, const char* p_args[]);
static void                       clp_printFormatted        (const char* p_string);
static unsigned int               clp_printHelpLine         (const char* p_string, unsigned int ui_len);
static void                       clp_printHelp             (const char* p_help, int i_startOffset);
static int                        clp_menuEntryCheckInList  (const s_clp_menuEntry_t* p_entry);
static const s_clp_menuEntry_t*   clp_findEntry             (const char* p_cmd, const s_clp_menuEntry_t* p_parent);

#if (CLP_HAVE_STRTOK_R == FALSE)
char* strtok_r (char *str, const char *delim, char **nextp);
#endif /* CLP_HAVE_STRTOK_R */

/*==============================================================================
                                LOCAL FUNCTIONS
 =============================================================================*/


#if (CLP_HAVE_STRTOK_R == FALSE)

char* strtok_r (char *str, const char *delim, char **nextp)
{
  char *ret;

  if (str == NULL)
  {
    str = *nextp;
  } /* if */

  str += strspn(str, delim);

  if (*str == '\0')
  {
    return NULL;
  } /* if */

  ret = str;

  str += strcspn(str, delim);

  if (*str)
  {
    *str++ = '\0';
  } /* if */

  *nextp = str;

  return ret;
} /* strtok_r() */
#endif /* CLP_HAVE_STRTOK_R */

#if CLP_HAVE_STRCASECMP == FALSE
#define CCMP(a, b) ((a) == (b) ? 0 : ((a) > (b) ? 1 : -1))
#define IS_ASCII(c) (((int)(c) >= 0) && ((c) < 128))
int strcasecmp (const char *s1, const char *s2)
{
  char c1, c2;
  for (;;)
  {
    if ((*s1 == '\0') || (*s2 == '\0'))
      return CCMP(*s1, *s2);
    c1 = (IS_ASCII(*s1) && isupper(*s1)) ? tolower(*s1) : *s1;
    c2 = (IS_ASCII(*s2) && isupper(*s2)) ? tolower(*s2) : *s2;
    if (c1 != c2)
      return CCMP(c1, c2);
    ++s1;
    ++s2;
  } /* for */
} /* strcasecmp() */
#undef CCMP
#undef IS_ASCII
#endif /* CLP_HAVE_STRCASECMP */

/*============================================================================*/
/*!
    \brief   Check if a given entry is part of the list

    \param   p_entry The entry to check
    \return  1 if the entry is in the list
    \return  0 if the entry is not part in the list
*/
/*============================================================================*/
static int clp_menuEntryCheckInList (const s_clp_menuEntry_t* p_entry)
{
  const s_clp_menuEntry_t* p_current;
  int i_return = 0;

  /*
   * Check if the entry matches the the head. This is a special case, since
   * there exists no other reference to the entry.
   */
  if (p_entry == gs_clp.p_first)
  {
    i_return = 1;
  }
  else
  {
    /*
     * Walk through the list of entries and check whether a reference to the
     * entry that we are looking for is found.
     */
    for(p_current = gs_clp.p_first; p_current; p_current=p_current->p_next)
     if (p_current->p_next == p_entry)
      {
        i_return = 1;
        break;
      }
  }

  return i_return;
} /* clp_menuEntryCheckInList() */

/*============================================================================*/
/*!
    \brief   Clear the command line that is currently in the buffer.

             Reset the command line buffer to all '0' and set the write index
             to 0.
*/
/*============================================================================*/
static void clp_cmdLineClear (void)
{
  gs_clp.i_cmdLineWriteIndex = 0;
  memset(gs_clp.c_cmdLine, 0, CLP_COMMAND_LINE_LEN);
} /* clp_cmdLineClear() */

/*============================================================================*/
/*!
    \brief   Find a menu entry according to a command

    \param   p_cmd     Pointer to the command that the entry must match
    \param   p_parent  Parent menu entry that must match. If set to NULL it must
                       be a menu entry in the main trunk.

    \return  Pointer to the menu entry where p_cmd and the parent entry
             match.

    \return  NULL, if the entry can not be found
*/
/*============================================================================*/
static const s_clp_menuEntry_t* clp_findEntry (const char* p_cmd, const s_clp_menuEntry_t* p_parent)
{
  const s_clp_menuEntry_t* p_search;
  const s_clp_menuEntry_t* p_match = NULL;

  for (p_search = gs_clp.p_first; p_search; p_search = p_search->p_next)
   {
     /*
      * Check if the command and parent pointer match
      */
     if ((p_search->p_parent == p_parent) && (strcmp(p_cmd, p_search->p_cmd) == 0))
      {
        p_match = p_search;
        break;
      } /* if */
   } /* for */

  return p_match;
} /* clp_findEntry() */

/*============================================================================*/
/*!
    \brief   Parse a command line and call the specific handler for the command.

    \param   p_cmdLine  The command line as zero terminated string.
*/
/*============================================================================*/
static void clp_parse (char* p_cmdLine)
{
  /*
   * Parse the beginning of the command for a known command
   */
  const char* p_args[10];
  unsigned int ui_argNum = 0;
  char* gc_strtok;
  const s_clp_menuEntry_t* p_entryMatch = NULL;

  /*
   * Break the command line into tokens, the first token is
   * the command itself
   */
  p_args[ui_argNum] = STRTOK_R(p_cmdLine, " ", &gc_strtok);

  while ((ui_argNum < ((sizeof(p_args)/sizeof(p_args[0]))-1)) && p_args[ui_argNum])
   {
     p_args[++ui_argNum] = STRTOK_R(NULL, " ", &gc_strtok);
   }

  /*
   * One special command exists: ".." This command is used to move up
   * the command line depth
   */
  if (strcmp(p_args[0], "..") == 0)
   {
     /*
      * Get the entry where the current parent entry is referenced as parent
      * If no current parent menu exists yet then this command is ignored.
      */
     if (gs_clp.p_currMenuParent)
       {
         gs_clp.p_currMenuParent = gs_clp.p_currMenuParent->p_parent;

         if (gs_clp.p_currMenuParent)
           EBTTR_DBG_PRINTF("Switched to context \"%s\", type \"..\" to go back", gs_clp.p_currMenuParent->p_cmd);
       } /* if */
   }
  else
   {

     /*
      * Start searching with the current command and the parent is the current
      * context
      */
     unsigned int ui_currArg = 0;
     const s_clp_menuEntry_t* p_currCtx = gs_clp.p_currMenuParent;

     for (;;)
      {
         /*
          * Check if the current argument matches an entry, then set the
          * new context that we start searching from
          */
         p_currCtx = clp_findEntry(p_args[ui_currArg], p_currCtx);

         /*
          * Check if the entry was found. If it was found then check if the
          * following argument of the command is again a menu entry then start
          * search for a valid entry based on this context again.
          */
         if (p_currCtx)
           {
             /*
              * Remember the current entry as a match
              */
             p_entryMatch = p_currCtx;

             /*
              * The current argument matches a menu entry, therefore increment
              * the counter for the arguments to parse.
              */
             ui_currArg++;

             /*
              * If no more arguments are available, stop searching here
              */
             if (ui_argNum <= ui_currArg)
               break;
           }
         else
           break;
      } /* for */

      /*
       * Check now if the command was found
       */
      if (p_entryMatch)
       {
          /*
           * Call the function associated with this command. If not available
           * switch to the context of this menu command
           */
          if (p_entryMatch->pfn_menuCallBack)
           {
             p_entryMatch->pfn_menuCallBack(ui_argNum - ui_currArg, &p_args[ui_currArg]);
           }
          else if(p_entryMatch->pfn_menuCallBackCtx)
           {
             p_entryMatch->pfn_menuCallBackCtx(ui_argNum - ui_currArg, &p_args[ui_currArg], p_entryMatch->p_ctx);
           }
          else
           {
             gs_clp.p_currMenuParent = p_entryMatch;

             EBTTR_DBG_PRINTF("Switched to context \"%s\", type \"..\" to go back", p_entryMatch->p_cmd);
           }
       }
      else
       {
         unsigned int i;

         /*
          * Display the error message
          */
         EBTTR_DBG_PRINTF("Command \"");

         for (i=0; i < ui_argNum; i++)
          {
            EBTTR_DBG_PRINTF("%s", p_args[i]);

            if (i+1 < ui_argNum)
              EBTTR_DBG_PRINTF(" ");
          } /* for */

         EBTTR_DBG_PRINTF("\" unknown");

         if (gs_clp.p_currMenuParent)
           EBTTR_DBG_PRINTF(" in current context \"%s\"", gs_clp.p_currMenuParent->p_cmd);

         EBTTR_DBG_PRINTF(". Try:\r\n");
         clp_menuPrintHelp(gs_clp.p_currMenuParent);
       }
  } /* if ... else */

 clp_outputPrompt();

} /* clp_parse() */

/*============================================================================*/
/*!
    \brief   Built in menu command "help" that can be added.

             Output of all menu entries in the current context. If a parent
             entry is set then only the children entries are output.

    \param   i_argNum Not used in this function
    \param   p_args   Not used in this function.
*/
/*============================================================================*/
static void clp_outputHelp (int i_argNum, const char* p_args[])
{
  clp_menuPrintHelp(NULL);
} /* clp_outputHelp() */

/*============================================================================*/
/*!
    \brief   Print a string with format commands

             Checks whether the title has format information per line and
             then prints it out accordingly.

    \param   p_string Pointer to the string to print with format characters.
*/
/*============================================================================*/
static void clp_printFormatted (const char* p_string)
{
  const char* p_lineStart;
  const char* p_titleEnd = p_string + strlen(p_string);

  /*
   * The first line starts with the beginning of the line
   */
  p_lineStart = p_string;

  /*
   * Run through all the lines in the title menu and print it out accordingly
   */
  while(p_lineStart < p_titleEnd)
  {
    /*
     * Find the end of the line
     */
    const char* p_format;
    const char* p_lineEnd = p_lineStart;
    int i_fillSpaces = 0;

    while((p_lineEnd < p_titleEnd) && (*p_lineEnd != '\n'))
      p_lineEnd++;

    /*
     * Check if the line starts with the format instruction to center the text
     */
    p_format = strstr(p_lineStart, CLP_ALIGN_CENTER);

    if ((p_format != NULL) && (p_format < p_lineEnd))
     {
        int i_lineSize;

        /*
         * Move ahead for the number of format characters
         */
        p_lineStart += strlen(CLP_ALIGN_CENTER);

        /*
         * Calculate the number of characters to be printed
         */
        for (i_lineSize = 0; p_lineStart + i_lineSize < p_lineEnd; i_lineSize++);

        i_fillSpaces = (80 - i_lineSize) / 2;
     } /* if */

    /*
     * Print any leading filling spaces
     */
    for(;i_fillSpaces; i_fillSpaces--)
      EBTTR_DBG_PRINTF(" ");

    /*
     * Print the line
     */
    for (;p_lineStart <= p_lineEnd; p_lineStart++)
      EBTTR_DBG_PRINTF("%c", *p_lineStart);

  } /* while */
} /* clp_printFormatted() */

/*============================================================================*/
/*!
    \brief   Print a line of the help string and detect, whether it contains
             possibly a new line character.

    \param   p_string       String to print out.
    \param   ui_len         Number of characters to print.
    \return  Number of characters printed out.
*/
/*============================================================================*/
static unsigned int clp_printHelpLine (const char* p_string, unsigned int ui_len)
{
  unsigned int ui_ret = 0;

  while (ui_ret < ui_len)
  {
    /* Ignore any carriage-returns */
    if (p_string[ui_ret] == '\r')
    {
      ui_ret++;
    }
    else if (p_string[ui_ret] == '\n')
    {
      /* Force a line break even when we are not done yet */
      ui_ret++;
      break;
    }
    else
    {
      EBTTR_DBG_PRINTF("%c", p_string[ui_ret]);
      ui_ret++;
    } /* if ... else if ... */
  } /* while */

  return ui_ret;
} /* clp_printHelpLine() */

/*============================================================================*/
/*!
    \brief   Print the help part of an entry.

             This function is used to print the help string of an entry. Since
             this string can be rather long, it breaks at the position
             specified through CLP_PRINT_MARGIN.

             A line break occurs whenever the print margin would be exceeded.
             The new line is then filled up to the start offset.

             If a single word would exceed the remaining space, a hyphen is
             inserted. This is at an arbitrary position.

             Furthermore, if the string contains a \n character, a new
             line is inserted and the content is aligned to the given offset.

    \param   p_help         Pointer to a zero terminated string
    \param   i_startOffset  Offset (column) where the help text should start.
*/
/*============================================================================*/
static void clp_printHelp (const char* p_help, int i_startOffset)
{
  const char* p_currentHelpWord = p_help;
  int i_currentPos = i_startOffset;

  while (p_currentHelpWord < (p_help + strlen(p_help)))
  {
    int i;

    /*
     * First case: the help is shorter than the print margin. In this
     * case just print it out and leave the loop
     */
    if (i_currentPos + strlen(p_currentHelpWord) < CLP_PRINT_MARGIN)
    {
      unsigned int ui_printed = clp_printHelpLine(p_currentHelpWord, strlen(p_currentHelpWord));

      if (ui_printed >= strlen(p_currentHelpWord))
      {
        /* The entire string was printed. Quit the loop */
        break;
      }
      else
      {
        /*
         * A line break was detected within a string that is shorter than
         * the print margin. Therefore continue.
         */
        p_currentHelpWord += ui_printed;
      } /* if ... else */
    }
    else
    {
      /* Calculate the last character of the word to print out */
      const char* p_wordNextLine = p_currentHelpWord - i_currentPos + CLP_PRINT_MARGIN;

      /* Roll-back until a space is found */
      while ((*p_wordNextLine != ' ') && (p_wordNextLine != p_currentHelpWord))
        p_wordNextLine--;

      /* Check if a space between words was detected */
      if (p_wordNextLine > p_currentHelpWord)
      {
        /* Now print out everything until the end of the string */
        p_currentHelpWord += clp_printHelpLine(p_currentHelpWord, (unsigned int)p_wordNextLine - (unsigned int)p_currentHelpWord);
      }
      else
      {
        unsigned int ui_printed;

        /* Print out as much as possible, add a hyphen at the end of the line */
        ui_printed = clp_printHelpLine(p_currentHelpWord, CLP_PRINT_MARGIN - i_currentPos - 1);

        /* Add the hyphen only when no new-line was in the string */
        if (ui_printed >= (CLP_PRINT_MARGIN - i_currentPos - 1))
          EBTTR_DBG_PRINTF("-");

        /* Move ahead the write pointer */
        p_currentHelpWord += ui_printed;
      } /* if ... else */

      /* Skip any leading spaces */
      while (*p_currentHelpWord == ' ')
        p_currentHelpWord++;
    } /* if ... else */

    /* Insert a new line */
    EBTTR_DBG_PRINTF("\r\n");

    /* Current printing position restarts at 0 */
    i_currentPos = 0;

    /* Fill with spaces */
    for (i = 0; i < i_startOffset; i++)
      i_currentPos += EBTTR_DBG_PRINTF(" ");

  } /* while */

  /* Terminate with a new-line */
  EBTTR_DBG_PRINTF("\r\n");
} /* clp_printHelp() */

/*==============================================================================
                                 API FUNCTIONS
 =============================================================================*/

/*============================================================================*/
/*  clp_init()                                                                */
/*============================================================================*/
void clp_init (void *p_uart)
{
   /*
    * Reset the current entries and set the parent to NULL
    */
   gs_clp.p_first  = NULL;
   gs_clp.p_currMenuParent = NULL;

   /*
    * Clear the command line
    */
   clp_cmdLineClear();

   /*
    * Echo is off by default
    */
   clp_echoEnable(0);

   /*
    * Reset the title for the menu print function
    */
   gs_clp.p_menuPrintTitle = NULL;

   /*
    * Initialize UART
    */
   gs_clp.p_uart = p_uart;

   /*
    * Add the default entry "help"
    */
   clp_menuAddEntry(&gs_clp_helpCmd, NULL, "help", "Display all commands", clp_outputHelp);

} /* clp_init() */

/*============================================================================*/
/*  clp_echoEnable()                                                          */
/*============================================================================*/
int clp_echoEnable (int i_enable)
{
  int i_return = gs_clp.i_echoEnabled;
  gs_clp.i_echoEnabled = i_enable;
  return i_return;
} /* clp_echoEnable() */

/*============================================================================*/
/*  clp_setTitle()                                                            */
/*============================================================================*/
void clp_setTitle (const char* p_title)
{
  gs_clp.p_menuPrintTitle = p_title;
} /* clp_setTitle() */

/*============================================================================*/
/*  clp_menuAddEntry()                                                        */
/*============================================================================*/
int clp_menuAddEntry (s_clp_menuEntry_t* p_entry, s_clp_menuEntry_t* p_parent, const char* p_cmd, const char* p_help, pfn_clp_menuCallBack_t pfn_callBack)
{
  s_clp_menuEntry_t* p_current;
  int i_return = -1;

  /*
   * Check if the current command exists already in the list. The condition
   * of a match would be that the parent and the command itself are equal
   */
  for(p_current = (s_clp_menuEntry_t*)gs_clp.p_first; p_current; p_current = p_current->p_next)
   {
     if ((p_current->p_parent == p_parent) && (strcmp(p_current->p_cmd, p_cmd) == 0))
      {
        #if DBG_CLP
        EBTTR_DBG_PRINTF(DBG_STRING " command \"%s\" already in the list", DBG_FILE_NAME, __LINE__, p_cmd);
        #endif
        break;
      } /* if */
   } /* for */

  /*
   * The for()-loop exits with p_current!=NULL when the command already exists
   */
  if (p_current == NULL)
   {
     /*
      * If an item is to be added to root level it is slightly different to
      * an entry to be added as a child. In the second case the parent entry
      * must be found and it must be a parent entry, thus no call-back function
      * added to the entry
      */
     if (p_parent)
      {
        if (clp_menuEntryCheckInList(p_parent))
         {
           s_clp_menuEntry_t* p_prev;

           /*
            * Add the entry to the last entry where the parent is the same
            */
           for (p_prev = p_parent; ; p_prev = p_prev->p_next)
            {
              /*
               * The entry to append this entry to will be the one where the
               * parent is still the same but the next entry (if available)
               * will not have the same parent anymore
               */
              if (p_prev->p_next)
               {
                 if ((p_prev->p_next->p_parent != p_parent)
                 #if CLP_SORT_ALPHA == TRUE
                     || (CLP_SORT_ALPHA_DECIDE(p_prev->p_next->p_cmd, p_cmd) == FALSE)
                 #endif
                     )
                    break;
               }
              else
                break;
            } /* for */

            /*
             * Insert the entry into the list
             */
            p_entry->p_next = p_prev->p_next;
            p_prev->p_next  = p_entry;

            i_return = 0;
         }
        #if DBG_CLP
        else
          EBTTR_DBG_PRINTF(DBG_STRING " can not add command \"%s\" since parent is not in list", DBG_FILE_NAME, __LINE__, p_cmd);
        #endif

      }
     else
      {
        s_clp_menuEntry_t* p_prev = NULL;

        #if CLP_SORT_ALPHA == TRUE
        /*
         * Search the place where to insert this entry
         */
        if (gs_clp.p_first)
         {
          p_prev = (s_clp_menuEntry_t*)gs_clp.p_first;

          /*
           * Find the previous entry before the current entry to add it to
           * the list. This depends on the alphabetical order.
           */
          while(p_prev)
          {
            /*
             * Go ahead in the list under the following conditions:
             *  - nor the current entry neither the next entry is a main-entry
             *  - the next main-entry is alphabetically smaller than the entry to add
             */
            if (((p_prev->p_parent != NULL) && (p_prev->p_next) && (p_prev->p_next->p_parent != NULL)) ||
                ((p_prev->p_next) && ((p_prev->p_next->p_parent != NULL) || (CLP_SORT_ALPHA_DECIDE(p_prev->p_next->p_cmd, p_cmd) == TRUE))))
            {
              /* Go ahead in the list */
              p_prev = p_prev->p_next;
            }
            else
            {
              /* The entry was found, therefore exit the loop here */
              break;
            } /* if ... else */
          } /* while */
         } /* if */

        /*
         * If this is the first entry or this entries command should be first in list
         * then add the reference to the first entry in the global structure
         */
        if ((p_prev == NULL) || (CLP_SORT_ALPHA_DECIDE(p_cmd, gs_clp.p_first->p_cmd) == TRUE))
         {
            p_entry->p_next = (s_clp_menuEntry_t*)gs_clp.p_first;
            gs_clp.p_first = p_entry;
         }
        else
        {
          /* All other cases */
          p_entry->p_next = p_prev->p_next;
          p_prev->p_next = p_entry;
        } /* if ... else */

        #else /* #if CLP_SORT_ALPHA == TRUE */
        /*
         * Add the entry at the end of the list. If no entry exists the pointer
         * p_prev will be NULL.
         */
        if (gs_clp.p_first)
          for (p_prev = (s_clp_menuEntry_t*)gs_clp.p_first; p_prev->p_next; p_prev=p_prev->p_next);

        /*
         * If this is the first entry then add the reference to the first
         * entry in the global structure
         */
        if (p_prev == NULL)
         {
           gs_clp.p_first = p_entry;
           p_prev = p_entry;
         } /* if */

        /*
         * Adjust the references for the last element
         */
        p_prev->p_next  = p_entry;
        p_entry->p_next = NULL;
        #endif

        i_return = 0;
      } /* if ... else */
   } /* if */


  /*
   * Check if the entry could be added to the list
   */
  if (i_return != -1)
   {
     p_entry->p_cmd  = p_cmd;
     p_entry->p_help = p_help;
     p_entry->p_parent = p_parent;
     p_entry->pfn_menuCallBack = pfn_callBack;

     #if DBG_CLP
     EBTTR_DBG_PRINTF(DBG_STRING " command \"%s\" to menu added", DBG_FILE_NAME, __LINE__, p_cmd);
     #endif
   } /* if */

  return i_return;
} /* clp_menuAddEntry() */

/*============================================================================*/
/*  clp_menuSetCtxBasedCB()                                                   */
/*============================================================================*/
int clp_menuSetCtxBasedCB (s_clp_menuEntry_t* p_entry, pfn_clp_menuCallBackCtx_t pfn_callBack, void* p_ctx)
{
  int i_return = 0;

  /*
   * Check if the entry is not null and already part of the list.
   */
  if (p_entry)
   {
     if (clp_menuEntryCheckInList(p_entry))
      {
         p_entry->pfn_menuCallBack     = NULL;
         p_entry->pfn_menuCallBackCtx  = pfn_callBack;
         p_entry->p_ctx                = p_ctx;

         #if DBG_CLP
         EBTTR_DBG_PRINTF(DBG_STRING " entry \"%s\" set to use context based call-back", DBG_FILE_NAME, __LINE__, p_entry->p_cmd);
         #endif
      }
     #if DBG_CLP
     else
       EBTTR_DBG_PRINTF(DBG_STRING " can not find entry in list, unable to set context based call-back", DBG_FILE_NAME, __LINE__);
     #endif

   }
  #if DBG_CLP
  else
    EBTTR_DBG_PRINTF(DBG_STRING " pointer to the entry is NULL!", DBG_FILE_NAME, __LINE__);
  #endif

  return i_return;
} /* clp_menuSetCtxBasedCB() */


/*============================================================================*/
/*  clp_rxChar()                                                              */
/*============================================================================*/
void clp_rxChar (const char c_char)
{
  /*
   * Check if the end of a line was received
   */
  if ((c_char == '\r') || (c_char == '\n'))
   {
     /*
      * If echo is enabled print an empty line
      */
     if (gs_clp.i_echoEnabled)
       EBTTR_DBG_PRINTF("\r\n");

     /*
      * Call the menu parser if the line is not zero length
      */
     if (strlen(gs_clp.c_cmdLine))
       clp_parse(gs_clp.c_cmdLine);

     /*
      * Reset the current command line buffer
      */
     clp_cmdLineClear();
   }
  else if (c_char == 127)
   {
      /*
       * Backspace character received, clear the last character if the buffer
       * has one
       */
      if (gs_clp.i_cmdLineWriteIndex)
       {
         gs_clp.c_cmdLine[gs_clp.i_cmdLineWriteIndex] = 0;
         gs_clp.i_cmdLineWriteIndex--;

         /*
          * If echo is enabled, then output the character
          */
         if (gs_clp.i_echoEnabled)
           EBTTR_DBG_PRINTF("%c", c_char);
       }
   }
  else
   {
     /*
      * Add the character to the buffer
      */
     gs_clp.c_cmdLine[gs_clp.i_cmdLineWriteIndex++] = c_char;

     /*
      * Check that the buffer does not overflow, leave one space at the end
      * for a '0' character
      */
     if (gs_clp.i_cmdLineWriteIndex > CLP_COMMAND_LINE_LEN - 2)
      {
        EBTTR_DBG_PRINTF("Command too long, buffer reset\r\n");
        clp_cmdLineClear();
        clp_outputPrompt();
      }
     else
      {
        /*
         * If echo is enabled, then output the character
         */
        if (gs_clp.i_echoEnabled)
          EBTTR_DBG_PRINTF("%c", c_char);
      } /* if ... else */
   } /* if ... else */
} /* clp_rxChar() */

/*============================================================================*/
/*  clp_menuPrint()                                                           */
/*============================================================================*/
void clp_menuPrint (void)
{
  EBTTR_DBG_PRINTF("\r\n");

  /*
   * Check if a title has to be printed out
   */
  clp_outputTitle();

  EBTTR_DBG_PRINTF("\r\n");

  /*
   * Show the entire menu command structure
   */
  clp_menuPrintHelp(NULL);

  clp_outputPrompt();
} /* clp_menuPrint() */

/*============================================================================*/
/*  clp_menuPrintHelp()                                                       */
/*============================================================================*/
void clp_menuPrintHelp (const s_clp_menuEntry_t* p_parent)
{
  size_t sz_cmdMaxLen = 0;
  const s_clp_menuEntry_t* p_startWith;
  const s_clp_menuEntry_t* p_current;

  /*
   * Check if all entries should be displayed as help or just the children of
   * a parent entry
   */
  if (p_parent && clp_menuEntryCheckInList(p_parent))
    p_startWith = p_parent;
  else
    p_startWith = gs_clp.p_first;

  /*
   * Iterate through all entries calculate the required space between command
   * and description.
   */
  for (p_current = p_startWith; p_current; p_current = p_current->p_next)
   {
     /*
      * Display only entries that are related to the parent entry, if given.
      * Otherwise skip the entry.
      */
     if ((p_parent == NULL) || ((p_current == p_parent) || (p_current->p_parent == p_parent)))
      {
         if (strlen(p_current->p_cmd) > sz_cmdMaxLen)
           sz_cmdMaxLen = strlen(p_current->p_cmd);
      } /* if */
   } /* for */

  /*
   * Iterate through all entries and display the command and the help message
   */
  for (p_current = p_startWith; p_current; p_current = p_current->p_next)
   {
     /*
      * Display only entries that are related to the parent entry, if given.
      * Otherwise skip the entry.
      */
     if ((p_parent == NULL) || ((p_current == p_parent) || (p_current->p_parent == p_parent)))
      {
         size_t i;
         int i_currentPos = 0;

         /*
          * Check if it is a main root entry, then start to print without space
          * Otherwise, for children entries, show some space before
          */
         if (p_current->p_parent)
           i_currentPos += EBTTR_DBG_PRINTF("  ");

         /* Print the command */
         i_currentPos += EBTTR_DBG_PRINTF("%s", p_current->p_cmd);

         /* Print the spaces */
         for (i = strlen(p_current->p_cmd); i < sz_cmdMaxLen + 3; ++i)
           i_currentPos += EBTTR_DBG_PRINTF(" ");

         /* Print the menu entry formatted and aligned */
         clp_printHelp(p_current->p_help, i_currentPos);

      } /* if */
   } /* for */
} /* clp_menuPrintHelp() */

/*============================================================================*/
/*  clp_outputPrompt()                                                        */
/*============================================================================*/
void clp_outputPrompt (void)
{
  /*
   * Start with a new line
   */
  EBTTR_DBG_PRINTF("\r\n");

  /*
   * Print the current context
   */
  if (gs_clp.p_currMenuParent)
   {
     EBTTR_DBG_PRINTF("%s", gs_clp.p_currMenuParent->p_cmd);
   } /* if */

  EBTTR_DBG_PRINTF("> ");
} /* clp_outputPrompt() */

/*============================================================================*/
/*  clp_outputTitle()                                                         */
/*============================================================================*/
void clp_outputTitle (void)
{
  /* Check if a title was set, otherwise be quiet */
  if (gs_clp.p_menuPrintTitle)
    clp_printFormatted(gs_clp.p_menuPrintTitle);
} /* clp_outputTitle() */

int clp_output (const char *format, ...)
{
  int i_ret = 0;
  char buf[250];
  va_list argList;

  va_start(argList, format);
  i_ret = vsnprintf( buf, 250, format, argList );
  va_end(argList);

  bsp_uartTx( gs_clp.p_uart, (uint8_t *)buf, i_ret );

  return i_ret;
}
#endif /* #if (HAL_SUPPORT_UART == TRUE) */
