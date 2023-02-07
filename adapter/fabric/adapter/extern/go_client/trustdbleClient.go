/*
Copyright 2020 IBM All Rights Reserved.

SPDX-License-Identifier: Apache-2.0
*/

package main

import "C"

import (
	"crypto/x509"
	"fmt"
	"github.com/hyperledger/fabric-gateway/pkg/client"
	"github.com/hyperledger/fabric-gateway/pkg/identity"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"io/fs"
	"io/ioutil"
	"log"
	"os"
	"path"
	"strings"
	"time"
)

// newGrpcConnection creates a gRPC connection to the Gateway server.
func newGrpcConnection(tlsCertPath, peerEndpoint, gatewayPeer string) (*grpc.ClientConn, error) {
	certificate, err := loadCertificate(tlsCertPath)
	if err != nil {
		return nil, fmt.Errorf("failed to load certificate: %w", err)
	}

	certPool := x509.NewCertPool()
	certPool.AddCert(certificate)
	transportCredentials := credentials.NewClientTLSFromCert(certPool, gatewayPeer)

	connection, err := grpc.Dial(peerEndpoint, grpc.WithTransportCredentials(transportCredentials))
	if err != nil {
		return nil, fmt.Errorf("failed to create gRPC connection: %w", err)
	}

	return connection, nil
}

// newIdentity creates a client identity for this Gateway connection using an X.509 certificate.
func newIdentity(mspID, certPath string) (*identity.X509Identity, error) {
	certificate, err := loadCertificate(certPath)
	if err != nil {
		return nil, fmt.Errorf("failed to load certificate: %w", err)
	}

	id, err := identity.NewX509Identity(mspID, certificate)
	if err != nil {
		return nil, fmt.Errorf("failed to create X509 identity: %w", err)
	}

	return id, nil
}

func loadCertificate(filename string) (*x509.Certificate, error) {
	certificatePEM, err := readFile(filename)
	if err != nil {
		return nil, fmt.Errorf("failed to read certificate file: %w", err)
	}
	return identity.CertificateFromPEM(certificatePEM)
}

// newSign creates a function that generates a digital signature from a message digest using a private key.
func newSign(keyPath string) (identity.Sign, error) {
	files, err := readDir(keyPath)
	if err != nil {
		return nil, fmt.Errorf("failed to read private key directory: %w", err)
	}
	privateKeyPEM, err := readFile(path.Join(keyPath, files[0].Name()))

	if err != nil {
		return nil, fmt.Errorf("failed to read private key file: %w", err)
	}

	privateKey, err := identity.PrivateKeyFromPEM(privateKeyPEM)
	if err != nil {
		return nil, fmt.Errorf("failed to get private key from file: %w", err)
	}

	sign, err := identity.NewPrivateKeySign(privateKey)
	if err != nil {
		return nil, fmt.Errorf("failed to get sign function: %w", err)
	}

	return sign, nil
}

func readFile(filename string) ([]byte, error) {
	homeDir, err := os.UserHomeDir()
	if err != nil {
		return nil, fmt.Errorf("failed to get home dir: %w", err)
	}
	absoluteFilename := strings.ReplaceAll(filename, "~", homeDir)
	return ioutil.ReadFile(absoluteFilename)
}

func readDir(dirname string) ([]fs.FileInfo, error) {
	homeDir, err := os.UserHomeDir()
	if err != nil {
		return nil, fmt.Errorf("failed to get home dir: %w", err)
	}
	absoluteDirname := strings.ReplaceAll(dirname, "~", homeDir)
	return ioutil.ReadDir(absoluteDirname)
}

//export Write
func Write(json_value, function, channel, contractName, mspID, certPath, keyPath, tlsCertPath, peerEndpoint, gatewayPeer string) int {

	log.Println("============ " + function + " ============")

	// There is significant overhead associated with establishing gRPC connections, so this connection should be retained by the application and used for all interactions with the Fabric Gateway.
	clientConnection, err := newGrpcConnection(tlsCertPath, peerEndpoint, gatewayPeer)
	if err != nil {
		log.Printf("Failed to establish grpc connection: %v", err)
		return 1
	}
	defer clientConnection.Close()

	id, err := newIdentity(mspID, certPath)
	if err != nil {
		log.Printf("Failed to create new client identiy: %v", err)
		return 1
	}

	sign, err := newSign(keyPath)
	if err != nil {
		log.Printf("Failed to create function for signing with clients private key: %v", err)
		return 1
	}

	// Create a Gateway connection for a specific client identity
	gateway, err := client.Connect(
		id,
		client.WithSign(sign),
		client.WithClientConnection(clientConnection),
		// Default timeouts for different gRPC calls
		client.WithEvaluateTimeout(5*time.Second),
		client.WithEndorseTimeout(15*time.Second),
		client.WithSubmitTimeout(5*time.Second),
		client.WithCommitStatusTimeout(1*time.Minute),
	)
	if err != nil {
		log.Printf("Failed to connect to gateway: %v", err)
		return 1
	}
	defer gateway.Close()

	network := gateway.GetNetwork(channel)
	contract := network.GetContract(contractName)

	_, err = contract.SubmitTransaction(function, json_value)
	if err != nil {
		log.Printf("Failed to submit transaction: %v", err)
		return 1
	}

	return 0
}

//export Read
func Read(json_value, function, channel, contractName, mspID, certPath, keyPath, tlsCertPath, peerEndpoint, gatewayPeer string) (*C.char, int, int) {

	log.Println("============ " + function + " ============")

	// There is significant overhead associated with establishing gRPC connections, so this connection should be retained by the application and used for all interactions with the Fabric Gateway.
	clientConnection, err := newGrpcConnection(tlsCertPath, peerEndpoint, gatewayPeer)
	if err != nil {
		log.Printf("Failed to establish grpc connection: %v", err)
		return nil, 0, 1
	}
	defer clientConnection.Close()

	id, err := newIdentity(mspID, certPath)
	if err != nil {
		log.Printf("Failed to create new client identiy: %v", err)
		return nil, 0, 1
	}

	sign, err := newSign(keyPath)
	if err != nil {
		log.Printf("Failed to create function for signing with clients private key: %v", err)
		return nil, 0, 1
	}

	// Create a Gateway connection for a specific client identity
	gateway, err := client.Connect(
		id,
		client.WithSign(sign),
		client.WithClientConnection(clientConnection),
		// Default timeouts for different gRPC calls
		client.WithEvaluateTimeout(5*time.Second),
		client.WithEndorseTimeout(15*time.Second),
		client.WithSubmitTimeout(5*time.Second),
		client.WithCommitStatusTimeout(1*time.Minute),
	)
	if err != nil {
		log.Printf("Failed to connect to gateway: %v", err)
		return nil, 0, 1
	}
	defer gateway.Close()

	network := gateway.GetNetwork(channel)
	contract := network.GetContract(contractName)

	value, err := contract.EvaluateTransaction(function, json_value)
	if err != nil {
		log.Printf("Failed to evaluate transaction: %v", err)
		return nil, 0, 1
	}

	return C.CString(string(value)), len(value), 0
}

// empty main is needed
func main() {}
