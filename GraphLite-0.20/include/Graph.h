/**
 * @file Graph.h
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
 * This file defined Graph class as an interface for users to config arguments
 * and control system runtime to some degree.
 *
 * System needs to load user files dynamically. That is, we can't know details
 * about subclass implemented by users before whole program linked up. So by 
 * means of C++ language features, some pointers of abstract classes are
 * necessary.
 *
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <string.h>

#include "VertexBase.h"
#include "InputFormatter.h"
#include "OutputFormatter.h"
#include "AggregatorBase.h"
#include "Addr.h"


/** Definition of Graph class. */
class Graph {
public:
    int m_machine_cnt;       /**< machine count, one master and some workers */
    Addr* m_paddr_table;     /**< address table, master 0 workers from 1 */
    int m_hdfs_flag;         /**< read input from hdfs or local-fs, hdfs 1 local-fs 0 */
    const char* m_pfs_host;  /**< hdfs host */
    int m_fs_port;           /**< hdfs port */
    const char* m_pin_path;  /**< input file path */
    const char* m_pout_path; /**< output file path */
    InputFormatter* m_pin_formatter;   /**< pointer of InputFormatter */
    OutputFormatter* m_pout_formatter; /**< pointer of OutputFormatter */
    int m_aggregator_cnt;              /**< aggregator count */
    AggregatorBase** m_paggregator;    /**< pointers of AggregatorBase */
    VertexBase* m_pver_base;           /**< pointer of VertexBase */

public:
    void setNumHosts(int num_hosts) {
        if (num_hosts <= 0) return;

        m_machine_cnt = num_hosts;
        m_paddr_table = new Addr[num_hosts];
    }

    void setHost(int id, const char *hostname, int port) {
        if (id<0 || id>=m_machine_cnt) return;

        m_paddr_table[id].id = id;
        strncpy(m_paddr_table[id].hostname, hostname, NAME_LEN-1);
        m_paddr_table[id].hostname[NAME_LEN-1]= '\0';
        m_paddr_table[id].port = port;
    }

    void regNumAggr(int num) {
        if (num <= 0) return;

        m_aggregator_cnt= num;
        m_paggregator= new AggregatorBase*[num];
    }

    void regAggr(int id, AggregatorBase *aggr) {
        if (id<0 || id>=m_aggregator_cnt) return;

        m_paggregator[id]= aggr;
    }


public:
    Graph(){
      setNumHosts(1);
      setHost(0, "localhost", 1411);

      m_hdfs_flag= 0; // use local file by default

      m_aggregator_cnt= 0;
      m_paggregator= NULL;
    }

    /**
     * Initialize, virtual method. All arguments from command line.
     * @param argc algorithm argument number 
     * @param argv algorithm arguments
     */
    virtual void init(int argc, char* agrv[]) {}

    /**
     * Master computes per superstep to judge if supersteps terminate,
     * virtual method.
     * @param superstep current superstep number
     * @param paggr_base aggregator base pointer
     * @retval 1 supersteps terminate
     * @retval 0 supersteps not terminate
     */
    virtual int masterComputePerstep(int superstep, AggregatorBase** paggr_base) {
        return 0;
    }

    /** Terminate, virtual method. */
    virtual void term() {}

    /** Destructor, virtual method. */
    virtual ~Graph() {
      if(m_paggregator) delete[] m_paggregator;
      if(m_paddr_table) delete[] m_paddr_table;
    }
}; // definition of Graph class

/**
 * A type definition for function pointer.
 * Function has no arguments and return value of Graph* type.
 */
typedef Graph* (* GraphCreateFn)();

/**
 * A type definition for function pointer.
 * Function has one argument of Graph* type and no return value.
 */
typedef void (* GraphDestroyFn)(Graph*);

#endif /* GRAPH_H */
