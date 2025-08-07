#ifndef STATUS_H
#define STATUS_H

#define SUCCESS 	0
#define ENMEM 	    1  // No memory error
#define EIO	  	    2  // Input/Output error
#define EIARG	    3  // Invalid argument
#define ENFOUND     4  // Not found error
#define EIUSE       5  // In use error
#define EOF         6  // End of file
#define EFSYSTEM    7  // File system error
#define EWMODE      8  // Wrong mode
#define EHARDWARE   9  // Hardware error
#define ENOPEN     10  // No open error
#define EFORMAT    11  // Wrong format
#define EDISKSPACE 12  // Disk space error
#define ELONG      13  // Too long error
#define ENSPACE    14  // No space error

#define ENIMPLEMENTED 255 // Not implemented error
#endif
