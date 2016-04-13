/**
 * @file OutputFormatter.cc
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
 * @see OutputFormatter.h
 *
 */

#include "OutputFormatter.h"
#include "Worker.h"

extern Worker worker;

void OutputFormatter::open(const char* pout_path) {
    if (worker.m_hdfs_flag) {
        m_hdfs_file = hdfsOpenFile(worker.m_fs_handle, pout_path,
                                     O_WRONLY|O_CREAT, 0, 0, 0);
        if (! m_hdfs_file) {
            perror("output file open");
            exit(1);
        }
    } else {
        m_local_file.open(pout_path);
    }
}

void OutputFormatter::close() {
    if (worker.m_hdfs_flag) {
        hdfsCloseFile(worker.m_fs_handle, m_hdfs_file);
    } else {
        m_local_file.close();
    }
}

void OutputFormatter::writeNextResLine(char* pbuffer, int len) {
    if (worker.m_hdfs_flag) {
        hdfsWrite(worker.m_fs_handle, m_hdfs_file, (void*)pbuffer, len);
    } else {
        m_local_file.write(pbuffer, len);
    }
}

void OutputFormatter::ResultIterator::getIdValue(int64_t& vid, void* pvalue) {
    worker.res_iter.getIdValue(vid, pvalue);
}

void OutputFormatter::ResultIterator::next() {
    worker.res_iter.next();
}

bool OutputFormatter::ResultIterator::done() {
    return worker.res_iter.done();
}
