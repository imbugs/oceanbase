/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_log_reader.h for ...
 *
 * Authors:
 *   yanran <yanran.hfs@taobao.com>
 *
 */
#ifndef OCEANBASE_COMMON_OB_LOG_READER_H_
#define OCEANBASE_COMMON_OB_LOG_READER_H_

#include "ob_log_entry.h"
#include "ob_single_log_reader.h"
#include "ob_atomic.h"

namespace oceanbase
{
  namespace common
  {
    /**
     * 日志读取类, 从一个指定的日志id开始, 遇到日志文件结束, 则打开下一个日志文件
     * 主要有两种使用方法:
     *   1. Master启动时回放日志, 读到所有日志结束则回放完成
     *   2. Slave的日志回放线程使用: Slave采用一个线程同步日志, 另一个线程回放日志时, 
     *      回放线程不停追赶日志文件, 当读到日志结束时, 应该等待小段时间后, 再次读日志
     */
    class ObLogReader
    {
    public:
      static const int64_t WAIT_TIME = 1000000; //us
      static const int FAIL_TIMES = 60;
    public:
      ObLogReader();
      virtual ~ObLogReader();

      /**
       * 初始化
       * @param [in] log_dir 日志目录
       * @param [in] log_file_id_start 日志读取起始文件id
       * @param [in] is_wait 在打开文件和读取数据出错时，是否重试
       */
      int init(const char* log_dir, const uint64_t log_file_id_start, bool is_wait);

      /**
       * @brief 读日志
       * 遇到SWITCH_LOG日志时直接打开下一个日志文件
       * 在打开下一个日志文件时, 如果遇到文件不存在, 可能是产生日志的地方正在切文件
       * 等待1ms后重试, 最多重试10次, 如果还不存在则报错
       * @return OB_SUCCESS 成功
       * @return OB_READ_NOTHING 没有读取到内容, 可能是日志结束了, 
       *                         也可能是日志正在产生, 读到了中间状态数据
       *                         由调用者根据自己逻辑做不同处理
       * @return otherwise 失败
       */
      int read_log(LogCommand &cmd, uint64_t &seq, char* &log_data, int64_t &data_len);

      inline void set_max_log_file_id(uint64_t max_log_file_id)
      {
        atomic_exchange(&max_log_file_id_, max_log_file_id);
        has_max_ = true;
      }

      inline uint64_t get_max_log_file_id() const
      {
        return max_log_file_id_;
      }

      inline bool get_has_max() const
      {
        return has_max_;
      }

      inline void set_has_no_max()
      {
        has_max_ = false;
      }

    private:
      int open_log_(const uint64_t log_file_id, const uint64_t last_log_seq = 0);

      int read_log_(LogCommand &cmd, uint64_t &log_seq, char *&log_data, int64_t &data_len);

    private:
      uint64_t cur_log_file_id_;
      uint64_t max_log_file_id_;
      ObSingleLogReader log_file_reader_;
      bool is_initialized_;
      bool is_wait_;
      bool has_max_;
    };
  } // end namespace common
} // end namespace oceanbase

#endif // OCEANBASE_COMMON_OB_LOG_READER_H_
