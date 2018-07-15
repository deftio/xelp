/**
 begin parser state machine model.
 */
#define _EF_TS     (0x01) /* set token 0 start (1st token from buf start) */
#define _EF_TE     (0x02) /* set token 0 end */
#define _EF_LE     (0x04) /* set line end */
#define _EF_BS     (0x08) /* set bracket start */
#define _EF_BE     (0x10) /* set bracket end */

#define _PS_SEEK   (0x00)  /* seek next token0 */
#define _PS_ESCA   (0x01)  /* esc sequence */
#define _PS_TOK0   (0x02)  /* token0 (the command / operator) */
#define _PS_CMNT   (0x03)  /* single-line comment */
#define _PS_SEOL   (0x04)  /* seek end-of-line */
#define _PS_QUOT   (0x05)  /* quoted string */
#define _PS_QESC   (0x06)  /* quoted esc char */
#define _PS_QEND   (0x07)  /* quoted end */
#define _PS_PREV   (0x08)  /* use previous state (spec case) */
#define _PS_BRKT   (0x09)  /* bracket */
#define _PS_EOS    (0x0a)  /* end of table.  actually never used */

static const char gPSMStates[109]= {
/* _PS_SEEK */ ' '             ,0                ,_PS_SEEK, /*space is token separator                        */
/* _PS_SEEK */ '\t'            ,0                ,_PS_SEEK, /*tab is also token sep                           */
/* _PS_SEEK */ '\n'            ,0                ,_PS_SEEK, /*newline is token sep                            */
/* _PS_SEEK */ ';'             ,0                ,_PS_SEEK, /*; don't bother with termi if no tokn started    */
/* _PS_SEEK */ 'XELP_CLI_ESC'  ,0                ,_PS_ESCA, /*enter CLI escape mode                           */
/* _PS_SEEK */ '#'             ,0                ,_PS_CMNT, /*enter single line comment                       */
/* _PS_SEEK */ '\"'            ,_EF_TS           ,_PS_QUOT, /*enter quoted string token                       */
/* _PS_SEEK */ '['             ,_EF_TS | _EF_BS  ,_PS_BRKT, /*enter bracket inc bracket level count           */
/* _PS_SEEK */  0              ,_EF_TS           ,_PS_TOK0, /*default .. enter token                          */
/* _PS_ESCA */  0              ,0                ,_PS_PREV, /*any char retrns from esc state to pre state     */
/* _PS_BRKT */ 'XELP_CLI_ESC'  ,0                ,_PS_ESCA, /*enter CLI escape mode                           */
/* _PS_BRKT */ '['             ,_EF_BS           ,_PS_BRKT, /*enter bracket, inc bracket level count          */
/* _PS_BRKT */ ']'             ,_EF_BE           ,_PS_PREV, /*decrement bracket                               */
/* _PS_BRKT */  0              ,0                ,_PS_BRKT, /*eat up chars in side bracket                    */
/* _PS_TOK0 */ ' '             ,_EF_TE           ,_PS_SEOL, /*end of 1st token                                */
/* _PS_TOK0 */ '\t'            ,_EF_TE           ,_PS_SEOL, /*end of 1st token                                */
/* _PS_TOK0 */ '#'             ,_EF_TE | _EF_LE  ,_PS_CMNT, /*end of line due to commnt, aslo end of token    */
/* _PS_TOK0 */ ';'             ,_EF_TE | _EF_LE  ,_PS_SEEK, /*end of tok, terminator end of line              */
/* _PS_TOK0 */ '\n'            ,_EF_TE | _EF_LE  ,_PS_SEEK, /*end of line, end of line                        */
/* _PS_TOK0 */  0              ,0                ,_PS_TOK0, /*keep adding to token                            */
/* _PS_CMNT */ '\n'            ,0                ,_PS_SEEK, /*end of line terminates comment                  */
/* _PS_CMNT */  0              ,0                ,_PS_CMNT, /*keep eating chars until eol reached             */
/* _PS_SEOL */ ';'             ,_EF_LE           ,_PS_SEEK, /*end of statement reached                        */
/* _PS_SEOL */ '\n'            ,_EF_LE           ,_PS_SEEK, /*end of line reached                             */
/* _PS_SEOL */ '#'             ,_EF_LE           ,_PS_CMNT, /*comment start                                   */
/* _PS_SEOL */ 'XELP_CLI_ESC'  ,0                ,_PS_ESCA, /*esc char -- skip next char                      */
/* _PS_SEOL */ '\"'            ,0                ,_PS_QUOT, /*enter quoted str (uses diff esc, exit states)   */
/* _PS_SEOL */  0              ,0                ,_PS_SEOL, /*keep seeking EOL                                */
/* _PS_QUOT */ '\"'            ,0                ,_PS_QEND, /*hit end of quote, go to QEND to advnce 1 char   */
/* _PS_QUOT */ 'XELP_QUO_ESC'  ,0                ,_PS_QESC, /*handle esc inside quoted str                    */
/* _PS_QUOT */  0              ,0                ,_PS_QUOT, /*keep going thru quoted string                   */
/* _PS_QESC */  0              ,0                ,_PS_QUOT, /*skip over next char (esc'd)                     */
/* _PS_QEND */ '#'             ,_EF_TE | _EF_LE  ,_PS_CMNT, /*exit quote in to comment                        */
/* _PS_QEND */ ';'             ,_EF_TE | _EF_LE  ,_PS_SEEK, /*exit quote with terminal                        */
/* _PS_QEND */ '\n'            ,_EF_TE | _EF_LE  ,_PS_SEEK, /*exit quote at end of line                       */
/* _PS_QEND */  0              ,_EF_TE           ,_PS_SEOL, /*exit quote                                      */
                _PS_EOS
};

char gPSMJumpTable[9]= {
 0,/* _PS_SEEK */
 27,/* _PS_ESCA */
 30,/* _PS_BRKT */
 42,/* _PS_TOK0 */
 60,/* _PS_CMNT */
 66,/* _PS_SEOL */
 84,/* _PS_QUOT */
 93,/* _PS_QESC */
 96 /* _PS_QEND */
};
