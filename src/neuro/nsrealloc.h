#ifndef __NSREALLOC_H
#define __NSREALLOC_H

/**
 * \file nsrealloc.h
 * \author Rudi Cilibrasi
 */

struct NSByteHolder;

/**
 * \brief Creates a new NSByteHolder to dynamical store an arbitrary number of bytes.
 * \return a pointer to a struct NSByteHolder.
 * Note that the initial struct NSByteHolder will contain 0 bytes.
 */
struct NSByteHolder *newNSByteHolder(void);

/**
 * \brief Adds a single character to a struct NSByteHolder.
 * \param nsb a pointer to a struct NSByteHolder
 * \param c the character to add to the end of the data block.
 */
void addCharacter(struct NSByteHolder *nsb, char c);
/**
 * \brief Adds a block of bytes to a struct NSByteHolder.
 * \param nsb a pointer to a struct NSByteHolder
 * \param buf a pointer to the start address of the block to add
 * \param len the number of bytes in the block to add
 */
void addBlock(struct NSByteHolder *nsb, char *buf, int len);
/**
 * \brief Returns the number of bytes added to this struct NSByteHolder.
 * \param nsb The struct NSByteHolder to query.
 * \return the number of bytes added to this struct NSByteHolder
 */
int getSize(struct NSByteHolder *nsb);
/**
 * \brief Returns a pointer to a contiguous block of data held by nsb.
 * \param nsb The struct NSByteHolder to query.
 * \return A pointer to data accumulated into the struct NSByteHolder
 *
 * This function returns a pointer to the start of the data being
 * held by nsb.  This data will come from addCharacter() or addBlock()
 * calls that have previously occurred.
 */
const char *getData(struct NSByteHolder *nsb);

/**
 * \brief Frees the resources associated with a struct NSByteHolder.
 * \param nsb The struct NSByteHolder to deallocate.
 */
void freeNSByteHolder(struct NSByteHolder *nsb);

#endif
