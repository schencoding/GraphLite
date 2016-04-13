/**
 * @file GenericLinkIterator.h
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
 * This file defined node-node Message struct and GenericLinkIterator class to
 * iterate on link structure implemented in pointer vector.
 * Mainly for in-message list in Node.h.
 *
 * @see Node class
 *
 */

#ifndef GENERICLINKITERATOR_H
#define GENERICLINKITERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace std;

/** Definition of node-node Message struct. */
typedef struct Msg {
    int64_t s_id;    /**< source vertex id of the message */
    int64_t d_id;    /**< destination vertex id of the message */
    char message[0]; /**< start positon of memory to store message content */

    static int m_value_size; /**< messge value size in character */
    static int m_size;       /**< total size of a piece of message, sizeof(Msg) + m_value_size */
} Msg;

/** Definition of GenericLinkIterator class. */
class GenericLinkIterator {
public:
    vector<Msg*>*  m_pvector;     /**< pointer of vector to be iterated on */
    int            m_vector_size; /**< vector size */
    int            m_cur_index;   /**< index of current element in vector */

public:
    /**
     * Constructor.
     * @param pvector pointer of vector to be iterated on
     */
    GenericLinkIterator(vector<Msg*>* pvector): m_pvector(pvector) {
        m_vector_size = pvector->size();
        m_cur_index = 0;
    }

    /**
     * Get current element.
     * @return current element of vector
     */
    char* getCurrent() { return (char *)(*m_pvector)[m_cur_index]; }

    /** Go to visit next element. */
    void next() { ++m_cur_index; }

    /**
     * Judge if iterator terminates or not.
     * @retval true done
     * @retval false not
     */
    bool done() { return m_cur_index == m_vector_size; }
}; // definition of GenericLinkIterator class

#endif /* GENERICLINKITERATOR_H */
