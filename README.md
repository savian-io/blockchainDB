# BlockchainDB
BlockchainDB is a novel trusted database management system. It combines Blockchain and Database technology to provide trustworthy and auditable data management.

The current code base is build on top of Oracle MySQL and introduces a special storage engine (Blockchain Storage Engine) that allows to store data in a blockchain. This storage engine can be used to store tables on different blockchains. Therefore an adapter interface has to be implemented for each Blockchain-Technology. Currently there is an Ethereum adapter, and a Fabric adapter, which allows to store data inside Ethereum or Fabric blockchain network.

More detailed architectural information can be found in the documentation.

## Getting Started

clone mysql
clone blockchain DB
run init
run build

22.04
20.04

## Project Structure

```
├── adapter
│   ├── ethereum        # Assets to implement the Ethereum adapter
│   │   ├── adapter     # C++ implementation of the adapter interface for ethereum
│   │   ├── contract    # Assets for the deployment of the required blockchain contracts
│   |   └── scripts     # Scripts to orchestrate the contract assets and prepare the blockchain for an interaction with the blockchain
│   ├── fabric          # Assets to implement the Fabric adapter
│   │   ├── adapter     # C++ implementation of the adapter interface for fabric
│   │   ├── contract    # Assets for the deployment of the required blockchain contracts
│   |   └── scripts     # Scripts to orchestrate the contract assets and prepare the blockchain for an interaction with the blockchain
│   ├── factory         # C++ Factory module that helps to create the different types of adapters
│   ├── interface       # C++ Definition of the generic adapter interface, contains also generic tests that should work for all
│   ├── utils
│   ├── .clang-format
│   ├── .clang-tidy
├── engine              # This storage engine can be used to store tables on different blockchains.
├── tests
├── .gitignore          # Top-level gitignore
└── README.md           # The file you are looking at currently
```

## Blockchain Table

CREATE BLOCKCHAIN TABLE is used to create a bc-table (blockchain table) which stores data in a blockchain. The used blockchain is determined by the CONNECTION information, parameter bc_type. A sharable table enables other TRUSTDBLE users to load it and interact with it by reading and writing data. Before creating bc-table, blockchain network should be started, and CONNECTION information to that blockchain network is a nessesary parameter to the CREATE statement.

```
    Name: 'CREATE BLOCKCHAIN TABLE'
    Description:
    Syntax:

        CREATE TABLE <table_name> (<table_schema>) ENGINE=BLOCKCHAIN CONNECTION=<connection_string>;

    Ethereum-Example:
    CREATE TABLE bc_tbl_ETH (id int, value int) ENGINE=BLOCKCHAIN CONNECTION='{"bc_type":"ETHEREUM","join-ip":"172.17.0.1","rpc-port":"8000"}';

    FABRIC-Example:
    CREATE TABLE bc_tbl_FAB (id int, value int) ENGINE=BLOCKCHAIN CONNECTION='{"bc_type":"FABRIC","cert_path":"/home/amerdeev/dev/bc_fabric/fabric-samples/test-network/organizations/peerOrganizations/org1.example.com/users/User1@org1.example.com/msp/signcerts/cert.pem","channel_name":"test-network-caf09b23-8fe3-43f4-acd1-9e9859e5ec59","gateway_peer":"test_network","key_path":"/home/amerdeev/dev/bc_fabric/fabric-samples/test-network/organizations/peerOrganizations/org1.example.com/users/User1@org1.example.com/msp/keystore/","msp_id":"Org1MSP","peer_endpoint":"localhost:7056","peer_operations_port":"9446","peer_port":"7056","test_network_path":"/home/amerdeev/dev/bc_fabric/fabric-samples/test-network","tls_cert_path":"/home/amerdeev/dev/bc_fabric/fabric-samples/test-network/organizations/peerOrganizations/org1.example.com/peers/test_network/tls/ca.crt"}';
```

## Blockchain Adapters

The adapter interface defines how a TrustDBle can interact with a blockchain to store/retrieve data. There are currently implementations of this interface for Hyperledger Fabric and Ethereum blockchain.

### Adapter Implementations

Currently there exists an implementation for the following blockchains

-   [Fabric](fabric) - Adapter implementation for the [Hyperledger Fabric](https://www.hyperledger.org/use/fabric) blockchain
-   [Ethereum](ethereum) - Adapter implementation for the [Ethereum](https://ethereum.org/) blockchain

### Blockchain Adapter for Ethereum

#### Dependencies
- npm
- node
- truffle (npm install -g truffle)
- curl

#### Getting Started
0. Search and replace, ie. adjust, all paths beginning with `/home/simon/Projects/dm`
1. Install truffle `sudo npm install -g truffle)
2. Compile the Ethereum smart contract `cd contract/truffle && truffle compile`

### Blockchain Adapter for Hyperledger Fabric

#### Requirements
The following additional software is required to run a Hyperledger Fabric blockchain and use this TrustDBle adapter implementation to interact with it
- Docker
- Docker-Compose
- Java

## Contribution guidelines
* Please write clean and self explanatory code. Document your code where required.
* Please adhere to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) when writing new code.

## Requirements
We inherit most of the requirements and dependencies from the mysql repo see https://dev.mysql.com/doc/refman/8.0/en/source-installation-prerequisites.html.
All requirements can be installed by using the `init` command of the `trustdble` helper script (see step 1) in [Getting Started](#Getting%20Started)).

General requirements
- C++ std-17
- CMake build system

Specific requirements
- We use the git [subrepo](https://github.com/ingydotnet/git-subrepo) command to manage the original mysql code as a repository
- We use [docker](https://docs.docker.com/engine/install/ubuntu/) to package our code as a container. **Note: Make sure to perform the docker post-installation steps for Managing Docker as a non-root user!**

## Getting Started

1. Clone this repository
2. Install [boost version 1.73.0](https://www.boost.org/users/history/version_1_73_0.html) on your system.

### Build mysql-server and blockchain storage engine

1. Change to the main src directory
```
cd src
```
2. Install the requirements using the `trustdble` helper script (currently, we only support linux as dev environment):
```
./trustdble init -s linux
```
3. Build the repo with the help of the helper script. NOTE: This might take very long!
```
./trustdble build
```
4. Use the `trustdble` helper script to start a server/client.

### Get Help

The script `trustdble` contains two predefined commands.

* **help** shows a general help message
* **list** lists all available script commands included in `trustdble_scripts`

Every script command of `trustdble` has an own help message, which can be accessed by using the `-h` or `--help` flag.
For example to get help for starting a server, you can run

    ./trustdble start server --help

### How to add a Command

To add a new command script to the `trustdble` script, follow the instructions stated [here](trustdble_scripts/README.md)
