/**
 * @file InputFormatter.cc
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
 * @see InputFormatter.h
 *
 */

#include "InputFormatter.h"
#include "Worker.h"

extern Worker worker;

void InputFormatter::open(const char* pin_path) {
    if (worker.m_hdfs_flag) {
        if ( hdfsExists(worker.m_fs_handle, pin_path) ) {
            perror("input file");
            exit(1);
        }

        m_hdfs_file = hdfsOpenFile(worker.m_fs_handle, pin_path,
                                O_RDONLY, 0, 0, 0);
        if (! m_hdfs_file) {
            perror("Error opening input file on HDFS");
            exit(1);
        }

        m_vertex_num_size = 0;
        m_edge_num_size = 0;
        m_next_edge_offset = 0;

        m_ptotal_vertex_line = new char[MAXINPUTLEN];
        m_ptotal_edge_line = new char[MAXINPUTLEN];
        getVertexNumLine();
        getEdgeNumLine();
    } else {
        m_local_file.open(pin_path);
        if (! m_local_file.good()) {
            fprintf(stderr, "Error opening local file %s\n", pin_path);
            exit(1);
        }

        getVertexNumLine();
        getEdgeNumLine();
    }

    m_total_vertex = 0;
    m_total_edge = 0;
    m_n_value_size = 0;
    m_e_value_size = 0;
    m_m_value_size = 0;
}

void InputFormatter::close() {
    if (worker.m_hdfs_flag) {
        hdfsCloseFile(worker.m_fs_handle, m_hdfs_file);
    } else {
        m_local_file.close();
    }
}

void InputFormatter::getVertexNumLine() { 
  if (worker.m_hdfs_flag) {
    // notice hdfs type: int64_t->tOffset, int32_t->tSize
    tOffset offset = 0;
    tSize num_read_bytes = hdfsPread( worker.m_fs_handle, m_hdfs_file, offset,
                                     (void*)m_ptotal_vertex_line, MAXINPUTLEN * sizeof(char) );
    for (int i = 0; i < num_read_bytes; ++i) {
        if (m_ptotal_vertex_line[i] == '\n') {
            m_vertex_num_size = i + 1;
            break;
        }
    }
  }
  else {
    getline(m_local_file, m_total_vertex_line);
    m_ptotal_vertex_line = m_total_vertex_line.c_str();
  }
}

void InputFormatter::getEdgeNumLine() { 
  if (worker.m_hdfs_flag) {
    // notice hdfs type: int64_t->tOffset, int32_t->tSize
    tOffset offset = m_vertex_num_size;
    tSize num_read_bytes = hdfsPread( worker.m_fs_handle, m_hdfs_file, offset,
                                     (void*)m_ptotal_edge_line, MAXINPUTLEN * sizeof(char) );
    for (int i = 0; i < num_read_bytes; ++i) {
        if (m_ptotal_edge_line[i] == '\n') {
            m_edge_num_size = i + 1;
            break;
        }
    }
    m_next_edge_offset = m_vertex_num_size + m_edge_num_size;
  }
  else {
    getline(m_local_file, m_total_edge_line);
    m_ptotal_edge_line = m_total_edge_line.c_str();
  }
}

const char* InputFormatter::getEdgeLine() {
  if (worker.m_hdfs_flag) {
    // notice hdfs type: int64_t->tOffset, int32_t->tSize
    tOffset offset = m_next_edge_offset;
    tSize num_read_bytes = hdfsPread( worker.m_fs_handle, m_hdfs_file, offset,
                                     (void*)m_buf_line, sizeof(m_buf_line) );
    for (int i = 0; i < num_read_bytes; ++i) {
        if (m_buf_line[i] == '\n') {
            m_next_edge_offset += (i + 1);
            break;
        }
    }

    return (num_read_bytes>0 ? m_buf_line : 0);
  }
  else {
    getline(m_local_file, m_buf_string);
    return (m_local_file.good() ? m_buf_string.c_str() : NULL);
  }
}

void InputFormatter::addVertex(int64_t vid, void* pvalue, int outdegree) {
    worker.addVertex(vid, pvalue, outdegree);
}

void InputFormatter::addEdge(int64_t from, int64_t to, void* pweight) {
    worker.addEdge(from, to, pweight);
}

InputFormatter::~InputFormatter() {
    if (worker.m_hdfs_flag) {
        delete[] m_ptotal_vertex_line;
        delete[] m_ptotal_edge_line;
    }
}
