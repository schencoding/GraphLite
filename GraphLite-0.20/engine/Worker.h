/**
 * @file Worker.h
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
 * This file defined Worker class to manage local computation of subgraph.
 * Mention that we choose hdfs as file system.
 *
 */

#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>
#include <string.h>

#include "hdfs.h"
#include "Utility.h"
#include "Graph.h"
#include "InputFormatter.h"
#include "OutputFormatter.h"
#include "AggregatorBase.h"
#include "VertexBase.h"
#include "Node.h"
#include "FreeList.h"
#include "ChunkedList.h"
#include "Sender.h"
#include "Receiver.h"
#include "WM.begin.pb-c.h"
#include "MW.begin.pb-c.h"
#include "WM.curss_finish.pb-c.h"
#include "MW.nextss_start.pb-c.h"
#include "MW.end.pb-c.h"
#include "WM.end.pb-c.h"
#include "WW.nodemsg_list.pb-c.h"

extern int main_term;

/** Definition of Worker class. */
class Worker {
public:
    /** Definition of ResultIterator class. */
    class ResultIterator {
    public:
        char* m_pbegin;       /**< pointer of iterator begin position */
        char* m_pend;         /**< pointer of iterator end position */
        int   m_element_size; /**< size of element */
        
    public:
        /**
         * Initialization.
         * @param pbegin iterator begin position
         * @param pend iterator end position
         */
        void init(Node* pnode, int total_vertex) {
            m_pbegin = (char *)pnode;
            m_pend = (char *)pnode + total_vertex * Node::n_size;
            m_element_size = Node::n_size;
        }

        /**
         * Get vertex value result after computation.
         * @param vid reference of vertex id to be got
         * @param pvalue pointer of vertex value
         */
        void getIdValue(int64_t& vid, void* pvalue) { // Reference is necessary.
            Node* pnode = (Node *)m_pbegin;
            vid = pnode->m_v_id;
            memcpy(pvalue, pnode->value, Node::n_value_size);
        }

        /** Go to visit next element. */
        void next() { m_pbegin += m_element_size; }

        /**
         * Judge if iterator terminates or not.
         * @retval true done
         * @retval false not
         */
        bool done() { return (m_pbegin >= m_pend); }
    };

public:
    const char* m_puser_file;  /**< user file path */
    IMDM        m_imdm;        /**< in-message deliver method */
    void* m_puf_handle;        /**< handle of opened user file */
    Graph* m_pmy_graph;        /**< configuration class */
    int m_machine_cnt;         /**< machine count, one master and some workers */
    Addr* m_paddr_table;       /**< address table, master 0 workers from 1 */
    Addr m_addr_self;          /**< self address */
    int m_hdfs_flag;           /**< read input from hdfs or local-fs, hdfs 1 local-fs 0 */
    hdfsFS m_fs_handle;        /**< handle of file system, here hdfs */
    const char* m_pfs_host;    /**< hdfs host */
    int m_fs_port;             /**< hdfs port */
    char* m_pin_path;          /**< input file path */
    char* m_pout_path;         /**< output file path */
    int m_my_aggregator_cnt;              /**< aggregator count */
    AggregatorBase** m_pmy_aggregator;    /**< pointers of AggregatorBase */
    InputFormatter* m_pmy_in_formatter;   /**< pointer of InputFormatter */
    OutputFormatter* m_pmy_out_formatter; /**< pointer of OutputFormatter */
    VertexBase* m_pmy_vertex;             /**< pointer of VertexBase */
    int64_t m_total_vertex;   /**< total vertex number in local subgraph */
    int64_t m_total_edge;     /**< total edge number in local subgraph */
    Node* m_pnode;            /**< node array */
    Edge* m_pedge;            /**< edge array */
    int64_t m_edge_cnt;       /**< current edge count to help add vertices */
    Edge* m_pcur_edge;        /**< current position in edge to help add edges */
    FreeList m_free_list;     /**< freelist to store node-node messages */
    // Msg* m_pnext_all_in_msg;  /**< next superstep messages in IMDM_SEQ */
    // Msg* m_pnext2_all_in_msg; /**< next next superstep messages in IMDM_SEQ */
    ChunkedList* m_pnext_all_in_msg_chunklist;  /**< next superstep messages in IMDM_OPT */
    ChunkedList* m_pnext2_all_in_msg_chunklist; /**< next next superstep messages in IMDM_OPT */
    ResultIterator res_iter;                    /**< computation result iterator */

    Sender    m_sender;      /**< to manage activities about send */
    Receiver  m_receiver;    /**< to manage activities about receive */
    pthread_t m_pth_send;    /**< send thread */
    pthread_t m_pth_receive; /**< receive thread */

    Wm__Begin          m_wm_begin;         /**< worker requests for whole supersteps begin */
    Mw__Begin*         m_pmw_begin;        /**< master responds to whole supersteps begin */
    Wm__CurssFinish    m_wm_curssfinish;   /**< worker current superstep finishes */
    Mw__NextssStart*   m_pmw_nextssstart;  /**< master next superstep starts */
    Mw__End*           m_pmw_end;          /**< master notifies workers to end supersteps */
    Wm__End            m_wm_end;           /**< worker reports to master after ending supersteps */
    Ww__NodemsgList*   m_pww_sendlist;     /**< node2node message send list to other workers */
    size_t*            m_psendlist_curpos; /**< to record current insert-position of sendlist for every worker, from [1] */
    Ww__NodemsgList**  m_pww_recvlist;     /**< node2node message receive list from other workers */

    int m_term;          /**< to mark if supersteps end, 1/0 yes/no */
    int m_from_master;   /**< to mark if received message from master, 1/0 yes/no */
    int* m_pfinish_send; /**< to mark if WW_LARVEREDGELIST/WW_FINISHSENDNODEMSG message sent successfully to every worker, from [1] */
    int m_finishnn_wk;   /**< count of workers having finished sending node2node messages in current superstep */
    int64_t m_node_msg;  /**< count of node messages the worker should receive before next superstep */

public:
    /**
     * Run function.
     * Worker process entrance, which consists of child methods.
     * @param argc command line argument number
     * @param argv command line arguments
     */
    void run(int argc, char* argv[]);

    /**
     * Parse command line arguments.
     * @param argv command line arguments
     */
    void parseCmdArg(char* argv[]);

    /**
     * Load user file.
     * @param argc command line argument number
     * @param argv command line arguments
     */
    void loadUserFile(int argc, char* argv[]);

    /** Initialize some global/member variables. */
    void init();

    /**
     * Add a vertex to system storage for graph.
     * @param vid vertex id
     * @param pvalue pointer of vertex value
     * @param outdegree vertex outdegree
     */
    void addVertex(int64_t vid, void* pvalue, int outdegree);

    /**
     * Add an edge to system storage for graph.
     * @param from edge source vertex id
     * @param to edge destination vertex id
     * @param pweight pointer of edge weight
     */
    void addEdge(int64_t from, int64_t to, void* pweight);

    /** Read graph from input file. */
    void readInput();

    /**
     * Receive a new piece of node-node message for next superstep.
     * @param pmsg pointer of the message
     */
    void recvNewNodeMsg(Msg* pmsg);

    /**
     * Receive a new piece of node-node message for next next superstep.
     * @param pmsg pointer of the message
     */
    void recvNewNodeMsg2(Msg* pmsg);

    /** Deliver all new node-node messages to destination node. */
    void deliverAllNewNodeMsg();

    /** Send request to master for whole supersteps begin. */
    void sendBegin();

    /** Send currrent superstep finish message to master. */
    void sendCurssfinish();

    /** Send superstep already end message to master. */
    void sendEnd();

    /**
     * Send node2node message to a worker.
     * @param worker_id destination machine id
     * @param num_msg number of node messsages
     * @retval 0 send successfully
     * @retval 1 send unsuccessfully
     */
    int sendNodeMessage(int worker_id, int num_msg);

    /**
     * Receive all kinds of messages from master or a worker.
     * @param machine_id source machine id
     */
    void receiveMessage(int machine_id);

    /** Perform a series of supersteps. */
    void performSuperstep();

    /** Write computation results to output file. */
    void writeOutput();

    /** Free some global/member variables. */
    void terminate();
}; // definition of Worker class

#endif /* WORKER_H */
