
//this is manually created - do not delete!

[CCode(
	cheader_filename = "stdint.h",
	lower_case_cprefix = "", cprefix = "")]

public int _debug_;

[CCode(cname = "dbg", cheader_filename = "debug/debug.h")]
public void dbg(int n, char* format);

[CCode(cname = "warnprintf2", cheader_filename = "debug/debug.h")]
void pwarn(char* func, char* format, ...);
