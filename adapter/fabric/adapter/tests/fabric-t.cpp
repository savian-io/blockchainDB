//#include "adapter_interface_test.h"

#include "adapter_fabric/adapter_fabric.h"
#include "adapter_interface_test.h"

// Instantiate AdapterInterfaceTest suite
// NOLINTNEXTLINE(modernize-use-trailing-return-type)
INSTANTIATE_TEST_SUITE_P(
    FabricAdapterTests, AdapterInterfaceTest,
    testing::Values(std::make_tuple(
        std::make_shared<FabricAdapter>(), "./test-config.ini",
        "{     \"Network\": {         \"channel_name\": "
        "\"test-81b1b43e-01f1-470e-8469-4c320b1c255b\",         \"msp_id\": "
        "\"Org1MSP\",         \"cert_path\": "
        "\"~/TrustDBle/fabric_newest_version/test-network/organizations/"
        "peerOrganizations/org1.example.com/users/User1@org1.example.com/msp/"
        "signcerts/cert.pem\",         \"key_path\": "
        "\"~/TrustDBle/fabric_newest_version/test-network/organizations/"
        "peerOrganizations/org1.example.com/users/User1@org1.example.com/msp/"
        "keystore/\",         \"tls_cert_path\": "
        "\"~/TrustDBle/fabric_newest_version/test-network/organizations/"
        "peerOrganizations/org1.example.com/peers/test.org1.example.com/tls/"
        "ca.crt\",         \"gateway_peer\": \"test.org1.example.com\",    "
        "\"test_network_path\": "
        "\"~/TrustDBle/fabric_newest_version/test-network\",     "
        "\"peer_port\": \"7056\",         \"peer_operations_port\": \"9446\",  "
        "       \"peer_endpoint\": \"localhost:7056\"     } }")));

// Single Test-Cases - TEST(test_suite_name, test_name)

TEST(FabricAdapterTests, singlefabrictest) {
  EXPECT_EQ(5, 5);
  EXPECT_TRUE(true);
}
