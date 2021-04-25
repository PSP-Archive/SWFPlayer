// container.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Some non-inline implementation help for generic containers.


#include "base/container.h"
#include "base/utf8.h"
#include "base/tu_random.h"
#include <stdarg.h>


void tu_string::append_wide_char(uint16 c)
{
	char buf[8];
	int index = 0;
	utf8::encode_unicode_character(buf, &index, (uint32) c);
	buf[index] = 0;

	*this += buf;
}


void tu_string::append_wide_char(uint32 c)
{
	char buf[8];
	int index = 0;
	utf8::encode_unicode_character(buf, &index, c);
	buf[index] = 0;

	*this += buf;
}


void	tu_string::resize(int new_size)
{
	QASSERT(new_size >= 0);

	if (using_heap() == false)
	{
		if (new_size < 15)
		{
			// Stay with internal storage.
			m_local.m_size = (char) (new_size + 1);
			m_local.m_buffer[new_size] = 0;	// terminate
		}
		else
		{
			// need to allocate heap buffer.
			int	capacity = new_size + 1;
			// round up.
			capacity = (capacity + 15) & ~15;
			char*	buf = (char*) tu_malloc(capacity);
                        memset(buf, 0, capacity);

			// Copy existing data.
			strcpy(buf, m_local.m_buffer);

			// Set the heap state.
			m_heap.m_buffer = buf;
			m_heap.m_all_ones = char(~0);
			m_heap.m_size = new_size + 1;
			m_heap.m_capacity = capacity;
		}
	}
	else
	{
		// Currently using heap storage.
		if (new_size < 15)
		{
			// Switch to local storage.

			// Be sure to get stack copies of m_heap info, before we overwrite it.
			char*	old_buffer = m_heap.m_buffer;
			int	old_capacity = m_heap.m_capacity;
			UNUSED(old_capacity);

			// Copy existing string info.
			m_local.m_size = (char) (new_size + 1);
			strncpy(m_local.m_buffer, old_buffer, 15);
			m_local.m_buffer[new_size] = 0;	// ensure termination.

			tu_free(old_buffer, old_capacity);
		}
		else
		{
			// Changing size of heap buffer.
			int	capacity = new_size + 1;
			// Round up.
			capacity = (capacity + 15) & ~15;
			if (capacity != m_heap.m_capacity)	// @@ TODO should use hysteresis when resizing
			{
				m_heap.m_buffer = (char*) tu_realloc(m_heap.m_buffer, capacity, m_heap.m_capacity);
				m_heap.m_capacity = capacity;
			}
			// else we're OK with existing buffer.

			m_heap.m_size = new_size + 1;

			// Ensure termination.
			m_heap.m_buffer[new_size] = 0;
		}
	}
}


template<class char_type>
/*static*/ void	encode_utf8_from_wchar_generic(tu_string* result, const char_type* wstr)
{
	const char_type*	in = wstr;

	// First pass: compute the necessary string length.
	int	bytes_needed = 0;
	char	dummy[10];
	int	offset;
	for (;;)
	{
		Uint32	uc = *in++;
		offset = 0;
		utf8::encode_unicode_character(dummy, &offset, uc);
		bytes_needed += offset;

		QASSERT(offset <= 6);

		if (uc == 0)
		{
			break;
		}
	}

	// Second pass: transfer the data.
	result->resize(bytes_needed - 1);	// resize() adds 1 for the \0 terminator
	in = wstr;
	char*	out = &((*result)[0]);
	offset = 0;
	for (;;)
	{
		QASSERT(offset < bytes_needed);

		Uint32	uc = *in++;
		utf8::encode_unicode_character(out, &offset, uc);

		QASSERT(offset <= bytes_needed);

		if (uc == 0)
		{
			break;
		}
	}

	QASSERT(offset == bytes_needed);
	QASSERT((*result)[offset - 1] == 0);
	QASSERT(result->length() == (int) strlen(result->c_str()));
}


void tu_string::encode_utf8_from_wchar(tu_string* result, const uint32* wstr)
{
	encode_utf8_from_wchar_generic<uint32>(result, wstr);
}


void tu_string::encode_utf8_from_wchar(tu_string* result, const uint16* wstr)
{
	encode_utf8_from_wchar_generic<uint16>(result, wstr);
}


/*static*/ int	tu_string::stricmp(const char* a, const char* b)
{
	return ::stricmp(a, b);}


uint32	tu_string::utf8_char_at(int index) const
{
	const char*	buf = get_buffer();
	uint32	c;

	do
	{
		c = utf8::decode_next_unicode_character(&buf);
		index--;

		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			QASSERT(index == 0);
			return c;
		}
	}
	while (index >= 0);

	return c;
}


tu_string	tu_string::utf8_to_upper() const
{
	const char*	buf = get_buffer();
	tu_string str;
	for (;;)
	{
		uint32 c = utf8::decode_next_unicode_character(&buf);
          
		if (c == 0)
		{
			// We've hit the end of the string; don't go further.
			return str;
		}
		str += toupper(c);
	}
  
	return str;
}


tu_string	tu_string::utf8_to_lower() const
{
	const char*	buf = get_buffer();
	tu_string str;
	for (;;)
	{
		uint32 c = utf8::decode_next_unicode_character(&buf);
    
		if (c == 0) {
			// We've hit the end of the string; don't go further.
			return str;
		}
		str += tolower(c);
	}
  
	return str;
}


/*static*/ int	tu_string::utf8_char_count(const char* buf, int buflen)
{
	const char*	p = buf;
	int	length = 0;

	while (p - buf < buflen)
	{
		uint32	c = utf8::decode_next_unicode_character(&p);
		if (c == 0)
		{
			break;
		}

		length++;
	}

	return length;
}


tu_string	tu_string::utf8_substring(int start, int end) const
{
	QASSERT(start <= end);

	if (start == end)
	{
		// Special case, always return empty string.
		return tu_string();
	}

	const char*	p = get_buffer();
	int	index = 0;
	const char*	start_pointer = p;
	const char*	end_pointer = p;

	for (;;)
	{
		if (index == start)
		{
			start_pointer = p;
		}

		uint32	c = utf8::decode_next_unicode_character(&p);
		index++;

		if (index == end)
		{
			end_pointer = p;
			break;
		}

		if (c == 0)
		{
			if (index < end)
			{
				QASSERT(0);
				end_pointer = p;
			}
			break;
		}
	}

	if (end_pointer < start_pointer)
	{
		end_pointer = start_pointer;
	}

	return tu_string(start_pointer, end_pointer - start_pointer);
}


#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif // _WIN32

tu_string string_printf(const char* fmt, ...)
// Handy sprintf wrapper.
{
	static const int	BUFFER_SIZE = 500;
	char	s_buffer[BUFFER_SIZE];

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(s_buffer, BUFFER_SIZE, fmt, ap);
	va_end(ap);

	return s_buffer;
}



#ifdef CONTAINER_UNIT_TEST


// Compile this test case with something like:
//
// gcc container.cpp utf8.cpp tu_random.cpp -g -I.. -DCONTAINER_UNIT_TEST -lstdc++ -o container_test
//
//    or
//
// cl container.cpp utf8.cpp tu_random.cpp -Zi -Od -DCONTAINER_UNIT_TEST -I..


void	test_hash()
{
	// Collect a bunch of random key/value pairs.
	array<Uint32>	data;
	for (int i = 0; i < 1000; i++)
	{
		data.push_back(tu_random::next_random());
	}

	// Push into hash.
	hash<Uint32, Uint32>	h;
	{for (int i = 0; i < data.size() / 2; i++)
	{
		h.add(data[i*2], data[i*2 + 1]);

		// Verify the contents of the hash so far.
		for (int j = 0; j < i; j++)
		{
			Uint32	key = data[j*2];
			Uint32	val;
			bool	got = h.get(key, &val);
			QASSERT(got);
			QASSERT(val == data[j*2 + 1]);
		}
	}}

	// Manually copy stuff over to h2, using iterator interface.
	hash<Uint32, Uint32>	h2;
	{for (hash<Uint32, Uint32>::iterator it = h.begin(); it != h.end(); ++it)
	{
		//printf("first = 0x%X, second = 0x%X\n", it->first, it->second);//xxxxx
		QASSERT(h.get(it->first, NULL) == true);

		h2.add(it->first, it->second);

		Uint32	val;
		bool	got = h2.get(it->first, &val);
		QASSERT(got);
		QASSERT(val == it->second);
	}}

	// Verify the contents of h2.
	{for (int i = 0; i < data.size() / 2; i++)
	{
		Uint32	key = data[i*2];
		Uint32	val;
		bool	got = h.get(key, &val);
		QASSERT(got);
		QASSERT(val == data[i*2 + 1]);
	}}

	h.clear();
	QASSERT(h.size() == 0);

	// Verify that h really is missing the stuff it had before, and h2 really has it.
	{for (hash<Uint32, Uint32>::iterator it = h2.begin(); it != h2.end(); ++it)
	{
		QASSERT(h.get(it->first, NULL) == false);
		QASSERT(h2.get(it->first, NULL) == true);
		QASSERT(h.find(it->first) == h.end());
		QASSERT(h2.find(it->first) != h2.end());
	}}
}


//#include <ext/hash_map>
//#include <hash_map>
//#include <map>

void	test_hash_speed()
// Test function for hash performance of adding keys and doing lookup.
{

// Hash type, for doing comparative tests.
//
// tu_hash tests faster than the map and hash_map included with GCC
// 3.3 as well as map and hash_map from MSVC7.  In some cases, several
// times faster.  GCC's hash_map is the closest to tu_hash, but is
// still 33% slower in my tests.

// // tu's hash
#define HASH hash<uint32, uint32 >
#define HASH_ADD(h, k, v) h.add(k, v)

// STL's hash
//#define HASH __gnu_cxx::hash_map<uint32, uint32>
//#define HASH std::hash_map<uint32, uint32>
//#define HASH_ADD(h, k, v) h[k] = v

// STL's map
//#define HASH std::map<uint32, uint32>
//#define HASH_ADD(h, k, v) h[k] = v

//	const int	SIZE = 10000000;
	const int	SIZE = 1000000;

	// Make an array of random numbers.
	array<uint32>	numbers;
	numbers.resize(SIZE);

	for (int i = 0, n = numbers.size(); i < n; i++)
	{
		numbers[i] = tu_random::next_random();
	}

	// Uniquify the array.
	HASH	new_index;
	int	next_new_index = 0;
	{for (int i = 0, n = numbers.size(); i < n; i++)
	{
		HASH::iterator	it = new_index.find(numbers[i]);
		if (it == new_index.end())
		{
			// First time this number has been seen.
			HASH_ADD(new_index, numbers[i], next_new_index);
			next_new_index++;
		}
		else
		{
			// This number already appears in the list.
// 			printf("duplicate entry %x, prev new index %d, current array index %d\n",
// 				   numbers[i],
// 				   it->second,
// 				   i);
		}
	}}

	printf("next_new_index = %d\n", next_new_index);
}


void	test_stringi()
{
	tu_stringi	a, b;

	// Equality.
	a = "this is a test";
	b = "This is a test";
	QASSERT(a == b);

	b = "tHiS Is a tEsT";
	QASSERT(a == b);

	a += "Hello";
	b += "hellO";
	QASSERT(a == b);

	tu_string	c(b);
	QASSERT(a.to_tu_string() != c);

	// Ordering.
	a = "a";
	b = "B";
	QASSERT(a < b);

	a = "b";
	b = "A";
	QASSERT(a > b);
}


void	test_stringi_hash()
{
	stringi_hash<int>	a;

	QASSERT(a.is_empty());

	a.add("bobo", 1);

	QASSERT(a.is_empty() == false);

	a.add("hello", 2);
	a.add("it's", 3);
	a.add("a", 4);
	a.add("beautiful day!", 5);

	int	result = 0;
	a.get("boBO", &result);
	QASSERT(result == 1);

	a.set("BObo", 2);
	a.get("bObO", &result);
	QASSERT(result == 2);

	QASSERT(a.is_empty() == false);
	a.clear();
	QASSERT(a.is_empty() == true);

	// Hammer on one key that differs only by case.
	tu_stringi	original_key("thisisatest");
	tu_stringi	key(original_key);
	a.add(key, 1234567);

	int	variations = 1 << key.length();
	for (int i = 0; i < variations; i++)
	{
		// Twiddle the case of the key.
		for (int c = 0; c < key.length(); c++)
		{
			if (i & (1 << c))
			{
				key[c] = toupper(key[c]);
			}
			else
			{
				key[c] = tolower(key[c]);
			}
		}

		a.set(key, 7654321);

		// Make sure original entry was modified.
		int	value = 0;
		a.get(original_key, &value);
		QASSERT(value == 7654321);

		// Make sure hash keys are preserving case.
		QASSERT(a.find(key)->first.to_tu_string() == original_key.to_tu_string());

		// Make sure they're actually the same entry.
		QASSERT(a.find(original_key) == a.find(key));
		
		a.set(original_key, 1234567);
		QASSERT(a.find(key)->second == 1234567);
	}
}


void test_unicode()
{
	tu_string a;

	tu_string::encode_utf8_from_wchar(&a, L"19 character string");
	QASSERT(a.length() == 19);

	// TODO add some more tests; should test actual UTF-8 conversions.
}



int	main()
{
#if 1
	printf("sizeof(tu_string) == %d\n", sizeof(tu_string));

	array<tu_string>	storage;
	storage.resize(2);

	tu_string&	a = storage[0];
	tu_string&	b = storage[1];
	a = "test1";
	
	printf("&a = 0x%X, &b = 0x%X\n", int(&a), int(&b));

	printf("%s\n", a.c_str());

	QASSERT(a == "test1");
	QASSERT(a.length() == 5);

	a += "2";
	QASSERT(a == "test12");

	a += "this is some more text";
	QASSERT(a.length() == 28);

	QASSERT(a[2] == 's');
	QASSERT(a[3] == 't');
	QASSERT(a[4] == '1');
	QASSERT(a[5] == '2');
	QASSERT(a[7] == 'h');
	QASSERT(a[28] == 0);

	QASSERT(b.length() == 0);
	QASSERT(b[0] == 0);
	QASSERT(b.c_str()[0] == 0);

	tu_string c = a + b;

	QASSERT(c.length() == a.length());

	c.resize(2);
	QASSERT(c == "te");
	QASSERT(c == tu_string("te"));

	QASSERT(tu_string("fourscore and sevent") == "fourscore and sevent");

	b = "#sacrificial lamb";

	// Test growing & shrinking.
	a = "";
	for (int i = 0; i < 1000; i++)
	{
		QASSERT(a.length() == i);

		if (i == 8)
		{
			QASSERT(a == "01234567");
		}
		else if (i == 27)
		{
			QASSERT(a == "012345678901234567890123456");
		}

		a.resize(a.length() + 1);
		a[a.length() - 1] = '0' + (i % 10);
	}

	{for (int i = 999; i >= 0; i--)
	{
		a.resize(a.length() - 1);
		QASSERT(a.length() == i);

		if (i == 8)
		{
			QASSERT(a == "01234567");
		}
		else if (i == 27)
		{
			QASSERT(a == "012345678901234567890123456");
		}
	}}

	// Test larger shrinking across heap/local boundary.
	a = "this is a string longer than 16 characters";
	a = "short";

	// Test larger expand across heap/local boundary.
	a = "another longer string...";

	QASSERT(b == "#sacrificial lamb");

	test_hash();
	test_stringi();
	test_stringi_hash();

	test_unicode();

	// TODO: unit tests for array<>, string_hash<>
#endif

	test_hash_speed();

	return 0;
}


#endif // CONTAINER_UNIT_TEST


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
