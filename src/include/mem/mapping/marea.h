/**
 * @file
 *
 * @date Aug 21, 2014
 * @author: Anton Bondarev
 */

#ifndef MAREA_H_
#define MAREA_H_

struct marea;

extern struct marea *marea_create(uint32_t start, uint32_t end, uint32_t flags);

extern void marea_destroy(struct marea *marea);

#endif /* MAREA_H_ */
