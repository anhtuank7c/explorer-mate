#ifndef ET_DOMAIN_VERSION_H
#define ET_DOMAIN_VERSION_H

// The one place the product version is written down. Keep this file to plain #defines:
// it is included by the .rc files and parsed by scripts\Common.ps1 (Get-ProductVersion),
// which feeds the package manifests. tests/UnitTests checks the parts agree.
#define ET_VERSION_MAJOR 0
#define ET_VERSION_MINOR 1
#define ET_VERSION_PATCH 0
#define ET_VERSION_STRING "0.1.0"

#endif  // ET_DOMAIN_VERSION_H
