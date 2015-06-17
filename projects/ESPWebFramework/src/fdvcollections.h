/*
# Created by Fabrizio Di Vittorio (fdivitto2013@gmail.com)
# Copyright (c) 2015 Fabrizio Di Vittorio.
# All rights reserved.

# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/


#ifndef _FDVCOLLECTIONS_H_
#define _FDVCOLLECTIONS_H_


#include "fdv.h"


namespace fdv
{



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// CharChunk

struct CharChunk
{
	CharChunk* next;
	uint32_t   items;
	uint32_t   capacity;
	char*      data;
	bool       freeOnDestroy;	// free "data" on destroy
	
	CharChunk(uint32_t capacity_)
		: next(NULL), items(0), capacity(capacity_), data(new char[capacity_]), freeOnDestroy(true)
	{
	}
	CharChunk(char* data_, uint32_t items_, bool freeOnDestroy_)
		: next(NULL), items(items_), capacity(items_), data(data_), freeOnDestroy(freeOnDestroy_)
	{
	}
	~CharChunk()
	{
		if (freeOnDestroy)
			delete[] data;
	}
};


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// CharChunksIterator

struct CharChunksIterator
{
	CharChunksIterator(CharChunk* chunk = NULL)
		: m_chunk(chunk), m_pos(0), m_absPos(0)
	{
	}
	char& operator*();
	CharChunksIterator operator++(int);
	CharChunksIterator& operator++();
	CharChunksIterator& operator+=(int32_t rhs);
	CharChunksIterator operator+(int32_t rhs);
	int32_t operator-(CharChunksIterator rhs);
	bool operator==(CharChunksIterator const& rhs);
	bool operator!=(CharChunksIterator const& rhs);
	uint32_t getPosition();
	bool isLast();
	bool isValid();

private:
	void next();

private:
	CharChunk* m_chunk;
	uint32_t   m_pos;  	   // position inside this chunk
	uint32_t   m_absPos;   // absolute position (starting from beginning of LinkedCharChunks)
};


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// LinkedCharChunks
//
// Warn: copy constructor copies only pointers. Should not used when with heap or stack buffers.

struct LinkedCharChunks
{	
	
	LinkedCharChunks()
		: m_chunks(NULL), m_current(NULL)
	{
	}
	
	// copy constructor
	// Only data pointers are copied and they will be not freed
	LinkedCharChunks(LinkedCharChunks& c)
		: m_chunks(NULL), m_current(NULL)
	{
        *this = c;
	}
	
	
	~LinkedCharChunks()
	{
		clear();
	}
	
	
	void clear();
	CharChunk* addChunk(uint32_t capacity);
	CharChunk* addChunk(char* data, uint32_t items, bool freeOnDestroy);
	CharChunk* addChunk(char const* data, uint32_t items, bool freeOnDestroy);
	void addChunk(char const* str, bool freeOnDestroy = false);
	void addChunks(LinkedCharChunks* src);
	void append(char value, uint32_t newChunkSize = 1);
	CharChunk* getFirstChunk();
	CharChunksIterator getIterator();
	uint32_t getItemsCount();
	void dump();
    void operator=(LinkedCharChunks& c);

private:
	CharChunk* m_chunks;
	CharChunk* m_current;
};



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// IterDict
// A dictionary where key and value are iterators

template <typename KeyIterator, typename ValueIterator>
class IterDict
{
public:

	struct Item
	{
		Item*         next;
		KeyIterator   key;
		KeyIterator   keyEnd;		
		ValueIterator value;
		ValueIterator valueEnd;
		APtr<char>    valueStr;	// dynamically allocated zero terminated value string (created by operator[])
		
		Item(KeyIterator key_, KeyIterator keyEnd_, ValueIterator value_, ValueIterator valueEnd_)
			: next(NULL), key(key_), keyEnd(keyEnd_), value(value_), valueEnd(valueEnd_)
		{
		}
		Item()
			: next(NULL), key(KeyIterator()), keyEnd(KeyIterator()), value(ValueIterator()), valueEnd(ValueIterator())
		{
		}
		bool TMTD_FLASHMEM operator==(Item const& rhs)
		{
			return next == rhs.next && key == rhs.key && keyEnd == rhs.keyEnd && value == rhs.value && valueEnd == rhs.valueEnd;
		}
		bool TMTD_FLASHMEM operator!=(Item const& rhs)
		{
			return !(*this == rhs);
		}
	};

	IterDict()
		: m_items(NULL), m_current(NULL), m_itemsCount(0), m_urlDecode(false)
	{
	}
	
	~IterDict()
	{
		clear();
	}
	
	void TMTD_FLASHMEM clear()
	{
		Item* item = m_items;
		while (item)
		{
			Item* next = item->next;
			delete item;
			item = next;
		}
		m_items = m_current = NULL;
		m_itemsCount = 0;
	}
	
	void TMTD_FLASHMEM add(KeyIterator key, KeyIterator keyEnd, ValueIterator value, ValueIterator valueEnd)
	{
		if (m_items)
		{
			m_current = m_current->next = new Item(key, keyEnd, value, valueEnd);
			++m_itemsCount;
		}
		else
		{
			m_current = m_items = new Item(key, keyEnd, value, valueEnd);
			++m_itemsCount;
		}
	}
	
	// key and value must terminate with a Zero
	void TMTD_FLASHMEM add(KeyIterator key, ValueIterator value)
	{
		add(key, key + t_strlen(key), value, value + t_strlen(value));
	}
	
	// automatically embeds key and value into CharIterator, so key and value can stay in RAM or Flash
	// key and value must terminate with a Zero
	// Applies only when KeyIterator == ValueIterator == CharIterator
	void TMTD_FLASHMEM add(char const* key, char const* value)
	{
		add(CharIterator(key), CharIterator(value));
	}
	
	uint32_t TMTD_FLASHMEM getItemsCount()
	{
		return m_itemsCount;
	}
	
	// warn: this doesn't check "index" range!
	Item* TMTD_FLASHMEM getItem(uint32_t index)
	{
		Item* item = m_items;
		for (; index > 0; --index)
			item = item->next;
		return item;
	}

	// key stay in RAM or Flash
	Item* TMTD_FLASHMEM getItem(char const* key, char const* keyEnd)
	{
		Item* item = m_items;
		while (item)
		{
			if (t_compare(item->key, item->keyEnd, CharIterator(key), CharIterator(keyEnd)))
				return item;	// found
			item = item->next;
		}
		return NULL;	// not found
	}
	
	// warn: this doesn't check "index" range!
	Item* TMTD_FLASHMEM operator[](uint32_t index)
	{
		return getItem(index);
	}
		
	// key can stay in RAM or Flash and must terminate with zero
	// creates a RAM stored temporary (with the same lifetime of IterDict class) zero terminated string with the value content
	// if m_urlDecode is true then the temporary in RAM string is url decoded
	char const* TMTD_FLASHMEM operator[](char const* key)
	{
		Item* item = getItem(key, key + f_strlen(key));
		if (item)
		{
			if (item->valueStr.get() == NULL)
			{
				item->valueStr.reset(t_strdup(item->value, item->valueEnd));
				if (m_urlDecode)
					inplaceURLDecode(item->valueStr.get());
			}
			return item->valueStr.get();
		}
		return NULL;
	}
	
	void TMTD_FLASHMEM setUrlDecode(bool value)
	{
		m_urlDecode = value;
	}
	
	// debug
	void TMTD_FLASHMEM dump()
	{
		for (uint32_t i = 0; i != m_itemsCount; ++i)
		{
			Item* item = getItem(i);			
			for (KeyIterator k = item->key; k != item->keyEnd; ++k)
				debug(*k);
			debug(" = ");
			for (KeyIterator v = item->value; v!= item->valueEnd; ++v)
				debug(*v);
			debug("\r\n");			
		}
	}		
	
private:
	
	Item*    m_items;
	Item*    m_current;
	uint32_t m_itemsCount;
	bool     m_urlDecode;
};


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// ObjectDict

template <typename T>
struct ObjectDict
{
	struct Item
	{
		Item*         next;
		char const*   key;
		char const*   keyEnd;
		T             value;
		
		Item(char const* key_, char const* keyEnd_, T value_)
			: next(NULL), key(key_), keyEnd(keyEnd_), value(value_)
		{
		}
		Item()
			: next(NULL), key(NULL), keyEnd(NULL), value(T())
		{
		}
		bool MTD_FLASHMEM operator==(Item const& rhs)
		{
			return next == rhs.next && key == rhs.key && keyEnd == rhs.keyEnd && value == rhs.value;
		}
		bool MTD_FLASHMEM operator!=(Item const& rhs)
		{
			return !(*this == rhs);
		}
	};

	ObjectDict()
		: m_items(NULL), m_current(NULL), m_itemsCount(0)
	{
	}
	
	~ObjectDict()
	{
		clear();
	}
	
	void TMTD_FLASHMEM clear()
	{
		Item* item = m_items;
		while (item)
		{
			Item* next = item->next;
			delete item;
			item = next;
		}
		m_items = m_current = NULL;
		m_itemsCount = 0;
	}
	
	void TMTD_FLASHMEM add(char const* key, char const* keyEnd, T value)
	{
		if (m_items)
		{
			m_current = m_current->next = new Item(key, keyEnd, value);
			++m_itemsCount;
		}
		else
		{
			m_current = m_items = new Item(key, keyEnd, value);
			++m_itemsCount;
		}
	}
	
	// add zero terminated string
	void TMTD_FLASHMEM add(char const* key, T value)
	{
		add(key, key + f_strlen(key), value);
	}
	
	// add all items of source (shallow copy for keys, value copy for values)
	void TMTD_FLASHMEM add(ObjectDict<T>* source)
	{
		Item* srcItem = source->m_items;
		while (srcItem)
		{
			add(srcItem->key, srcItem->keyEnd, srcItem->value);
			srcItem = srcItem->next;
		}
	}
	
	// add an empty item, returning pointer to the created object value
	T* TMTD_FLASHMEM add(char const* key)
	{
		T value;
		add(key, value);
		return &(getItem(key)->value);
	}
	
	uint32_t TMTD_FLASHMEM getItemsCount()
	{
		return m_itemsCount;
	}
	
	// warn: this doesn't check "index" range!
	Item* TMTD_FLASHMEM getItem(uint32_t index)
	{
		Item* item = m_items;
		for (; index > 0; --index)
			item = item->next;
		return item;
	}

	// key stay in RAM or Flash
	Item* TMTD_FLASHMEM getItem(char const* key, char const* keyEnd)
	{
		Item* item = m_items;
		while (item)
		{
			if (t_compare(CharIterator(item->key), CharIterator(item->keyEnd), CharIterator(key), CharIterator(keyEnd)))
				return item;	// found
			item = item->next;
		}
		return NULL;	// not found
	}
	
	// key stay in RAM or Flash
	Item* TMTD_FLASHMEM getItem(char const* key)
	{
		return getItem(key, key + f_strlen(key));
	}
	
	// warn: this doesn't check "index" range!
	Item* TMTD_FLASHMEM operator[](uint32_t index)
	{
		return getItem(index);
	}
		
	// key can stay in RAM or Flash and must terminate with zero
	Item* TMTD_FLASHMEM operator[](char const* key)
	{
		return getItem(key);
	}
	
	void TMTD_FLASHMEM dump()
	{
		Item* item = m_items;
		while (item)
		{
			debugstrn(item->key, item->keyEnd - item->key);
			debug(FSTR(" = "));
			item->value.dump();
			debug(FSTR("\r\n"));
			item = item->next;
		}
	}
	
private:
	
	Item*    m_items;
	Item*    m_current;
	uint32_t m_itemsCount;
};



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// FlashDictionary
// Maximum one page (4096 bytes) of storage
// Maximum key length = 254 bytes
// Maximum value length = 65536 bytes (actually less than 4096!)
// All methods which accepts key and/or value allows Flash or RAM storage
//
// Examples:
//   FlashDictionary::setString("name", "Fabrizio");
//   debug("name = %s\r\n", FlashDictionary::getString("name", ""));
//
//   FlashDictionary::setInt("code", 1234);
//   debug("code = %d\r\n", FlashDictionary::getInt("name", 0));
//
// No need to call eraseContent the first time because it is automatically called if MAGIC is not found
// Call eraseContent only if you want to erase old dictionary

struct FlashDictionary
{
	static uint32_t const FLASH_DICTIONARY_POS = 0x16000;
	static uint32_t const MAGIC                = 0x46445631;
	
	// clear the entire available space and write MAGIC at the beginning of the dictionary
	// This is required only if you want to remove previous content
	// erase all values to 0xFF
	static void eraseContent();

	// requires 4K free heap to execute!
	static void setValue(char const* key, void const* value, uint32_t valueLength);
	
	// return NULL if key point to a free space (aka key doesn't exist)
	// return pointer may be Unaligned pointer to Flash
	static uint8_t const* getValue(char const* key, uint32_t* valueLength = NULL);
		
	static void setString(char const* key, char const* value);
	
	// always returns a Flash stored string (if not found return what contained in defaultValue)
	static char const* getString(char const* key, char const* defaultValue);
	
	static void setInt(char const* key, int32_t value);
	
	static int32_t getInt(char const* key, int32_t defaultValue);
	
	static void setBool(char const* key, bool value);
	
	static bool getBool(char const* key, bool defaultValue);
	
	// return starting of specified key or starting of next free position
	// what is stored:
	//   byte: key length (or 0xFF for end of keys, hence starting of free space). Doesn't include ending zero
	//   ....: key string
	//   0x00: key ending zero
	//   word: value length (little endian)
	//   ....: value data
	// automatically erase content if not already initialized
	static void const* findKey(char const* key);
	
	static bool isContentValid();
	
	static uint32_t getUsedSpace();	
};



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// FlashFileSystem
// It is just a files extractor from the flash.
// You can write files into the flash using "binarydir.py" (to prepare) and "esptool.py" (to flash) tools.
// For example, having some files in webcontent subdirectory you can do:
//   python binarydir.py webcontent webcontent.bin 167936
//   python ../esptool.py --port COM7 write_flash 0x17000 webcontent.bin
// Then you can use FlashFileSystem static methods to get actual files content
// Maximum content size is 167936 bytes and starts from 0x17000 of the flash memory

struct FlashFileSystem
{
	static uint32_t const FLASHFILESYSTEM_POS = 0x17000;
	static uint32_t const MAGIC               = 0x93841A03;
		
	static bool find(char const* filename, char const** mimetype, void const** data, uint16_t* dataLength);
};







}

#endif