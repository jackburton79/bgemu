/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PATH_H
#define _PATH_H

#include "SupportDefs.h"

class Path {
public:
							Path();
							Path(const Path& path);
							Path(const char* dir, const char* leaf = NULL,
								bool normalize = false);

	virtual					~Path();

			status_t		InitCheck() const;

			status_t		SetTo(const char* path, const char* leaf = NULL,
								bool normalize = false);
			void			Unset();

			status_t		Append(const char* path, bool normalize = false);

			const char*		String() const;
			const char*		Leaf() const;
			status_t		GetParent(Path* path) const;

			bool			operator==(const Path& item) const;
			bool			operator==(const char* path) const;
			bool			operator!=(const Path& item) const;
			bool			operator!=(const char* path) const;
			Path&			operator=(const Path& item);
			Path&			operator=(const char* path);

private:
			status_t		_SetPath(const char* path);
	static	bool			_MustNormalize(const char* path, status_t* _error);

			char*			fName;
			status_t		fCStatus;
};

#endif	// _PATH_H
