#!/usr/bin/env python
from __future__ import print_function #just for parenthesis wrapping in python 2

"""
Python based parser state machine generator for xelp

Kona F Pance  <kona (at) deftio (dot) com>

history:
	replaces original C hand coded version.  manu originally wanted it in C but python is just easier.  -kfp

	note these are not general purpose state-machines but specifically designed to work the the compact state
	machine parser in xelp.c

"""
import argparse
import sys
import re
import pprint

"""
Simple parser state machine has these params per state:
[current_state] [action-char] [action-flags] [ next-state]

notes:
	the current-state is not stored.  It is addressed via the companion jump table. 
	this saves storing redundant state info and improves run-time perf by not checking each state.

	each state *MUST* have a default catch-all or the run-time parser loop *WILL* hang.  This is
	accomplished by having an action-char of 0 or '\0'

	execFlags, parserStates are emitted as #defines in C.

"""   

gLineTokStateMachine = { 
	"execFlags" : [
		["_EF_TS" ,	"(0x01)", "set token 0 start (1st token from buf start)"],
		["_EF_TE" ,	"(0x02)", "set token 0 end"                             ],  
		["_EF_LE" ,	"(0x04)", "set line end"                                ],
	],
	"parserStates" : [	 # list of state-names.  must be in desired order. ["state-name","C comment"]
		["_PS_SEEK", "seek next token0"					  ],
		["_PS_ESCA", "esc sequence"    					  ],
		["_PS_TOK0", "token0 (the command / operator)"    ],
		["_PS_CMNT", "single-line comment"				  ],
		["_PS_SEOL", "seek end-of-line"					  ],
		["_PS_QUOT", "quoted string"					  ],
		["_PS_QESC", "quoted esc char"					  ],
		["_PS_QEND", "quoted end"				          ],
		["_PS_PREV", "use previous state (spec case)"	  ],
		["_PS_EOS ", "end of table.  actually never used" ]  #this should be last.
	],
	"transTableName" : "gPSMStates",	
	"transTable" : [	 
		# state     ,  char         , exec             , next_state, C comment                                      
		["_PS_SEEK" ," "  			,                 0, "_PS_SEEK", "space is token separator"                     ],
		["_PS_SEEK" ,"\\t"          ,                 0, "_PS_SEEK", "tab is also token sep"                        ],
		["_PS_SEEK" ,"\\n"          ,                 0, "_PS_SEEK", "newline is token sep"                         ],
		["_PS_SEEK" ,";"            ,                 0, "_PS_SEEK", "; don't bother with termi if no tokn started" ],
		["_PS_SEEK" ,"XELP_CLI_ESC"  ,                0, "_PS_ESCA", "enter CLI escape mode"                        ],
		["_PS_SEEK" ,"#"            ,                 0, "_PS_CMNT", "enter single line comment"                    ],
		["_PS_SEEK" ,"\\\""         ,"_EF_TS"	       , "_PS_QUOT", "enter quoted string token"				    ],
		["_PS_SEEK" ,0			    ,"_EF_TS"	       , "_PS_TOK0", "default .. enter token"                       ],
		["_PS_ESCA" ,0              ,                 0, "_PS_PREV", "any char returns from esc state to pre stte"  ],
		["_PS_TOK0" ," "			,"_EF_TE"		   , "_PS_SEOL", "end of 1st token"	 							],
		["_PS_TOK0" ,"\\t"			,"_EF_TE"		   , "_PS_SEOL", "end of 1st token"	 							],
		["_PS_TOK0" ,"#"			,"_EF_TE | _EF_LE" , "_PS_CMNT", "end of line due to commnt, aslo end of token" ],
		["_PS_TOK0" ,";"			,"_EF_TE | _EF_LE" , "_PS_SEEK", "end of tok, terminator end of line"           ],
		["_PS_TOK0" ,"\\n"			,"_EF_TE | _EF_LE" , "_PS_SEEK", "end of line, end of line"						],
		["_PS_TOK0" ,0				,				  0, "_PS_TOK0", "keep adding to token"							],
		["_PS_CMNT" ,"\\n"			,				  0, "_PS_SEEK", "end of line terminates comment"				],
		["_PS_CMNT" ,0				,				  0, "_PS_CMNT", "keep eating chars until eol reached"			],
		["_PS_SEOL" ,";"  			,"_EF_LE"          , "_PS_SEEK", "end of statement reached"						],
		["_PS_SEOL" ,"\\n" 			,"_EF_LE"          , "_PS_SEEK", "end of line reached"							],
		["_PS_SEOL" ,"#" 			,"_EF_LE"          , "_PS_CMNT", "comment start"								],
		["_PS_SEOL" ,"XELP_CLI_ESC" 	,			  0, "_PS_ESCA", "esc char -- skip next char"					],
		["_PS_SEOL" ,"\\\"" 		,				  0, "_PS_QUOT", "enter quoted str (uses diff esc, exit states)"],
		["_PS_SEOL" ,0 				,				  0, "_PS_SEOL", "keep seeking EOL"								],
		["_PS_QUOT" ,"\\\""			,                 0, "_PS_QEND", "hit end of quote, go to QEND to advnce 1 char"],
		["_PS_QUOT" ,"XELP_QUO_ESC"  ,                0, "_PS_QESC", "handle esc inside quoted str"   			    ], 
		["_PS_QUOT" ,0				,                 0, "_PS_QUOT", "keep going thru quoted string"				],
		["_PS_QESC" ,0              ,                 0, "_PS_QUOT", "skip over next char (esc'd)"                  ],
		["_PS_QEND" ,"#"            ,"_EF_TE | _EF_LE" , "_PS_CMNT", "exit quote in to comment"                     ],
		["_PS_QEND" ,";"            ,"_EF_TE | _EF_LE" , "_PS_SEEK", "exit quote with terminal"						],
		["_PS_QEND" ,"\\n"          ,"_EF_TE | _EF_LE" , "_PS_SEEK", "exit quote at end of line"					],
		["_PS_QEND" ,0              ,"_EF_TE"          , "_PS_SEOL", "exit quote"									],
		],
		"jumpTableName" : "gPSMJumpTable"
}
#*********************************************************************************************************************
gLineTokStateMachineWBrackets = { 
	"execFlags" : [
		["_EF_TS" ,	"(0x01)", "set token 0 start (1st token from buf start)"],
		["_EF_TE" ,	"(0x02)", "set token 0 end"                             ],  
		["_EF_LE" ,	"(0x04)", "set line end"                                ],
		["_EF_BS" , "(0x08)", "set bracket start"                           ],
		["_EF_BE" , "(0x10)", "set bracket end"                             ]
			
	],
	"parserStates" : [	 # list of state-names.  must be in desired order. ["state-name","C comment"]
		["_PS_SEEK", "seek next token0"					  ],
		["_PS_ESCA", "esc sequence"    					  ],
		["_PS_TOK0", "token0 (the command / operator)"    ],
		["_PS_CMNT", "single-line comment"				  ],
		["_PS_SEOL", "seek end-of-line"					  ],
		["_PS_QUOT", "quoted string"					  ],
		["_PS_QESC", "quoted esc char"					  ],
		["_PS_QEND", "quoted end"				          ],
		["_PS_PREV", "use previous state (spec case)"	  ],
		["_PS_BRKT", "bracket"                            ],
		["_PS_EOS ", "end of table.  actually never used" ]  #this should be last.
	],
	"transTableName" : "gPSMStates",	
	"transTable" : [	 
		# state     ,  char         , exec             , next_state, C comment                                      
		["_PS_SEEK" ," "  			,                 0, "_PS_SEEK", "space is token separator"                     ],
		["_PS_SEEK" ,"\\t"          ,                 0, "_PS_SEEK", "tab is also token sep"                        ],
		["_PS_SEEK" ,"\\n"          ,                 0, "_PS_SEEK", "newline is token sep"                         ],
		["_PS_SEEK" ,";"            ,                 0, "_PS_SEEK", "; don't bother with termi if no tokn started" ],
		["_PS_SEEK" ,"XELP_CLI_ESC"  ,                 0, "_PS_ESCA", "enter CLI escape mode"                        ],
		["_PS_SEEK" ,"#"            ,                 0, "_PS_CMNT", "enter single line comment"                    ],
		["_PS_SEEK" ,"\\\""         ,"_EF_TS"	       , "_PS_QUOT", "enter quoted string token"				    ],
		["_PS_SEEK" ,"["            ,"_EF_TS | _EF_BS" , "_PS_BRKT", "enter bracket inc bracket level count"		],
		["_PS_SEEK" ,0			    ,"_EF_TS"	       , "_PS_TOK0", "default .. enter token"                       ],
		["_PS_ESCA" ,0              ,                 0, "_PS_PREV", "any char retrns from esc state to pre state"  ],
		["_PS_BRKT" ,"XELP_CLI_ESC"  ,                 0, "_PS_ESCA", "enter CLI escape mode"                        ],
		["_PS_BRKT" ,"["            ,"_EF_BS"          , "_PS_BRKT", "enter bracket, inc bracket level count"       ],
		["_PS_BRKT" ,"]"            ,"_EF_BE"          , "_PS_PREV", "decrement bracket"                            ],
		["_PS_BRKT" ,0              ,                 0, "_PS_BRKT", "eat up chars in side bracket"                 ],
		["_PS_TOK0" ," "			,"_EF_TE"		   , "_PS_SEOL", "end of 1st token"	 							],
		["_PS_TOK0" ,"\\t"			,"_EF_TE"		   , "_PS_SEOL", "end of 1st token"	 							],
		["_PS_TOK0" ,"#"			,"_EF_TE | _EF_LE" , "_PS_CMNT", "end of line due to commnt, aslo end of token" ],
		["_PS_TOK0" ,";"			,"_EF_TE | _EF_LE" , "_PS_SEEK", "end of tok, terminator end of line"           ],
		["_PS_TOK0" ,"\\n"			,"_EF_TE | _EF_LE" , "_PS_SEEK", "end of line, end of line"						],
		["_PS_TOK0" ,0				,				  0, "_PS_TOK0", "keep adding to token"							],
		["_PS_CMNT" ,"\\n"			,				  0, "_PS_SEEK", "end of line terminates comment"				],
		["_PS_CMNT" ,0				,				  0, "_PS_CMNT", "keep eating chars until eol reached"			],
		["_PS_SEOL" ,";"  			,"_EF_LE"          , "_PS_SEEK", "end of statement reached"						],
		["_PS_SEOL" ,"\\n" 			,"_EF_LE"          , "_PS_SEEK", "end of line reached"							],
		["_PS_SEOL" ,"#" 			,"_EF_LE"          , "_PS_CMNT", "comment start"								],
		["_PS_SEOL" ,"XELP_CLI_ESC" 	,				  0, "_PS_ESCA", "esc char -- skip next char"					],
		["_PS_SEOL" ,"\\\"" 		,				  0, "_PS_QUOT", "enter quoted str (uses diff esc, exit states)"],
		["_PS_SEOL" ,0 				,				  0, "_PS_SEOL", "keep seeking EOL"								],
		["_PS_QUOT" ,"\\\""			,                 0, "_PS_QEND", "hit end of quote, go to QEND to advnce 1 char"],
		["_PS_QUOT" ,"XELP_QUO_ESC"  ,                 0, "_PS_QESC", "handle esc inside quoted str"   			    ], 
		["_PS_QUOT" ,0				,                 0, "_PS_QUOT", "keep going thru quoted string"				],
		["_PS_QESC" ,0              ,                 0, "_PS_QUOT", "skip over next char (esc'd)"                  ],
		["_PS_QEND" ,"#"            ,"_EF_TE | _EF_LE" , "_PS_CMNT", "exit quote in to comment"                     ],
		["_PS_QEND" ,";"            ,"_EF_TE | _EF_LE" , "_PS_SEEK", "exit quote with terminal"						],
		["_PS_QEND" ,"\\n"          ,"_EF_TE | _EF_LE" , "_PS_SEEK", "exit quote at end of line"					],
		["_PS_QEND" ,0              ,"_EF_TE"          , "_PS_SEOL", "exit quote"									],
		],
		"jumpTableName" : "gPSMJumpTable"
}


def emitStateMachineFile(fname, smdata):
	with open(fname, 'w') as f:
		#f.write(str(gLineTokStateMachine));
		p = smdata
		f.write("/**\n begin parser state machine model.\n */\n")
		#first write #defines for exec-flags
		for i in range(len(p["execFlags"])):
			f.write("#define "+p["execFlags"][i][0]+"     "+p["execFlags"][i][1]+ " /* " + p["execFlags"][i][2] + " */\n")
		f.write("\n")

		#write out the #defines for the parser states
		for i in range(len(p["parserStates"])):
			f.write("#define "+p["parserStates"][i][0]+"   (0x"+'%02x'%i +")  /* " + p["parserStates"][i][1] + " */\n")
		f.write("\n")

		#write out table for the state machine transitions.
		#todo add error checking such as missing default (0) clause in a state
		f.write("static const char "+ p["transTableName"]+"["+ str(len(p["transTable"])*3+1)+ "]= {\n")
		sz = 0
		ez = 0
		cz = 0
		for i in range(len(p["transTable"])):
			#print("i " + str(i) + " : " + str(p["transTable"][i][0]))
			if len(str(p["transTable"][i][1])) > sz:
				sz = len(str(p["transTable"][i][1]))
			if len(str(p["transTable"][i][2])) > ez:
				ez = len(str(p["transTable"][i][2]))
			if len(str(p["transTable"][i][4])) > cz:
				cz = len(str(p["transTable"][i][4]))
		sz += 2
		ez += 2
		cz += 2
		for i in range(len(p["transTable"])):
			r = p["transTable"][i]
			#f.write(str(r)+"\n")
			s = (sz-len(str(r[1])))*" "
			e = (ez-len(str(r[2])))*" "
			c = (cz-len(str(r[4])))*" "
			if ((str(r[1])[0:3] == "XELP")  or (r[1]==0)):
				q=" "
			else:
				q="'"
			f.write("/* "+r[0]+" */ "+q + str(r[1])+q+s + "," + str(r[2]) +e+ "," + r[3] + ", /*"  + r[4] +c+" */\n")
		f.write("                _PS_EOS\n")
		f.write("};\n\n")

		#write out jump table
		
		state = ""
		jumpt = []
		for i in range(len(p["transTable"])):
			r = p["transTable"][i]
			if r[0] != state:
				jumpt.append( [r[0], str(i*3)])
			state = r[0]
		f.write("char "+p["jumpTableName"]+"[" +str(len(jumpt))+"]= {\n")
		k = 0
		for i in range(len(jumpt)):
			if k< (len(jumpt)-1):
				com = ","
			else:
				com = " "
			k += 1
			f.write(" " + jumpt[i][1] + com + "/* "+ jumpt[i][0] + " */\n")
		f.write("};\n")

def main():
	parser = argparse.ArgumentParser(description='deftio xelp parser statemachine builder')
	parser.add_argument('-p','--psm_tables', help='output file for basic parser state table', required=False, default="xelp_psm_tables.c")
	parser.add_argument('-b','--psm_w_brkts', help='output file parser state table with brackets support', required=False,default="xelp_psm_tables_brkts.c")

	args = vars(parser.parse_args())

	#print("xelp state-machine parser table generator\n")
	emitStateMachineFile(args["psm_tables"],gLineTokStateMachine)
	emitStateMachineFile(args["psm_w_brkts"],gLineTokStateMachineWBrackets)
if __name__ == '__main__':
    main()
