#pragma once

// Identity constants for the running build.
//
// These are normally injected as compile definitions by cmake/KitesPackaging.cmake
// (which reads them from cmake/KitesVersion.cmake). The fallbacks below keep this
// module compiling standalone if that packaging module is ever not included -
// a dev build then reports version 0.0.0-dev, which compares older than any
// real release and so never claims to be up to date incorrectly.

#ifndef KITES_VERSION
    #define KITES_VERSION "0.0.0-dev"
#endif

#ifndef KITES_APP_ID
    #define KITES_APP_ID "io.github.hghit_harshit.Kites"
#endif

#ifndef KITES_GITHUB_OWNER
    #define KITES_GITHUB_OWNER "hghit-harshit"
#endif

#ifndef KITES_GITHUB_REPO
    #define KITES_GITHUB_REPO "Kites"
#endif

#ifndef KITES_HOMEPAGE_URL
    #define KITES_HOMEPAGE_URL "https://github.com/hghit-harshit/Kites"
#endif
