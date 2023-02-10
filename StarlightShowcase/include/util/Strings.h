#pragma once

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * Ben's strings - in search of better string design.
 * 
 * Overview:
 *  - StringView strings must be valid C strings (null-terminated) and also have the correct size (including the null terminator)
 *  - Pass read-only access to a string as `const StringView&`
 *  - Pass read-write access to a string as `MutableString&`
 *  - Store & manipulate strings of a fixed max length using `InlineString<Capacity>` (be careful of stack overflow though)
 *  - Store & manipulate strings that automatically resize using `DynamicString` (uses malloc, realloc, and free)
 */

/**
 * A read-only view of a string. Its data must be null-terminated.
 */
class StringView
{
private:
	const char* _Data;
	size_t _Size;

public:
	StringView() = delete; // Please use one of the approved ways of initializing a StringView.
	inline StringView(const char* data, size_t size) : _Data(data), _Size(size) { }
	template <size_t Size>
	inline StringView(const char (&const stringLiteral)[Size]) : _Data(stringLiteral), _Size(Size) { }

	inline const char* GetData() const { return this->_Data; }
	inline size_t GetSize() const { return this->_Size; }
	inline void SetData(const char* data, size_t size) { this->_Data = data; this->_Size = size; }

	/**
	 * Implicit conversion to a read-only null-terminated C string.
	 */
	inline operator const char*() const { return this->GetData(); }

	/**
	 * Creates a StringView pointing to the given string data and ending at its first null byte.
	 */
	inline static StringView FromCString(const char* string) { return StringView(string, strlen(string) + 1); }
};

class MutableString
{
protected:
	char* _Data;
	size_t _Size;
	size_t _Capacity;

	MutableString(char* buffer, size_t capacity) : _Data(buffer), _Capacity(capacity)
	{
		this->Clear();
	}

	virtual bool Reallocate(size_t requestedCapacity) = 0;

public:
	virtual ~MutableString()
	{
#ifdef _DEBUG
		this->_Data = nullptr;
		this->_Size = 0;
		this->_Capacity = 0;
#endif
	}

	inline char* GetData() { return this->_Data; }
	inline const char* GetData() const { return this->_Data; }
	inline size_t GetSize() const { return this->_Size; }

	inline void Clear() { this->_Size = 1; this->_Data[0] = '\0'; }

	void CopyFrom(const StringView& source)
	{
		if (source.GetSize() > this->_Size)
		{
			this->Reallocate(source.GetSize());
		}
		size_t count = (this->_Capacity < source.GetSize()) ? this->_Capacity : source.GetSize();
		memcpy(this->_Data, source.GetData(), count);
		this->_Size = count;
		this->_Data[this->_Size - 1] = '\0'; // Make sure it is null-terminated (important when this copy was truncated)
	}

	void Format(const StringView format, ...)
	{
		va_list args;
		va_start(args, format);
		int written = vsnprintf(this->_Data, this->_Capacity, format, args);
		va_end(args);
		if (written >= this->_Capacity && this->Reallocate(written))
		{
			va_start(args, format);
			vsnprintf(this->_Data, this->_Capacity, format, args);
			va_end(args);
		}
	}

	inline operator StringView() const { return StringView(this->_Data, this->_Size); }
	inline operator const char*() const { return this->_Data; }
	inline MutableString& operator=(const StringView& value) { this->CopyFrom(value); return *this; }
};

template <size_t Size>
class InlineString : public MutableString
{
private:
	char _Buffer[Size];

	virtual bool Reallocate(size_t requestedCapacity) override { return false; }

public:
	InlineString() : MutableString(this->_Buffer, Size) { }
	InlineString(const StringView& contents) : InlineString() { this->CopyFrom(contents); }

	inline InlineString& operator=(const StringView& value) { this->CopyFrom(value); return *this; }
};

class DynamicString : public MutableString
{
private:
	virtual bool Reallocate(size_t requestedCapacity) override
	{
		this->_Data = (char*)realloc(this->_Data, requestedCapacity);
		this->_Capacity = requestedCapacity;
		return true;
	}

public:
	DynamicString(size_t initialCapacity = 256) : MutableString((char*)malloc(initialCapacity), initialCapacity) { }
	DynamicString(const StringView& contents) : DynamicString(contents.GetSize()) { this->CopyFrom(contents); }
	virtual ~DynamicString()
	{
		free(this->_Data);
	}

	inline DynamicString& operator=(const StringView& value) { this->CopyFrom(value); return *this; }
};