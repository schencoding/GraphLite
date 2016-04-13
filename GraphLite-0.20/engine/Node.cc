/**
 * @file Node.cc
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
 * @see Node.h
 *
 */

#include <new>
#include <string.h>

#include "Node.h"
#include "Worker.h"

extern Worker worker;

int Edge::e_value_size;
int Edge::e_size;
int Msg::m_value_size;
int Msg::m_size;
int Node::n_value_size;
int Node::n_size;

Node& Node::getNode(int index) {
    return *( (Node *)( (char *)(worker.m_pnode) + index * Node::n_size ) );
}

Edge& Node::getEdge(int index) {
    return *( (Edge *)( (char *)(worker.m_pedge) + index * Edge::e_size ) );
}

void Node::initInMsg() {
    new(&m_cur_in_msg) std::vector<Msg*>();
}

void Node::recvNewMsg(Msg* pmsg) {
    m_cur_in_msg.push_back(pmsg);
    if (! m_active) {
        m_active = true;
        ++worker.m_wm_curssfinish.act_vertex;
    }
}

void Node::clearCurInMsg() {
    if ( m_cur_in_msg.size() ) {

        for (int i = m_cur_in_msg.size() - 1; i >= 0; --i) {
            worker.m_free_list.free(m_cur_in_msg[i]);
        }

        m_cur_in_msg.clear();
    }
}

void Node::freeInMsgVector() {
    (&m_cur_in_msg)->~vector<Msg*>();
}

int Node::getSuperstep() const { return worker.m_wm_curssfinish.superstep; }

int64_t Node::getVertexId() const { return m_v_id; }

void Node::voteToHalt() {
    m_active = false;
    --worker.m_wm_curssfinish.act_vertex;
}

GenericLinkIterator* Node::getGenericLinkIterator() {
    return new GenericLinkIterator(&m_cur_in_msg);
}

/*
GenericArrayIterator* Node::getGenericArrayIterator() {
    return new GenericArrayIterator(
      (char *)(worker.m_pedge) + m_edge_index * Edge::e_size,
      (char *)(worker.m_pedge) + (m_edge_index + m_out_degree) * Edge::e_size,
      Edge::e_size );
}
*/

void Node::sendMessageTo(int64_t dest_vertex, const char* pmessage) {
    int wk = dest_vertex % (worker.m_machine_cnt - 1) + 1; // hash partition

    if (wk == worker.m_addr_self.id) { // self message, can be commented for debug 1

        Msg* pmsg = (Msg *)( worker.m_free_list.allocate() );
        pmsg->s_id = m_v_id;
        pmsg->d_id = dest_vertex;
        memcpy(pmsg->message, pmessage, Msg::m_value_size);

        worker.recvNewNodeMsg(pmsg); // self message certainly not for next next
                                     // superstep but for next
    } else { // message to another worker

        // check if we should send messages
        if (worker.m_psendlist_curpos[wk] == SENDLIST_LEN) {

            while ( worker.sendNodeMessage(wk, worker.m_psendlist_curpos[wk]) );
            // How to change into wait ?

            worker.m_psendlist_curpos[wk] = 0;
            ++worker.m_wm_curssfinish.worker_msg[wk];
        }

        // copy the message into send message list
        Msg* pmsg = (Msg *)(worker.m_pww_sendlist[wk].msgs.data + worker.m_psendlist_curpos[wk] * Msg::m_size);
        pmsg->s_id = m_v_id;
        pmsg->d_id = dest_vertex;
        memcpy(pmsg->message, pmessage, Msg::m_value_size);
        ++worker.m_psendlist_curpos[wk];
        // printf("wrote one piece of node2node message to sendlist[%d]\n", wk); //
    }

    ++worker.m_wm_curssfinish.sent_msg;
}

void Node::sendMessageToAllNeighbors(const char* pmessage) {
    char* pedge = (char *)(worker.m_pedge) + m_edge_index * Edge::e_size;
    for (int i = 0; i < m_out_degree; ++i) {
        sendMessageTo( ( (Edge *)pedge )->to, pmessage );
        pedge += Edge::e_size;
    }
}

const void* Node::getAggrGlobal(int aggr) {
    return worker.m_pmy_aggregator[aggr]->getGlobal();
}

void Node::accumulateAggr(int aggr, const void* p) {
    worker.m_pmy_aggregator[aggr]->accumulate(p);
}
