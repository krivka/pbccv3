#ifndef VERSION_H
#define VERSION_H

	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] = "14.04";
	
	//Software Status
	static const char STATUS[] = "Beta";
	static const char STATUS_SHORT[] = "b";
	
	//Standard Version Type
	static const long MAJOR = 3;
	static const long MINOR = 1;
	static const long BUILD = 0;
	static const long REVISION = 0;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 197;
	#define RC_FILEVERSION 3,1,0,0
	#define RC_FILEVERSION_STRING "3, 1, 0, 0\0"
	static const char FULLVERSION_STRING[] = "3.1.0.0";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 11;
	

#endif //VERSION_H
