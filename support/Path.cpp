/*
 * Copyright 2002-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */

/*!
	\file Path.cpp
	TPath implementation.
*/

#include "Path.h"

#include <errno.h>
#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

using namespace std;


static bool
is_absolute_path(const char *path)
{
	return (path && path[0] == '/');
}



//! Creates an uninitialized BPath object.
TPath::TPath()
	:
	fName(NULL),
	fCStatus(-1)
{
}


/*! Creates a copy of the given BPath object.
	\param path the object to be copied
*/
TPath::TPath(const TPath& path)
	:
	fName(NULL),
	fCStatus(-1)
{
	*this = path;
}



/*! \brief Creates a BPath object and initializes it to the specified path or
		   path and filename combination.

	\param dir The base component of the pathname. May be absolute or relative.
		   If relative, it is reckoned off the current working directory.
	\param leaf The (optional) leaf component of the pathname. Must be
		   relative. The value of leaf is concatenated to the end of \a dir
		   (a "/" will be added as a separator, if necessary).
	\param normalize boolean flag used to force normalization; normalization
		   may occur even if false (see \ref _MustNormalize).
*/
TPath::TPath(const char* dir, const char* leaf, bool normalize)
	:
	fName(NULL),
	fCStatus(-1)
{
	SetTo(dir, leaf, normalize);
}


//! Destroys the BPath object and frees any of its associated resources.
TPath::~TPath()
{
	Unset();
}


/*! \brief Returns the status of the most recent construction or SetTo() call.
	\return \c B_OK, if the BPath object is properly initialized, an error
			code otherwise.
*/
status_t
TPath::InitCheck() const
{
	return fCStatus;
}



/*!	\brief Reinitializes the object to the specified path or path and file
	name combination.
	\param path the path name
	\param leaf the leaf name (may be \c NULL)
	\param normalize boolean flag used to force normalization; normalization
		   may occur even if false (see \ref _MustNormalize).
	\return
	- \c B_OK: The initialization was successful.
	- \c B_BAD_VALUE: \c NULL \a path or absolute \a leaf.
	- \c B_NAME_TOO_LONG: The pathname is longer than \c B_PATH_NAME_LENGTH.
	- other error codes.
	\note \code path.SetTo(path.Path(), "new leaf") \endcode is safe.
*/
status_t
TPath::SetTo(const char* path, const char* leaf, bool normalize)
{
	status_t error = (path ? 0 : EINVAL);
	if (error == 0 && leaf && is_absolute_path(leaf))
		error = EINVAL;
	char newPath[PATH_MAX];
	if (error == 0) {
		// we always normalize relative paths
		normalize |= !is_absolute_path(path);
		// build a new path from path and leaf
		// copy path first
		uint32 pathLen = strlen(path);
		if (pathLen >= sizeof(newPath))
			error = EINVAL;
		if (error == 0)
			strcpy(newPath, path);
		// append leaf, if supplied
		if (error == 0 && leaf) {
			bool needsSeparator = (pathLen > 0 && path[pathLen - 1] != '/');
			uint32 wholeLen = pathLen + (needsSeparator ? 1 : 0)
							  + strlen(leaf);
			if (wholeLen >= sizeof(newPath))
				error = EINVAL;
			if (error == 0) {
				if (needsSeparator) {
					newPath[pathLen] = '/';
					pathLen++;
				}
				strcpy(newPath + pathLen, leaf);
			}
		}
		// check, if necessary to normalize
		if (error == 0 && !normalize)
			normalize = normalize || _MustNormalize(newPath, &error);

		// normalize the path, if necessary, otherwise just set it
		if (error == 0) {
			if (normalize) {
				char realPath[PATH_MAX];
				if (realpath(newPath, realPath) != NULL) {
					return SetTo(realPath);
				} else
					error = errno;
			} else
				error = _SetPath(newPath);
		}
	}
	// cleanup, if something went wrong
	if (error != 0)
		Unset();
	fCStatus = error;
	return error;
}


/*!	\brief Returns the object to an uninitialized state. The object frees any
	resources it allocated and marks itself as uninitialized.
*/
void
TPath::Unset()
{
	_SetPath(NULL);
	fCStatus = -1;
}


/*!	\brief Appends the given (relative) path to the end of the current path.
	This call fails if the path is absolute or the object to which you're
	appending is uninitialized.
	\param path relative pathname to append to current path (may be \c NULL).
	\param normalize boolean flag used to force normalization; normalization
		   may occur even if false (see \ref _MustNormalize).
	\return
	- \c B_OK: The initialization was successful.
	- \c B_BAD_VALUE: The object is not properly initialized.
	- \c B_NAME_TOO_LONG: The pathname is longer than \c B_PATH_NAME_LENGTH.
	- other error codes.
*/
status_t
TPath::Append(const char* path, bool normalize)
{
	status_t error = (InitCheck() == 0 ? 0 : EINVAL);
	if (error == 0)
		error = SetTo(Path(), path, normalize);
	if (error != 0)
		Unset();
	fCStatus = error;
	return error;
}


/*! \brief Returns the object's complete path name.
	\return
	- the object's path name, or
	- \c NULL, if it is not properly initialized.
*/
const char*
TPath::Path() const
{
	return fName;
}


/*! \brief Returns the leaf portion of the object's path name.
	The leaf portion is defined as the string after the last \c '/'. For
	the root path (\c "/") it is the empty string (\c "").
	\return
	- the leaf portion of the object's path name, or
	- \c NULL, if it is not properly initialized.
*/
const char*
TPath::Leaf() const
{
	if (InitCheck() != 0)
		return NULL;

	const char* result = fName + strlen(fName);
	// There should be no need for the second condition, since we deal
	// with absolute paths only and those contain at least one '/'.
	// However, it doesn't harm.
	while (*result != '/' && result > fName)
		result--;
	result++;

	return result;
}


/*! \brief Calls the argument's SetTo() method with the name of the
	object's parent directory.
	No normalization is done.
	\param path the BPath object to be initialized to the parent directory's
		   path name.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- \c B_ENTRY_NOT_FOUND: The object represents \c "/".
	- other error code returned by SetTo().
*/
status_t
TPath::GetParent(TPath* path) const
{
	if (path == NULL)
		return EINVAL;

	status_t error = InitCheck();
	if (error != 0)
		return error;

	int32 length = strlen(fName);
	if (length == 1) {
		// handle "/" (path is supposed to be absolute)
		return -1;
	}

	char parentPath[PATH_MAX];
	length--;
	while (fName[length] != '/' && length > 0)
		length--;
	if (length == 0) {
		// parent dir is "/"
		length++;
	}
	memcpy(parentPath, fName, length);
	parentPath[length] = '\0';

	return path->SetTo(parentPath);
}


/*! \brief Performs a simple (string-wise) comparison of paths.
	No normalization takes place! Uninitialized BPath objects are considered
	to be equal.
	\param item the BPath object to be compared with
	\return \c true, if the path names are equal, \c false otherwise.
*/
bool
TPath::operator==(const TPath& item) const
{
	return *this == item.Path();
}


/*! \brief Performs a simple (string-wise) comparison of paths.
	No normalization takes place!
	\param path the path name to be compared with
	\return \c true, if the path names are equal, \c false otherwise.
*/
bool
TPath::operator==(const char* path) const
{
	return (InitCheck() != 0 && path == NULL)
		|| (fName != NULL && path != NULL && strcmp(fName, path) == 0);
}


/*! \brief Performs a simple (string-wise) comparison of paths.
	No normalization takes place! Uninitialized BPath objects are considered
	to be equal.
	\param item the BPath object to be compared with
	\return \c true, if the path names are not equal, \c false otherwise.
*/
bool
TPath::operator!=(const TPath& item) const
{
	return !(*this == item);
}


/*! \brief Performs a simple (string-wise) comparison of paths.
	No normalization takes place!
	\param path the path name to be compared with
	\return \c true, if the path names are not equal, \c false otherwise.
*/
bool
TPath::operator!=(const char* path) const
{
	return !(*this == path);
}


/*! \brief Initializes the object to be a copy of the argument.
	\param item the BPath object to be copied
	\return \c *this
*/
TPath&
TPath::operator=(const TPath& item)
{
	if (this != &item)
		*this = item.Path();
	return *this;
}


/*! \brief Initializes the object to be a copy of the argument.
	Has the same effect as \code SetTo(path) \endcode.
	\param path the path name to be assigned to this object
	\return \c *this
*/
TPath&
TPath::operator=(const char* path)
{
	if (path == NULL)
		Unset();
	else
		SetTo(path);
	return *this;
}


/*!	\brief Sets the supplied path.
	The path is copied. If \c NULL, the object's path is set to NULL as well.
	The object's old path is deleted.
	\param path the path to be set
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_MEMORY: Insufficient memory.
*/
status_t
TPath::_SetPath(const char* path)
{
	status_t error = 0;
	delete[] fName;
	fName = NULL;
	
	// set the new path
	if (path) {
		fName = new(nothrow) char[strlen(path) + 1];
		if (fName)
			strcpy(fName, path);
		else
			error = ENOMEM;
	}
	
	return error;
}


/*! \brief Checks a path to see if normalization is required.

	The following items require normalization:
		- Relative pathnames (after concatenation; e.g. "boot/ltj")
		- The presence of "." or ".." ("/boot/ltj/../ltj/./gwar")
		- Redundant slashes ("/boot//ltj")
		- A trailing slash ("/boot/ltj/")

	\param _error A pointer to an error variable that will be set if the input
		is not a valid path.
	\return
		- \c true: \a path requires normalization
		- \c false: \a path does not require normalization
*/
bool
TPath::_MustNormalize(const char* path, status_t* _error)
{
	// Check for useless input
	if (path == NULL || path[0] == 0) {
		if (_error != NULL)
			*_error = EINVAL;
		return false;
	}

	int len = strlen(path);

	/* Look for anything in the string that forces us to normalize:
			+ No leading /
			+ any occurence of /./ or /../ or //, or a trailing /. or /..
			+ a trailing /
	*/;
	if (path[0] != '/')
		return true;	//	not "/*"
	else if (len == 1)
		return false;	//	"/"
	else if (len > 1 && path[len-1] == '/')
		return true;	// 	"*/"
	else {
		enum ParseState {
			NoMatch,
			InitialSlash,
			OneDot,
			TwoDots
		} state = NoMatch;

		for (int i = 0; path[i] != 0; i++) {
			switch (state) {
				case NoMatch:
					if (path[i] == '/')
						state = InitialSlash;
					break;

				case InitialSlash:
					if (path[i] == '/')
						return true;		// "*//*"

					if (path[i] == '.')
						state = OneDot;
					else
						state = NoMatch;
					break;

				case OneDot:
					if (path[i] == '/')
						return true;		// "*/./*"

					if (path[i] == '.')
						state = TwoDots;
					else
						state = NoMatch;
					break;

				case TwoDots:
					if (path[i] == '/')
						return true;		// "*/../*"

					state = NoMatch;
					break;
			}
		}

		// If we hit the end of the string while in either
		// of these two states, there was a trailing /. or /..
		if (state == OneDot || state == TwoDots)
			return true;

		return false;
	}
}
