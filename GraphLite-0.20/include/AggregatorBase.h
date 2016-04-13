/**
 * @file AggregatorBase.h
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
 * This file defined AggregatorBase class to load user Aggregator class.
 * AggregatorBase is an abstract class with virtual methods.
 *
 * There is a 3-layer inheritance hierarchy about Aggregator: AggregatorBase is
 * on top, Aggregator class of template is in the middle, and user Aggregator
 * class is at the bottom.
 *
 * Both AggregatorBase and Aggreagator can be abstract class(include pure
 * virtual method), and pure virtual methods in AggregatorBase can also be
 * written into virtual methods.
 * The differrence is, pure virtual methods need not have implementation, but
 * virtual methods need just like other common functions.
 *
 */

#ifndef AGGREGATORBASE_H
#define AGGREGATORBASE_H

/** Definition of AggregatorBase class. */
class AggregatorBase {
public:
    /** Initialize, mainly for aggregator value. */
    virtual void init() = 0;

    /**
     * Get aggregator value type size.
     * @return aggregator value type size.
     */
    virtual int getSize() const = 0;

    /**
     * Get aggregator global value.
     * @return pointer of aggregator global value
     */
    virtual void* getGlobal() = 0;

    /**
     * Set aggregator global value.
     * @param p pointer of value to set global as
     */
    virtual void setGlobal(const void* p) = 0;

    /**
     * Get aggregator local value.
     * @return pointer of aggregator local value
     */
    virtual void* getLocal() = 0;

    /**
     * Merge method for global.
     * @param p pointer of value to be merged
     */
    virtual void merge(const void* p) = 0;

    /**
     * Accumulate method for local.
     * @param p pointer of value to be accumulated
     */
    virtual void accumulate(const void* p) = 0;
}; // definition of AggregatorBase class

#endif /* AGGREGATORBASE_H */
