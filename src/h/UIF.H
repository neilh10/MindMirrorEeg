/*
  uif.h

  Defines interfaces for UIF modules
*/
 enum UIFE_SCREEN_STATE {
  /* User Interface display status */
  UIFE_SCREEN_ACTIVE,  /* Normal displays enabled               */
  UIFE_SCREEN_FROZEN,  /* No updates to be performed on display */
  UIFE_SCREEN_STATE_INVALID
 };
 enum UIFE_MODE {UIFE_DISPLAY,    /* Displaying output of filters */
                 UIFE_REVIEW,     /* Reviewing stored data        */
                 UIFE_REVIEW_PLAY,/* Reviewing stored data        */
                 UIFE_OPTS,       /* Presenting options to user   */
                 UIFE_TEST,       /* Self test mode               */
                 UIFE_DEMO,       /* Demonstration mode           */
                 UIFE_INVALID};
 enum UIFE_TEST_MODE { UIFE_TEST_CONTACTS, UIFE_TEST_SELF,UIFE_TEST_MODE_INVALID};

 enum UIFE_OPTS_RET {
    UIFC_NO_ACTION, /* No Action on return */
    UIFC_END_MENUS  /* Menu indicated return from menuiing */
 };

 enum UIFE_OPTS {
    UIFE_OPTS_NEW,     /* Draw menu screen */
    UIFE_OPTS_END,     /* Leave menuing screens */
    UIFE_OPTS_UP_ITEM,  /* Move up one item   */
    UIFE_OPTS_DOWN_ITEM, /* Move down one item */
    UIFE_OPTS_ACTION     /* Take action on this menu item */
 };

 enum UIFE_DEMO_OPTIONS {
              UIFE_DEMO_START,
              UIFE_DEMO_FINISH,
              UIFE_DEMO_FORWARD,
              UIFE_DEMO_BACK
 };

/* enum UIFE_MONTAGE {
      UIFC_LATERAL,     /* Bipolar montage red-black on left
                         *                 green-grey on right   *!/
      UIFC_RED_GREEN,   /* Monopolar montage red-blue on left
                         *                   green-blue on right *!/
      UIFC_BLACK_GREY   /* Monopolar montage black-blue on left
                         *                   grey-blue on right  *!/
 };
*/

/*
 * Prototypes
 */
#ifndef MMIR_PC
void uif_poll(enum sche_entry sch_type);

/* Internal uif prototypes */
void update_status(void);
void uif_display_pattern_initialisation(Initt_system_state *ssp);
void uif_display_pattern(enum UIFE_DEMO_OPTIONS options);
void uif_opts_initialisation(Initt_system_state *ssp);
void uif_revw_initialisation(Initt_system_state *ssp);
enum UIFE_OPTS_RET uif_opts(enum UIFE_OPTS action);

enum UIFE_OPTS_RET uif_opts_entry(enum key_pressed_enum key);
void uif_set_mode_demo(void);
void uif_upld(void);
void uif_misc(void);
void review_play_action(void);
void review_play_incr(void);
void review_play_decr(void);

#endif





