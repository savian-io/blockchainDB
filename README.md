# BlockchainDB
BlockchainDB is a novel trusted database management system. It combines Blockchain and Database technology to provide trustworthy and auditable data management.

The current code base is build on top of Oracle MySQL and introduces a special storage engine (BlockchainDB Storage Engine) that allows to store data in a blockchain. This storage engine can be used to store tables on different blockchains. Therefore an adapter interface has to be implemented for each Blockchain-Technology.

## Project Structure

```
├── adapter
│   ├── ethereum        # Assets to implement the Ethereum adapter
│   │   ├── adapter     # C++ implementation of the adapter interface for ethereum
│   │   ├── contract    # Assets for the deployment of the required blockchain contracts
│   |   └── scripts     # Scripts to orchestrate the contract assets and prepare the blockchain for an interaction with the blockchain
│   ├── factory         # C++ Factory module that helps to create the different types of adapters
│   ├── interface       # C++ Definition of the generic adapter interface, contains also generic tests that should work for all
│   └── utils           # Contains utility and helper methods
├── engine              # This storage engine can be used to store tables on different blockchains.
|   ├── include         # Contains header files of the blockchain storage engine
|   └── src             # Contains source files of the blockchain storage engine
├── tests
├── .gitignore          # Top-level gitignore
└── README.md           # The file you are looking at currently
```

## Blockchain Table

CREATE BLOCKCHAIN TABLE is used to create a bc-table (blockchain table) which stores data in a blockchain. The used blockchain is determined by the CONNECTION information, parameter bc_type. Before creating bc-table, a blockchain network should be started, and CONNECTION information to that blockchain network is a nessesary parameter to the CREATE statement.

Syntax:

    CREATE TABLE <table_name> (<table_schema>) ENGINE=BLOCKCHAIN CONNECTION=<connection_string>;

Example for Ethereum:

    CREATE TABLE bc_tbl_ETH (id int, value int) ENGINE=BLOCKCHAIN CONNECTION='{"bc_type":"ETHEREUM","join-ip":"172.17.0.1","rpc-port":"8000"}';

## Blockchain Adapters

The adapter interface defines how BlockchainDB interacts with a blockchain to store/retrieve data. Currently there exists an implementation for the following blockchains:

-   Ethereum - Adapter implementation for the [Ethereum](https://ethereum.org/) blockchain


## Getting Started
Download the shared libaray file and plug it into your running MySQL-Server instance by using the following command:

    INSTALL PLUGIN blockchainDB SONAME 'blockchain_db.so';

More details how to install plugins into MySQL-Server can be found [here](https://dev.mysql.com/doc/refman/8.0/en/plugin-loading.html#server-plugin-installing-install-plugin).


## Build mysql-server and blockchainDB storage engine
If you want to build the shared library from source you have include our storage engine in to storage folder of the mysql-server source.

1. Clone mysql-server repository without whole commit history

        git clone https://github.com/mysql/mysql-server.git --depth 1

2. Install all dependencies for building mysql-server

        sudo apt-get -y install build-essential cmake ninja-build curl libssl-dev libncurses5-dev pkg-config bison libtirpc-dev libudev-dev libldap-dev libsasl2-dev libsasl2-modules-gssapi-mit libcurl4-openssl-dev

3. Build mysql-server ( This may take a while)

        cmake -S . -B build -GNinja -DDOWNLOAD_BOOST=1 -DWITH_BOOST=$HOME/my_boost -DCMAKE_CXX_STANDARD_LIBRARIES="-lcurl"
        cmake --build build --parallel

4. Go to storage folder and clone blockchainDB repository

        cd storage
        git clone https://github.com/savian-io/blockchainDB

5. Install dependencies for building blockchainDB storage engine

        sudo apt-get ...

6. Back to the root directory of mysql-server and build it again

        cd ..
        cmake -S . -B build ...
        cmake --build build --parallel

## Example of usage

1. Initializing the Data Directory for mysql-server. More details can be found [here](https://dev.mysql.com/doc/refman/5.7/en/data-directory-initialization.htmln).

        mysql-server/build/bin/mysqld --initialize-insecure --user=mysql

2. Start mysql-server

        bin/mysqld --user=mysql

3. Start mysql-client, turn on debug mode, create database, create blockchain tabel, insert and select data

        bin/mysql -u root
        set debug ='d,blockchain';
        
        CREATE DATABASE demo_db;
        show databases;
        use demo_db;

        CREATE TABLE bc_tbl_ETH (id int, value int) ENGINE=BLOCKCHAIN CONNECTION='{"bc_type":"ETHEREUM","join-ip":"172.17.0.1","rpc-port":"8000"}';

        SELECT * FROM bc_tbl_ETH;
        SET SESSION binlog_format = 'STATEMENT';
        INSERT INTO bc_tbl_ETH VALUES(1,1);
        SELECT * FROM bc_tbl_ETH;

## Contribution guidelines
* Please write clean and self explanatory code. Document your code where required.
* Please adhere to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) when writing new code.
