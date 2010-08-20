/* this file autogenerated from qconf.x, do not edit! */
#include "headers.h"

cfgstr_t configtab[CFG_NNN+1]={
	{"address",C_ADDRL,1,NULL,NULL},
	{"aftermail",C_STR,0,NULL,NULL},
	{"aftersession",C_STR,0,NULL,NULL},
	{"alwayskillfiles",C_YESNO,0,NULL,"no"},
	{"alwaysoverwrite",C_STRL,0,NULL,NULL},
	{"asooutbound",C_PATH,2,NULL,NULL},
	{"autohold",C_STRL,0,NULL,NULL},
	{"autoskip",C_STRL,0,NULL,NULL},
	{"autosuspend",C_STRL,0,NULL,NULL},
	{"autoticskip",C_YESNO,0,NULL,"no"},
	{"binkpopt",C_STR,0,NULL,"MB"},
	{"bsooutbound",C_PATH,2,NULL,NULL},
	{"callonflavors",C_STR,0,NULL,"NDCI"},
	{"cancall",C_STR,0,NULL,"CM"},
	{"chathallostr",C_STR,0,NULL,"\n * Hello %s!\n"},
	{"chatlog",C_PATH,0,NULL,NULL},
	{"chatlognetmail",C_YESNO,0,NULL,"no"},
	{"chattoemail",C_STR,0,NULL,NULL},
	{"clearundial",C_INT,0,NULL,"0"},
	{"defboxflv",C_STR,0,NULL,"H"},
	{"defperm",C_OCT,0,NULL,"644"},
	{"dialdelay",C_INT,0,NULL,"60"},
	{"dialdelta",C_INT,0,NULL,"0"},
	{"dialprefix",C_STR,0,NULL,"ATD"},
	{"dialsuffix",C_STR,0,NULL,"|"},
	{"dirperm",C_OCT,0,NULL,"755"},
	{"domain",C_STR,0,NULL,"fidonet.org"},
	{"emsifreqtime",C_STR,0,NULL,NULL},
	{"emsilog",C_PATH,0,NULL,NULL},
	{"estimatedtime",C_YESNO,0,NULL,"no"},
	{"extrp",C_STR,0,NULL,NULL},
	{"failpolls",C_YESNO,0,NULL,"yes"},
	{"fails_hold_div",C_INT,0,NULL,"0"},
	{"fails_hold_time",C_INT,0,NULL,"0"},
	{"filebox",C_ADRSTRL,0,NULL,NULL},
	{"flags",C_STR,0,NULL,""},
	{"freqfrom",C_STR,0,NULL,"Freq Processor"},
	{"freqsubj",C_STR,0,NULL,"File request report"},
	{"freqtime",C_STR,0,NULL,NULL},
	{"history",C_PATH,0,NULL,NULL},
	{"holdonnodial",C_INT,0,NULL,"0"},
	{"holdonsuccess",C_INT,0,NULL,"0"},
	{"holdout",C_YESNO,0,NULL,"no"},
	{"hrxwin",C_INT,0,NULL,"0"},
	{"hstimeout",C_INT,0,NULL,"60"},
	{"htxwin",C_INT,0,NULL,"0"},
	{"hydracrc16",C_YESNO,0,NULL,"no"},
	{"hydrahdx",C_YESNO,0,NULL,"no"},
	{"hydralogverbose",C_YESNO,0,NULL,"no"},
	{"hydrarh1",C_YESNO,0,NULL,"yes"},
	{"ignorenrq",C_YESNO,0,NULL,"no"},
	{"immonflavors",C_STR,0,NULL,"CI"},
	{"inbound",C_PATH,1,NULL,NULL},
	{"jrxwin",C_INT,0,NULL,"0"},
	{"jtxwin",C_INT,0,NULL,"0"},
	{"killbadpkt",C_YESNO,0,NULL,"no"},
	{"localcp",C_PATH,0,NULL,"none"},
	{"lockdir",C_PATH,0,NULL,"/tmp"},
	{"log",C_PATH,1,NULL,NULL},
	{"loginscript",C_STR,0,NULL,NULL},
	{"loglevels",C_STR,0,NULL,""},
	{"longboxpath",C_PATH,0,NULL,NULL},
	{"longrescan",C_INT,0,NULL,"0"},
	{"mailonly",C_STR,0,NULL,NULL},
	{"mapin",C_STR,0,NULL,NULL},
	{"mapout",C_STR,0,NULL,NULL},
	{"mappath",C_STRL,0,NULL,NULL},
	{"masterlog",C_PATH,3,NULL,NULL},
	{"max_fails",C_INT,0,NULL,"20"},
	{"maxrings",C_INT,0,NULL,"0"},
	{"maxsession",C_INT,0,NULL,"0"},
	{"mincpsdelay",C_INT,0,NULL,"10"},
	{"mincpsin",C_INT,0,NULL,"0"},
	{"mincpsout",C_INT,0,NULL,"0"},
	{"minspeed",C_INT,0,NULL,"0"},
	{"modemalive",C_STR,0,NULL,"AT|"},
	{"modembusy",C_STRL,0,NULL,NULL},
	{"modemcheckdsr",C_YESNO,0,NULL,"no"},
	{"modemconnect",C_STRL,0,NULL,NULL},
	{"modemerror",C_STRL,0,NULL,NULL},
	{"modemhangup",C_STRL,0,NULL,NULL},
	{"modemnodial",C_STRL,0,NULL,NULL},
	{"modemok",C_STRL,0,NULL,NULL},
	{"modemreset",C_STRL,0,NULL,NULL},
	{"modemringing",C_STR,0,NULL,"RINGING"},
	{"modemstat",C_STRL,0,NULL,NULL},
	{"needalllisted",C_YESNO,0,NULL,"no"},
	{"nlpath",C_PATH,3,NULL,NULL},
	{"nodelist",C_STRL,3,NULL,NULL},
	{"nodial",C_PATH,0,NULL,NULL},
	{"osname",C_STR,0,NULL,NULL},
	{"password",C_ADRSTRL,0,NULL,NULL},
	{"perlfile",C_PATH,2,NULL,NULL},
	{"phone",C_STR,0,NULL,""},
	{"phonetr",C_STRL,0,NULL,NULL},
	{"pidfile",C_PATH,3,NULL,NULL},
	{"place",C_STR,0,NULL,""},
	{"pollflavor",C_STR,0,NULL,"N"},
	{"port",C_STRL,0,NULL,NULL},
	{"progname",C_STR,0,NULL,NULL},
	{"protorder",C_STR,0,NULL,"JHZ1C"},
	{"proxy",C_STR,0,NULL,NULL},
	{"qstoutbound",C_PATH,2,NULL,NULL},
	{"realmincps",C_YESNO,0,NULL,"yes"},
	{"recodepkts",C_YESNO,0,NULL,"yes"},
	{"remotecp",C_PATH,0,NULL,"none"},
	{"rescanperiod",C_INT,0,NULL,"300"},
	{"resolvehostname",C_YESNO,0,NULL,"no"},
	{"rmboxes",C_YESNO,0,NULL,"no"},
	{"rootdir",C_PATH,2,NULL,NULL},
	{"runoncall",C_STR,0,NULL,NULL},
	{"runonchat",C_STR,0,NULL,NULL},
	{"runonemsi",C_STR,0,NULL,NULL},
	{"sendonly",C_YESNO,0,NULL,"no"},
	{"server",C_STR,2,NULL,NULL},
	{"serverpwd",C_STR,0,NULL,NULL},
	{"showintro",C_YESNO,0,NULL,"yes"},
	{"showpkt",C_YESNO,0,NULL,"no"},
	{"socks",C_STR,0,NULL,NULL},
	{"speed",C_INT,0,NULL,"300"},
	{"srifrp",C_PATH,0,NULL,NULL},
	{"standardemsi",C_YESNO,0,NULL,"yes"},
	{"station",C_STR,0,NULL,""},
	{"subst",C_ADRSTRL,0,NULL,NULL},
	{"sysop",C_STR,0,NULL,""},
	{"translatesubst",C_YESNO,0,NULL,"no"},
	{"useproctitle",C_YESNO,0,NULL,"yes"},
	{"version",C_STR,0,NULL,NULL},
	{"waitcarrier",C_INT,0,NULL,"60"},
	{"waithrq",C_INT,0,NULL,"60"},
	{"waitreset",C_INT,0,NULL,"10"},
	{"worktime",C_STR,0,NULL,""},
	{"zmh",C_STR,0,NULL,NULL},
	{"zrxwin",C_INT,0,NULL,"0"},
	{"ztxwin",C_INT,0,NULL,"0"},
	{NULL,0,0,NULL,NULL}
};
