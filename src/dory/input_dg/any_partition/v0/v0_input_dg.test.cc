/* <dory/input_dg/any_partition/v0/v0_input_dg.test.cc>

   ----------------------------------------------------------------------------
   Copyright 2013-2014 if(we)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   ----------------------------------------------------------------------------

   Unit test for <dory/input_dg/input_dg_util.h>,
   <dory/input_dg/any_partition/v0/v0_write_dg.h>, and
   <dory/input_dg/any_partition/v0/v0_write_msg.h>.
 */

#include <dory/input_dg/input_dg_util.h>
#include <dory/input_dg/any_partition/v0/v0_write_msg.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <capped/pool.h>
#include <capped/reader.h>
#include <dory/anomaly_tracker.h>
#include <dory/client/status_codes.h>
#include <dory/config.h>
#include <dory/msg.h>
#include <dory/msg_state_tracker.h>
#include <dory/test_util/misc_util.h>

#include <gtest/gtest.h>

using namespace Capped;
using namespace Dory;
using namespace Dory::InputDg;
using namespace Dory::TestUtil;

namespace {

  struct TTestConfig {
    std::vector<const char *> Args;

    std::unique_ptr<Dory::TConfig> Cfg;

    std::unique_ptr<TPool> Pool;

    TDiscardFileLogger DiscardFileLogger;

    TAnomalyTracker AnomalyTracker;

    TMsgStateTracker MsgStateTracker;

    TTestConfig();
  };  // TTestConfig

  TTestConfig::TTestConfig()
      : Pool(new TPool(128, 16384, TPool::TSync::Mutexed)),
        AnomalyTracker(DiscardFileLogger, 0,
                       std::numeric_limits<size_t>::max()) {
    Args.push_back("dory");
    Args.push_back("--config_path");
    Args.push_back("/nonexistent/path");
    Args.push_back("--msg_buffer_max");
    Args.push_back("1");  // dummy value
    Args.push_back("--receive_socket_name");
    Args.push_back("dummy_value");
    Args.push_back(nullptr);
    Cfg.reset(new Dory::TConfig(Args.size() - 1, const_cast<char **>(&Args[0]),
        true));
  }

  /* The fixture for testing reading/writing of v0 AnyPartition input
     datagrams. */
  class TV0InputDgTest : public ::testing::Test {
    protected:
    TV0InputDgTest() = default;

    ~TV0InputDgTest() override = default;

    void SetUp() override {
    }

    void TearDown() override {
    }
  };  // TV0InputDgTest

  TEST_F(TV0InputDgTest, Test1) {
    TTestConfig cfg;
    int64_t timestamp = 8675309;
    std::string topic("dumb jokes");
    std::string key("Why did the chicken cross the road?");
    std::string value("Because he got bored writing unit tests.");
    std::vector<uint8_t> buf;
    size_t dg_size = 0;
    int result = input_dg_any_p_v0_compute_msg_size(&dg_size, topic.size(),
        key.size(), value.size());
    ASSERT_EQ(result, DORY_OK);
    buf.resize(dg_size);
    input_dg_any_p_v0_write_msg(&buf[0], timestamp, topic.data(),
        topic.data() + topic.size(), key.data(), key.data() + key.size(),
        value.data(), value.data() + value.size());
    TMsg::TPtr msg = BuildMsgFromDg(&buf[0], buf.size(), *cfg.Cfg, *cfg.Pool,
        cfg.AnomalyTracker, cfg.MsgStateTracker);
    ASSERT_TRUE(!!msg);
    SetProcessed(msg);
    ASSERT_EQ(msg->GetTimestamp(), timestamp);
    ASSERT_EQ(msg->GetTopic(), topic);
    ASSERT_TRUE(KeyEquals(msg, key));
    ASSERT_TRUE(ValueEquals(msg, value));
  }

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
