/**
 * @file Vertex.h
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
 * This file defined Vertex class to provide computation interface for users.
 * Template arguments are value types of vertex, edge and message.
 *
 * Given template arguments, Vertex/Edge/Msg value types will be definite. So we
 * define MessageIterator(subclass of GenericLinkIterator) and OutEdgeIterator
 * (subclass of GenericArrayIterator) as inner classes.
 *
 * Almost all methods in Vertex class will call corresponding implementation
 * in Node class. Users need to inherit Vertex class, fill in template arguments
 * and override virtual compute() method. In consideration of only access to
 * base class before whole program linked up, namely system files and user files,
 * compute(GenericLinkIterator*) need to cast to pure virtual function
 * compute(MessageIterator*) to fit interface, which will execute at each active
 * vertex during every superstep.
 *
 * @see VertexBase class
 * @see GenericLinkIterator class
 * @see GenericArrayIterator class
 * @see Node class
 * @see Vertex::GenericLinkIterator
 * @see Vertex::GenericArrayIterator
 *
 */

#ifndef VERTEX_H
#define VERTEX_H

#include <stddef.h>

#include "VertexBase.h"
#include "GenericLinkIterator.h"
#include "GenericArrayIterator.h"
#include "Node.h"

/** Definition of Vertex class. */
template<typename VertexValue, typename EdgeValue, typename MessageValue>
class Vertex: public VertexBase {
public:

    /** Definition of MessageIterator class. */
    class MessageIterator: public GenericLinkIterator {
    public:
        /**
         * Get message value.
         * @return message value
         */
        const MessageValue& getValue() {
            return *( (MessageValue *)( getCurrent() + offsetof(Msg, message) ) );
        }
    };

    /** Definition of OutEdgeIterator class. */
    class OutEdgeIterator: public GenericArrayIterator {
    public:
        /**
         * Constructor with arguments.
         * @param pbegin iterator begin position
         * @param pend iterator end position
         * @param size array element size
         */
        OutEdgeIterator(char* pbegin, char* pend, int size):
            GenericArrayIterator(pbegin, pend, size) {}

        /**
         * Get current edge target vertex id.
         * @see current()
         * @return target vertex id
         */
        int64_t target() {
            char* p = current();
            return ( (Edge *)p )->to;
        }
        /**
         * Get edge value.
         * @see current()
         * @return edge value
         */
        const EdgeValue& getValue() {
            char* p = current();
            return *( (EdgeValue *)( (Edge *)p )->weight );
        }
    };

public:
    /**
     * Compute at active vertex, pure virtual method.
     * @param pmsgs specialized pointer of received message iterator
     */
    virtual void compute(MessageIterator* pmsgs) = 0; // Virtual is necessary.

    /**
     * Compute at active vertex.
     * @see compute(MessageIterator*)
     * @param pmsgs generic pointer of received message iterator
     */
    void compute(GenericLinkIterator* pmsgs) {
        compute( (MessageIterator *)pmsgs ); // cast to compute() below
    }

    /**
     * Get vertex value type size.
     * @return vertex value type size
     */
    int getVSize() const {
        return sizeof(VertexValue);
    }

    /**
     * Get edge value type size.
     * @return edge value type size
     */
    int getESize() const {
        return sizeof(EdgeValue);
    }

    /**
     * Get message value type size.
     * @return message value type size
     */
    int getMSize() const {
        return sizeof(MessageValue);
    }

    /**
     * Get current superstep number.
     * @see Node::getSuperstep()
     * @return current superstep number
     */
    int getSuperstep() const {
        return m_pme->getSuperstep();
    }

    /**
     * Get vertex id.
     * @see Node::getVertexId()
     * @return vertex id
     */
    int64_t getVertexId() const {
        return m_pme->getVertexId();
    }

    /**
     * Vote to halt.
     * @see Node::voteToHalt()
     * Change vertex state to be inactive.
     */
    void voteToHalt() {
        return m_pme->voteToHalt();
    }

    /**
     * Get vertex value.
     * @see Node::value
     * @return vertex value
     */
    const VertexValue& getValue() {
        return *( (VertexValue *)m_pme->value );
    } // Type cast has lower priority than "->" operator.

    /**
     * Mutate vertex value.
     * @see Node::value
     * @return vertex value position
     */
    VertexValue* mutableValue() {
        return (VertexValue *)m_pme->value;
    } // Type cast has lower priority than "->" operator.

    /**
     * Get an out-edge iterator.
     * @see OutEdgeIterator::OutEdgeIterator()
     * @return an object of OutEdgeIterator class
     */
    OutEdgeIterator getOutEdgeIterator() {
        OutEdgeIterator out_edge_iterator(
        (char *)&(m_pme->getEdge(m_pme->m_edge_index)),
        (char *)&(m_pme->getEdge(m_pme->m_edge_index + m_pme->m_out_degree)),
        Edge::e_size );
        return out_edge_iterator;
    }

    /**
     * Send a piece of node-node message to target vertex.
     * @param dest_vertex destination vertex id
     * @param message content of the message to be sent
     * @see Node::sendMessageTo()
     */
    void sendMessageTo(int64_t dest_vertex, const MessageValue& message) {
        m_pme->sendMessageTo(dest_vertex, (const char *)&message);
    }

    /**
     * Send a piece of node-node message to all outedge-target vertex.
     * @param message content of the message to be sent
     * @see Node::sendMessageToAllNeighbors()
     */
    void sendMessageToAllNeighbors(const MessageValue& message) {
        m_pme->sendMessageToAllNeighbors( (const char *)&message );
    }

    /**
     * Get global value of some aggregator.
     * @param aggr index of aggregator, count from 0
     * @see Node::getAggrGlobal()
     */
    const void* getAggrGlobal(int aggr) {
        return m_pme->getAggrGlobal(aggr);
    }

    /**
     * Accumulate local value of some aggregator.
     * @param aggr index of aggregator, count from 0
     * @see Node::accumulateAggr()
     */
    void accumulateAggr(int aggr, const void* p) {
        m_pme->accumulateAggr(aggr, p);
    }

    /** Destructor, virtual method. */
    virtual ~Vertex() {}
}; // definition of Vertex class

#endif /* VERTEX_H */
