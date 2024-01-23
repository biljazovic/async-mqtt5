#include <boost/test/unit_test.hpp>

#include <boost/asio/io_context.hpp>

#include <async_mqtt5/impl/subscribe_op.hpp>

#include "test_common/test_service.hpp"

using namespace async_mqtt5;

BOOST_AUTO_TEST_SUITE(subscribe_op/*, *boost::unit_test::disabled()*/)

BOOST_AUTO_TEST_CASE(pid_overrun) {
	constexpr int expected_handlers_called = 1;
	int handlers_called = 0;

	asio::io_context ioc;
	using client_service_type = test::overrun_client<asio::ip::tcp::socket>;
	auto svc_ptr = std::make_shared<client_service_type>(ioc.get_executor(), "");

	auto handler = [&handlers_called](error_code ec, std::vector<reason_code> rcs, suback_props) {
		++handlers_called;
		BOOST_CHECK(ec == client::error::pid_overrun);
		BOOST_ASSERT(rcs.size() == 1);
		BOOST_CHECK_EQUAL(rcs[0], reason_codes::empty);
	};

	detail::subscribe_op<
		client_service_type, decltype(handler)
	> { svc_ptr, std::move(handler) }
	.perform(
		{ { "topic", { qos_e::exactly_once } } }, subscribe_props {}
	);

	ioc.run_for(std::chrono::milliseconds(500));
	BOOST_CHECK_EQUAL(handlers_called, expected_handlers_called);
}

void run_test(
	error_code expected_ec, const std::string& topic_filter,
	const subscribe_props& sprops = {}, const connack_props& cprops = {}
) {
	constexpr int expected_handlers_called = 1;
	int handlers_called = 0;

	asio::io_context ioc;
	using client_service_type = test::test_service<asio::ip::tcp::socket>;
	auto svc_ptr = std::make_shared<client_service_type>(ioc.get_executor(), cprops);

	auto handler = [&handlers_called, expected_ec]
		(error_code ec, std::vector<reason_code> rcs, suback_props) {
			++handlers_called;

			BOOST_CHECK(ec == expected_ec);
			BOOST_ASSERT(rcs.size() == 1);
			BOOST_CHECK_EQUAL(rcs[0], reason_codes::empty);
		};

	detail::subscribe_op<
		client_service_type, decltype(handler)
	> { svc_ptr, std::move(handler) }
	.perform(
		{ { topic_filter, { qos_e::exactly_once } } }, sprops
	);

	ioc.run_for(std::chrono::milliseconds(500));
	BOOST_CHECK_EQUAL(handlers_called, expected_handlers_called);
}

BOOST_AUTO_TEST_CASE(invalid_topic_filter_1) {
	run_test(client::error::invalid_topic, "");
}

BOOST_AUTO_TEST_CASE(invalid_topic_filter_2) {
	run_test(client::error::invalid_topic, "+topic");
}

BOOST_AUTO_TEST_CASE(invalid_topic_filter_3) {
	run_test(client::error::invalid_topic, "topic+");
}

BOOST_AUTO_TEST_CASE(invalid_topic_filter_4) {
	run_test(client::error::invalid_topic, "#topic");
}

BOOST_AUTO_TEST_CASE(invalid_topic_filter_5) {
	run_test(client::error::invalid_topic, "some/#/topic");
}

BOOST_AUTO_TEST_CASE(invalid_topic_filter_6) {
	run_test(client::error::invalid_topic, "$share//topic#");
}

BOOST_AUTO_TEST_CASE(malformed_user_property_1) {
	subscribe_props sprops;
	sprops[prop::user_property].push_back(std::string(10, char(0x01)));

	run_test(client::error::malformed_packet, "topic", sprops);
}

BOOST_AUTO_TEST_CASE(malformed_user_property_2) {
	subscribe_props sprops;
	sprops[prop::user_property].push_back(std::string(75000, 'a'));

	run_test(client::error::malformed_packet, "topic", sprops);
}

BOOST_AUTO_TEST_CASE(wildcard_subscriptions_not_available_1) {
	connack_props cprops;
	cprops[prop::wildcard_subscription_available] = uint8_t(0);

	run_test(
		client::error::wildcard_subscription_not_available, "topic/#",
		subscribe_props {}, cprops
	);
}

BOOST_AUTO_TEST_CASE(wildcard_subscriptions_not_available_2) {
	connack_props cprops;
	cprops[prop::wildcard_subscription_available] = uint8_t(0);

	run_test(
		client::error::wildcard_subscription_not_available, "$share/grp/topic/#",
		subscribe_props {}, cprops
	);
}

BOOST_AUTO_TEST_CASE(shared_subscriptions_not_available) {
	connack_props cprops;
	cprops[prop::shared_subscription_available] = uint8_t(0);

	run_test(
		client::error::shared_subscription_not_available, "$share/group/topic",
		subscribe_props {}, cprops
	);
}

BOOST_AUTO_TEST_CASE(subscription_id_not_available) {
	connack_props cprops;
	cprops[prop::subscription_identifier_available] = uint8_t(0);

	subscribe_props sprops;
	sprops[prop::subscription_identifier] = 23;

	run_test(
		client::error::subscription_identifier_not_available, "topic", sprops, cprops
	);
}

BOOST_AUTO_TEST_CASE(large_subscription_id) {
	connack_props cprops;
	cprops[prop::subscription_identifier_available] = uint8_t(1);

	subscribe_props sprops;
	sprops[prop::subscription_identifier] = std::numeric_limits<uint32_t>::max();

	run_test(
		client::error::malformed_packet, "topic", sprops, cprops
	);
}

BOOST_AUTO_TEST_CASE(packet_too_large) {
	connack_props cprops;
	cprops[prop::maximum_packet_size] = 10;

	run_test(
		client::error::packet_too_large, "very large topic", subscribe_props {}, cprops
	);
}

BOOST_AUTO_TEST_SUITE_END()
