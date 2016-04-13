/**
 * @file ChunkedList.h
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
 * This file defined ChunkedList class serving as pointer array to optimize
 * visit to messages and better to prefetch.
 *
 * If there is no freespace on ChunkedList, we allocate a block of
 * m_mem_pool_capacity size once from memory, so as to reduce request times
 * for memory.
 *
 * Also defined ChunkedList::Iterator class, so as to provide interface for
 * users to visit ChunkedList.
 *
 */

#ifndef CHUNKEDLIST_H
#define CHUNKEDLIST_H

#include <vector>
#include <malloc.h>
#include <stdlib.h>

#include "Utility.h"

using namespace std;

/** Definition of ChunkedList class. */
class ChunkedList {
public:
    vector<char*> m_mem_pools; /**< pointers of all memory pools */
    int   m_mem_pool_capacity; /**< memory pool capacity */
    int   m_num_ele_per_buf;   /**< element number of each memory pool */

    int    m_cur_mem_pool; /**< current memory pool index */
    char** m_pavail;       /**< pointer of next element to be appended */
    char** m_pavail_begin; /**< pointer of current block begin */
    char** m_pavail_end;   /**< pointer of whole blocks end */

public:
    /** Constructor. */
    ChunkedList() {
        m_mem_pool_capacity = MEMPOOL_CAPACITY;
        m_num_ele_per_buf = m_mem_pool_capacity / sizeof(char*);
        m_cur_mem_pool = -1;
        m_pavail = m_pavail_begin = m_pavail_end = NULL;
    }

    /**
     * Allocate a new memory pool.
     * Called when there is no freespace on ChunkedList to append.
     */
    void allocateNewBlock() {
        char* p = (char *)memalign(64, m_mem_pool_capacity); // cache line: 64B
        if (!p) { perror("memalign"); exit(1); }
        m_mem_pools.push_back(p);

        ++m_cur_mem_pool;
        m_pavail = (char **)p;
        m_pavail_begin = m_pavail;
        m_pavail_end = m_pavail + m_num_ele_per_buf;
    }

    /**
     * Append a new element.
     * If there is no freespace on ChunkedList to append a new element, we need
     * to request for a new memory pool before allocation.
     * @param p pointer of the new element to be appended
     * @see allocateNewBlock()
     */
    void append(void* p) {
        if (m_pavail == m_pavail_end) allocateNewBlock();
        *m_pavail = (char *)p;
        ++m_pavail;

        if (m_pavail == m_pavail_end) allocateNewBlock();
        else if (m_pavail == m_pavail_begin + m_num_ele_per_buf) {
            m_pavail_begin = (char **)m_mem_pools[++m_cur_mem_pool];
            m_pavail = m_pavail_begin;
        }
    }

    /**
     * Judge whether ChunkedList is empty.
     * @retval 0 not empty
     * @retval 1 empty
     */
    bool isEmpty() {
        if ( ! m_mem_pools.size() || (m_pavail == (char **)m_mem_pools[0]) ) {
        /*
          Actual condition is
          ( ! m_mem_pools.size() || (m_mem_pools.size() && m_pavail == mem_pool[0]) ),
          here cuz of "||" property.
        */
            return 1;
        }
        return 0;
    }

    /**
     * Get ChunkedList tail element.
     * @return tail element
     */
    char* getTail() {
        if (m_pavail == m_pavail_begin) { // will not remove current empty block
            m_pavail_begin = (char **)m_mem_pools[--m_cur_mem_pool];
            m_pavail = m_pavail_begin + m_num_ele_per_buf;
        }

        --m_pavail;
        return *m_pavail;
    }

    /**
     * Get total number of elements on ChunkedList.
     * @see isEmpty()
     * @return total number of elements
     */
    int64_t total() {
        if ( isEmpty() ) return 0;

        int64_t r = m_num_ele_per_buf;
        r *= (m_cur_mem_pool + 1);
        r -= (m_pavail_begin + m_num_ele_per_buf - m_pavail);
        return r;
    }

    /**
     * Destructor.
     * Free requested memory of whole runtime.
     */
    ~ChunkedList() {
        for (int i = m_mem_pools.size()-1; i>=0; i--) {
            ::free(m_mem_pools[i]); // system global function
            m_mem_pools[i] = NULL;
        }
        m_mem_pools.clear();
        m_pavail = m_pavail_begin = m_pavail_end = NULL;
    }

public:
    /** Definition of ChunkedList::Iterator class */
    class Iterator {
    private:
        ChunkedList* m_plist; /**< pointer of ChunkedList to be iterated on */
        int    m_which_chunk; /**< index of chunk to be visited */
        char** m_pmy_next;    /**< pointer of next element on ChunkedList */
        char** m_pchunk_end;  /**< pointer of current chunk end */
  
    public:
        /**
         * Constructor.
         * @param pl pointer of ChunkedList to be iterated on
         * @see init()
         */
        Iterator(ChunkedList* pl) { init(pl); }

        /**
         * Initialization.
         * @param pl pointer of ChunkedList to be iterated on
         * @see startNextChunk()
         */
        void init(ChunkedList* pl) {
            m_plist = pl;
            m_which_chunk = 0;
            startNextChunk(0);
        }

        /**
         * Set chunk state value when start to visit next chunk.
         * @param which index of chunk to be visited
         */
        void startNextChunk(int which) {
            if ( which < (int)m_plist->m_mem_pools.size() ) {
                m_pmy_next = (char **)(m_plist->m_mem_pools[which]);
                m_pchunk_end = m_pmy_next + m_plist->m_num_ele_per_buf;
            } else {
                m_pmy_next = NULL;
            }
        }

        /**
         * Get next element on ChunkedList.
         * @see startNextChunk()
         * @return next element on ChunkedList
         */
        void* next() {
            if ( (m_pmy_next == NULL) || (m_pmy_next == m_plist->m_pavail) ) {
                return NULL;
            }

            void* ret = *m_pmy_next;
            ++m_pmy_next;
            if (m_pmy_next == m_pchunk_end) {
                ++m_which_chunk;
                startNextChunk(m_which_chunk);
            }
            return ret;
        }
    }; // definition of ChunkedList::Iterator class

public:
    /*
     Previous implementation below leads to the case that nowhere to delete
     pointer from new. To avoid this, return only the iterator pointer.
     Similiar to Node::getGenericArrayIterator().
    */
    // Iterator getIterator() { return *( new Iterator(this) ); }
    /**
     * Get pointer of Iterator.
     * @see Iterator()
     * @return pointer of Iterator
     */
    Iterator* getIterator() { return new Iterator(this); }

    /**
     * Initialize Iterator.
     * @param pit pointer of Iterator
     * @see init()
     */
    void initIterator(Iterator* pit) { pit->init(this); }
}; // definition of ChunkedList class

#endif /* CHUNKEDLIST_H */
