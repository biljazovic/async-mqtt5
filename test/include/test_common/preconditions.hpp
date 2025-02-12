//
// Copyright (c) 2025 Ivica Siladic, Bruno Iljazovic, Korina Simicevic
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MQTT5_TEST_PRECONDITIONS_HPP
#define BOOST_MQTT5_TEST_PRECONDITIONS_HPP

#include <boost/test/unit_test.hpp>

#include <utility>

namespace boost::mqtt5::test {

struct env_condition {
    std::string env;
    boost::test_tools::assertion_result operator()(boost::unit_test::test_unit_id) {
        return (bool)(std::getenv(env.c_str()));
    }
};
static const env_condition public_broker_cond =
    env_condition { "BOOST_MQTT5_PUBLIC_BROKER_TESTS" };

} // end namespace boost::mqtt5::test

#endif // BOOST_MQTT5_TEST_PRECONDITIONS_HPP
