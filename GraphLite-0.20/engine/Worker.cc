/**
 * @file Worker.cc
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
 * @see Worker.h
 *
 */

/**
 * 1. There are some assumptions:
 *   (1) Vertex id counts continuously from 0;
 *   (2) Input file has format as below.
 *         ------------------------------------
 *          vertex number
 *          edge number
 *          edge_source_1, edge_destination_1
 *          edge_source_1, edge_destination_2
 *                        .
 *                        .
 *          edge_source_1, edge_destination_m1
 *          edge_source_2, edge_destination_1
 *          edge_source_2, edge_destination_2
 *                        .
 *                        .
 *          edge_source_2, edge_destination_m2
 *                        .
 *                        .
 *                        .
 *          edge_source_n, edge_destination_1
 *          edge_source_n, edge_destination_2
 *                        .
 *                        .
 *          edge_source_n, edge_destination_mn
 *         ------------------------------------
 *       Notice that edge_source_i is in ascending order and n equals
 *       vertex number may not happen, which means some vertices may have
 *       no outedges.
 * 2. We temporarily use hash partition for whole graph, so that vertex
 *    with vertex id of vid and edges from this vertex will be located at
 *    the worker with id of vid%worker_cnt-1, and vertex index of node array
 *    on this worker is vid/worker_cnt.
 *    Worker id counts from 1 and vertex index of node counts from 0.
 * 3. If we change graph partition method later, addVertex() and node array
 *    initialization in readInput() should be changed accordingly.
 */

#include <dlfcn.h>
#include <string.h>
// #include <sys/time.h>

#include "Worker.h"

#define prefetcht0(mem_var)     \
        __asm__ __volatile__ ("prefetcht0 %0": :"m"(mem_var))
#define prefetcht1(mem_var)     \
        __asm__ __volatile__ ("prefetcht1 %0": :"m"(mem_var))
#define prefetcht2(mem_var)   \
        __asm__ __volatile__ ("prefetcht2 %0": :"m"(mem_var))
#define prefetchnta(mem_var)    \
        __asm__ __volatile__ ("prefetchnta %0": :"m"(mem_var))

#define prefetch(ptr)  prefetcht0(*((char *)(ptr)))

extern Worker worker;

// struct timeval b_time, e_time;
// double elapsed = 0;

void* w_receiveFun(void *args) {
    worker.m_receiver.bindServerAddr(worker.m_addr_self.port);
    worker.m_receiver.listenClient();
    worker.m_receiver.acceptClient();
    worker.m_receiver.recvMsg();
    worker.m_receiver.closeAllSocket();
    return NULL;
}

void* w_sendFun(void *args) {
    worker.m_sender.getSocketFd();
    worker.m_sender.getServerAddr(worker.m_paddr_table);
    worker.m_sender.connectServer(worker.m_addr_self.id);
    worker.m_sender.sendMsg();
    worker.m_sender.closeAllSocket();
    return NULL;
}

void Worker::run(int argc, char* argv[]) {
    parseCmdArg(argv);
    loadUserFile(argc, argv);
    init();
    // gettimeofday(&b_time, NULL);
    readInput();
    // gettimeofday(&e_time, NULL);
    // elapsed = (double)(e_time.tv_sec - b_time.tv_sec) + ((double)e_time.tv_usec - (double)b_time.tv_usec)/1e6;
    // printf("readInput() elapsed: %f\n", elapsed); fflush(stdout);
    performSuperstep();
    writeOutput();
    terminate();
}

void Worker::parseCmdArg(char* argv[]) {
    printf("parseCmdArg\n"); fflush(stdout);

    m_addr_self.id = atoi(argv[1]);
    m_puser_file = argv[2];
    int imdm = 2;
    switch (imdm) {
    case 0:
        m_imdm = IMDM_OPT_PLAIN;
        break;
    case 1:
        m_imdm = IMDM_OPT_GROUP_PREF;
        break;
    case 2:
        m_imdm = IMDM_OPT_SWPL_PREF;
        break;
    default:
        break;
    }
}

void Worker::loadUserFile(int argc, char* argv[]) {
    printf("loadUserFile\n"); fflush(stdout);

    // 1. Open user file.
    m_puf_handle = dlopen(m_puser_file, RTLD_NOW);
    if (! m_puf_handle) {
       fprintf( stderr, "%s\n", dlerror() );
       exit(1);
    }

    // 2. Create Graph class.
    GraphCreateFn create_graph;
    create_graph = (GraphCreateFn)dlsym(m_puf_handle, "create_graph");
    m_pmy_graph = create_graph();
    m_pmy_graph->init(argc - 2, (char **)&argv[2]);

    // 3. Read in configuration from Graph.
    m_machine_cnt = m_pmy_graph->m_machine_cnt;
    m_paddr_table = m_pmy_graph->m_paddr_table;
    m_hdfs_flag = m_pmy_graph->m_hdfs_flag;
    m_pfs_host = m_pmy_graph->m_pfs_host;
    m_fs_port = m_pmy_graph->m_fs_port;

    // input/output file name convention
    m_pin_path = (char *)malloc(SPRINTF_LEN);
    if (! m_pin_path) {
        perror("malloc");
        exit(1);
    }
    sprintf(m_pin_path, "%s_%d", m_pmy_graph->m_pin_path, m_addr_self.id);
    m_pout_path = (char *)malloc(SPRINTF_LEN);
    if (! m_pout_path) {
        perror("malloc");
        exit(1);
    }
    sprintf(m_pout_path, "%s_%d", m_pmy_graph->m_pout_path, m_addr_self.id);

    m_pmy_in_formatter = m_pmy_graph->m_pin_formatter;
    m_pmy_out_formatter = m_pmy_graph->m_pout_formatter;
    m_my_aggregator_cnt = m_pmy_graph->m_aggregator_cnt;
    m_pmy_aggregator = m_pmy_graph->m_paggregator;
    m_pmy_vertex = m_pmy_graph->m_pver_base;
}

void Worker::init() {
    printf("init\n"); fflush(stdout);

    // 1. Get self address from address table through id.
    strcpy(m_addr_self.hostname, m_paddr_table[m_addr_self.id].hostname);
    m_addr_self.port = m_paddr_table[m_addr_self.id].port;

    // 2. Connect to hdfs.
    if (m_hdfs_flag) {
        m_fs_handle = hdfsConnect(m_pfs_host, m_fs_port);
        if (! m_fs_handle) {
            perror("connect to hdfs");
            exit(1);
        }
    }

    // 3. Get node/edge/message size from user vertex.
    Node::n_value_size = m_pmy_vertex->getVSize();
    Node::n_size = offsetof(Node, value) + Node::n_value_size;
    Edge::e_value_size = m_pmy_vertex->getESize();
    Edge::e_size = offsetof(Edge, weight) + Edge::e_value_size;
    Msg::m_value_size = m_pmy_vertex->getMSize();
    Msg::m_size = offsetof(Msg, message) + Msg::m_value_size;

    // 4. Initialize freelist element size.
    m_free_list.setEle(Msg::m_size);

    // 5. Initialize aggregators for superstep 0.
    for (int i = 0; i < m_my_aggregator_cnt; ++i) {
        m_pmy_aggregator[i]->init();
    }

    // 6. Initialize messages with master.
    wm__begin__init(&m_wm_begin);
    m_pmw_begin = NULL;
    wm__curss_finish__init(&m_wm_curssfinish);
    m_wm_curssfinish.n_worker_msg = m_machine_cnt;
    m_wm_curssfinish.worker_msg = (int64_t *)malloc( m_machine_cnt * sizeof(int64_t) );
    if (! m_wm_curssfinish.worker_msg) {
        perror("malloc");
        exit(1);
    }
    m_wm_curssfinish.n_aggr_local = m_my_aggregator_cnt;
    m_wm_curssfinish.aggr_local = (ProtobufCBinaryData *)malloc(
    (m_my_aggregator_cnt < 1 ? 1 : m_my_aggregator_cnt) * sizeof(ProtobufCBinaryData) );
    if (! m_wm_curssfinish.aggr_local) {
        perror("malloc");
        exit(1);
    }
    for (size_t i = 0; i < m_wm_curssfinish.n_aggr_local; ++i) {
        m_wm_curssfinish.aggr_local[i].len = m_pmy_aggregator[i]->getSize();
    }
    m_pmw_nextssstart = NULL;
    m_pmw_end = NULL;
    wm__end__init(&m_wm_end);

    // 7. Initialize messages with workers.

    // send message list
    // Actually [0] for master is not used.
    m_pww_sendlist = (Ww__NodemsgList *)malloc( m_machine_cnt * sizeof(Ww__NodemsgList) );
    int msgs_max_len = SENDLIST_LEN * Msg::m_size;
    char* msgs_buf = (char *)malloc((m_machine_cnt-1) * msgs_max_len);
    if (! m_pww_sendlist || ! msgs_buf) {
        perror("malloc");
        exit(1);
    }
    for (int i = 1; i < m_machine_cnt; ++i) { // 0 is master
        ww__nodemsg_list__init(&m_pww_sendlist[i]);

        m_pww_sendlist[i].num_msgs = 0;
        m_pww_sendlist[i].msg_size = Msg::m_size;
        m_pww_sendlist[i].msgs.len = 0;
        m_pww_sendlist[i].msgs.data = (uint8_t*) msgs_buf;
        msgs_buf += msgs_max_len;
    }

    m_psendlist_curpos = (size_t *)malloc( m_machine_cnt * sizeof(size_t) );
    if (! m_psendlist_curpos) {
        perror("malloc");
        exit(1);
    }

    m_pfinish_send = (int *)malloc( m_machine_cnt * sizeof(int) );
    if (! m_pfinish_send) {
        perror("malloc");
        exit(1);
    }

    // receive message list
    // Actually [0] for master is not used.
    m_pww_recvlist = (Ww__NodemsgList **)malloc( m_machine_cnt * sizeof(Ww__NodemsgList *) );
    if (! m_pww_recvlist) {
        perror("malloc");
        exit(1);
    }

    // 8. Initialize sender and receiver.
    m_receiver.init(m_machine_cnt);
    m_sender.init(m_machine_cnt);

    // 9. Create send and receive thread.
    if ( pthread_create(&m_pth_receive, NULL, w_receiveFun, NULL) ) {
        fprintf(stderr, "Error creating receive thread !\n");
        exit(1);
    }
    if ( pthread_create(&m_pth_send, NULL, w_sendFun, NULL) ) {
        fprintf(stderr, "Error creating send thread !\n");
        exit(1);
    }
}

/*
  pnode can't accumulate by Node::n_size cuz previous and current vertices
  maynot be stored continuously in node array in consideration of vertices
  with no outedges.
*/
void Worker::addVertex(int64_t vid, void* pvalue, int outdegree) {
    int worker_cnt = m_machine_cnt - 1;
    int64_t index = vid / worker_cnt; // hash partition
    Node* pnode = (Node *)( (char *)m_pnode + index * Node::n_size );
    pnode->m_out_degree = outdegree;
    pnode->m_edge_index = m_edge_cnt;
    m_edge_cnt += (pnode->m_out_degree);
    memcpy(pnode->value, pvalue, Node::n_value_size);
}

/*
  m_pcur_edge can accumulate by Edge::e_size cuz previous and current edges
  must be stored continuously in edge array.
*/
void Worker::addEdge(int64_t from, int64_t to, void* pweight) {
    m_pcur_edge->from = from;
    m_pcur_edge->to = to;
    memcpy(m_pcur_edge->weight, pweight, Edge::e_value_size);
    m_pcur_edge = (Edge *)( (char *)m_pcur_edge + Edge::e_size );
}

void Worker::readInput() {
    printf("readInput\n"); fflush(stdout);

    // 1. Open input file and get basic information about graph.
    m_pmy_in_formatter->open(m_pin_path);
    m_total_vertex = m_pmy_in_formatter->getVertexNum();
    m_total_edge = m_pmy_in_formatter->getEdgeNum();
    m_pmy_in_formatter->getVertexValueSize();
    m_pmy_in_formatter->getEdgeValueSize();
    m_pmy_in_formatter->getMessageValueSize();

    // 2. Allocate memory for vertices.
    m_pnode = (Node *)malloc(m_total_vertex * Node::n_size);
    if (! m_pnode) {
        perror("malloc");
        exit(1);
    }

    // 3. Initialize node array. cur_vid related to hash partition
    char* p = (char *)m_pnode;
    int64_t cur_vid = m_addr_self.id - 1;
    int worker_cnt = m_machine_cnt - 1;
    for (int64_t i = 0; i < m_total_vertex;
          ++i, p += Node::n_size, cur_vid += worker_cnt) {
        Node* pnode = (Node *)p;
        pnode->m_active = true;
        pnode->m_v_id = cur_vid;
        pnode->m_out_degree = 0;
        pnode->m_edge_index = -1;
        memset(pnode->value, 0, Node::n_value_size);
        pnode->initInMsg();
    }

    // 4. Allocate memory for edges.
	m_pedge = (Edge *)malloc(m_total_edge * Edge::e_size);
	if (! m_pedge) {
		perror("malloc");
		exit(1);
	}
    m_edge_cnt = 0;

    // 5. Judge if vertex/edge/message types are consistent between user file
    //    and input file.
	if (Node::n_value_size != m_pmy_in_formatter->m_n_value_size) {
        perror("vertex type inconsistent between user file and input file");
        exit(1);
    }
    if (Edge::e_value_size != m_pmy_in_formatter->m_e_value_size) {
        perror("edge type inconsistent between user file and input file");
        exit(1);
    }
    if (Msg::m_value_size != m_pmy_in_formatter->m_m_value_size) {
        perror("message type inconsistent between user file and input file");
        exit(1);
    }

    // Notice that it's useless to assign cur_node/cur_edge in constructor.
    // The proper time is after memory allocation for vertices/edges.
    m_pcur_edge = m_pedge;
	m_pmy_in_formatter->loadGraph();

    m_pmy_in_formatter->close();
}

void Worker::recvNewNodeMsg(Msg* pmsg) {
    switch (m_imdm) {
    case IMDM_OPT_PLAIN:
    case IMDM_OPT_GROUP_PREF:
    case IMDM_OPT_SWPL_PREF:
        {
        m_pnext_all_in_msg_chunklist->append(pmsg);
        }
        break;
    default:
        break;
    }
}

void Worker::recvNewNodeMsg2(Msg* pmsg) {
    switch (m_imdm) {
    case IMDM_OPT_PLAIN:
    case IMDM_OPT_GROUP_PREF: 
    case IMDM_OPT_SWPL_PREF:
        {
        m_pnext2_all_in_msg_chunklist->append(pmsg);
        }
        break;
    default:
        break;
    }
}

void Worker::deliverAllNewNodeMsg() {
    Node* np;
    
    // Deliver messages to next_in_msg in each node.
    switch (m_imdm) {
    case IMDM_OPT_PLAIN: { // This uses the data structure but does not optimize.
        Msg *mp;
        int worker_cnt = m_machine_cnt - 1;

        ChunkedList::Iterator* pit = m_pnext_all_in_msg_chunklist->getIterator();
        for ( mp = (Msg *)pit->next(); mp; mp = (Msg *)pit->next() ) {
            int64_t index = mp->d_id / worker_cnt;
            np = (Node *)( (char *)m_pnode + index * Node::n_size );
            np->recvNewMsg(mp);
        }

        // for the new superstep
        delete pit;
        delete m_pnext_all_in_msg_chunklist;
        m_pnext_all_in_msg_chunklist = m_pnext2_all_in_msg_chunklist;
        m_pnext2_all_in_msg_chunklist = new ChunkedList();
        }
        break;

    case IMDM_OPT_GROUP_PREF: {
        Msg *mp;
        int worker_cnt = m_machine_cnt - 1;
        ChunkedList::Iterator* pit = m_pnext_all_in_msg_chunklist->getIterator();

        // preparation
        const int pref_group_size = 8;  // This is a tunable parameter
        int64_t total_num = m_pnext_all_in_msg_chunklist->total();

        Msg  *msg[pref_group_size];
        Node *nd[pref_group_size];

        // for each group
        for (int64_t i = pref_group_size; i < total_num; i += pref_group_size) {

             // (1) obtain the Msg* and prefetch
             for (int j = 0; j < pref_group_size; j++) {
                 msg[j] = (Msg *)pit->next();
                 prefetch(msg[j]);
             }

             // (2) compute node and prefetch
             for (int j = 0; j < pref_group_size; j++) {
                int64_t index = msg[j]->d_id / worker_cnt;
                nd[j] = (Node *)( (char *)m_pnode + index * Node::n_size );
                prefetch(nd[j]);
             }
        
             // (3) deliver message
             for (int j = 0; j < pref_group_size; j++) {
                nd[j]->recvNewMsg(msg[j]);
             }
        }

        // process the rest of the messages
        for ( mp = (Msg *)pit->next(); mp; mp = (Msg *)pit->next() ) {
            int64_t index = mp->d_id / worker_cnt;
            np = (Node *)( (char *)m_pnode + index * Node::n_size );
            np->recvNewMsg(mp);
        }

        // for the new superstep
        delete pit;
        delete m_pnext_all_in_msg_chunklist;
        m_pnext_all_in_msg_chunklist = m_pnext2_all_in_msg_chunklist;
        m_pnext2_all_in_msg_chunklist = new ChunkedList();
        }
        break;

    case IMDM_OPT_SWPL_PREF: {
        int worker_cnt = m_machine_cnt - 1;
        ChunkedList::Iterator* pit = m_pnext_all_in_msg_chunklist->getIterator();
        int64_t total_num = m_pnext_all_in_msg_chunklist->total();

        // preparation
        const int pref_dist = 4;  // This is a tunable parameter
        const int pref_group_size = 16; // pref_group_size >= pref_dist*3
                                        // must be a power of 2
        Msg  *msg[pref_group_size];
        Node *nd[pref_group_size];
        int j1, j2, j3;  // j2 = j1 + pref_dist, j3 = j2 + pref_dist

        int64_t i, i_end;

        // start up
        i = 0;
        i_end = 2*pref_dist;
        if (i_end > total_num) i_end = total_num;

        for (; i < i_end; i++) {

             // (1) obtain the Msg* and prefetch
             j1 = i%pref_group_size;
             msg[j1] = (Msg *)pit->next();
             prefetch(msg[j1]);

             // (2) compute node and prefetch
             if (i >= pref_dist) {
                 j2= (i - pref_dist)%pref_group_size;
                 int64_t index = msg[j2]->d_id / worker_cnt;
                 nd[j2] = (Node *)( (char *)m_pnode + index * Node::n_size );
                 prefetch(nd[j2]);
             }

        }

        // main pipeline
        i_end = total_num;
        for (; i < i_end; i++) {

             // (1) obtain the Msg* and prefetch
             j1 = i%pref_group_size;
             msg[j1] = (Msg *)pit->next();
             prefetch(msg[j1]);

             // (2) compute node and prefetch
             j2 = (i - pref_dist)%pref_group_size;
             int64_t index = msg[j2]->d_id / worker_cnt;
             nd[j2] = (Node *)( (char *)m_pnode + index * Node::n_size );
             prefetch(nd[j2]);

             // (3) deliver message
             j3 = (i - 2 * pref_dist) % pref_group_size;
             nd[j3]->recvNewMsg(msg[j3]);
        }

        // drain down
        i_end = total_num + 2 * pref_dist;
        for (; i < i_end; i++) {

             // (2) compute node and prefetch
             if (i >= pref_dist && i - pref_dist < total_num) {
                 // "&&" has lower priority than ">=" "-" "<" .
                 j2 = (i - pref_dist) % pref_group_size;
                 int64_t index = msg[j2]->d_id / worker_cnt;
                 nd[j2] = (Node *)( (char *)m_pnode + index * Node::n_size );
                 prefetch(nd[j2]);
             }

             // (3) deliver message
             if (i >= 2 * pref_dist) {
                 j3 = (i - 2 * pref_dist) % pref_group_size;
                 nd[j3]->recvNewMsg(msg[j3]);
             }
        }

        // for the new superstep
        delete pit;
        delete m_pnext_all_in_msg_chunklist;
        m_pnext_all_in_msg_chunklist = m_pnext2_all_in_msg_chunklist;
        m_pnext2_all_in_msg_chunklist = new ChunkedList();
        }
        break;
    default:
        break;
    }
}

void Worker::sendBegin() {

    // 1. Set message content to be sent.
    m_wm_begin.s_id = m_addr_self.id;
    m_wm_begin.d_id = 0;
    m_wm_begin.state = 0;

    // 2. Send to master.
    pthread_mutex_lock(&m_sender.m_out_mutex);
    while (m_sender.m_out_buffer[0].m_state) {
        pthread_cond_wait(&m_sender.m_out_cond, &m_sender.m_out_mutex);
    }

    // totally empty, can write to m_out_buffer
    m_sender.m_out_buffer[0].m_msg_len = sizeof(int) +
        wm__begin__pack(&m_wm_begin,
        &m_sender.m_out_buffer[0].m_buffer[2 * sizeof(int)]);
    m_sender.m_out_buffer[0].m_buf_len = m_sender.m_out_buffer[0].m_msg_len + sizeof(int);
    * (int *)m_sender.m_out_buffer[0].m_buffer = m_sender.m_out_buffer[0].m_buf_len;
    * (int *)&(m_sender.m_out_buffer[0].m_buffer[sizeof(int)]) = WM_BEGIN;
    m_sender.m_out_buffer[0].m_head = 0;
    m_sender.m_out_buffer[0].m_tail = m_sender.m_out_buffer[0].m_buf_len;
    m_sender.m_out_buffer[0].m_state = 1;

    pthread_mutex_unlock(&m_sender.m_out_mutex);
}

void Worker::sendCurssfinish() {

    // 1. Set message content to be sent.
    m_wm_curssfinish.s_id = m_addr_self.id;
    m_wm_curssfinish.d_id = 0;
    for (size_t i = 0; i < m_wm_curssfinish.n_aggr_local; ++i) {
        m_wm_curssfinish.aggr_local[i].data = (uint8_t *)( m_pmy_aggregator[i]->getLocal() );
    }

    // 2. Send to master.
    pthread_mutex_lock(&m_sender.m_out_mutex);
    while (m_sender.m_out_buffer[0].m_state) {
        pthread_cond_wait(&m_sender.m_out_cond, &m_sender.m_out_mutex);
    }

    // totally empty, can write to m_out_buffer
    m_sender.m_out_buffer[0].m_msg_len = sizeof(int) +
        wm__curss_finish__pack(&m_wm_curssfinish,
        &m_sender.m_out_buffer[0].m_buffer[2 * sizeof(int)]);
    m_sender.m_out_buffer[0].m_buf_len = m_sender.m_out_buffer[0].m_msg_len + sizeof(int);
    * (int *)m_sender.m_out_buffer[0].m_buffer = m_sender.m_out_buffer[0].m_buf_len;
    * (int *)&(m_sender.m_out_buffer[0].m_buffer[sizeof(int)]) = WM_CURSSFINISH;
    m_sender.m_out_buffer[0].m_head = 0;
    m_sender.m_out_buffer[0].m_tail = m_sender.m_out_buffer[0].m_buf_len;
    m_sender.m_out_buffer[0].m_state = 1;

    pthread_mutex_unlock(&m_sender.m_out_mutex);
}

void Worker::sendEnd() {

    // 1. Set message content to be sent.
    m_wm_end.s_id = m_addr_self.id;
    m_wm_end.d_id = 0;
    m_wm_end.state = 0;

    // 2. Send to master.
    pthread_mutex_lock(&m_sender.m_out_mutex);
    while (m_sender.m_out_buffer[0].m_state) {
        pthread_cond_wait(&m_sender.m_out_cond, &m_sender.m_out_mutex);
    }

    // totally empty, can write to m_out_buffer
    m_sender.m_out_buffer[0].m_msg_len = sizeof(int) +
           wm__end__pack(&m_wm_end,
           &m_sender.m_out_buffer[0].m_buffer[2 * sizeof(int)]);
    m_sender.m_out_buffer[0].m_buf_len = m_sender.m_out_buffer[0].m_msg_len + sizeof(int);
    * (int *)m_sender.m_out_buffer[0].m_buffer = m_sender.m_out_buffer[0].m_buf_len;
    * (int *)&(m_sender.m_out_buffer[0].m_buffer[sizeof(int)]) = WM_END;
    m_sender.m_out_buffer[0].m_head = 0;
    m_sender.m_out_buffer[0].m_tail = m_sender.m_out_buffer[0].m_buf_len;
    m_sender.m_out_buffer[0].m_state = 1;

    pthread_mutex_unlock(&m_sender.m_out_mutex);
}

int Worker::sendNodeMessage(int worker_id, int num_msg) {

    // 1. Get new messages in current superstep from other workers if necessary.
    for (int j = 1; j < m_machine_cnt; ++j) receiveMessage(j);

    // 2. Set message content to be sent.
    m_pww_sendlist[worker_id].s_id = m_addr_self.id;
    m_pww_sendlist[worker_id].d_id = worker_id;
    m_pww_sendlist[worker_id].superstep = m_wm_curssfinish.superstep;
    m_pww_sendlist[worker_id].num_msgs = num_msg;
    m_pww_sendlist[worker_id].msgs.len = num_msg * m_pww_sendlist[worker_id].msg_size;
    // m_pww_sendlist[worker_id].msgs.data has been set in Node::sendMessageTo()

    // 3. Send to dest-worker.
    if (! m_sender.m_out_buffer[worker_id].m_state) { // just test, totally empty, can write to m_out_buffer

        pthread_mutex_lock(&m_sender.m_out_mutex);
        if (! m_sender.m_out_buffer[worker_id].m_state) { // totally empty, can write to m_out_buffer
            m_sender.m_out_buffer[worker_id].m_msg_len = sizeof(int) +
                ww__nodemsg_list__pack(&m_pww_sendlist[worker_id],
                &m_sender.m_out_buffer[worker_id].m_buffer[2 * sizeof(int)]);
            m_sender.m_out_buffer[worker_id].m_buf_len = m_sender.m_out_buffer[worker_id].m_msg_len + sizeof(int);
            * (int *)m_sender.m_out_buffer[worker_id].m_buffer = m_sender.m_out_buffer[worker_id].m_buf_len;
            * (int *)&(m_sender.m_out_buffer[worker_id].m_buffer[sizeof(int)]) = WW_NODEMSGLIST;
            m_sender.m_out_buffer[worker_id].m_head = 0;
            m_sender.m_out_buffer[worker_id].m_tail = m_sender.m_out_buffer[worker_id].m_buf_len;
            m_sender.m_out_buffer[worker_id].m_state = 1;
        }
        pthread_mutex_unlock(&m_sender.m_out_mutex);

        return 0;
    } // else can't write to m_out_buffer

    return 1;
}

void Worker::receiveMessage(int machine_id) {

    if (m_receiver.m_in_buffer[machine_id].m_state) { // just test, totally full, can read from m_in_buffer

        pthread_mutex_lock(&m_receiver.m_in_mutex);
        if (m_receiver.m_in_buffer[machine_id].m_state) { // totally full, can read from m_in_buffer
            m_receiver.m_in_buffer[machine_id].m_buf_len = * (int *)m_receiver.m_in_buffer[machine_id].m_buffer;
            m_receiver.m_in_buffer[machine_id].m_msg_type = * (int *)&(m_receiver.m_in_buffer[machine_id].m_buffer[sizeof(int)]);

            if (m_receiver.m_in_buffer[machine_id].m_buf_len) {
                m_receiver.m_in_buffer[machine_id].m_msg_len = m_receiver.m_in_buffer[machine_id].m_buf_len - sizeof(int);
                int pack_len = m_receiver.m_in_buffer[machine_id].m_msg_len - sizeof(int);

                 switch (m_receiver.m_in_buffer[machine_id].m_msg_type) {
                 case MW_BEGIN:
                     m_pmw_begin = mw__begin__unpack(NULL, pack_len, &m_receiver.m_in_buffer[machine_id].m_buffer[2 * sizeof(int)]);
                     if (m_pmw_begin->state == 0) { // this worker ready to begin
                         m_from_master = 1;
                     }
                     mw__begin__free_unpacked(m_pmw_begin, NULL);
                     break;
                 case MW_NEXTSSSTART:
                     // We keep the m_pmw_nextssstart until the next use, e.g. visit ->superstep/node_msg/aggr_global.
                     if (m_pmw_nextssstart) {
                         mw__nextss_start__free_unpacked(m_pmw_nextssstart, NULL);
                         m_pmw_nextssstart = NULL;
                     }
                     m_pmw_nextssstart = mw__nextss_start__unpack(NULL, pack_len, &m_receiver.m_in_buffer[machine_id].m_buffer[2 * sizeof(int)]);
                     m_node_msg = m_pmw_nextssstart->node_msg;
                     // m_node_msg can be used to control time of worker waiting
                     // to receive messages from other workers, but here we use
                     // m_finishnn_wk to control.
                     for (int i = 0; i < m_my_aggregator_cnt; ++i) {
                         m_pmy_aggregator[i]->setGlobal(m_pmw_nextssstart->aggr_global[i].data);
                     }
                     m_from_master = 1;
                     break;
                 case MW_END:
                     m_pmw_end = mw__end__unpack(NULL, pack_len, &m_receiver.m_in_buffer[machine_id].m_buffer[2 * sizeof(int)]);
                     if (m_pmw_end->state == 0) { // OK to end
                         m_term = 1;
                     }
                     mw__end__free_unpacked(m_pmw_end, NULL);
                     m_from_master = 1;
                     break;
                 case WW_NODEMSGLIST:
                     m_pww_recvlist[machine_id] = ww__nodemsg_list__unpack(NULL, pack_len, &m_receiver.m_in_buffer[machine_id].m_buffer[2 * sizeof(int)]);
                     if (m_pww_recvlist[machine_id]->num_msgs == 0) {
                         ++m_finishnn_wk;
                     } else {
                         int ss = m_pww_recvlist[machine_id]->superstep;
                         if (ss == m_wm_curssfinish.superstep) {

                             Msg* pnode_msg;
                             char* p = (char *)(m_pww_recvlist[machine_id]->msgs.data);
                             for (int i = 0; i < m_pww_recvlist[machine_id]->num_msgs; ++i, p+=Msg::m_size) {
                                 pnode_msg = (Msg *)( m_free_list.allocate() );
                                 memcpy(pnode_msg, p, Msg::m_size);
                                 recvNewNodeMsg(pnode_msg);
                             }

                         } else if (ss == m_wm_curssfinish.superstep + 1) {

                             Msg* pnode_msg;
                             char* p = (char *)(m_pww_recvlist[machine_id]->msgs.data);
                             for (int i = 0; i < m_pww_recvlist[machine_id]->num_msgs; ++i, p+=Msg::m_size) {
                                 pnode_msg = (Msg *)( m_free_list.allocate() );
                                 memcpy(pnode_msg, p, Msg::m_size);
                                 recvNewNodeMsg2(pnode_msg);
                             }

                         }

                         // This is the total number of received node-node messages
                         m_wm_curssfinish.recv_msg += m_pww_recvlist[machine_id]->num_msgs;
                     }
                     ww__nodemsg_list__free_unpacked(m_pww_recvlist[machine_id], NULL);
                     break;
                 default:
                     fprintf(stderr, "There is no such message type !\n");
                     break;
                 }

                m_receiver.m_in_buffer[machine_id].m_buf_len = 0;
                m_receiver.m_in_buffer[machine_id].m_state = 0;
            }
        }
        pthread_mutex_unlock(&m_receiver.m_in_mutex);

    } // else can't read from m_in_buffer
}

void Worker::performSuperstep() {
    printf("performSuperstep\n"); fflush(stdout);

    // 1. Initialize before the first superstep.
    // The in-message lists are empty.
    switch (m_imdm) {
    case IMDM_OPT_PLAIN:
    case IMDM_OPT_GROUP_PREF: 
    case IMDM_OPT_SWPL_PREF:
        {
        m_pnext_all_in_msg_chunklist = new ChunkedList();
        m_pnext2_all_in_msg_chunklist = new ChunkedList();
        }
        break;
    default:
        break;
    }
    m_wm_curssfinish.superstep = -1;
    m_wm_curssfinish.act_vertex = m_total_vertex;

    // 2. Send requests for whole supersteps begin to master.
    sendBegin();
    printf("sent WM_BEGIN\n"); fflush(stdout);

    // 3. Receive master respond to whole supersteps begin.
    m_from_master = 0;
    while (! m_from_master) receiveMessage(0);
    printf("received MW_BEGIN\n"); fflush(stdout);
 
    // 4. Run into supersteps.
    int msg2send; // count of messages to send, used in 4.4 & 4.5
    m_term = 0;
    while (! m_term) {

        // 4.1 Initialize before every superstep.
        ++m_wm_curssfinish.superstep;
        printf("-----------------------------------------\n"); fflush(stdout);
        printf("superstep: %d\n", m_wm_curssfinish.superstep); fflush(stdout);
        m_wm_curssfinish.compute = 0;
        m_wm_curssfinish.recv_msg = 0;
        m_wm_curssfinish.sent_msg = 0;
        memset( m_wm_curssfinish.worker_msg, 0, m_machine_cnt * sizeof(int32_t) );
        memset( m_psendlist_curpos, 0, m_machine_cnt * sizeof(size_t) );
        memset( m_pfinish_send, 0, m_machine_cnt * sizeof(int) );
        m_finishnn_wk = 2; // excluding master and myself
        // m_finishnn_wk = 1; // excluding master only, for debug 2

        // 4.2 Deliver node messages to current in-message list.
        deliverAllNewNodeMsg();

        // 4.3 Local compute.
        // printf("before compute()\n"); fflush(stdout);
        char* p = (char *)m_pnode;
        for (int64_t i = 0; i < m_total_vertex; ++i, p += Node::n_size) {
            Node* pnode = (Node *)p;

            if (pnode->m_active) { // active node
                m_pmy_vertex->setMe(pnode);

                GenericLinkIterator* pgeneric_link_iterator =
                                     pnode->getGenericLinkIterator();
                m_pmy_vertex->compute(pgeneric_link_iterator);
                delete pgeneric_link_iterator;

                pnode->clearCurInMsg();
                ++m_wm_curssfinish.compute;
             }

             // get new messages in current superstep from other workers if necessary
             for (int j = 1; j < m_machine_cnt; ++j) receiveMessage(j);
        } // end of local compute
        // printf("after compute()\n"); fflush(stdout);

        // 4.4 Send last node2node message to some workers.
        do {
            msg2send = 0;
            for (int i = 1; i < m_machine_cnt; ++i) {
                if (m_psendlist_curpos[i] > 0) {
                    ++msg2send;
                    if ( sendNodeMessage(i, m_psendlist_curpos[i]) ) continue;
                    printf("sent WW_NODEMSGLIST, %lld msgs to worker[%d]\n",
                        (long long)m_psendlist_curpos[i], i); fflush(stdout);
                    ++m_wm_curssfinish.worker_msg[i];
                    m_psendlist_curpos[i] = 0;
                    --msg2send;
                }
            }
        } while (msg2send);

        // 4.5 Notify the other workers to have finished sending node messages.
        /*
           Here just need a flag array, if not consider time for memset size_t
           costs more than int, can take m_psendlist_curpos as convenience.
           If change all flags in whole program as bit representation later,
           these two flag array can be combined to one.
           memset( m_psendlist_curpos, -1, m_machine_cnt * sizeof(size_t) );
           m_pfinish_send[i] = 0;
        */
        do {
            msg2send = 0;
            for (int i = 1; i < m_machine_cnt; ++i) {
                if (i == m_addr_self.id) continue; // do not send myself finish of sending, can be commented for debug 3
                if (! m_pfinish_send[i]) {
                    ++msg2send;
                    if ( sendNodeMessage(i, 0) ) continue; // here argument 0 means this is a signal messsage
                    printf("sent WW_FINISHSENDNODEMSG to worker[%d]\n", i); fflush(stdout);
                    m_pfinish_send[i] = 1;
                    --msg2send;
                }
            }
        } while (msg2send);

        // 4.6 Wait for all the other workers' signals of having finished
        // sending node messages.
        while (m_finishnn_wk < m_machine_cnt) {
            for (int j = 1; j < m_machine_cnt; ++j) receiveMessage(j);
        }
        printf("received all WW_NODEMSGLIST && WW_FINISHNODEMSG\n"); fflush(stdout);
        
        // 4.7 Send current superstep finish to master.
        sendCurssfinish();
        printf("sent WM_CURSSFINISH\n"); fflush(stdout);

        // 4.8 Clear aggregator values before barrier rather than in 4.1,
        // cuz global value will be set later and used in compute() of next
        // superstep.
        for (int i = 0; i < m_my_aggregator_cnt; ++i) m_pmy_aggregator[i]->init();

        // 4.9 Wait for barrier message from master.
        m_from_master = 0;
        while (! m_from_master) receiveMessage(0);
        if (m_term) {
            printf("received MW_END\n"); fflush(stdout);
            sendEnd();
            printf("sent WM_END\n"); fflush(stdout);
        } else {
            printf("received MW_NEXTSSSTART\n"); fflush(stdout);
        }
    } // end of while

    // 5. Set main thread to terminate.
    main_term = 1;
}

void Worker::writeOutput() {
    printf("writeOutput\n"); fflush(stdout);

    m_pmy_out_formatter->open(m_pout_path);
    
    res_iter.init(m_pnode, m_total_vertex);

    m_pmy_out_formatter->writeResult();
    
    m_pmy_out_formatter->close();
}


void Worker::terminate() {
    printf("terminate\n"); fflush(stdout);

    // 1. Disconnect from hdfs.
    if ( m_hdfs_flag && hdfsDisconnect(m_fs_handle) ) {
        perror("disconnect from hdfs");
        exit(1);
    }

    // 2. Destroy Graph class.
    if (m_pin_path) free(m_pin_path);
    if (m_pout_path) free(m_pout_path);

    m_pmy_graph->term();
    GraphDestroyFn destroy_graph;
    destroy_graph = (GraphDestroyFn)dlsym(m_puf_handle, "destroy_graph");
    destroy_graph(m_pmy_graph);

    // 3. Free memory allocated.
    if (m_pnext_all_in_msg_chunklist) delete m_pnext_all_in_msg_chunklist;
    if (m_pnext2_all_in_msg_chunklist) delete m_pnext2_all_in_msg_chunklist;
    if (m_pedge) free(m_pedge);

    char* p = (char *)m_pnode;
    for (int64_t i = 0; i < m_total_vertex; ++i, p += Node::n_size) {
        Node* pnode = (Node *)p;
        pnode->freeInMsgVector();
    }
    if (m_pnode) free(m_pnode);

    if (m_pmw_nextssstart) {
        mw__nextss_start__free_unpacked(m_pmw_nextssstart, NULL);
    }
    if (m_pww_recvlist) free(m_pww_recvlist);
    if (m_pfinish_send) free(m_pfinish_send);
    if (m_psendlist_curpos) free(m_psendlist_curpos);
    if (m_pww_sendlist[1].msgs.data) free(m_pww_sendlist[1].msgs.data); // msgs_buf
    if (m_pww_sendlist) free(m_pww_sendlist);
    if (m_wm_curssfinish.aggr_local) free(m_wm_curssfinish.aggr_local);
    if (m_wm_curssfinish.worker_msg) free(m_wm_curssfinish.worker_msg);

    // 4. Quit receive & send threads.
    void* recv_retval;
    pthread_join(m_pth_receive, &recv_retval);
    void* send_retval;
    pthread_join(m_pth_send, &send_retval);
}
