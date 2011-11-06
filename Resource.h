#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "Stream.h"
#include "Types.h"


struct res_ref;
class TArchive;
class TMemoryStream;
class ResourceManager;
class Resource : public TStream {
public:
	Resource(uint8 *data, uint32 size, uint32 key);
	Resource();

	virtual ~Resource();
	
	virtual bool Load(TArchive *archive, uint32 key);
	void Write(const char *fileName);

	const uint32 Key() const;
	const uint16 Type() const;
	
	static Resource *Create(const res_ref &name, uint16 type);
	
protected:
	friend class ResourceManager;

	virtual ssize_t ReadAt(int pos, void *dst, int count);
	
	virtual int32 Seek(int32 where, int whence = SEEK_CUR);
	virtual int32 Position() const;
	
	template<typename T>
	ssize_t ReadAt(int pos, T &dst) {
		return ReadAt(pos, &dst, sizeof(dst));
	};
	
	bool ReplaceData(TMemoryStream *stream);

private:
	void _Acquire();
	bool _Release();

protected:
	TMemoryStream *fData;
	uint32 fKey;
	uint16 fType;
	res_ref fName;

private:
	int32 fRefCount;
};


#endif
