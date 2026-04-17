/*


make state-machine table for parser

gcc make_sm_tab.c -o make_sm_tab.out 

this program was added for the open-source release to allow easier changes to the parser statemachine.

it still uses the "old school" hard coded way as oppossed to specifying a grammar and using a true parser generator.

good luck!

-manu

|cur_state	| inpt char set | next_state	| actions on transition         	| notes 												    |
|-------------------------------------------------------------------------------------------------------------------------------------------|
|_PS_SEEK	|  " \t\n;:"	|	_PS_SEEK	|  									| any whitespace etc										|
|			|  '^'			| 	_PS_ESCA	|  pre_state=cur_state				| enter parser esc											|
|			|  '#'			|   _PS_CMNT	|									| enter single line comment									|
|			|  *     		|	_PS_TOK0	| _t0s = p							| really any letter/number but that's too expensive			|
|_PS_ESCA	|  *	   		|   pre_state	|   								| allows escaping of special chars ";#\n" etc				|
|_PS_TOK0	|  " \t"		| 	_PS_SEOL	| _t0e = p, 						| end of 1st token, look for end of line  					|
|			|  "#"			|   _PS_CMNT    | _t0e = p, _cl=p, exec()			| ends now due to comment									|
|			|  ";\n"		|	_PS_SEEK	| _t0e = p, _cl=p, exec()    		| 															|
|			|  *			|	_PS_TOK0	|   								| keep adding to token length								|
|_PS_CMNT	|  "\n"			| 	_PS_SEEK 	|  									|															|
|			|  *			|   _PS_CMNT 	|  									| eat up rest of line										|
|_PS_SEOL	|  ";\n"		|	_exec		| _cl=p, exec()						| end of line, so we got all the args						|
|			|  "#"			|	_PS_CMNT	| _cl=p, exec() 					| enter single line comment									|
|			|  "^"			|   _PS_ESCA	| pre_state = cur_state				|															|
|			| "\""   		|   _PS_QUOT    |                                   | see's a quote start										|	
|			|  *		    |   _PS_SEOL    |                                   | seek end of line											|
|_PS_QUOT	| "\""			|	_PS_SEOL 	|   								| end of quote 												|
| 			| "\\"			|	_PS_QESC	|									| esaped in q quote (a string esc not a scip esc)			|
|			|  *            |   _PS_QUOT    |									| any other char is in the quoted str 						|
|_PS_QESC	|  *			|   _PS_QUOT	|									| only allowed thing is to skip over this char 				|

*/

#include <stdio.h>
#include <string.h>

//+++
//exec_flags
#define _EF_PS   		(0x01)	//save current state before state transition
#define _EF_TS			(0x02)	//set t0s
#define _EF_TE			(0x04)  //set t0e
#define _EF_CL			(0x08)  //set cl
#define _EF_EX   		(0x10)	//exec cmd
#define _EF_TN			(0x20)  //inc token counter

//SCIP parser states
#define _PS_SEEK		(0x00)	//seek next token0
#define _PS_ESCA		(0x01)	//esc sequence
#define _PS_TOK0        (0x02)	//token0 the command/operator
#define _PS_CMNT        (0x03)	//comment
#define _PS_SEOL        (0x04)	//seek end of line
#define _PS_QUOT		(0x05)	//quoted string
#define _PS_QESC		(0x06)	//quoted esc sequence
#define _PS_PREV		(0x0f)  //use prev state
//---

char* gSCIPdefines= "                                                           \n \
//exec_flags		                                                            \n \
#define _EF_PS          (0x01)	//save current state before state transition	\n \
#define _EF_TS          (0x02)	//set t0s                                       \n \
#define _EF_TE          (0x04)  //set t0e                                       \n \
#define _EF_CL          (0x08)  //set cl                                        \n \
#define _EF_EX          (0x10)	//exec cmd                                      \n \
#define _EF_TN          (0x20)  //inc token counter                             \n \
//SCIP parser states                                                            \n \
#define _PS_SEEK		(0x00)	//seek next token0                              \n \
#define _PS_ESCA		(0x01)	//esc sequence                                  \n \
#define _PS_TOK0		(0x02)	//token0 (the command/operator)                 \n \
#define _PS_CMNT		(0x03)	//comment                                       \n \
#define _PS_SEOL		(0x04)	//seek end of line                              \n \
#define _PS_QUOT		(0x05)	//quoted string                                 \n \
#define _PS_QESC		(0x06)	//quoted esc sequence                           \n \
#define _PS_PREV		(0x0f)  //use prev state                                \n \
";

typedef struct {
	char cs; //cur state
	char ef; //exec flags
	char ns; //next state
	char *d; //input chars to change state
}scip_psm_state;

scip_psm_state gStates[] = {
	//cur state ,  execute flags 	     ,  next state , characters ""=default       
	{_PS_SEEK, 							0,	   _PS_SEEK,  " \t\n;:"			}, // any whitespace etc										
	{_PS_SEEK,  _EF_PS          		 ,     _PS_ESCA,  "^"      			}, // enter escaped char mode
	{_PS_SEEK, 				    		0,     _PS_CMNT,  "#"	   			}, // enter single line comment
	{_PS_SEEK,  _EF_TS                   ,     _PS_TOK0,  ""       		    }, // enter token
	{_PS_ESCA, 							0,	   _PS_PREV,  ""	            }, // any char returns from scip escape mode
	{_PS_TOK0,  _EF_TE                   ,     _PS_SEOL,  " \t"             }, // end of 1st token, look for end of line
	{_PS_TOK0,  _EF_TE&_EF_CL&_EF_EX     ,	   _PS_CMNT,  "#"	            }, // end of line due to comment, also end of token
	{_PS_TOK0,  _EF_TE&_EF_CL&_EF_EX     ,     _PS_SEEK,  ";\n"             }, // ; or eol, dispatch command and start again
	{_PS_TOK0,                          0,     _PS_TOK0,  ""                }, // keep adding to token length
	{_PS_CMNT,                          0,     _PS_SEEK,  "\n"              }, // end of line terminates comment
	{_PS_CMNT,                          0,     _PS_CMNT,  ""                }, // keep eating chars until eol reached
	{_PS_SEOL,  _EF_CL&_EF_EX            ,     _PS_SEEK,  ";\n"             }, // end of statement or line reached. exec and start seeking again
	{_PS_SEOL,  _EF_CL&_EF_EX            ,     _PS_CMNT,  "#"               }, // comment starts.
	{_PS_SEOL,  _EF_PS                   ,     _PS_ESCA,  "^"               }, // hit esc char
	{_PS_SEOL,  _EF_PS                   ,     _PS_QUOT,  "\""              }, // enter quoted string, which uses different exc, exit states
	{_PS_SEOL,                          0,     _PS_SEOL,  ""                }, // keep seeking end of line
	{_PS_QUOT,                          0,     _PS_SEOL,  "\""              }, // exit quote
	{_PS_QUOT,                          0,     _PS_QESC,  "\\"              }, // c-style escape inside quoted string. only handles single char escapes.
	{_PS_QUOT,                          0,     _PS_QUOT,  ""                }, // keep going through the quoted string
	{_PS_QESC,                          0,     _PS_QUOT,  ""                }  // skip over quote.
};


//state machine vars
/*
ts		//tokn start
te 		//tokn end
end 	//end position of line sent to command
tn 		//token num

*/
void main (void) {
	FILE *fp;

	char x = 0,*p=0,*p2 = "hello there that is fun";
	char z[] = {'a','b','c'};
	printf("table gen\nsizeof(static_full_sm): %d\n",(int)(sizeof(scip_psm_state)));
	printf("sizeof(char): %d, sizeof(char *): %d, sizeof(char *p2=%s): %d\n",(int)(sizeof(x)),(int)(sizeof(p)),p2,(int)(sizeof(p2)));
	printf("sizeof(psm_states):  %d\n\n",(int)(sizeof(psm_states)));

	for (x=0; x< sizeof(gStates)/sizeof(scip_psm_state); x++) {
		printf("%2d: %d\t%d\t%d\t%d\n",x,(int)(gStates[x].cs),(int)(gStates[x].ef),(int)(gStates[x].ns),(int)(strlen(gStates[x].d)));
	}
	printf("\n");
	fp = fopen("xelp_table.c","wt"); 


	if (fp) {
		fprintf(fp,"/* XELP Parser table generator\n\n */");

		fprintf(fp,"%s\n",gSCIPdefines);

		for (x=0; x< sizeof(gStates)/sizeof(scip_psm_state); x++) {
			fprintf(fp,"%2d: %d\t%d\t%d\t%d\n",x,(int)(gStates[x].cs),(int)(gStates[x].ef),(int)(gStates[x].ns),(int)(strlen(gStates[x].d)));
		}
		fclose(fp);
	}


}