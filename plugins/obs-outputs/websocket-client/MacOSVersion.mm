#include "MacOSVersion.h"
#include <string>
#import <Foundation/Foundation.h>

std::string getMacOSVersion()
{
	@autoreleasepool {
		NSProcessInfo *processInfo = [NSProcessInfo processInfo];
		NSString *osVersion =
			[processInfo operatingSystemVersionString];
		return std::string([osVersion UTF8String]);
	}
}
