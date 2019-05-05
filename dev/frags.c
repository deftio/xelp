
//Piece from XELPParse
#ifdef XELP_ENABLE_LCORE			
			if (0==(f->mpCmd)) {  /* at this moment t0,t1,t2 are t0k0-start, tok0-end, line-end */
				c = XELP_L_Cmds; i=0;  
				while(*c) {
					if (XELP_S_OK == XELPStrEq(t0,(int)(t1-t0),*c)){
						XELPTokLine(t1,t2,&t0,&t1,0,XELP_TOK_ONLY);// get next token to pass to cmd
						/*
						do built-in command stuff here.  remember we have buf, cur position, and ths context available
						here in this fn already. We've consumed the 1st tokn in the line so we don't have
						to do that again by calling another fn ptr etc so this save space in very small targets
						*/
						switch(i) {
							case 0:   /* peek   <addr>  */
								i = XELPStr2Int(t0,(int)(t1-t0));
								{
									t = t+i;
									j=1;
									if (XELP_S_OK == XELPTokLine(t1,t2,&t0,&t1,0,XELP_TOK_ONLY) ) 
										j = XELPStr2Int(t0,(int)(t1-t0));
									while (j--) {
										_PUTC(*t++);
										if (!(j%0xf))
											_PUTC('\n');
											
									}
								}	
											
								break;
							case 1:   /* poke  <addr> <byte_value> */
								i = XELPStr2Int(t0,(int)(t1-t0));
								{
									if (XELP_S_OK == XELPTokLine(t1,t2,&t0,&t1,0,XELP_TOK_ONLY) ) {
										j = XELPStr2Int(t0,(int)(t1-t0));
										t = t+i;
										*t=j&0xff;
									}
								}
								break;
							/*
							case 2:	  / * go     * /
								j++;
								//i++;
								break;
							case 3:   / *  if     * /
								j--;
								//i--;
								break;
							*/
						}
						
						_PUTC('\n');
						break;
					}
					c++; 
					i++;
				}				
			}
#endif 	/* XELP_ENABLE_LCORE */	

/*****************************/
/*
 This is the original tokenzier / line parser.
 It works fine but its been replaced by the new one which doesn't use [ *buf, blen] ==> buffer ptr, buffer len 
 with XelpBuf style parser which uses startPtr, posPtr,endPtr 
 this later style is easier to use in successive calls by functions which are "consuming" a line

 */
XELPRESULT XELPTokLineOld (const char *buf, int blen, const char **t0s, const char **t0e, const char **eol, int srchType) {
 	const char *s;		 /* state ptr */
	char cs=_PS_SEEK,prev=_PS_SEEK,tmp;   
	int tm=1; /*  (token mode) allows capture of t0e, t0s only for first token seen */

	while (blen--) {
		s = gPSMStates+(int)(gPSMJumpTable[(unsigned int)cs]);//index in to state array quickly
		/* while (*s != _PS_EOS) { //technically can be while(1) since each state _MUST_ have a default */
		while (1) { 
			if ( ( 0 == *s) || (*buf == (*s)) )// default in this state or char is match
				break;	
			s+=3; //goto next iteration in this state.  
		}	/* now we've found the correct state.  do any actions */

		s++; /* advance ptr to exec flags byte */
		/* if (*s)		// if there are any exec flags.. technically not needed but it can speed things up */
		{ 
			if (tm) {
				if ((*s) & _EF_TS) { *t0s =  buf; };
				if ((*s) & _EF_TE) { *t0e =  buf;  if (XELP_TOK_ONLY == srchType) return XELP_S_OK; tm=0;};
			}
			if ((*s) & _EF_LE) { *eol  = buf; return XELP_S_OK;};
		}
		s++; /* advance ptr to next_state byte */
		tmp = cs;
		cs = (*s == _PS_PREV) ? prev : (*s);
		prev = tmp;
		/* end of parser state update */

		buf++; /* advance char ptr */
	}
	return XELP_S_NOTFOUND;
}

/**********************************************************
 DIOSCIPFuncMapEntry declares functions that provide "langauge" features
  such as _if, _wh, _go, _pshi, _popi, _inc _dec
  most of these require parser-context (such as _go)
  */
typedef struct
{
	void* mpThs; //this is cast back to a DIO* in actual use
	char* mpCmd;
	char* mpHelpString;
	DIORESULT (*mFunPtr)(const char *pArgString, int maxbuflen);	
}DIOSCIPFuncMapEntry; 

// inside of DIO struct
	//if compiled in, searches built in library functions
	DIOSCIPFuncMapEntry		*mpSCIPFns;
/* 
	DIOParse()
	attempts to parse and execute as many commands as possible.
	(working version)
 */
/*
DIORESULT DIOParseOld (DIO* ths, const char *buf, int blen) {
	const char *p = buf;
 	char cs=_PS_SEEK,prev=_PS_SEEK,tmp;
    const char *s;		//state pointer
	const char *t0s=0, *t0e=0,*el=0;
	int tm=1; //tm (token mode) allows capture of t0e, t0s only for first token seen
	DIOCLIFuncMapEntry *f;

	while (blen--) {
		//printw("%d",(int)cs);
		s = (const char *)gDIOPSMStates+(int)(gDIOPSMJmpTable[(unsigned int)cs]);//index in to state array quickly
		//while (*s != _PS_EOS) { //technically can be while(1) since each state _MUST_ have a default
		while (1) { //technically can be while(1) since each state _MUST_ have a default
			if ( ( 0 == *s) || (*p == (*s)) )// char match
				break;	
			s+=3; //goto next iteration in this state.  
		}
		//now we've found the correct state.  do any actions
		s++; // advance ptr to exec flags byte 
		if (*s)		{ // if there are any exec flags.. 
			if ((*s) & _EF_TS) { t0s = (tm==1) ? p : t0s; };
			if ((*s) & _EF_TE) { t0e = (tm==1) ? p : t0e; tm=0; };
			if ((*s) & _EF_LE) { el  = p; };
			if ((*s) & _EF_EX) { 
				tm=1;
				f=ths->mpCLIModeFuncs;
				while(f->mpCmd) {
					if (DIOStrCmp(t0s,(int)(t0e-t0s),f->mpCmd)==DIO_S_OK) {
						(f->mFunPtr)(t0s,(int)(el-t0s));//TODO note error handling should use mp->Err
						break;
					}
					f++;
				}
			}
		}
		//now advance to next state
		s++; // advance ptr to next_state byte
		tmp = cs;
		cs = (*s == _PS_PREV) ? prev : (*s);
		prev = tmp;
		//end of parser state update

		p++; //advance char ptr
	}
	return DIO_S_OK;
}
*/
/**
 * DIONextTok() finds next token [start, end] in bufferusing same parser state machine as Parse
 (working version)
 */
/*
DIORESULT DIONextTokOld(const char *buf, int blen,const char **tok_s, const char **tok_e) {
	const char *p = buf;
	char cs=_PS_SEEK,prev=_PS_SEEK,tmp;
    const char *s;		//state pointer

	while (blen--) {
		s = (const char *)gDIOPSMStates+(int)(gDIOPSMJmpTable[(unsigned int)cs]);//index in to state array quickly
		//while (*s != _PS_EOS) { //technically can be while(1) since each state _MUST_ have a default
		while(1){
			if ( (0 == *s) || (*p == (*s)) )// char match
				break;	
			s+=3; //goto next iteration in this state.  
		}
		s++; //
		//printw("[%c:%d]",*p,(int)(cs));
		if ((*s) & _EF_TS) { *tok_s = p;};
		if ((*s) & _EF_TE) { *tok_e = p; break;};
		//now advance to next state
		s++; // advance ptr to next_state byte
		tmp = cs;
		cs = (*s == _PS_PREV) ? prev : (*s);
		prev = tmp;
		//end of parser state update

		p++; //advance char ptr
	}
	if (*tok_e == p)
		return DIO_S_OK;
	return DIO_S_NOTFOUND;
}
*/
/**
	DIOParseKey() 
	live command line handling. 
	first looks for mode-switches.
	then if in single-key mode looks up single key.
	then if in command mode looks for <ENTER> and the attempts to parse current buffer.

*/
DIORESULT DIOParseKey (DIO *ths, char key)
{
	int i=ths->mCurMode;
	
	//first we test to see if we should switch modes.  this is a "key" difference  btw 
	//just submitting a buffer to be parsed and running at command line
	/*
	if ((key == ths->mKeyCLI) &&  (ths->mpCLIModeFuncs)) {
		ths->mCurMode = DIO_MODE_CLI;
	}
	if ((key == ths->mKeyKEY) &&  (ths->mpKeyModeFuncs)) {
		ths->mCurMode = DIO_MODE_KEY;
	}
	if ((key == ths->mKeyTHR) &&  (ths->mpfPassThru)) {
		ths->mCurMode = DIO_MODE_THR;
	}
	*/
	switch (key) {
		case DIOKEY_CLI:
			ths->mCurMode = (ths->mpCLIModeFuncs) ? DIO_MODE_CLI : i;
			break;
		case DIOKEY_KEY:
			ths->mCurMode = (ths->mpKeyModeFuncs) ? DIO_MODE_KEY : i;
			break;
		case DIOKEY_THR:
			ths->mCurMode = (ths->mpfPassThru)	  ? DIO_MODE_THR : i;
			break;
		default:
			break;
	}
	
	if (ths->mCurMode != i) {
		if (ths->mpfEditModeChg)
			ths->mpfEditModeChg(ths->mCurMode);
	}
	else {
		switch(ths->mCurMode) {
			case DIO_MODE_KEY:
				DIOExecKC(ths,key);
				break;
			case DIO_MODE_THR:
				if (ths->mpfPassThru)
					ths->mpfPassThru(key);
				break;
			default: //DIO_MODE_CLI
				if ((key == DIOKEY_BKSP) && (ths->mCmdMsgIndex)) {
					ths->mCmdMsgIndex --;
					if(ths->mpfBksp)
						ths->mpfBksp();
				}
				else {
					_PUTC(key);
					if (key == DIOKEY_ENTER )	{
						ths->mCmdMsgBuf[ths->mCmdMsgIndex]='\n'; //'\n' is used by the parser
						ths->mCmdMsgIndex++;
						DIOParse(ths,ths->mCmdMsgBuf,ths->mCmdMsgIndex);
						ths->mCmdMsgIndex=0; // reset after command attempt
					}
					else {
						if (ths->mCmdMsgIndex<(DIO_CMDBUFSZ-2) ) 	{ //make sure still be able to press ENTER even if buf full
							ths->mCmdMsgBuf[ths->mCmdMsgIndex]=key;
							ths->mCmdMsgIndex++;
						}
					}
				}
		}
	}
	/*
	if (DIO_MODE_KEY == ths->mCurMode) { // in single-key mode
		DIOExecKC(ths, key);
	}

	if (DIO_MODE_THR == ths->mCurMode) { // in pass thru mode
		if (ths->mpfPassThru)
			ths->mpfPassThru(key);
	}
	else {	
		if ((key == DIOKEY_BKSP) && (ths->mCmdMsgIndex)) {
			ths->mCmdMsgIndex --;
			if(ths->mpfBksp)
				ths->mpfBksp();
		}
		else {
			_PUTC(key);
			if (key == DIOKEY_ENTER )	{
				ths->mCmdMsgBuf[ths->mCmdMsgIndex]='\n'; //'\n' is used by the parser
				ths->mCmdMsgIndex++;
				DIOParse(ths,ths->mCmdMsgBuf,ths->mCmdMsgIndex);
				ths->mCmdMsgIndex=0; // reset after command attempt
			}
			else {
				if (ths->mCmdMsgIndex<(DIO_CMDBUFSZ-2) ) 	{ //make sure still be able to press ENTER even if buf full
					ths->mCmdMsgBuf[ths->mCmdMsgIndex]=key;
					ths->mCmdMsgIndex++;
				}
			}
		}
	}
	*/
	return DIO_S_OK;
}

