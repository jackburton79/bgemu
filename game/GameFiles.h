/*
 * GameFiles.h
 *
 * Loose files of the game installation outside the resource archives
 * (music/, sounds/, ...), found without regard to case: the installs mix
 * "Music/Bc1/Bc1a1.ACM" and "music/BC1/BC1A1.acm" and the file system here is
 * case sensitive.
 */

#pragma once

#include <string>

// The path of `relativePath` (components separated by '/') below the game's
// directory, each component matched case-insensitively; "" if there is no such
// file or directory.
std::string FindGameFile(const std::string& relativePath);
