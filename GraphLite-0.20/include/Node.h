/**
 * @file Node.h
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
 * This file defined Edge struct and Node class. node-node Message struct is in
 * GenericLinkIterator.h.
 * Node class implements interfaces of Vertex class to hide inner details from
 * users.
 *
 * @see Edge struct
 * @see Msg struct
 * @see GenericLinkIterator class
 * @see GenericArrayIterator class
 *
 */

#ifndef NODE_H
#define NODE_H

#include <stdint.h>
#include <vector>

#include "GenericLinkIterator.h"
#include "GenericArrayIterator.h"

/** Definition of Edge struct. */
typedef struct Edge {
    int64_t from;   /**< start vertex id of the edge */
    int64_t to;     /**< end vertex id of the edge */
    char weight[0]; /**< start positon of memory to store edge weight */

    static int e_value_size; /**< edge value size in character */
    static int e_size;       /**< total size of an edge, sizeof(Edge) + e_value_size */
} Edge;

/** Definition of Node class. */
class Node {
public:
    bool m_active;        /**< vertex state: active m_active=1, inactive m_active=0 */
    int64_t m_v_id;       /**< vertex id */
    int m_out_degree;     /**< vertex outdegree */
    int64_t m_edge_index; /**< index of first edge from this vertex in edge array */
    std::vector<Msg*> m_cur_in_msg; /**< current superstep in-message pointer list */
    char value[0];                  /**< start positon of memory to store node value */

public:
    static int n_value_size; /**< node value size in character */
    static int n_size;	     /**< total size of a node, sizeof(Node) + n_value_size */

public:
    /**
     * Get a Node structure of index(begin at 0) in node array.
     * @param index node index
     * @return a Node structure of index in node array
     */
    static Node& getNode(int index);

    /**
     * Get an Eode structure of index in edge array.
     * @param index edge index
     * @return an Edge structure of index in edge array
     */
    static Edge& getEdge(int index);

    /** Initialize pointers of in-message lists. */
    void initInMsg();

    /**
     * Receive a new piece of message for next superstep.
     * Link the message to next_in_msg list.
     * @param pmsg pointer of the message
     */
    void recvNewMsg(Msg* pmsg);

    /** Free current in-message list to freelist. */
    void clearCurInMsg();

    /** Free memory of in-message lists vector allocation. */
    void freeInMsgVector();

    /**
     * Get current superstep number.
     * @return current superstep number
     */
    int getSuperstep() const;

    /**
     * Get vertex id.
     * @return vertex id
     */
    int64_t getVertexId() const;

    /**
     * Vote to halt.
     * Change vertex state to be inactive.
     */
    void voteToHalt();

    /**
     * Get a generic link iterator.
     * @return a pointer of GenericLinkIterator
     */
    GenericLinkIterator* getGenericLinkIterator();

    /**
     * Get a generic array iterator.
     * @return a pointer of GenericArrayIterator
     */
    // GenericArrayIterator* getGenericArrayIterator();

    /**
     * Send a piece of node-node message to target vertex.
     * @param dest_vertex destination vertex id
     * @param pmessage pointer of the message to be sent
     */
    void sendMessageTo(int64_t dest_vertex, const char* pmessage);

    /**
     * Send a piece of node-node message to all outedge-target vertex.
     * Call sendMessageTo() for all outedges.
     * @param pmessage pointer of the message to be sent
     * @see sendMessageTo()
     */
    void sendMessageToAllNeighbors(const char* pmessage);

    /**
     * Get global value of some aggregator.
     * @param aggr index of aggregator, count from 0
     */
    const void* getAggrGlobal(int aggr);

    /**
     * Accumulate local value of some aggregator.
     * @param aggr index of aggregator, count from 0
     */
    void accumulateAggr(int aggr, const void* p);
}; // definition of Node class

#endif /* NODE_H */
