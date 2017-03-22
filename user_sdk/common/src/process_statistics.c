/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : process_statistics.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年10月24日
 * 描述                 : 
 ****************************************************************************/
#include "../inc/common.h"
#include "common_define.h"
/*
* hashlittle from lookup3.c, by Bob Jenkins, May 2006, Public Domain.
* http://burtleburtle.net/bob/c/lookup3.c
* minor modifications to make functions static so no symbols are exported
* minor mofifications to compile with -Werror
*/

#define HASH_LITTLE_ENDIAN (1)
/*
-------------------------------------------------------------------------------
lookup3.c, by Bob Jenkins, May 2006, Public Domain.

These are functions for producing 32-bit hashes for hash table lookup.
hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final()
are externally useful functions.  Routines to test the hash are included
if SELF_TEST is defined.  You can use this free for any purpose.  It's in
the public domain.  It has no warranty.

You probably want to use hashlittle().  hashlittle() and hashbig()
hash byte arrays.  hashlittle() is is faster than hashbig() on
little-endian machines.  Intel and AMD are little-endian machines.
On second thought, you probably want hashlittle2(), which is identical to
hashlittle() except it returns two 32-bit hashes for the price of one.
You could implement hashbig2() if you wanted but I haven't bothered here.

If you want to find a hash of, say, exactly 7 integers, do
a = i1;  b = i2;  c = i3;
mix(a,b,c);
a += i4; b += i5; c += i6;
mix(a,b,c);
a += i7;
final(a,b,c);
then use c as the hash value.  If you have a variable length array of
4-byte integers to hash, use hashword().  If you have a byte array (like
a character string), use hashlittle().  If you have several byte arrays, or
a mix of things, see the comments above hashlittle().

Why is this so big?  I read 12 bytes at a time into 3 4-byte integers,
then mix those integers.  This is fast (you can do a lot more thorough
mixing with 12*3 instructions on 3 integers than you can with 3 instructions
on 1 byte), but shoehorning those bytes into integers efficiently is messy.
-------------------------------------------------------------------------------
*/

#define hashsize(n) ((uint32_t)1 << (n))
#define hashmask(n) (hashsize(n) -1)
#define rot(x,k) (((x) << (k)) | ((x) >> (32-(k))))
/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
of top bits of (a,b,c), or in any combination of bottom bits of
(a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
is commonly produced by subtraction) look like a single 1-bit
difference.
* the base values were pseudorandom, all zero but one bit set, or
all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
4  6  8 16 19  4
9 15  3 18 27 15
14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}
/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
of top bits of (a,b,c), or in any combination of bottom bits of
(a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
is commonly produced by subtraction) look like a single 1-bit
difference.
* the base values were pseudorandom, all zero but one bit set, or
all zero plus a counter that starts at zero.

These constants passed:
14 11 25 16 4 14 24
12 14 25 16 4 14 24
and these came close:
4  8 15 26 3 22 24
10  8 15 26 3 22 24
11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}


/*
-------------------------------------------------------------------------------
HashLittle() -- hash a variable-length key into a 32-bit value
k       : the key (the unaligned variable-length array of bytes)
length  : the length of the key, counting by bytes
initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Two keys differing by one or two bits will have
totally different hash values.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo Hashw!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (uint8_t **)k, do it like this:
for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);

By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
-------------------------------------------------------------------------------
*/

static uint32_t HashLittle(const void *key, size_t length, uint32_t initval)
{
	uint32_t a, b, c; /* internal state */
	union
	{
		const void *ptr;
		size_t i;
	} u; /* needed for Mac Powerbook G4 */

	/* Set up the internal state */
	a = b = c = 0xdeadbeef + ((uint32_t) length) + initval;

	u.ptr = key;
	if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0))
	{
		const uint32_t *k = (const uint32_t *) key; /* read 32-bit chunks */

		/*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
		while (length > 12)
		{
			a += k[0];
			b += k[1];
			c += k[2];
			mix(a, b, c);
			length -= 12;
			k += 3;
		}

		/*----------------------------- handle the last (probably partial) block */
		/*
		 * "k[2]&0xffffff" actually reads beyond the end of the string, but
		 * then masks off the part it's not allowed to read.  Because the
		 * string is aligned, the masked-off tail is in the same word as the
		 * rest of the string.  Every machine with memory protection I've seen
		 * does it on word boundaries, so is OK with this.  But VALGRIND will
		 * still catch it and complain.  The masking trick does make the hash
		 * noticably faster for short strings (like English words).
		 */
#ifndef VALGRIND

		switch (length)
		{
		case 12:
			c += k[2];
			b += k[1];
			a += k[0];
			break;
		case 11:
			c += k[2] & 0xffffff;
			b += k[1];
			a += k[0];
			break;
		case 10:
			c += k[2] & 0xffff;
			b += k[1];
			a += k[0];
			break;
		case 9:
			c += k[2] & 0xff;
			b += k[1];
			a += k[0];
			break;
		case 8:
			b += k[1];
			a += k[0];
			break;
		case 7:
			b += k[1] & 0xffffff;
			a += k[0];
			break;
		case 6:
			b += k[1] & 0xffff;
			a += k[0];
			break;
		case 5:
			b += k[1] & 0xff;
			a += k[0];
			break;
		case 4:
			a += k[0];
			break;
		case 3:
			a += k[0] & 0xffffff;
			break;
		case 2:
			a += k[0] & 0xffff;
			break;
		case 1:
			a += k[0] & 0xff;
			break;
		case 0:
			return c; /* zero length strings require no mixing */
		}

#else /* make valgrind happy */

		const uint8_t *k8 = (const uint8_t *)k;
		switch (length)
		{
			case 12: c += k[2]; b += k[1]; a += k[0]; break;
			case 11: c += ((uint32_t)k8[10]) << 16; /* fall through */
			case 10: c += ((uint32_t)k8[9]) << 8; /* fall through */
			case 9: c += k8[8]; /* fall through */
			case 8: b += k[1]; a += k[0]; break;
			case 7: b += ((uint32_t)k8[6]) << 16; /* fall through */
			case 6: b += ((uint32_t)k8[5]) << 8; /* fall through */
			case 5: b += k8[4]; /* fall through */
			case 4: a += k[0]; break;
			case 3: a += ((uint32_t)k8[2]) << 16; /* fall through */
			case 2: a += ((uint32_t)k8[1]) << 8; /* fall through */
			case 1: a += k8[0]; break;
			case 0: return c;
		}

#endif /* !valgrind */

	}
	else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0))
	{
		const uint16_t *k = (const uint16_t *) key; /* read 16-bit chunks */
		const uint8_t *k8;

		/*--------------- all but last block: aligned reads and different mixing */
		while (length > 12)
		{
			a += k[0] + (((uint32_t) k[1]) << 16);
			b += k[2] + (((uint32_t) k[3]) << 16);
			c += k[4] + (((uint32_t) k[5]) << 16);
			mix(a, b, c);
			length -= 12;
			k += 6;
		}

		/*----------------------------- handle the last (probably partial) block */
		k8 = (const uint8_t *) k;
		switch (length)
		{
		case 12:
			c += k[4] + (((uint32_t) k[5]) << 16);
			b += k[2] + (((uint32_t) k[3]) << 16);
			a += k[0] + (((uint32_t) k[1]) << 16);
			break;
		case 11:
			c += ((uint32_t) k8[10]) << 16; /* fall through */
			/* no break */
		case 10:
			c += k[4];
			b += k[2] + (((uint32_t) k[3]) << 16);
			a += k[0] + (((uint32_t) k[1]) << 16);
			break;
		case 9:
			c += k8[8]; /* fall through */
			/* no break */
		case 8:
			b += k[2] + (((uint32_t) k[3]) << 16);
			a += k[0] + (((uint32_t) k[1]) << 16);
			break;
		case 7:
			b += ((uint32_t) k8[6]) << 16; /* fall through */
			/* no break */
		case 6:
			b += k[2];
			a += k[0] + (((uint32_t) k[1]) << 16);
			break;
		case 5:
			b += k8[4]; /* fall through */
			/* no break */
		case 4:
			a += k[0] + (((uint32_t) k[1]) << 16);
			break;
		case 3:
			a += ((uint32_t) k8[2]) << 16; /* fall through */
			/* no break */
		case 2:
			a += k[0];
			break;
		case 1:
			a += k8[0];
			break;
		case 0:
			return c; /* zero length requires no mixing */
		}

	}
	else
	{ /* need to read the key one byte at a time */
		const uint8_t *k = (const uint8_t *) key;

		/*--------------- all but the last block: affect some 32 bits of (a,b,c) */
		while (length > 12)
		{
			a += k[0];
			a += ((uint32_t) k[1]) << 8;
			a += ((uint32_t) k[2]) << 16;
			a += ((uint32_t) k[3]) << 24;
			b += k[4];
			b += ((uint32_t) k[5]) << 8;
			b += ((uint32_t) k[6]) << 16;
			b += ((uint32_t) k[7]) << 24;
			c += k[8];
			c += ((uint32_t) k[9]) << 8;
			c += ((uint32_t) k[10]) << 16;
			c += ((uint32_t) k[11]) << 24;
			mix(a, b, c);
			length -= 12;
			k += 12;
		}

		/*-------------------------------- last block: affect all 32 bits of (c) */
		switch (length)
		/* all the case statements fall through */
		{
		case 12:
			c += ((uint32_t) k[11]) << 24;
			/* no break */
		case 11:
			c += ((uint32_t) k[10]) << 16;
			/* no break */
		case 10:
			c += ((uint32_t) k[9]) << 8;
			/* no break */
		case 9:
			c += k[8];
			/* no break */
		case 8:
			b += ((uint32_t) k[7]) << 24;
			/* no break */
		case 7:
			b += ((uint32_t) k[6]) << 16;
			/* no break */
		case 6:
			b += ((uint32_t) k[5]) << 8;
			/* no break */
		case 5:
			b += k[4];
			/* no break */
		case 4:
			a += ((uint32_t) k[3]) << 24;
			/* no break */
		case 3:
			a += ((uint32_t) k[2]) << 16;
			/* no break */
		case 2:
			a += ((uint32_t) k[1]) << 8;
			/* no break */
		case 1:
			a += k[0];
			break;
		case 0:
			return c;
		}
	}

	final(a, b, c);
	return c;
}

/* get the address of entity cursor from entity address */
static inline StHashCursor *GetEntityCursorFromEntityInline(StHash *pHash, void *pEntity)
{
	return (StHashCursor *)((uint32_t)pEntity +
		(pHash->u32EntityWithIndexSize - sizeof(StHashCursor)));
}
/* get the address of the entity */
static inline void *GetEntityAddrInline(StHash *pHash, uint16_t u16Index)
{
	return (void *)((uint32_t)pHash + pHash->u32EntityOffset +
		(uint32_t)u16Index * pHash->u32EntityWithIndexSize);
}
/* get the cursor of the entity from the index */
static inline StHashCursor *GetEntityCursorInline(StHash *pHash, uint16_t u16Index)
{
#if 0
	StHashCursor *pCursor;
	uint32_t u32EntitySize = pHash->u32EntityWithIndexSize;
	uint32_t u32EntityAddr = (uint32_t)GetEntityAddrInline(pHash, u16Index);
	/* get the entity size and the entity cursor*/
	u32EntitySize -= sizeof(StHashCursor);
	pCursor = (StHashCursor *)(u32EntityAddr + u32EntitySize);
	return pCursor;
#else
	return GetEntityCursorFromEntityInline(pHash, GetEntityAddrInline(pHash, u16Index));
#endif
}

/* get the address of the addition data, if there is no addition data in the Hash, it will return NULL */
static inline void *GetAdditionDataAddr(StHash *pHash)
{
	if (pHash->u32AdditionDataSize == 0)
	{
		return NULL;
	}
	else
	{
		return (void *)((uint32_t)pHash + pHash->u32AdditionDataOffset);
	}
}

#define FLAG_FALG(addr, index)	addr[index >> 5] |= (1 << (index & 0x1F))
/* flag the flag bit for this index */
static inline void FlagFlag(StHash *pHash, uint16_t u16Index)
{
	uint32_t *pFlag = (uint32_t *)((uint32_t)pHash + pHash->u32FlagDataOffset);
#if 1
	FLAG_FALG(pFlag, u16Index);
#else
	pFlag[u16Index >> 5] |= (1 << (u16Index & 0x1F));
#endif
}

#define CLEAR_FLAG(addr, index)	addr[index >> 5] &= (~(1 << (index & 0x1F)))
/* clear the flag bit for this index */
static inline void ClearFlag(StHash *pHash, uint16_t u16Index)
{
	uint32_t *pFlag = (uint32_t *)((uint32_t)pHash + pHash->u32FlagDataOffset);
#if 1
	CLEAR_FLAG(pFlag, u16Index);
#else
	pFlag[u16Index >> 5] &= (~(1 << (u16Index & 0x1F)));
#endif
}

#define IS_FALG(addr, index)	((addr[index >> 5] >> (index & 0x1F)) & 0x01)
/* check whether the index is been flagged */
static inline bool IsFlag(StHash *pHash, uint16_t u16Index)
{
	uint32_t *pFlag = (uint32_t *)((uint32_t)pHash + pHash->u32FlagDataOffset);
#if 1
	if (IS_FALG(pFlag, u16Index) == 0x01)
#else
	if (((pFlag[u16Index >> 5] >> (u16Index & 0x1F)) & 0x01) == 0x01)
#endif
	{
		return true;
	}
	else
	{
		return false;
	}
}
/* reset the cursor clear the flag, and reduce the capacity */
static inline void DestroyACursor(StHash *pHash, StHashCursor *pCursor)
{
	pCursor->u16SIL = HASH_FREED;
	pCursor->u16Next = HASH_INVALID_INDEX;
	pCursor->u16Prev = HASH_INVALID_INDEX;
	ClearFlag(pHash, pCursor->u16Index);
	pHash->u16HashCurrentCapacity--;
}

/*
 * 函数名      : HashDestroy
 * 功能        : 释放该进程空间中的句柄占用的资源
 * 参数        : pHandle[in] (StHashHandle *): HashInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void HashDestroy(StHashHandle *pHandle)
{
	if (NULL == pHandle)
	{
		return;
	}

	shmdt(pHandle->pHash);
	free(pHandle);
}

/*
 * 函数名      : HashTombDestroy
 * 功能        : 释放该进程空间中的句柄占用的资源, 并从系统中删除资源
 * 参数        : pHandle[in] (StHashHandle *): HashInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void HashTombDestroy(StHashHandle *pHandle)
{
	if (NULL == pHandle)
	{
		return;
	}
	shmdt(pHandle->pHash);
	ReleaseAShmId(pHandle->s32SHMHandle);
	if (pHandle->boNeedLock)
	{
		LockClose(pHandle->s32LockHandle);
	}
	free(pHandle);
}

/* reset all cursor, flag data and additional data of the pHash */
static void HashResetStatic(StHash *pHash)
{
	/* clear all the cursor */
	StHashCursor *pCursor = (StHashCursor *)GetEntityCursorInline(pHash, 0);
	uint16_t i;
	for (i = 0; i < pHash->u16HashTotalCapacity; i++)
	{
		pCursor->u16Prev = HASH_INVALID_INDEX;
		pCursor->u16Next = HASH_INVALID_INDEX;
		pCursor->u16Index = i;
		pCursor->u16SIL = HASH_EMPTY;
		pCursor = (StHashCursor *)((uint32_t)pCursor + pHash->u32EntityWithIndexSize);
	}

	/* clear flag data */
	memset((void *)((uint32_t)pHash + pHash->u32FlagDataOffset), 0, pHash->u32FlagDataSize);
	/* clear addition data */
	if (pHash->u32AdditionDataSize != 0)
	{
		memset(GetAdditionDataAddr(pHash), 0, pHash->u32AdditionDataSize);
	}
	pHash->emPingPong = _Ping_Pong_Unused;
	pHash->u32Collisions = 0;
};

/*
 * 函数名      : HashReset
 * 功能        : 重置哈希表
 * 参数        : pHandle[in] (StHashHandle *类型): HashInit的返回值
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t HashReset(StHashHandle *pHandle)
{
	int32_t s32Err = 0;
	if (pHandle == NULL)
	{
		return  MY_ERR(_Err_InvalidParam);
	}
	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}

	HashResetStatic(pHandle->pHash);

	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
	return 0;
}


/*
 * 函数名      : HashInit
 * 功能        : 初始化哈希表，并返回句柄，必须与HashDestroy或者HashTombDestroy成对使用
 * 参数        : pName[in] (const char *类型): 共享内存的名字
 * 			   : u16Capacity[in] (uint16_t): 期望的容量
 * 			   : u32EntitySize[in] (uint32_t): 实体的大小
 * 			   : u32AdditionDataSize[in] (uint32_t): 附加数据的大小, 当值为(~0)时, 该区域大小和u32EntitySize一致
 * 			   : boNeedLock[in] (bool): 指明是否需要锁
 * 			   : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : StHashHandle * 型数据, NULL失败, 否则成功
 * 作者        : 许龙杰
 */
StHashHandle *HashInit(const char *pName, uint16_t u16Capacity, uint32_t u32EntitySize,
	uint32_t u32AdditionDataSize, bool boNeedLock, int32_t *pErr)
{
	uint32_t u32MemSize = 0;
	uint32_t i;
	StHash *pHash;
	/*StHashCursor *pCursor;*/
	int32_t s32LockHandle = -1, s32SHMHandle = -1;
	int32_t s32Err = 0;
	uint32_t u32FlagCnt = 0;
	uint32_t u32RealEntitySize;
	uint32_t u32RealCapacity;
	StHashHandle *pHashHandle;

	if ((pName == NULL) || (u16Capacity == 0) || (u32EntitySize == 0))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto err1;
	}
	if (strlen(pName) >= HASH_NAME_MAX_LENGTH)
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto err1;
	}
	if (u16Capacity > HASH_MAX_CAPACITY)
	{
		s32Err = MY_ERR(_Err_Hash_ToLarge);
		goto err1;
	}

	u32RealEntitySize = u32EntitySize;
	u32EntitySize = ((u32EntitySize + 3) / 4) * 4;

	if (u32AdditionDataSize == (~0))
	{
		u32FlagCnt = ((u16Capacity + 31) / 32) * 4;
		u32MemSize = sizeof(StHash) + u32FlagCnt * 2 +
			(u32EntitySize + sizeof(StHashCursor)) * u16Capacity;
	}
	else
	{
		u32AdditionDataSize = ((u32AdditionDataSize + 3) / 4) * 4;
		u32FlagCnt = ((u16Capacity + 31) / 32) * 4;
		u32MemSize = sizeof(StHash) + u32FlagCnt + u32AdditionDataSize +
			(u32EntitySize + sizeof(StHashCursor)) * u16Capacity;
	}



	PRINT("before aligning, the memory size is %d\n", u32MemSize);
	/* integral multiple of the system's page size */
	{
		int32_t s32PageSize = getpagesize();
		u32MemSize = ((u32MemSize + (s32PageSize - 1)) / s32PageSize) * s32PageSize;
	}
	PRINT("memory size is %d\n", u32MemSize);

	u32RealCapacity = (u32MemSize - (sizeof(StHash) + u32FlagCnt + u32AdditionDataSize)) /
		(u32EntitySize + sizeof(StHashCursor));
	if (u32RealCapacity > HASH_MAX_CAPACITY)
	{
		u32RealCapacity = HASH_MAX_CAPACITY;
	}
	/* readjust the capacity */
	if (u32AdditionDataSize == (~0))
	{
		uint32_t u32RealMemUsing = 0;
		u32RealCapacity += 1;
		do
		{
			u32RealCapacity -= 1;
			u32FlagCnt = ((u32RealCapacity + 31) / 32) * 4;
			u32RealMemUsing = sizeof(StHash) + u32FlagCnt  * 2 +
				(u32EntitySize + sizeof(StHashCursor)) * u32RealCapacity;
		} while (u32RealMemUsing > u32MemSize);
		u32AdditionDataSize = u32FlagCnt;
	}
	else
	{
		uint32_t u32RealMemUsing = 0;
		u32RealCapacity += 1;
		do
		{
			u32RealCapacity -= 1;
			u32FlagCnt = ((u32RealCapacity + 31) / 32) * 4;
			u32RealMemUsing = sizeof(StHash) + u32FlagCnt + u32AdditionDataSize +
				(u32EntitySize + sizeof(StHashCursor)) * u32RealCapacity;
		} while (u32RealMemUsing > u32MemSize);
	}
	u16Capacity = u32RealCapacity;

	if (boNeedLock)
	{
		s32LockHandle = LockOpen(pName);
		if (s32LockHandle < 0)
		{
			s32Err = s32LockHandle;
			goto err1;
		}
	}
	s32SHMHandle = GetTheShmId(pName, u32MemSize);
	if (s32SHMHandle < 0)
	{
		s32Err = s32SHMHandle;
		goto err2;
	}

	pHashHandle = calloc(1, sizeof(StHashHandle));
	if (NULL == pHashHandle)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto err3;
	}
	pHash = (StHash *)shmat(s32SHMHandle, NULL, 0);
	if (pHash == (StHash *)(-1))
	{
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err4;
	}
	pHashHandle->pHash = pHash;
	if (boNeedLock)
	{
		pHashHandle->s32LockHandle = s32LockHandle;
	}
	pHashHandle->boNeedLock = boNeedLock;
	pHashHandle->s32SHMHandle = s32SHMHandle;

	if (boNeedLock)
	{
		s32Err = LockLock(s32LockHandle);
		if (s32Err != 0)
		{
			goto err4;
		}
	}

	if ((pHash->stMemCheck.s32FlagHead == 0x01234567) &&
			(pHash->stMemCheck.s32FlagTail == 0x89ABCDEF) &&
			(pHash->stMemCheck.s32CheckSum == CheckSum((int32_t *)pHash, offsetof(StMemCheck, s32CheckSum) / sizeof(int32_t))))
	{
		PRINT("the shared memory has been initialized\n");
		goto ok;
	}

	PRINT("I will initialize the shared memory\n");
	pHash->stMemCheck.s32FlagHead = 0x01234567;
	pHash->stMemCheck.s32FlagTail = 0x89ABCDEF;

	strcpy(pHash->c8Name, pName);
	pHash->u32HashTotalSize = u32MemSize;
	pHash->u32RealEntitySize = u32RealEntitySize;
	pHash->u32EntityWithIndexSize = u32EntitySize + sizeof(StHashCursor);
	pHash->u32FlagDataSize = u32FlagCnt;
	pHash->u32FlagDataOffset = sizeof(StHash);
	pHash->u32AdditionDataSize = u32AdditionDataSize;
	pHash->u32AdditionDataOffset = pHash->u32FlagDataOffset + u32FlagCnt;
	pHash->u32EntityOffset = pHash->u32AdditionDataOffset + u32AdditionDataSize;
	pHash->u16HashTotalCapacity = u16Capacity;
	pHash->u16HashCurrentCapacity = 0;
	pHash->u16HeadIdle = 0;
	pHash->u16TailIdle = u16Capacity - 1;
	pHash->u16Head = HASH_INVALID_INDEX;
	pHash->u16Tail = HASH_INVALID_INDEX;
	pHash->u32Collisions = 0;
	pHash->emPingPong = _Ping_Pong_Unused;


#if 1
	pHash->u32Initval = 0;
#else
	srand((uint32_t)time(NULL));
	pHash->u32Initval = rand() * rand();
#endif
	PRINT("entity offset is 0x%08x\n", pHash->u32EntityOffset);

#if 1
	HashResetStatic(pHash);
#else
	pCursor = (StHashCursor *)GetEntityCursorInline(pHash, 0);
	for (i = 0; i < u16Capacity; i++)
	{
		pCursor->u16Prev = HASH_INVALID_INDEX;
		pCursor->u16Next = HASH_INVALID_INDEX;
		pCursor->u16Index = i;
		pCursor->u16SIL = HASH_EMPTY;
		pCursor = (StHashCursor *)((uint32_t)pCursor + pHash->u32EntityWithIndexSize);
	}

	memset((void *)((uint32_t)pHash + pHash->u32FlagDataOffset), 0, pHash->u32FlagDataSize);
	if (u32AdditionDataSize != 0)
	{
		memset(GetAdditionDataAddr(pHash), 0, u32AdditionDataSize);
	}
#endif
	for (i = 0; i < 8; i++)
	{
		pHash->stMemCheck.s16RandArray[i] = rand();
	}
	pHash->stMemCheck.s32CheckSum = CheckSum((int32_t *)pHash, offsetof(StMemCheck, s32CheckSum) / sizeof(int32_t));

ok:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	if (boNeedLock)
	{
		LockUnlock(s32LockHandle);
	}
	return pHashHandle;

err4:
	free(pHashHandle);
err3:
	ReleaseAShmId(s32SHMHandle);
err2:
	if (boNeedLock)
	{
		LockClose(s32LockHandle);
	}
err1:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}

	return NULL;
}

#if 0
static int32_t HashEnlarge(StHash **p2Hash)
{
	StHash *pHash = *p2Hash, *pHashNew;
	uint16_t u16Capacity;
	int32_t s32Err = 0;

	if (pHash->u16HashTotalCapacity > (HASH_MAX_CAPACITY / 2))
	{
		u16Capacity = HASH_MAX_CAPACITY;
	}
	else
	{
		u16Capacity = pHash->u16HashTotalCapacity * 2;
	}
	pHashNew = HashInit(pHash->c8Name, u16Capacity, pHash->u32RealEntitySize,
		pHash->u32AdditionDataSize, &s32Err);
	if (pHashNew == NULL)
	{
		return s32Err;
	}

	/* copy addition data */
	memcpy(GetAdditionDataAddr(pHashNew),
		GetAdditionDataAddr(pHash), pHash->u32AdditionDataSize);

	/* copy entity data */
	{
	uint32_t u32FlagCnt = pHash->u32FlagDataSize / sizeof(uint32_t);
	uint32_t *pFlag = (uint32_t *)((uint32_t)pHash + pHash->u32FlagDataOffset);
	uint32_t i;
	for (i = 0; i < u32FlagCnt; i++)
	{
		uint32_t u32Flag = pFlag[i];
		uint32_t j;
		for (j = 0; j < 32; j++)
		{
			if (((u32Flag >> j) & 0x01) != 0)
			{
				uint32_t n = i * 32 + j;
				StHashCursor *pCursor = GetEntityCursorInline(pHash, n);
				HashInsert(&pHashNew, pCursor->c8Key, -1,
					GetEntityAddrInline(pHash, n), true, &s32Err);
				if (s32Err != 0)
				{
					HashDestroy(pHashNew);
					return s32Err;
				}
			}
		}
	}
	}
	HashDestroy(pHash);
	*p2Hash = pHashNew;
	return 0;
}
#endif
/* insert some specially data with the pKey in the hash */
static uint16_t HashInsertStatic(StHash *pHash, const char *pKey,
	int32_t s32Length, void *pData, int32_t *pErr)
{
	int32_t s32Err = 0;
	uint32_t h, n = HASH_INVALID_INDEX;
	StHashCursor *pCursor;
	if ((pHash == NULL) || (pKey == NULL))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}

	if (s32Length == -1)
	{
		s32Length = strlen(pKey);
	}

	if ((s32Length <= 0) || (s32Length >= HASH_KEY_MAX_LENGTH))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}

	if (pHash->u16HashCurrentCapacity >=
		(pHash->u16HashTotalCapacity * HASH_LOAD_FACTOR))
	{
		s32Err = MY_ERR(_Err_Hash_NoSpace);
		goto end;
	}

	/* Interpreting keys as natural numbers */
	h = HashLittle(pKey, s32Length, pHash->u32Initval);
	n = h % pHash->u16HashTotalCapacity;

	/* Linear probing */
	pCursor = GetEntityCursorInline(pHash, n);
	while (1)
	{
		if ((pCursor->u16SIL == HASH_EMPTY) || (pCursor->u16SIL == HASH_FREED))
		{
			break;
		}
		else
		{
			if (strcmp(pKey, pCursor->c8Key) == 0)
			{
				n = HASH_INVALID_INDEX;
				s32Err = MY_ERR(_Err_Hash_Exist);
				goto end;
			}
			n++;
			if (n == pHash->u16HashTotalCapacity)
			{
				n = 0;
				pCursor = GetEntityCursorInline(pHash, n);
			}
			else
			{
				pCursor = (StHashCursor *)((uint32_t)pCursor + pHash->u32EntityWithIndexSize);
			}
			pHash->u32Collisions++;
		}
	}

	/* copy data */
	memcpy(pCursor->c8Key, pKey, s32Length);
	pCursor->c8Key[s32Length] = 0;
	pCursor->u16SIL = HASH_USING;
	if (pData != NULL)
	{
		memcpy(GetEntityAddrInline(pHash, n), pData, pHash->u32RealEntitySize);
	}
	pHash->u16HashCurrentCapacity++;
	FlagFlag(pHash, n);
	pHash->emPingPong = _Ping_Pong_Using;
end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return n;
}

/*
 * 函数名      : HashInsert
 * 功能        : 在哈希表中增加数据
 * 参数        : pHandle[in] (StHashHandle *类型): HashInit的返回值
 * 			   : pKey[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 * 			   : s32Length[in] (int32_t): 关键字的长度, -1表示让程序计算长度
 * 			   : pData[in] (void *):数据的实体指针
 * 			   : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : uint16_t 型数据, HASH_INVALID_INDEX表示失败, 否则成功表示数据将存于的索引
 * 作者        : 许龙杰
 */
uint16_t HashInsert(StHashHandle *pHandle, const char *pKey,
	int32_t s32Length, void *pData, int32_t *pErr)
{
	int32_t s32Err = 0;
	uint32_t n = HASH_INVALID_INDEX;
	if (pHandle == NULL)
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	if (pHandle->boNeedLock)
	{

		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			goto end;
		}
	}

	n = HashInsertStatic(pHandle->pHash, pKey, s32Length, pData, &s32Err);
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return n;
}

/* search a key an get the entity data */
static void *HashSearchStatic(StHash *pHash, const char *pKey, int32_t s32Length, int32_t *pErr)
{
	int32_t s32Err = 0;
	uint32_t h, n;
	StHashCursor *pCursor;
	if ((pHash == NULL) || (pKey == NULL))
	{
		s32Err =  MY_ERR(_Err_InvalidParam);
		goto err;
	}

	if (s32Length == -1)
	{
		s32Length = strlen(pKey);
	}

	if ((s32Length <= 0) || (s32Length >= HASH_KEY_MAX_LENGTH))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto err;
	}
	h = HashLittle(pKey, s32Length, pHash->u32Initval);
	n = h % pHash->u16HashTotalCapacity;
	pCursor = GetEntityCursorInline(pHash, n);

	while (1)
	{
		if (pCursor->u16SIL == HASH_EMPTY)
		{
			s32Err = MY_ERR(_Err_Hash_NotExist);
			goto err;
		}
		else if (pCursor->u16SIL != HASH_FREED)
		{
			if (strcmp(pKey, pCursor->c8Key) == 0)
			{
				return GetEntityAddrInline(pHash, n);
			}
		}

		n++;
		if (n == pHash->u16HashTotalCapacity)
		{
			n = 0;
			pCursor = GetEntityCursorInline(pHash, n);
		}
		else
		{
			pCursor = (StHashCursor *)((uint32_t)pCursor + pHash->u32EntityWithIndexSize);
		}

	}
err:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return NULL;

}

/*
 * 函数名      : HashSearch
 * 功能        : 在哈希表中增加关键字是pKey数据
 * 参数        : pHandle[in] (StHashHandle *类型): 共享内存的名字
 * 			   : pKey[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 * 			   : s32Length[in] (int32_t): 关键字的长度, -1表示让程序计算长度
 * 			   : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : void * 型数据, NULL表示失败, 否则成功, 注意返回的指针不受互斥量的保护
 * 作者        : 许龙杰
 */
void *HashSearch(StHashHandle *pHandle, const char *pKey, int32_t s32Length, int32_t *pErr)
{
	int32_t s32Err = 0;
	void *pReturn = NULL;
	if (pHandle == NULL)
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}

	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			goto end;
		}
	}

	pReturn = HashSearchStatic(pHandle->pHash, pKey, s32Length, &s32Err);
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pReturn;
}

/* delete an entity which the key is pKey */
static int32_t HashDeleteStatic(StHash *pHash, const char *pKey, int32_t s32Length)
{
	int32_t s32Err = 0;
	void *pEntity = HashSearchStatic(pHash, pKey, s32Length, &s32Err);
	if (pEntity == NULL)
	{
		return s32Err;
	}
	else
	{
		StHashCursor *pCursor = GetEntityCursorFromEntityInline(pHash, pEntity);
		DestroyACursor(pHash, pCursor);
		return 0;
	}
}

/*
 * 函数名      : HashDelete
 * 功能        : 在哈希表中删除关键字是pKey
 * 参数        : pHandle[in] (StHashHandle *类型): 共享内存的名字
 * 			   : pKey[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 * 			   : s32Length[in] (int32_t): 关键字的长度, -1表示让程序计算长度
 * 			   : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t HashDelete(StHashHandle *pHandle, const char *pKey, int32_t s32Length)
{
	int32_t s32Err = 0;
	if (pHandle == NULL)
	{
		return  MY_ERR(_Err_InvalidParam);
	}
	if (pHandle->boNeedLock)
	{

		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}

	s32Err = HashDeleteStatic(pHandle->pHash, pKey, s32Length);
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
	return 0;
}

/* print a hash */
int32_t HashPrint(StHash *pHash)
{
	if (pHash == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		uint32_t u32FlagCnt = pHash->u32FlagDataSize / sizeof(uint32_t);
		uint32_t *pFlag = (uint32_t *)((uint32_t)pHash + pHash->u32FlagDataOffset);
		uint32_t i;
		uint32_t u32TotalPrintCnt = 0;
		for (i = 0; i < u32FlagCnt; i++)
		{
			uint32_t u32Flag = pFlag[i];
			uint32_t j;
			for (j = 0; j < 32; j++)
			{
				if (((u32Flag >> j) & 0x01) != 0)
				{
					PRINT("index: %d, key: %s\n", i * 32 + j, GetEntityCursorInline(pHash, i * 32 + j)->c8Key);
					u32TotalPrintCnt++;
				}
			}
		}
		PRINT("total print count: %d\n", u32TotalPrintCnt);
		PRINT("Collisions: %d\n", pHash->u32Collisions);
		return 0;
	}
}

/* transfer a hash for process statistic form pSrc to pDest */
static int32_t ProcessStatisticTansferStatic(StHash *pDest, StHash *pSrc)
{
	uint32_t *pFlag = (uint32_t *)GetAdditionDataAddr(pSrc);
	uint32_t u32FlagCnt = pSrc->u32AdditionDataSize / sizeof(uint32_t);
	uint32_t i, j;
	int32_t s32Err = 0;
	pSrc->emPingPong = _Ping_Pong_Transfer;
	for (i = 0; i < u32FlagCnt; i++)
	{
		uint32_t u32Flag = pFlag[i];
		for (j = 0; j < 32; j++)
		{
			if (((u32Flag >> j) & 0x01) != 0)
			{
				/* process information */
				uint32_t n = i * 32 + j;
				StHashCursor *pCursorDest;
				StHashCursor *pCursorSrc = GetEntityCursorInline(pSrc, n);
				uint16_t u16Index = HashInsertStatic(pDest, pCursorSrc->c8Key, -1,
						GetEntityAddrInline(pSrc, pCursorSrc->u16Index), &s32Err);

				if (u16Index == HASH_INVALID_INDEX)
				{
					PRINT("HashInsertStatic error: 0x%08x\n", s32Err);
					return s32Err;
				}
				else
				{
					/* flag process information */
					uint32_t *pFlag = (uint32_t *)GetAdditionDataAddr(pDest);
					FLAG_FALG(pFlag, u16Index);
				}

				PRINT("index: %d, key: %s\n", pCursorSrc->u16Index, pCursorSrc->c8Key);

				/* first, get the cursor information from destination process index */
				pCursorDest = GetEntityCursorInline(pDest, u16Index);

				/* thread information of this process */
				while (pCursorSrc->u16Next != HASH_INVALID_INDEX)
				{
					uint16_t u16IndexDest;

					pCursorSrc =  GetEntityCursorInline(pSrc, pCursorSrc->u16Next);

					PRINT("index: %d, key: %s\n", pCursorSrc->u16Index, pCursorSrc->c8Key);

					/* second, insert the thread information to the destination */
					u16IndexDest =  HashInsertStatic(pDest, pCursorSrc->c8Key, -1,
							GetEntityAddrInline(pSrc, pCursorSrc->u16Index), &s32Err);
					if (u16IndexDest == HASH_INVALID_INDEX)
					{
						PRINT("HashInsertStatic error: 0x%08x\n", s32Err);
						return s32Err;
					}
					else
					{
						StHashCursor *pTmp = GetEntityCursorInline(pDest, u16IndexDest);
						pTmp->u16Prev = pCursorDest->u16Index;
						pCursorDest->u16Next = pTmp->u16Index;
						pTmp->u16Next = HASH_INVALID_INDEX;
						/* update the cursor to latest  */
						pCursorDest = pTmp;
					}
				}
				PRINT("\n\n\n");
			}
		}
	}
	return 0;

}

/* transfer a hash for process statistic form pSrc to pDest */
static int32_t ProcessStatisticTansfer(StHashHandle *pDest, StHashHandle *pSrc)
{
	int32_t s32Err = 0;
	if ((pDest == NULL) || (pSrc == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pDest->boNeedLock)
	{
		s32Err = LockLock(pDest->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}
	if (pSrc->boNeedLock)
	{
		s32Err = LockLock(pSrc->s32LockHandle);
		if (s32Err != 0)
		{
			if (pDest->boNeedLock)
			{
				LockUnlock(pDest->s32LockHandle);
			}
			return s32Err;
		}
	}
	s32Err = ProcessStatisticTansferStatic(pDest->pHash, pSrc->pHash);

	if (pSrc->boNeedLock)
	{
		LockUnlock(pSrc->s32LockHandle);
	}
	if (pDest->boNeedLock)
	{
		LockUnlock(pDest->s32LockHandle);
	}
	return s32Err;
}


/*
 * 函数名      : ProcessStatisticInit
 * 功能        : 进程/线程统计函数初始化，必须与HashDestroy或者HashTombDestroy成对使用
 * 参数        : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : StHashHandle * 型数据, NULL失败, 否则成功
 * 作者        : 许龙杰
 */
StHashHandle *ProcessStatisticInit(int32_t *pErr)
{
	StHashHandle *pHandle = NULL, *pPing, *pPong;
	int32_t s32Err;

	pPing = HashInit(HASH_NAME_PING, HASH_INIT_SIZE, sizeof(UnProcessInfo), -1, false, &s32Err);
	if (pPing == NULL)
	{
		goto end;
	}

	pPong = HashInit(HASH_NAME_PONG, HASH_INIT_SIZE, sizeof(UnProcessInfo), -1, false, &s32Err);
	if (pPong == NULL)
	{
		HashTombDestroy(pPing);
		goto end;
	}

	if (pPing->pHash->emPingPong == pPong->pHash->emPingPong)
	{
		/* chaos, destroy the PONG and return */
		HashTombDestroy(pPong);
		pHandle = pPing;
		if (pHandle->pHash->emPingPong == _Ping_Pong_Transfer)
		{
			HashReset(pHandle);
		}
		goto end;
	}

	if ((pPing->pHash->emPingPong == _Ping_Pong_Using) && (pPong->pHash->emPingPong == _Ping_Pong_Unused))
	{
		/* destroy the PONG and return */
		HashTombDestroy(pPong);
		pHandle = pPing;
	}
	else if ((pPong->pHash->emPingPong == _Ping_Pong_Using) && (pPing->pHash->emPingPong == _Ping_Pong_Unused))
	{
		/* destroy the PING and return */
		HashTombDestroy(pPing);
		pHandle = pPong;
	}
	else if (pPing->pHash->emPingPong == _Ping_Pong_Transfer)
	{
		/* transfer PING to PONG */
		HashReset(pPong);
		ProcessStatisticTansfer(pPong, pPing);
		HashTombDestroy(pPing);
		pHandle = pPong;
	}
	else
	{
		/* transfer PONG to PING */
		HashReset(pPing);
		ProcessStatisticTansfer(pPing, pPong);
		HashTombDestroy(pPong);
		pHandle = pPing;
	}

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pHandle;
}

/* enlarge the hash for process statistic */
static StHashHandle *ProcessStatisticEnlarge(StHash *pHash, int32_t *pErr)
{
	StHashHandle *pHandle = NULL;
	int32_t s32Err = 0;
	const char *pName;
	if (strcmp(pHash->c8Name, HASH_NAME_PING) == 0)
	{
		pName = HASH_NAME_PONG;
	}
	else
	{
		pName = HASH_NAME_PING;
	}
	pHandle = HashInit(pName, pHash->u16HashTotalCapacity * 2, pHash->u32RealEntitySize, -1, false, pErr);

	if (pHandle == NULL)
	{
		return NULL;
	}
	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			goto err;
		}
	}
	HashResetStatic(pHandle->pHash);
	s32Err = ProcessStatisticTansferStatic(pHandle->pHash, pHash);

	if (s32Err != 0)
	{
		if (pHandle->boNeedLock)
		{
			LockUnlock(pHandle->s32LockHandle);
		}
		goto err;
	}

	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}

	return pHandle;
err:
	HashTombDestroy(pHandle);
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return NULL;
}

/*
 * 函数名      : ProcessStatisticInsertAProcess
 * 功能        : 插入一个进程名字
 * 参数        : p2Handle (StHashHandle **): 指向ProcessStatisticInit的返回值的指针
 *             : pName[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticInsertAProcess(StHashHandle **p2Handle, const char *pName)
{
	int32_t s32Err = 0;
	uint16_t u16Index;
	StHashHandle *pHandle;
	if (p2Handle == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pHandle = *p2Handle;
	if (pHandle == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}

	u16Index = HashInsertStatic(pHandle->pHash, pName, -1, NULL, &s32Err);

	if (u16Index == HASH_INVALID_INDEX)
	{
		if (s32Err == MY_ERR(_Err_Hash_NoSpace))
		{
			/* enlarge the hash */
			StHashHandle *pHandleNew;
			pHandleNew = ProcessStatisticEnlarge(pHandle->pHash, &s32Err);
			if (pHandleNew != NULL)
			{
				if(pHandle->boNeedLock)
				{
					LockUnlock(pHandle->s32LockHandle);
				}
				HashTombDestroy(pHandle);
				*p2Handle = pHandleNew;
				return ProcessStatisticInsertAProcess(p2Handle, pName);
			}
			else
			{
				PRINT("ProcessStatisticEnlarge error: 0x%08x\n", s32Err);
				goto end;
			}
		}
		else
		{
			PRINT("HashInsertStatic error: 0x%08x\n", s32Err);
			goto end;
		}
	}
	else
	{
		/* flag process information */
		uint32_t *pFlag = (uint32_t *)GetAdditionDataAddr(pHandle->pHash);
		FLAG_FALG(pFlag, u16Index);
		memset(GetEntityAddrInline(pHandle->pHash, u16Index), 0, pHandle->pHash->u32RealEntitySize);
	}

end:
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
	return s32Err;
}


/*
 * 函数名      : ProcessStatisticUpdateAThread
 * 功能        : 插入/更新已有进程的某一个线程
 * 参数        : p2Handle (StHashHandle **): 指向ProcessStatisticInit的返回值的指针
 *             : pProcessName[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 *             : s32Tid[in] (int32_t *): 该进程(pProcessName)中的一个线程ID号. 注意这里是TID应该是PID的表现方式
 *             : pFun (PFUN_Hash_OperateData): 当检索到(将_Hash_Traversal_Thread_Update传递给pFun的s32Flag参数)或者
 *               新增加(将_Hash_Traversal_Thread_New传递给pFun的s32Flag参数)时调用该函数. 此函数不理会pFun的返回值
 *             : pContext[in] (void *): 传递给pFun的上下文指针
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticUpdateAThread(StHashHandle **p2Handle, const char *pProcessName,
		int32_t s32Tid, PFUN_Hash_OperateData pFun, void *pContext)
{
	int32_t s32Err = 0;
	StHashHandle *pHandle;
	if ((p2Handle == NULL) || (pProcessName == NULL) || (s32Tid < 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pHandle = *p2Handle;
	if (pHandle == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}
	do
	{
		void *pMain = NULL;
		void *pEntity = NULL;
		StHashCursor *pCursor;

		char c8Name[64];
		pMain = HashSearchStatic(pHandle->pHash, pProcessName, -1, &s32Err);
		if (pMain == NULL)
		{
			goto end;
		}

		sprintf(c8Name, "%s_%d", pProcessName, s32Tid);

		pEntity = HashSearchStatic(pHandle->pHash, c8Name, -1, &s32Err);
		if (pEntity == NULL)
		{
			uint16_t u16Index;
			PRINT("%s is not in the hash\n", c8Name);
			/* first, get the information from proc */
			/* second, insert the name with the info */
			pCursor = GetEntityCursorFromEntityInline(pHandle->pHash, pMain);
			while (pCursor->u16Next != HASH_INVALID_INDEX)
			{
				pCursor = GetEntityCursorInline(pHandle->pHash, pCursor->u16Next);
			}
			u16Index = HashInsertStatic(pHandle->pHash, c8Name, -1, NULL, &s32Err);

			if (u16Index == HASH_INVALID_INDEX)
			{
				if (s32Err == MY_ERR(_Err_Hash_NoSpace))
				{
					/* enlarge the hash */
					StHashHandle *pHandleNew;
					pHandleNew = ProcessStatisticEnlarge(pHandle->pHash, &s32Err);
					if (pHandleNew != NULL)
					{
						if(pHandle->boNeedLock)
						{
							LockUnlock(pHandle->s32LockHandle);
						}
						HashTombDestroy(pHandle);
						*p2Handle = pHandleNew;
						return ProcessStatisticUpdateAThread(p2Handle, pProcessName, s32Tid, pFun, pContext);
					}
					else
					{
						PRINT("ProcessStatisticEnlarge error: 0x%08x\n", s32Err);
						goto end;
					}
				}
				else
				{
					PRINT("HashInsertStatic error: 0x%08x\n", s32Err);
					goto end;
				}
			}
			else
			{
				/* a new thread, and link it to the end */
				StHashCursor *pTmp = GetEntityCursorInline(pHandle->pHash, u16Index);
				pTmp->u16Prev = pCursor->u16Index;
				pCursor->u16Next = pTmp->u16Index;
				pTmp->u16Next = HASH_INVALID_INDEX;
				if (pFun != NULL)
				{
					pFun(_Hash_Traversal_Thread_New, pTmp, GetEntityAddrInline(pHandle->pHash, u16Index), pContext);
				}

			}
		}
		else
		{
			if (pFun != NULL)
			{
				pFun(_Hash_Traversal_Thread_Update,
						GetEntityCursorFromEntityInline(pHandle->pHash, pEntity),
						pEntity, pContext);
			}
		}
	}while(0);

end:
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}

	return s32Err;
}

/* destroy a process cursor and it's thread cursor */
static void  ProcessStatisticDestroyAProcessCursor(StHash *pHash, StHashCursor *pCursor)
{
	uint32_t *pFlag = (uint32_t *)GetAdditionDataAddr(pHash);

	CLEAR_FLAG(pFlag, pCursor->u16Index);

	while (pCursor->u16Next != HASH_INVALID_INDEX)
	{
		StHashCursor *pTmp = GetEntityCursorInline(pHash, pCursor->u16Next);
		DestroyACursor(pHash, pCursor);
		pCursor = pTmp;
	}
	DestroyACursor(pHash, pCursor);
}

/*
 * 函数名      : ProcessStatisticDeleteAProcess
 * 功能        : 从表中删除某一个进程
 * 参数        : p2Handle (StHashHandle *): ProcessStatisticInit的返回值
 *             : pName[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticDeleteAProcess(StHashHandle *pHandle, const char *pName)
{
	int32_t s32Err = 0;
	void *pData;
	if ((pHandle == NULL) || (pName == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}
	pData = HashSearchStatic(pHandle->pHash, pName, -1, &s32Err);
	if (pData == NULL)
	{
		goto end;
	}

#if 1

	ProcessStatisticDestroyAProcessCursor(pHandle->pHash,
			GetEntityCursorFromEntityInline(pHandle->pHash, pData));
#else
	do
	{
		StHashCursor *pCursor = GetEntityCursorFromEntityInline(pHandle->pHash, pData);
		uint32_t *pFlag = (uint32_t *)GetAdditionDataAddr(pHandle->pHash);

		CLEAR_FLAG(pFlag, pCursor->u16Index);

		while (pCursor->u16Next != HASH_INVALID_INDEX)
		{
			StHashCursor *pTmp = GetEntityCursorInline(pHandle->pHash, pCursor->u16Next);
			DestroyACursor(pHandle->pHash, pCursor);
			pCursor = pTmp;
		}
		DestroyACursor(pHandle->pHash, pCursor);

	}while(0);
#endif

end:
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
	return s32Err;

}

/* destroy a process cursor and it's thread cursor */
static void  ProcessStatisticDestroyAThreadCursor(StHash *pHash, StHashCursor *pCursor)
{
	if (IsFlag(pHash, pCursor->u16Index))
	{
		StHashCursor *pNext = NULL, *pPrev = NULL;
		if (pCursor->u16Next != HASH_INVALID_INDEX)
		{
			pNext = GetEntityCursorInline(pHash, pCursor->u16Next);
			pNext->u16Prev = pCursor->u16Prev;
		}
		if (pCursor->u16Prev != HASH_INVALID_INDEX)
		{
			pPrev = GetEntityCursorInline(pHash, pCursor->u16Prev);
			pPrev->u16Next = pCursor->u16Next;
		}
		DestroyACursor(pHash, pCursor);

	}
}
/*
 * 函数名      : ProcessStatisticDeleteAThread
 * 功能        : 删除已有进程的某一个线程
 * 参数        : p2Handle (StHashHandle *): ProcessStatisticInit的返回值
 *             : pProcessName[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 *             : s32Tid[in] (int32_t *): 该进程(pProcessName)中的一个线程ID号. 注意这里是TID应该是PID的表现方式
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticDeleteAThread(StHashHandle *pHandle, const char *pProcessName, int32_t s32Tid)
{
	int32_t s32Err = 0;
	if ((pHandle == NULL) || (pProcessName == NULL) || (s32Tid < 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}
	do
	{
#if 1
		void *pData;
		char c8Name[64];
		sprintf(c8Name, "%s_%d", pProcessName, s32Tid);
		pData = HashSearchStatic(pHandle->pHash, c8Name, -1, &s32Err);
		if (pData == NULL)
		{
			goto end;
		}
		ProcessStatisticDestroyAThreadCursor(pHandle->pHash,
				GetEntityCursorFromEntityInline(pHandle->pHash, pData));
#else
		void *pData;
		char c8Name[64];
		StHashCursor *pCursor, *pNext = NULL, *pPrev = NULL;
		sprintf(c8Name, "%s_%d", pProcessName, s32Tid);
		pData = HashSearchStatic(pHandle->pHash, c8Name, -1, &s32Err);
		if (pData == NULL)
		{
			goto end;
		}
		pCursor = GetEntityCursorFromEntityInline(pHandle->pHash, pData);
		if (pCursor->u16Next != HASH_INVALID_INDEX)
		{
			pNext = GetEntityCursorInline(pHandle->pHash, pCursor->u16Next);
			pNext->u16Prev = pCursor->u16Prev;
		}
		if (pCursor->u16Prev != HASH_INVALID_INDEX)
		{
			pPrev = GetEntityCursorInline(pHandle->pHash, pCursor->u16Prev);
			pPrev->u16Next = pCursor->u16Next;
		}

		DestroyACursor(pHandle->pHash, pCursor);
#endif
	}while(0);


end:
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
	return s32Err;
}

/*
 * 函数名      : ProcessStatisticSearchAProcess
 * 功能        : 插入/更新已有进程的某一个线程
 * 参数        : p2Handle (StHashHandle *): ProcessStatisticInit的返回值
 *             : pName[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 *             : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : void * 型数据, NULL表示失败, 否则成功, 注意返回的指针不受互斥量的保护
 * 作者        : 许龙杰
 */
void *ProcessStatisticSearchAProcess(StHashHandle *pHandle, const char *pName, int32_t *pErr)
{
	void *pData = NULL;
	int32_t s32Err = 0;
	if ((pHandle == NULL) || (pName == NULL))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	if (pHandle->boNeedLock)
	{

		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			goto end;
		}
	}
	pData = HashSearchStatic(pHandle->pHash, pName, -1, &s32Err);

	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pData;
}

/*
 * 函数名      : ProcessStatisticUpdateAProcess
 * 功能        : 更新已有的进程
 * 参数        : p2Handle (StHashHandle *): ProcessStatisticInit的返回值
 *             : pName[in] (const char *): 关键字, 必须是以'\0'结尾的字符串
 *             : pFun (PFUN_Hash_OperateData): 当检索到(将_Hash_Traversal_Process_Update传递给pFun的s32Flag参数),
 *               此函数不理会pFun的返回值
 *             : pContext[in] (void *): 传递给pFun的上下文指针
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticUpdateAProcess(StHashHandle *pHandle, const char *pName,
					PFUN_Hash_OperateData pFun, void *pContext)
{
	void *pData = NULL;
	int32_t s32Err = 0;
	if ((pHandle == NULL) || (pName == NULL))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}

	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			goto end;
		}
	}
	pData = HashSearchStatic(pHandle->pHash, pName, -1, &s32Err);
	if (pData != NULL)
	{
		StHashCursor *pCursor = GetEntityCursorFromEntityInline(pHandle->pHash, pData);
		uint32_t *pAdditionData = GetAdditionDataAddr(pHandle->pHash);
		if (IS_FALG(pAdditionData, pCursor->u16Index))
		{
			if (pFun != NULL)
			{
				s32Err = pFun(_Hash_Traversal_Process_Update, pCursor, pData, pContext);
			}
		}
		else
		{
			s32Err = MY_ERR(_Err_Hash_NotAProcess);
		}
	}
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
end:
	return s32Err;
}
/*
 * 函数名      : ProcessStatisticTraversal
 * 功能        : 以特有的方式遍历, 首先得到进程, 然后遍历该进程的所有线程
 * 参数        : p2Handle (StHashHandle *): ProcessStatisticInit的返回值针
 *             : pFun (PFUN_Hash_OperateData): 当检索到进程(将_Hash_Traversal_Process传递给pFun的s32Flag参数)或者
 *               当检索到线程(将_Hash_Traversal_Thread传递给pFun的s32Flag参数)时调用该函数, 如果该回调函数返回-1表示
 *               该线程或者进程存在问题, 程序将会将之从哈希表正删除, 返回0 表示正常, 其他表示错误
 *             : pContext[in] (void *): 传递给pFun的上下文指针
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticTraversal(StHashHandle *pHandle, PFUN_Hash_OperateData pFun, void *pContext)
{
	int32_t s32Err = 0;
	if (pHandle == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pHandle->boNeedLock)
	{
		s32Err = LockLock(pHandle->s32LockHandle);
		if (s32Err != 0)
		{
			return s32Err;
		}
	}
	do
	{
		StHash *pHash = pHandle->pHash;
		uint32_t *pFlag = (uint32_t *)GetAdditionDataAddr(pHash);
		uint32_t u32FlagCnt = pHash->u32AdditionDataSize / sizeof(uint32_t);
		uint32_t i, j;
		for (i = 0; i < u32FlagCnt; i++)
		{
			uint32_t u32Flag = pFlag[i];
			if (u32Flag == 0)
			{
				continue;
			}
			/* now, begin to get the process */
			for (j = 0; j < 32; j++)
			{
				/* now, I got the process, it's index is i * 32 + j */
				if (((u32Flag >> j) & 0x01) != 0)
				{
					uint32_t n = i * 32 + j;
					StHashCursor *pCursor = GetEntityCursorInline(pHash, n);
					if (pFun != NULL)
					{
						/* call the callback function, tell it that the program will begin to traversal it's thread */
						s32Err = pFun(_Hash_Traversal_Process, pCursor, GetEntityAddrInline(pHash, n), pContext);
						if (s32Err == -1)
						{
							ProcessStatisticDestroyAProcessCursor(pHash, pCursor);
						}
						else if (s32Err < 0)
						{
							goto end;
						}

					}
					while (pCursor->u16Next != HASH_INVALID_INDEX)
					{
						pCursor = GetEntityCursorInline(pHash, pCursor->u16Next);
						if (pFun != NULL)
						{
							/* call the callback function, tell it that it's a thread */
							s32Err = pFun(_Hash_Traversal_Thread, pCursor,
									GetEntityAddrInline(pHash, pCursor->u16Index), pContext);
							if (s32Err == -1)
							{
								StHashCursor *pTmp = pCursor;
								if(pCursor->u16Next == HASH_INVALID_INDEX)
								{
									ProcessStatisticDestroyAThreadCursor(pHash, pTmp);
									break;
								}
								pCursor = GetEntityCursorInline(pHash, pCursor->u16Next);
								ProcessStatisticDestroyAThreadCursor(pHash, pTmp);
							}
							else if (s32Err < 0)
							{
								goto end;
							}
						}
					}
					pCursor = GetEntityCursorInline(pHash, n);
					if (pFun != NULL)
					{
						/* call the callback function, tell it that the program has finished to traversal it's thread */
						s32Err = pFun(_Hash_Traversal_Process_Out, pCursor, GetEntityAddrInline(pHash, n), pContext);
						if (s32Err == -1)
						{
							ProcessStatisticDestroyAProcessCursor(pHash, pCursor);
						}
						else if (s32Err < 0)
						{
							goto end;
						}
					}
				}
			}
		}
	} while (0);

end:
	if (pHandle->boNeedLock)
	{
		LockUnlock(pHandle->s32LockHandle);
	}
	return s32Err;
}

/* callback function of ProcessStatisticPrint */
static int32_t ProcessStatisticPrintCB(int32_t s32Flag, const StHashCursor *pCursor, void *pData, void *pUnused)
{

	if (s32Flag == _Hash_Traversal_Process)
	{
		PRINT("Process(key): %s, hash index: %d\n", pCursor->c8Key, pCursor->u16Index);
	}
	else if (s32Flag == _Hash_Traversal_Thread)
	{
		PRINT("Thread(key): %s, hash index: %d\n", pCursor->c8Key, pCursor->u16Index);
	}
	else
	{
		PRINT("\n\n");
	}

	return 0;
}
/*
 * 函数名      : ProcessStatisticPrint
 * 功能        : 以特有的方式打印, 首先打印进程, 然后打印该进程的所有线程
 * 参数        : p2Handle (StHashHandle *): ProcessStatisticInit的返回值针
 * 返回值      : int32_t 型数据, 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticPrint(StHashHandle *pHandle)
{
	return ProcessStatisticTraversal(pHandle, ProcessStatisticPrintCB, NULL);
}

#if 0

typedef struct _tagStProcessStatisticInfo
{
	StSYSInfo stSYSInfo;				/* 当前的系统状态 */
	StProcessStat stThreadStat;			/* 进程/线程的当前状态 */
	StProcessInfo stThreadInfo;			/* 保存进程名字等一些信息 */
	StHashHandle *pHandle;				/* 哈希表的句柄 */
	UnProcessInfo *pProcessInfo;		/* 在统计结束后, 检查各个进程的状态时使用*/
}StProcessStatisticInfo;


const char *pProcessName[6] =
{
	"main_proc",
	"upgrade",
	"communication",
	"firefox",
	"test_upgrade",
	"test",
};

/* the callback of ProcessStatisticUpdateAThread called by TraversalDirThreadCallback */
/*
1, 采样两个足够短的时间间隔的Cpu快照，分别记作t1,t2，其中t1、t2的结构均为：
	(user, nice, system, idle, iowait, irq, softirq, stealstolen, guest)的9元组;
2,   计算总的Cpu时间片totalCpuTime
	a)         把第一次的所有cpu使用情况求和，得到s1;
	b)         把第二次的所有cpu使用情况求和，得到s2;
	c)         s2 - s1得到这个时间间隔内的所有时间片，即totalCpuTime = j2 - j1 ;
3, 计算空闲时间idle
	idle对应第四列的数据，用第二次的第四列 - 第一次的第四列即可
	idle=第二次的第四列 - 第一次的第四列
4, 计算cpu使用率
	pcpu =100* (total-idle)/total
*/
static int32_t UpdateAThreadCallback(int32_t s32Flag, const StHashCursor* pCursor, void *pData, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;
	StProcessInfo *pThreadInfo = &(((UnProcessInfo *)pData)->stThreadInfo);
	StProcessStat *pThreadStat = &(pOutsideInfo->stThreadStat);

	pThreadInfo->u32StatisticTimes = pOutsideInfo->stThreadInfo.u32StatisticTimes;
	if (s32Flag == _Hash_Traversal_Thread_New)
	{
		memcpy(pThreadInfo->c8Name, pOutsideInfo->stThreadInfo.c8Name, 64);
		pThreadInfo->u32Pid = pOutsideInfo->stThreadInfo.u32Pid;
		pThreadInfo->u32SysTime = pThreadStat->u32Stime;
		pThreadInfo->u32UserTime = pThreadStat->u32Utime;
		pThreadInfo->u32RSS = pThreadStat->s32Rss;
		pThreadInfo->u16CPUUsage = 0;
		pThreadInfo->u16CPUAverageUsage = 0;
		pThreadInfo->u16MemUsage = 0;
		pThreadInfo->u16MemAverageUsage = 0;
	}
	else
	{
		StSYSInfo *pSYSInfo = &(pOutsideInfo->stSYSInfo);
		uint64_t u64Tmp2 = pThreadInfo->u32SysTime + pThreadInfo->u32UserTime;
		uint64_t u64Tmp;
		pThreadInfo->u32SysTime = pThreadStat->u32Stime;
		pThreadInfo->u32UserTime = pThreadStat->u32Utime;

		u64Tmp = pThreadInfo->u32SysTime + pThreadInfo->u32UserTime;

		u64Tmp = (u64Tmp - u64Tmp2) * 10000 /
				(pSYSInfo->stCPU.u64Total - pSYSInfo->stCPU.u64PrevTotal);
		u64Tmp *= sysconf(_SC_NPROCESSORS_ONLN);
		pThreadInfo->u16CPUUsage = u64Tmp;
		u64Tmp2 = pThreadInfo->u16CPUAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pThreadInfo->u16CPUAverageUsage = u64Tmp;

		u64Tmp = pThreadInfo->u32RSS = pThreadStat->s32Rss;
		u64Tmp = ((u64Tmp / 1024 ) * 10000) / pSYSInfo->stMem.u32MemTotal;
		pThreadInfo->u16MemUsage = u64Tmp;

		u64Tmp2 = pThreadInfo->u16MemAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pThreadInfo->u16MemAverageUsage = u64Tmp;

		PRINT("Process(%s_%d) CPU: %d.%02d, %d.%02d \tMem: %d.%02d, %d.%02d\n",
				pThreadInfo->c8Name,
				pThreadInfo->u32Pid,
				pThreadInfo->u16CPUUsage / 100, pThreadInfo->u16CPUUsage % 100,
				pThreadInfo->u16CPUAverageUsage / 100, pThreadInfo->u16CPUAverageUsage % 100,
				pThreadInfo->u16MemUsage / 100, pThreadInfo->u16MemUsage % 100,
				pThreadInfo->u16MemAverageUsage / 100, pThreadInfo->u16MemAverageUsage % 100);

	}
	return 0;
}

/* the callback of TraversalDir called by TraversalDirCallback */
static int32_t TraversalDirThreadCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;
	int32_t s32Err;
	int32_t s32Pid;
	char c8Name[_POSIX_PATH_MAX];

	if (pCurPath[strlen(pCurPath) - 1] == '/')
	{
		sprintf(c8Name, "%s%s/%s", pCurPath, pInfo->d_name, "stat");
	}
	else
	{
		sprintf(c8Name, "%s/%s/%s", pCurPath, pInfo->d_name, "stat");
	}
	/* /proc/<PID>/task/<TID>/stat */
	s32Err = GetStatFileInfo(c8Name, &(pOutsideInfo->stThreadStat));
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Pid = atoi(pInfo->d_name);
	pOutsideInfo->stThreadInfo.u32Pid = s32Pid;
	ProcessStatisticUpdateAThread(&(pOutsideInfo->pHandle), pOutsideInfo->stThreadInfo.c8Name,
			s32Pid, UpdateAThreadCallback, pContext);
	return 0;
}

/* the callback of TraversalDir  */
static int32_t TraversalDirCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	/* not a directory */
	int32_t s32Pid;
#if !HAS_CROSS
	if ((pInfo->d_type & DT_DIR) == 0)
	{
		return 0;
	}
#endif
	/* not a process directory */
	if ((pInfo->d_name[0] > '9') || (pInfo->d_name[0] < '0'))
	{
		return 0;
	}
	s32Pid = atoi(pInfo->d_name);
	if (s32Pid > PROCESS_STATISTIC_BASE_PID)
	{
		char c8Name[_POSIX_PATH_MAX];
		StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;
		int32_t s32Err;

		s32Err = GetProcessNameFromPID(c8Name, _POSIX_PATH_MAX, s32Pid);
		if (s32Err != 0)
		{
			return 0;
		}

		if (ProcessStatisticSearchAProcess(pOutsideInfo->pHandle, c8Name, NULL) == NULL)
		{
			/* this process is not in the list */
			return 0;
		}
		/* the process is in the hash list */
		/* save the name */
		strcpy(pOutsideInfo->stThreadInfo.c8Name, c8Name);

		if (pCurPath[strlen(pCurPath) - 1] == '/')
		{
			sprintf(c8Name, "%s%s/%s", pCurPath, pInfo->d_name, "task/");
		}
		else
		{
			sprintf(c8Name, "%s/%s/%s", pCurPath, pInfo->d_name, "task/");
		}
		return TraversalDir(c8Name, false, TraversalDirThreadCallback, pContext);
	}

	return 0;
}


/* the callback of ProcessStatisticTraversal */
static int32_t TraversalCB(int32_t s32Flag, const StHashCursor *pCursor, void *pData, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;

	if (s32Flag == _Hash_Traversal_Process)
	{
		pOutsideInfo->pProcessInfo = pData;
	}
	else if (s32Flag == _Hash_Traversal_Thread)
	{
		UnProcessInfo *pThreadInfo = pData;
		if (pOutsideInfo->stThreadInfo.u32StatisticTimes == pThreadInfo->stThreadInfo.u32StatisticTimes)
		{
			pOutsideInfo->pProcessInfo->stMainProcess.u32StatisticTimes = pOutsideInfo->stThreadInfo.u32StatisticTimes;
		}
		else
		{
			PRINT("the thread(%s) is out\n", pCursor->c8Key);
			return -1;	/* the thread had been killed */
		}
	}
	else if (s32Flag == _Hash_Traversal_Process_Out)
	{
		if (pOutsideInfo->pProcessInfo->stMainProcess.u32StatisticTimes !=
				pOutsideInfo->stThreadInfo.u32StatisticTimes)
		{
			PRINT("the process(%s) is out\n", pCursor->c8Key);
			return -1;	/* the process had been killed */

		}
	}

	return 0;
}


static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;

	if (u32CmdNum == _Process_Statistic_Add)
	{
		return ProcessStatisticInsertAProcess(&(pOutsideInfo->pHandle), pCmdData);
	}
	else if (u32CmdNum == _Process_Statistic_Delete)
	{
		return ProcessStatisticDeleteAProcess(pOutsideInfo->pHandle, pCmdData);
	}
	return MY_ERR(_Err_CmdType);
}
int HashTest(bool *pIsExit)
{
	int32_t i;
	StProcessStatisticInfo stInfo;
	int32_t s32Err;
	int32_t s32StatisticSocket;

	PRINT("PROCESS_STATISTIC_BASE_PID id %d\n", PROCESS_STATISTIC_BASE_PID);
	memset(&stInfo, 0, sizeof(StProcessStatisticInfo));

	stInfo.pHandle = ProcessStatisticInit(&s32Err);
	if (stInfo.pHandle == NULL)
	{
		PRINT("ProcessStatisticInit error: 0x%08x\n", s32Err);
		return -1;
	}

	s32StatisticSocket = ServerListen(PROCESS_STATISTIC_SOCKET);
	if (s32StatisticSocket < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32StatisticSocket);
		s32Err = s32StatisticSocket;
		goto end;
	}


	for (i = 0; i < 6; i++)
	{
		uint16_t u16Index = ProcessStatisticInsertAProcess(&(stInfo.pHandle), pProcessName[i]);
		if (u16Index == HASH_INVALID_INDEX)
		{
			PRINT("ProcessStatisticInsertAProcess error: 0x%08x\n", s32Err);
		}
	}


	while (!(*pIsExit))
	{
		fd_set stSet;
		struct timeval stTimeout;

		int32_t s32Client = -1;
		stTimeout.tv_sec = 2;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32StatisticSocket, &stSet);

		if (select(s32StatisticSocket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
			goto statistic;
		}
		if (!FD_ISSET(s32StatisticSocket, &stSet))
		{
			goto statistic;
		}
		s32Client = ServerAccept(s32StatisticSocket);
		if (s32Client < 0)
		{
			PRINT("ServerAccept error: 0x%08x\n", s32Client);
			break;
		}
		else
		{
			uint32_t u32Size = 0;
			int32_t s32Err = 0;
			void *pMCSStream;
			pMCSStream = MCSSyncReceive(s32Client, true, 1000, &u32Size, &s32Err);
			if (pMCSStream == NULL)
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			else
			{
				MCSResolve((const char *)pMCSStream, u32Size, MCSCallBack, &stInfo);
			}
			MCSSyncFree(pMCSStream);
			close(s32Client);
			continue;
		}
statistic:
		CpuInfo(&(stInfo.stSYSInfo.stCPU));
		MemInfo(&(stInfo.stSYSInfo.stMem));
		TraversalDir("/proc/", false, TraversalDirCallback, &stInfo);

		ProcessStatisticTraversal(stInfo.pHandle, TraversalCB, &stInfo);

		stInfo.stThreadInfo.u32StatisticTimes++;
	}

	ProcessStatisticPrint(stInfo.pHandle);

	ServerRemove(s32StatisticSocket, PROCESS_STATISTIC_SOCKET);
end:
	HashTombDestroy(stInfo.pHandle);

	return 0;

}

#endif


