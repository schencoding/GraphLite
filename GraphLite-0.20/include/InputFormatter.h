/**
 * @file InputFormatter.h
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
 * This file defined InputFormatter class to provide input interface for user,
 * mainly to load local subgraph, supporting reading from both local file system
 * and hdfs.
 *
 * We provide abstract methods for users to override, and also some helpful
 * interfaces implemented by system.
 *
 */

#ifndef INPUTFORMATTER_H
#define INPUTFORMATTER_H

#define MAXINPUTLEN 100

#include <fstream>
#include <string>
#include <stdint.h>

#include "hdfs.h"

/** Definition of InputFormatter class. */
// 
// GraphLite engine will call an InputFormatter as follows:
//
//  open();
//  getVertexNum(); getEdgeNum();
//  getVertexValueSize(); getEdgeValueSize(); getMessageValueSize();
//  loadGraph();
//  close();
// 
class InputFormatter {
public:
    hdfsFile m_hdfs_file;       /**< input file handle, for hdfs */
    std::ifstream m_local_file; /**< input file stream, for local file system */
    const char* m_ptotal_vertex_line; /**< pointer of total vertex count line */
    const char* m_ptotal_edge_line;   /**< pointer of total edge count line */
    int64_t m_total_vertex;     /**< total vertex count of local subgraph */
    int64_t m_total_edge;       /**< total edge count of local subgraph */
    int m_n_value_size;         /**< vertex value type size */
    int m_e_value_size;         /**< edge value type size */
    int m_m_value_size;         /**< message value type size */
    int m_vertex_num_size;      /**< vertex number size */
    int m_edge_num_size;        /**< edge number size */
    tOffset m_next_edge_offset; /**< offset for next edge */
    std::string m_total_vertex_line; /**< buffer of local total vertex count line */
    std::string m_total_edge_line;   /**< buffer of local total edge count line */
    char m_buf_line[MAXINPUTLEN];    /**< buffer of hdfs file line */
    std::string m_buf_string;

public:
    /**
     * Open input file, virtual method.
     * @param pin_path input file path
     */
    virtual void open(const char* pin_path);

    /** Close input file, virtual method. */
    virtual void close();

    /**
     * Get vertex number, pure virtual method.
     * @return total vertex number in local subgraph
     */
    virtual int64_t getVertexNum() = 0;

    /** Get vertex number line */
    void getVertexNumLine();

    /**
     * Get edge number, pure virtual method.
     * @return total edge number in local subgraph
     */
    virtual int64_t getEdgeNum() = 0;

    /** Get edge number line */
    void getEdgeNumLine();

    /**
     * Get vertex value type size, pure virtual method.
     * @return vertex value type size
     */
    virtual int getVertexValueSize() = 0;

    /**
     * Get edge value type size, pure virtual method.
     * @return edge value type size
     */
    virtual int getEdgeValueSize() = 0;

    /**
     * Get message value type size, pure virtual method.
     * @return message value type size
     */
    virtual int getMessageValueSize() = 0;

    /**
     * Get edge line, for user.
     * Read from current file offset.
     * @return a string of edge in local subgraph
     */
    const char* getEdgeLine();

    /**
     * Add one vertex to Node array.
     * @param vid vertex id
     * @param pvalue pointer of vertex value
     * @param outdegree vertex outdegree
     */
    void addVertex(int64_t vid, void* pvalue, int outdegree);

    /**
     * Add one edge to Edge array.
     * @param from edge source vertex id
     * @param to edge destination vertex id
     * @param pweight pointer of edge weight
     */
    void addEdge(int64_t from, int64_t to, void* pweight);

    /** Load local subgraph, pure virtual method. */
    virtual void loadGraph() = 0;

    /** Destructor. */
    ~InputFormatter();
}; // definition of InputFormatter class

#endif /* INPUTFORMATTER_H */
