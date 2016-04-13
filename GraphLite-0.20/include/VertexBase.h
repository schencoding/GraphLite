/**
 * @file VertexBase.h
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
 * This file defined VertexBase class to load user Vertex class. VertexBase is
 * an abstract class with virtual methods. Node class is pointed to inside,
 * which implements inner details for corresponding Vertex.
 *
 * There is a 3-layer inheritance hierarchy about Vertex: VertexBase is on top,
 * Vertex class of template is in the middle, and user Vertex class is at the
 * bottom.
 *
 * @see Node class
 * @see GenericLinkIterator class
 *
 */

#ifndef VERTEXBASE_H
#define VERTEXBASE_H

#include "GenericLinkIterator.h"

class Node;

/** Definition of VertexBase class. */
class VertexBase {
public:
    Node* m_pme; /**< pointer of Node class */

public:
    /**
     * Set pointer to Node class.
     * @param p corresponding Node structure position
     */
    void setMe(Node* p) { m_pme = p; }

    /**
     * Get vertex value type size, pure virtual method.
     * @return vertex value type size
     */
    virtual int getVSize() const = 0;

    /**
     * Get edge value type size, pure virtual method.
     * @return edge value type size
     */
    virtual int getESize() const = 0;

    /**
     * Get message value type size, pure virtual method.
     * @return message value type size
     */
    virtual int getMSize() const = 0;

    /**
     * Get vertex id.
     * @return vertex id
     */
    virtual int64_t getVertexId() const = 0;

    /**
     * Get current superstep number, pure virtual method.
     * @return current superstep number
     */
    virtual int getSuperstep() const = 0;

    /**
     * Vote to halt, pure virtual method.
     * Change vertex state to be inactive.
     */
    virtual void voteToHalt() = 0;

    /**
     * Compute at active vertex, pure virtual method.
     * @param pmsgs generic pointer of received message iterator
     */
    virtual void compute(GenericLinkIterator* pmsgs) = 0;
}; // definition of VertexBase class

#endif /* VERTEXBASE_H */
