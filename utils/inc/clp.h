#ifndef __CLP_H__
#define __CLP_H__
#ifndef __DECL_CLP_H__
#define __DECL_CLP_H__ extern
#endif

/*============================================================================*/
/*! \file   clp.h

    \author (c) by STZEDN, Heitersheim, Germany, http://www.stzedn.de

    \brief  Command line parser and menu generator.

  \version  1.0.1
*/
/*============================================================================*/

/*==============================================================================
                                 INCLUDE FILES
==============================================================================*/



/*==============================================================================
                                     MACROS
==============================================================================*/

#ifndef TRUE
#define TRUE                            1
#endif

#ifndef FALSE
#define FALSE                           0
#endif

#ifndef CLP_COMMAND_LINE_LEN
//! Default command line length is 80 characters
#define CLP_COMMAND_LINE_LEN 80
#endif

#ifndef CLP_PRINT_MARGIN
//! Default print margin for menu output
#define CLP_PRINT_MARGIN 80
#endif

#ifndef DBG_CLP
//! By default this module does not output any debugging info
#define DBG_CLP FALSE
#endif

#ifndef STRTOK_R
//! Use the standard C-library string tokenizer.
#define STRTOK_R strtok_r
#endif

#ifndef CLP_HAVE_STRTOK_R
//! By default it is assumed that strtok_r() is available from the standard libraries
#define CLP_HAVE_STRTOK_R   TRUE
#endif

#ifndef CLP_STRCASECMP
//! Use the standard C-library string case comparator.
#define CLP_STRCASECMP strcasecmp
#endif

#ifndef CLP_HAVE_STRCASECMP
//! By default it is assumed that strcasecmp() is available from the standard libraries
#define CLP_HAVE_STRCASECMP   TRUE
#endif

#ifndef CLP_SORT_ALPHA
//! By default will all entries be sorted alphabetically
#define CLP_SORT_ALPHA FALSE
#endif

#ifndef CLP_ALIGNE_LINE_WIDTH
/*!
 * If alignment commands are used then the alignment is based on this size
 * of the line
 */
#define CLP_ALIGNE_LINE_WIDTH 80
#endif

/*!
 * Format tag to center align text on a line. The tag must be at the beginning
 * of the line´
 */
#define CLP_ALIGN_CENTER "$center"

/*==============================================================================
                                     ENUMS
==============================================================================*/


/*==============================================================================
                         STRUCTURES AND OTHER TYPEDEFS
==============================================================================*/
/*!
 *  Type of a CLP menu entry
 *
 * \sa S_CLP_MENU_ENTRY_T
 */
typedef struct S_CLP_MENU_ENTRY_T s_clp_menuEntry_t;

/*!
 * Menu entry call-back function type.
 *
 * When a menu entry was selected then this function is called with the
 * specified parameters
 */
typedef void (*pfn_clp_menuCallBack_t)(int i_argNum, const char* p_args[]);

/*!
 * Context based menu call-back function.
 *
 * This is an alternative call-back function when a context is given.
 */
typedef void (*pfn_clp_menuCallBackCtx_t)(int i_argNum, const char* p_args[], void *p_ctx);

/*!
 * Menu entry structure.
 */
struct S_CLP_MENU_ENTRY_T {
   //! Next element in the linked list
   s_clp_menuEntry_t* p_next;

   //! Pointer to the parent of this entry
   s_clp_menuEntry_t* p_parent;

   //! Menu entry command
   const char* p_cmd;

   //! Help string
   const char* p_help;

   //! Function to call when this menu entry is selected.
   pfn_clp_menuCallBack_t pfn_menuCallBack;

   //! Context based call-back
   pfn_clp_menuCallBackCtx_t pfn_menuCallBackCtx;
   //! Context to pass to the context based call-back
   void* p_ctx;
};


/*==============================================================================
                          GLOBAL VARIABLE DECLARATIONS
==============================================================================*/

/*==============================================================================
                         FUNCTION PROTOTYPES OF THE API
==============================================================================*/

/*============================================================================*/
/*!
    \brief   Initialisation of the command line parser.

             Reset all internal variables and clear the entire menu. Any
             menu items must be added after.
*/
/*============================================================================*/
void clp_init(void *p_uart);

/*============================================================================*/
/*!
    \brief   Enable/disable the echo.

             If echo is enabled, any character received with clp_rxChar() is
             sent back to the connected console.

    \param   i_enable 0 to disable, any other value enables echo.

    \return  The value that was previously set.
*/
/*============================================================================*/
int clp_echoEnable (int i_enable);

/*============================================================================*/
/*!
    \brief   Set the title that will be output begfore the menu structure is
             displayed.

             Allows a title line to be displayed before the menu structure is
             displayed.

             An alignment tag is allowed at the beginning of each line.

    \param   p_title  Pointer to a zero terminated string as title.


    \sa clp_menuPrint()
    \sa CLP_ALIGN_CENTER
*/
/*============================================================================*/
void clp_setTitle (const char* p_title);

/*============================================================================*/
/*!
    \brief   Initialize a menu entry and add it to the list of entries.

    \param   p_entry  Pointer to the structure of the menu entry
    \param   p_parent Parent of this menu entry allowing to enable a
                      hierarchical menu structure. If set to NULL the entry
                      will be added at root level.
    \param   p_cmd    The command that is assigned to this entry. This must
                      be unique in the current hierarchical context. It is
                      not allowed to define multiple commands.

    \param   p_help   A help message. Can be left to NULL
    \param   pfn_callBack Call-back function for this menu entry. If left to
                          NULL, then the entry is a hierarchical entry.

    \return  0 on success
    \return  -1 on error. Possible error sources are:
                 - the command is already in the list
                 - the pointer p_cmd is set to NULL
*/
/*============================================================================*/
int clp_menuAddEntry(s_clp_menuEntry_t*      p_entry,
                     s_clp_menuEntry_t*      p_parent,
                     const char*             p_cmd,
                     const char*             p_help,
                     pfn_clp_menuCallBack_t  pfn_callBack);

/*============================================================================*/
/*!
    \brief   Alternative call-back function with an additional context.

             The menu entry must be added prior with clp_menuAddEntry(). Then
             this function can be used to set the context based call-back
             function.

    \param   p_entry       Entry that is already in the list.
    \param   pfn_callBack  Context based call-back function
    \param   p_ctx         Pointer to the context of this function.

    \return  0 on success
    \return  -1 on error. Possible error sources are:
                 - the menu entry is not in the list yet
*/
/*============================================================================*/
int clp_menuSetCtxBasedCB (s_clp_menuEntry_t* p_entry, pfn_clp_menuCallBackCtx_t pfn_callBack, void* p_ctx);


/*============================================================================*/
/*!
    \brief   Add a character to the command line buffer.

             This function is used to add characters as received e.g. over
             the UART and add it to the buffer. Note that if a command line
             is received to the end (the current character is the last of
             the line) command line parsing starts and if it is a valid
             line then the corresponding function is called. It might not
             be safe to do this out of an interrupt.

    \param   c_char The currently received character.
*/
/*============================================================================*/
void clp_rxChar (const char c_char);

/*============================================================================*/
/*!
    \brief   Print the entire menu including the menu title.

             If a title is given using clp_setTitle(), then the title is
             printed followed by the entire menu structure.

    \sa clp_setTitle()
*/
/*============================================================================*/
void clp_menuPrint(void);

/*============================================================================*/
/*!
    \brief   Print the menu commands, optionally relative to a parent entry.

    \param   p_parent Pointer to a parent entry. If set and the entry is part
                      of the list, then this entry including all entries with
                      this entry as parent pointer are displayed. Useful when
                      displaying the help for a certain context.
*/
/*============================================================================*/
void clp_menuPrintHelp (const s_clp_menuEntry_t* p_parent);

/*============================================================================*/
/*!
    \brief   Print the prompt of the command line.

             The prompt of the command line is typically a ">". If the menu
             is in a context then the context is prompted as well.
             The prompt starts with a new line first.

*/
/*============================================================================*/
void clp_outputPrompt (void);

/*============================================================================*/
/*!
    \brief   Print the menu title if set.

             Print out the title that was set by calling clp_setTitle().

    \sa      clp_setTitle()
*/
/*============================================================================*/
void clp_outputTitle (void);

/*============================================================================*/

int clp_output (const char *format, ...);


/*============================================================================*/
#endif /* __CLP_H__ */
