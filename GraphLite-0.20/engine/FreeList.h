/**
 * @file FreeList.h
 * @author  Songjie Niu, Shimin Chen
 * @version 0.1
 *
 * @section LICENSE 
 * 
 * Copyright 2016 Shimin Chen (chensm@ict.ac.cn) and
 * Songjie Niu (niusongjie@ict.ac.cn)
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @section DESCRIPTION
 * 
 * This file defined FreeList class to manage memory allocation in program.
 *
 * If there is no freespace on FreeList, we allocate a block of
 * m_mem_pool_capacity size once from memory, called memory pool, split it into
 * items of element size, and store each address in m_chunked_list, so as to
 * reduce request times for memory.
 *
 * When we need m_element_size freespace, just pick up one item from FreeList;
 * when some item becomes unused, it'll be added to FreeList again.
 *
 * @see ChunkedList
 *
 */

#ifndef FREELIST_H
#define FREELIST_H

#include <malloc.h>
#include <stdlib.h>

#include "ChunkedList.h"
#include "Utility.h"

/** Definition of FreeList class. */
class FreeList {
public:
    vector<char*> m_mem_pools;  /**< pointers of all memory pools */
    int m_mem_pool_capacity;    /**< memory pool capacity */
    int m_element_size;         /**< size of FreeList element */
    ChunkedList m_chunked_list; /**< pointer array to store free element address */

public:
    /** Constructor. */
    FreeList(): m_mem_pool_capacity(MEMPOOL_CAPACITY) {}

    /**
     * Set element size.
     * @param ele_size memory pool element size
     */
    void setEle(int ele_size) { m_element_size = ele_size; }

    /**
     * Recycle unused memory to freespace.
     * @param p pointer of unused memory item
     * @see ChunkedList::append()
     */
    void free(void* p) {
        m_chunked_list.append(p);
    }

    /**
     * Allocate a new memory pool.
     * If there is no freespace on FreeList, we need to request for a new
     * memory pool. Then split it into m_element_size items and add them to
     * FreeList one by one through FreeList::free().
     * @see free()
     */
    void allocateNewBlock() {
        char* p = (char *)memalign(64, m_mem_pool_capacity); // cache line: 64B
        if (!p) {
            perror("memalign");
            exit(1);
        }
        m_mem_pools.push_back(p);

        /*
          If for-loop control condition stays as before-"i<m_mem_pool_capacity",
          the last free item on FreeList can be smaller than m_element_size.
          Because m_mem_pool_capacity may not be a multiple of m_element_size.
        */
        for (int i=0; i+m_element_size<=m_mem_pool_capacity; i+=m_element_size) {
            free(p + i); // FreeList::free
        }
    }

    /**
     * Allocate freespace on FreeList.
     * From m_chunked_list.avail forward. If there is no freespace on FreeList,
     * we need to request for a new memory pool before allocation.
     * @see ChunkedList::isEmpty()
     * @see allocateNewBlock()
     * @see ChunkedList::getTail()
     * @return pointer of m_element_size freespace
     */
    void* allocate() {
        if ( m_chunked_list.isEmpty() ) {
            allocateNewBlock();
        }
        return m_chunked_list.getTail();
    }

    /**
     * Destructor.
     * Free requested memory of whole runtime.
     */
    ~FreeList() {
        for (int i = m_mem_pools.size() - 1; i >= 0; --i) {
            ::free(m_mem_pools[i]); // system global function in stdlib.h
            m_mem_pools[i] = NULL;
        }
        m_mem_pools.clear();
    }
}; // definition of FreeList class

#endif /* FREELIST_H */
