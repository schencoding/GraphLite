/**
 * @file GenericArrayIterator.h
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
 * This file defined GenericArrayIterator class to iterate on array structure.
 *
 */

#ifndef GENERICARRAYITRATOR_H
#define GENERICARRAYITRATOR_H

/** Definition of GenericArrayIterator class. */
class GenericArrayIterator {
public:
    char* m_pbegin;       /**< pointer of iterator begin position */
    char* m_pend;         /**< pointer of iterator end position */
    int   m_element_size; /**< size of array element */

public:
    /**
     * Constructor.
     * @param pbegin iterator begin position
     * @param pend iterator end position
     * @param size array element size
     */
    GenericArrayIterator(char* pbegin, char* pend, int size):
        m_pbegin(pbegin), m_pend(pend), m_element_size(size) {}

    /**
     * Get iterator size.
     * @return count of elements to visit
     */
    int64_t size() { return (int64_t)(m_pend - m_pbegin) / m_element_size; }

    /**
     * Get current element position.
     * @return pointer of current element
     */
    char* current() { return m_pbegin; }

    /** Go to visit next element. */
    void next() { m_pbegin += m_element_size; }

    /**
     * Judge if iterator terminates or not.
     * @retval true done
     * @retval false not
     */
    bool done() { return (m_pbegin >= m_pend); }
}; // definition of GenericArrayIterator class

#endif /* GENERICARRAYITRATOR_H */
