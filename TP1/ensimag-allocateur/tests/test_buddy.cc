/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <gtest/gtest.h>

#include "../src/mem.h"

TEST(Variantes,buddy) {
  int multi = 0;
  
#ifndef BUDDY
  return;
#else
  multi = 1;
#endif
  
  ASSERT_EQ(multi, 1);
  ASSERT_EQ( mem_init(), 0 );

  void *mref = mem_alloc(ALLOC_MEM_SIZE);
  ASSERT_NE( mref, (void*) 0);
  ASSERT_EQ( mem_free(mref, ALLOC_MEM_SIZE), 0 );

  void *m1 = mem_alloc(64);
  ASSERT_NE( m1, (void *)0 );
  memset( m1, 0, 1);

  void *m2 = mem_alloc(64);
  ASSERT_NE( m2, (void *)0 );
  memset( m2, 0, 1);

  unsigned long vref = (unsigned long) mref;
  unsigned long v1 = (unsigned long)m1;
  unsigned long v2 = (unsigned long)m2;
  ASSERT_EQ( (v1-vref)^(v2-vref), 64 );

  ASSERT_EQ( mem_free( m1, 64 ), 0 );
  ASSERT_EQ( mem_free( m2, 64 ), 0 );

  mref = mem_alloc(ALLOC_MEM_SIZE);
  ASSERT_NE( mref, (void *)0 );
  memset(mref, 4, ALLOC_MEM_SIZE);
  ASSERT_EQ( mem_free( mref, ALLOC_MEM_SIZE ), 0 );
  ASSERT_EQ( mem_destroy(), 0);
}
