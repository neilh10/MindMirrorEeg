/*
 menu.h

 */

 enum menu_type {
    MENU_LIST,  /* List of menu elements follows */
    MENU_INFORMATION, /* Information screen follows */
    MENU_TYPE_END
 };

 enum menu_element_type {
   /* MENU_ELEMENT_RETURN, use _MENU /* Go up one menu         */
    MENU_ELEMENT_ROUTINE, /* Address of routine in *addr       */
    MENU_ELEMENT_ROUTINE_EXIT, /* As above but exit at end     */
    MENU_ELEMENT_MENU,     /* Address of another menu in *addr */
    MENU_ELEMENT_END

 };

/************************************************************
 * Structure to define a menu item
 *    type - defines the type of operation to be performed if
 *           the menu item is chosen
 *    *addr - contains an address that is relevant to type
 *    menu_description[] - a character array to be displayed on
 *           the screen. May contain <CR>s.
 *
 * type/addr relationship
 *   if type == MENU_TYPE    then 'addr' points to a menu struct
 *                              to be displayed.
 *   if type == ROUTINE_TYPE then 'addr' points to a routine() to
 *                              be  executed.
 *   else 'addr' points to a variable to store 'type'.
 */
 typedef struct {
    const enum menu_element_type type;  /* definition of addr           */
    const void *addr;        /* were/what to do with 'type'  */
    const char *description; /* to display on screen */
 } tMenuElement;

/************************************************************
 * Structure to define a menu.
 *    Type describes type of menu (eg layout). Set initialisation.
 *   *parent, points to the parent menu. Set when structure read.
 *            if NULL than reached top menu.
 *   *elem - points to a structure describing the menu item.
 *
 * If a menu item is chosen, other than the first menu item, the
 * action defined in the menu_elem struct is performed.
 */
#define MAX_MENU_ELEMENTS 8
 typedef struct menut {
    const enum menu_type type;     /* type of menu    */
   /* const struct menut *parent;    /* where came from */
    const int num_elements;        /* Number of *elem */
    const char *title;             /* title of menu   */
    const tMenuElement *elem[MAX_MENU_ELEMENTS];
 } menu;



